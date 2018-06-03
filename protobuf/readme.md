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

- TODO

### Reflection
- TODO
