### 概述

出于熟悉这个工具的目的和想看看别人是如何写golang的代码的，如何写的优雅一点。在实际看代码之前，需要知道一些背景知识：

- [防火墙机器发展](https://en.wikipedia.org/wiki/Firewall_(computing))
- [rfc1928 SOCKS Protocol Version 5](https://tools.ietf.org/html/rfc1928)
- [G.F.W的原理](http://www.oneyearago.me/2019/06/14/learn_gwf/)

源码是这个： [shadowsocks-go](https://github.com/shadowsocks/shadowsocks-go)


socks是一种网络传输协议，主要用于客户端与外网服务器之间通讯的中间传递。根据OSI七层模型来划分，SOCKS属于会话层协议，位于表示层与传输层之间。

当防火墙后的客户端要访问外部的服务器时，就跟socks代理服务器连接。该协议设计之初是为了让有权限的用户可以穿过过防火墙的限制，使得高权限用户可以访问外部资源。经过10余年的时间，大量的网络应用程序都支持socks5代理。当然对于一般的防火墙，只有socks5可能够了，但是对于社会主义的防火墙，它应该会进行包检测分析某些关键词，在sock5协议基础上，数据包加上加密一般的检测不会生效，会绕过他，当然也是人家让你过，睁一只眼闭一只眼罢了。

整体流程如图：

![image](https://202002-1251969284.cos.ap-shanghai.myqcloud.com/shadowsocks-src.png)


### Procedure for TCP-based clients

以下参考 RFC:

#### 客户端告诉代理服务器自已支持的验证方式
When a TCP-based client wishes to establish a connection to an object
   that is reachable only via a firewall (such determination is left up
   to the implementation), it must open a TCP connection to the
   appropriate SOCKS port on the SOCKS server system.  The SOCKS service
   is conventionally located on TCP port 1080.  If the connection
   request succeeds, the client enters a negotiation for the
    authentication method to be used, authenticates with the chosen
   method, then sends a relay request.  The SOCKS server evaluates the
   request, and either establishes the appropriate connection or denies
   it.
   
The client connects to the server, and sends a version
   identifier/method selection message:

```
                   +----+----------+----------+
                   |VER | NMETHODS | METHODS  |
                   +----+----------+----------+
                   | 1  |    1     | 1 to 255 |
                   +----+----------+----------+
```
#### 代理服务器收到上面的报文，选择自已所能支持的验证方式
The VER field is set to X'05' for this version of the protocol.  The
   NMETHODS field contains the number of method identifier octets that
   appear in the METHODS field.
   
The server selects from one of the methods given in METHODS, and
   sends a METHOD selection message:
```
                         +----+--------+
                         |VER | METHOD |
                         +----+--------+
                         | 1  |   1    |
                         +----+--------+
```

#### 两者版本与验证方式的确认 代码

```
func handShake(conn net.Conn) (err error) {
	const (
		idVer     = 0
		idNmethod = 1
	)
	// version identification and method selection message in theory can have
	// at most 256 methods, plus version and nmethod field in total 258 bytes
	// the current rfc defines only 3 authentication methods (plus 2 reserved),
	// so it won't be such long in practice

	buf := make([]byte, 258)

	var n int
	ss.SetReadTimeout(conn)
	// make sure we get the nmethod field
	if n, err = io.ReadAtLeast(conn, buf, idNmethod+1); err != nil {
		return
	}
	if buf[idVer] != socksVer5 {
		return errVer
	}
	nmethod := int(buf[idNmethod])
	msgLen := nmethod + 2
	if n == msgLen { // handshake done, common case
		// do nothing, jump directly to send confirmation
	} else if n < msgLen { // has more methods to read, rare case
		if _, err = io.ReadFull(conn, buf[n:msgLen]); err != nil {
			return
		}
	} else { // error, should not get extra data
		return errAuthExtraData
	}
	// send confirmation: version 5, no authentication required
	_, err = conn.Write([]byte{socksVer5, 0})
	return
}
```

#### 客户端发送要建立的的代理连接的地址及端口
Once the method-dependent subnegotiation has completed, the client
   sends the request details.  If the negotiated method includes
   encapsulation for purposes of integrity checking and/or
   confidentiality, these requests MUST be encapsulated in the method-
   dependent encapsulation.

```
        +----+-----+-------+------+----------+----------+
        |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
        +----+-----+-------+------+----------+----------+
        | 1  |  1  | X'00' |  1   | Variable |    2     |
        +----+-----+-------+------+----------+----------+
        Where:

          o  VER    protocol version: X'05'
          o  CMD
             o  CONNECT X'01'
             o  BIND X'02'
             o  UDP ASSOCIATE X'03'
          o  RSV    RESERVED
          o  ATYP   address type of following address
             o  IP V4 address: X'01'
             o  DOMAINNAME: X'03'
             o  IP V6 address: X'04'
          o  DST.ADDR       desired destination address
          o  DST.PORT desired destination port in network octet
             order
```

#### local收到客户端的请求后，创建到目标机器的连接

```
func connectToServer(serverId int, rawaddr []byte, addr string) (remote *ss.Conn, err error) {
	se := servers.srvCipher[serverId]
	remote, err = ss.DialWithRawAddr(rawaddr, se.server, se.cipher.Copy())
	if err != nil {
		log.Println("error connecting to shadowsocks server:", err)
		const maxFailCnt = 30
		if servers.failCnt[serverId] < maxFailCnt {
			servers.failCnt[serverId]++
		}
		return nil, err
	}
	debug.Printf("connected to %s via %s\n", addr, se.server)
	servers.failCnt[serverId] = 0
	return
}
```
其中，返回的连接是自定义的格式：

```
type Conn struct {
	net.Conn
	*Cipher
	readBuf  []byte
	writeBuf []byte
}
```
这个Conn结构，实现的Write方法是先加密后写socket,如下：

```
// 这里完成了加密 
func (c *Conn) Write(b []byte) (n int, err error) {
	var iv []byte
	if c.enc == nil {
		iv, err = c.initEncrypt()
		if err != nil {
			return
		}
	}

	cipherData := c.writeBuf
	dataSize := len(b) + len(iv)
	if dataSize > len(cipherData) {
		cipherData = make([]byte, dataSize)
	} else {
		cipherData = cipherData[:dataSize]
	}

	if iv != nil {
		// Put initialization vector in buffer, do a single write to send both
		// iv and data.
		copy(cipherData, iv)
	}

	c.encrypt(cipherData[len(iv):], b)
	n, err = c.Conn.Write(cipherData)
	return
}
```

#### 收发数据
数据转发，直接转发内容，没啥逻辑
```
    ...
    // remote 是ss-local与ss-server的连接，conn是 client与ss-local的连接
    remote, err := createServerConn(rawaddr, addr)
	if err != nil {
		if len(servers.srvCipher) > 1 {
			log.Println("Failed connect to all available shadowsocks server")
		}
		return
	}
	defer func() {
		if !closed {
			remote.Close()
		}
	}()

	go ss.PipeThenClose(conn, remote, nil)
	ss.PipeThenClose(remote, conn, nil)
	...
```
PipeThenClose收发数据使用的LeakyBuf -> TODO 

#### ss-server

ss-server与ss-local逻辑差不多

### 感受

```
➜  src cat shadowsocks*/* | wc -l
    2240
```
- 实现这个工具一共用了 2240行 golang代码
- 毕竟不是初恋，比起C 系有很多不适应，感觉很像 a new php
- Saas化还不够，还是要写这么多


### REF
- [防火墙机器发展](https://en.wikipedia.org/wiki/Firewall_(computing))
- [socks5 协议简介](http://zhihan.me/network/2017/09/24/socks5-protocol/)
- [rfc1928 SOCKS Protocol Version 5](https://tools.ietf.org/html/rfc1928)
- [G.F.W的原理](http://www.oneyearago.me/2019/06/14/learn_gwf/)