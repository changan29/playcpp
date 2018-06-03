### test pb

follow the example and modify the message : https://developers.google.com/protocol-buffers/docs/cpptutorial

```cpp
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

