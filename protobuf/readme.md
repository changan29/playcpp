### 引用
> 一个golang 网友的思考(http://www.cnblogs.com/wolfred7464/p/4673908.html)：

    1、我用二进制传输消息意义在哪？
    
        当然就是字节少，传输成本小。
    
    2、二进制不可读啊，怎么解码呢？
    
        调用Unmarshal函数，把对应类型的对象传入，会自动把二进制解码到传入的对象。
    
    3、拿到一大坨二进制数据，怎么知道能解码到哪个类呢？
    
        我仍然认为数据头加上类型ID是比较好的做法。
        官方C++版本中可以通过协议名称得到一个协议类型，
        优势是不需要我们自己维护一份switch case的代码。但是类型名称一般不如ID字节少。
        在网游使用可以把ID多级分类，例如按照背包、好友、帮派分类，
        背包中再分为装备和物品，然后依次细分，这样每次增删协议对其他部分影响较少。

我觉得他在使用的过程中是有自己的思考的，第三点我也是这么认为，如果两者通信，不可能只给二级制，也得给一些附加数据让对方解析这些二进制吧，带一个cmdId 给对方然后两方约定这个 id 对应的类型 应该是一种好的选择

### Standard Message Methods
每个消息类还包含许多其他方法，可以检查或处理整个消息。

- bool IsInitialized() const;           检查是否设置了所有必需的字段
- string DebugString() const;           返回消息的人类可读表示，特别用于调试。
- void CopyFrom(const Person& from);    用给定消息的值覆盖消息
- void Clear();                         清除所有元素返回空状态


### Parsing and Serialization

每个协议缓冲类都有用于通过二进制形式编写和读取所选类型消息的方法

- bool SerializeToString(string* output) const; 该消息序列化和存储的字节在给定的字符串。
- bool ParseFromString(const string& data);: parses a message from the given string.
- bool SerializeToOstream(ostream* output) const;   将消息写入给定的C ++ ostream
- bool ParseFromIstream(istream* input); parses a message from the given C++ istream

### Advanced Usage
协议消息类提供的一个关键特性是反射。您可以迭代消息的字段并操作它们的值，而无需针对任何特定消息类型编写代码。
使用反射的一个非常有用的方法是将协议消息转换为其他编码（例如XML或JSON）以及从其他编码转换。更高级的反射使用可能是找到两个相同类型的消息之间的差异，或者开发一种“协议消息的正则表达式”，在其中可以编写匹配特定消息内容的表达式。如果您使用自己的想象力，可以将Protocol Buffers应用于比您最初期望的更广泛的问题！

### 一个例子
官网的教程是 写和读，加了一个修改的操作

code: [https://github.com/changan29/playcpp/tree/master/protobuf](https://github.com/changan29/playcpp/tree/master/protobuf)

demo: 
```
[root@VM_80_251_centos pb]# g++ reader.cpp addressbook.pb.cc -o rr -lprotobuf
[root@VM_80_251_centos pb]# ./rr test
--- before modify
Person ID: 1
  Name: changan
  E-mail address: 909@163.com
  Mobile phone #: 186
  Home phone #: 147
Person ID: 2
  Name: cc
  E-mail address: 678
  Home phone #: 123
Person ID: 4
  Name: 123
  E-mail address: 123456
  Work phone #: 123456
--- after modify
Person ID: 666
  Name: changan
  E-mail address: another.email
  Home phone #: 987654321
  Home phone #: 987654321
Person ID: 666
  Name: cc
  E-mail address: another.email
  Home phone #: 987654321
Person ID: 666
  Name: 123
  E-mail address: another.email
  Home phone #: 987654321
```

### proto 2/3 的差别

- 版本
proto文件中的第一行非空白非注释行

```
syntax = "proto3"
表示使用proto3的语法，否则默认使用proto2的语法
```
- 字段

```
移除required，将optional改名为singular。
如果不加repeated，默认就是singular的。
```
- 语言

```
增加了Go,Ruby,JavaNano等的支持
未来还计划支持PHP等
```
- 移除了default选项

```
在proto2中，可使用default为field指定默认值。
在proto3中，field的默认值只依赖于field的类型，不再能够被指定。
当field的value为默认值时，该field不会被序列化，可节省空间。
不要依赖于字段的默认值的行为，因为无法区分是指定为默认值，还是未定义值。
```
- 枚举类型的第一个枚举值必须是0

```
proto3中必须提供一个枚举值为0作为枚举的默认值。
为了和proto2兼容（proto2使用第一个枚举值作为默认值），因此规定一个枚举值为0。
```

- 不再支持Extension，新增Any关键字

```
Any 类型是用来替代 proto2 中的扩展的。目前还在开发中。

proto2 中的扩展特性很像 Swift 语言中的扩展。
理解起来有点困难，使用起来更是会带来不少混乱。

相比之下，proto3 中新增的 Any 类型有点像 C/C++ 中的 void*
，好理解，使用起来逻辑也更清晰。
```

- 增加了 JSON 映射特性

```
语言的活力来自于与时俱进。当前，JSON 的流行有其充分的理由。
很多『现代化』的语言都内置了对 JSON 的支持，比如 Go、PHP 等。

### Reflection
- TODO
