### 类型

Go 语言里的引用类型有如下几个:切片、映射、通道、接口和函数类型。当声明上述类型的变量时，创建的变量被称作标头(header)值。
从技术细节上说，字符串也是一种引用类型。 每个引用类型创建的标头值是包含一个指向底层数据结构的指针。
每个引用类型还包含一组独特 的字段，用于管理底层数据结构。
因为标头值是为复制而设计的，所以永远不需要共享一个引用 类型的值。
标头值里包含一个指针，因此通过复制来传递一个引用类型的值的副本，本质上就是 在共享底层数据结构。



- slice

    在 64 位架构的机器上，一个切片需要 24 字节的内存:指针字段需要 8 字节，长度和容量字段分别需要 8 字节。
    类似vector实现

- map

    C++ map的实现


- interface实现

![image](https://202002-1251969284.cos.ap-shanghai.myqcloud.com/golang_innner_interface.png)

```
type notifier interface {
	notify()
}

type user struct {
	name string
	email string
}

func (u user) notify(){
	fmt.Printf("sending user email to %s<%s>\n",u.name,u.email)
}

type admin struct {
	user
	level string
}

func (a *admin) notify()  {
	fmt.Printf("sending user email to %s<%s>\n",a.name,a.email)
}

func main()  {
	bill := user{"bill","bill@tencent.com"}
	sendNotification(&bill)		//  can be a pointer

	lisa := admin{
		user:  user{"lisa","lisa@google.com"},
		level: "root",
	}
	sendNotification(&lisa)		// must be a pointer
}

func sendNotification(n notifier)  {
	n.notify()
}

```

### 并发与通道

- 并发与原子操作


```
var(
	counter int64
	wg		sync.WaitGroup		// 计数的信号量
	mutex	sync.Mutex			// 互斥锁	用于创建代码临界区
)

func main(){
	runtime.GOMAXPROCS(2)
	wg.Add(2)
	go IncCounter()
	go IncCounter()

	/* 如果 WaitGroup 的值大于 0，Wait 方法就会阻塞 */
	wg.Wait()
	fmt.Println("counter:" , counter)
}

func IncCounter()  {
	defer wg.Done()		/* 减少信号量的值 */

	for count := 0;  count < 2 ; count++  {
		// 原子操作  对应的还有 LoadInt64/StoreInt64 等原子操作
		atomic.AddInt64(&counter , 1)
		runtime.Gosched()
	}
}

func MutexIncCounter(){
	defer wg.Done()
	for count := 0;  count < 2 ; count++ {
		// 	当前 goroutine在执行，其他协程 不会再执行  当前临界区代码
		mutex.Lock()
		{
			value := counter
			runtime.Gosched()
			value++
			counter = value
		}
		mutex.Unlock()
	}
}
```

- channel

小人画的很萌

![image](https://202002-1251969284.cos.ap-shanghai.myqcloud.com/go_buf_chan_cute.png)

```
const (
	numberGroutines = 4
	taskLoad = 10
)

var wg sync.WaitGroup

func init()  {
	rand.Seed(time.Now().Unix())
}

func main()  {
	tasks := make(chan string , taskLoad)
	wg.Add(numberGroutines)
	for gr:=1 ;  gr <= numberGroutines; gr++  {
		go work(tasks , gr)
	}
	for post := 1; post <= taskLoad ; post++ {
		tasks <- fmt.Sprintf("Task:[%d]" , post)
	}
	close(tasks)
	wg.Wait()
}

func work(tasks chan string , worker int){
	defer wg.Done()
	for  {
		task , ok := <- tasks
		if !ok {
			fmt.Printf("worker: %d shutdown \n" , worker)
			return
		}
		fmt.Printf("worker: %d , start %s\n" ,worker , task)

		time.Sleep( time.Duration(rand.Int63n(100) ) * time.Microsecond)
		fmt.Printf("worker %d complete %s\n" , worker , task)
	}
}

```

- channel实现

Channel 在运行时的内部表示是 runtime.hchan，该结构体中包含了一个用于保护成员变量的互斥锁，
从某种程度上说，Channel 是一个用于同步和通信的有锁队列。
使用互斥锁解决程序中可能存在的线程竞争问题是很常见的，我们能很容易地实现有锁队列。

然而锁导致的休眠和唤醒会带来额外的上下文切换，如果临界区过小，加锁解锁导致的额外开销就会成为性能瓶颈。
1994 年的论文 Implementing lock-free queues 就研究了如何使用无锁的数据结构实现先进先出队列，
而 Go 语言社区也在 2014 年提出了无锁 Channel 的实现方案，该方案将 Channel 分成了以下三种类型：

1. 同步 Channel — 不需要缓冲区，发送方会直接将数据交给（Handoff）接收方；
1. 异步 Channel — 基于环形缓存的传统生产者消费者模型；
1. chan struct{} 类型的异步 Channel — struct{} 类型不占用内存空间，不需要实现缓冲区和直接发送（Handoff）的语义；

因为目前通过 CAS 实现的无锁 Channel 没有提供 FIFO 的特性，所以该提案暂时也被搁浅了

