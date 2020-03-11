### shadowsocks.0 维护配置

通过 shadowsocks源码学习golang

- json配置
- 反射更新
- 逃逸分析
- 类型判断
- [类型与反射底层](https://draveness.me/golang/docs/part2-foundation/ch04-basic/golang-interface/)

```
package main

import "encoding/json"
import "fmt"
import "time"
import "os"
import "flag"
import "reflect"
import "io/ioutil"


var readTimeout time.Duration
var configFile string

type Config struct {
	Server       interface{} `json:"server"`
	ServerPort   int         `json:"server_port"`
	LocalPort    int         `json:"local_port"`
	LocalAddress string      `json:"local_address"`
	Password     string      `json:"password"`
	Method       string      `json:"method"` // encryption method

	// following options are only used by server
	PortPassword map[string]string `json:"port_password"`
	Timeout      int               `json:"timeout"`

	// following options are only used by client

	// The order of servers in the client config is significant, so use array
	// instead of map to preserve the order.
	ServerPassword [][]string `json:"server_password"`
}

func ParseConfig(path string) (config *Config, err error) {
	file, err := os.Open(path) // For read access.
	if err != nil {
		return
	}
	defer file.Close()

	data, err := ioutil.ReadAll(file)
	if err != nil {
		return
	}

	/*
	   https://golang.org/doc/faq#stack_or_heap
	   如果一个变量被取地址（has its address taken），并且被逃逸分析（escape analysis）识别为 “逃逸到堆”（escapes to heap）
	*/
	config = &Config{}
	if err = json.Unmarshal(data, config); err != nil {
		return nil, err
	}
	readTimeout = time.Duration(config.Timeout) * time.Second
	return
}

/*
类型判断:
- value , ok:=a.(string)
- a.(type) ... switch
- reflect.TypeOf
*/
func (config *Config) GetServerArray() []string {
	// Specifying multiple servers in the "server" options is deprecated.
	// But for backward compatibility, keep this.
	if config.Server == nil {
		return nil
	}
	// fmt.Println("reflect.TypeOf config.Server: ", reflect.TypeOf( config.Server ) )
	_ , ok := config.Server.(string)
	if ok {
		fmt.Println("use config.Server.(string) check string pass  ")
	} else {
		fmt.Println("config.Server.(string) false ")
	}
	single, ok := config.Server.(string)
	if ok {
		return []string{single}
	}
	arr, ok := config.Server.([]interface{})
	if ok {
		/*
			if len(arr) > 1 {
				log.Println("Multiple servers in \"server\" option is deprecated. " +
					"Please use \"server_password\" instead.")
			}
		*/
		serverArr := make([]string, len(arr), len(arr))
		for i, s := range arr {
			serverArr[i], ok = s.(string)
			if !ok {
				goto typeError
			}
		}
		return serverArr
	}
typeError:
	panic(fmt.Sprintf("Config.Server type error %v", reflect.TypeOf(config.Server)))
}

/*
https://draveness.me/golang/docs/part2-foundation/ch04-basic/golang-interface/
https://draveness.me/golang/docs/part2-foundation/ch04-basic/golang-reflect/
反射 已经 底层的数据结构，可遍历
*/
func UpdateConfig(old, new *Config) {
	// http://golang.org/doc/articles/laws_of_reflection.html
	newVal := reflect.ValueOf(new).Elem()
	oldVal := reflect.ValueOf(old).Elem()

	// typeOfT := newVal.Type()
	for i := 0; i < newVal.NumField(); i++ {
		newField := newVal.Field(i)
		oldField := oldVal.Field(i)
		fmt.Fprintf(os.Stdout, "before: newField: %d: %s -> %v; oldField:  %s -> %v\n", i,oldField.Type(),oldField.Interface(),
			newField.Type(), newField.Interface())
		switch newField.Kind() {
		case reflect.Interface:
			if newField.Interface() != nil {
				fmt.Println("newField.Interface: ",
					newField.Interface() )
				oldField.Set(newField)
			}
		case reflect.String:
			s := newField.String()
			if s != "" {
				oldField.SetString(s)
			}
		case reflect.Int:
			i := newField.Int()
			if i != 0 {
				oldField.SetInt(i)
			}
		}
		fmt.Fprintf(os.Stdout, "after: newField: %d: %s -> %v; oldField:  %s -> %v\n", i,oldField.Type(),oldField.Interface(),
			newField.Type(), newField.Interface())
	}

	old.Timeout = new.Timeout
	readTimeout = time.Duration(old.Timeout) * time.Second
}


func main(){
	var cmdConfig Config
	flag.StringVar(&configFile, "c", "config.json", "specify config file")
	flag.StringVar(&cmdConfig.Method, "m", "", "encryption method, default: aes-256-cfb")
	flag.Parse()

	// use cmdConfig update config
	fmt.Println("cmdConfig:",cmdConfig)

	config, err := ParseConfig(configFile)

	if err != nil {
		if !os.IsNotExist(err) {
			fmt.Fprintf(os.Stderr, "error reading %s: %v\n", configFile, err)
			os.Exit(1)
		}
		config = &cmdConfig
		UpdateConfig(config, config)
	} else {
		UpdateConfig(config, &cmdConfig)
	}


	fmt.Println("config content ",config)
	config_json , err := json.Marshal(*config)
	fmt.Println( string( config_json ) )
	fmt.Println("config.GetServerArray: ", config.GetServerArray() )
}

```


```
{
    "server":"127.0.0.1",
    "server_port":8388,
    "local_port":1080,
    "local_address":"127.0.0.1",
    "password":"barfoo!",
    "method": "aes-128-cfb",
    "timeout":600
}
```
