[TOC]

## 日志系统
```
log4j
logger (Define Journal Type)
   |  
   |---------- formatter (Journal Type)
   |
appender (Journal Output Detination)
```

## 配置文件
```c++
template<T, FromStr, ToStr>
class ConfigVar

template<F, T>
class LexicalCast
```

key 相同，类型不同的不会报错

自定义类型，需要实现偏特化
实现后，就可以支持 config 解析自定义类型，自定义类型可以和常规 STL 容器一起使用。

## 配置的事件机制

当一个配置项发生修改，可以通知运行中的代码

## 日志系统整合
```yaml
logs:
    - name: root
      level: (debug, info, warn, error, fatal)
      formatter: '%d%T%p%T%t%T%m%n'
      appender:
        - type: (StdoutLogAppender, FileLogAppender)
          level: (debug, ...)
          file: /logs/xxx.log
```
```cpp
svher::Logger g_logger = 
LoggerMgr::GetInstance()->getLogger(name)
// 当 logger 的 appender 为空，使用 root 写 logger
LOG_INFO(g_logger) << "xxxx"
```

## 线程库
Mutex, RWMutex

日志库使用 Spinlock 加锁


## 协程库封装

```
main_fiber ----- child_fiber
scheduler --1:N-- thread --1:M-- fiber
```

1. 线程池，分配一组线程
2. 协程调度器，将协程指定到线程中执行

schedule(func/fiber)

1. 设置当前线程的 scheduler
2. 设置当前线程的 fiber
3. 执行消息循环
    - 协程消息队列是否有任务
    - 无任务时执行 idle

## socket 函数库


```
IOManager(epoll) ------ idle(epoll_wait)

信号量
message_queue (++sem)
   |---- Thread (--sem)
   |---- Thread (--sem)
```

Timer addTimer() cancel()
获取当前的定时器离现在的时间差
返回当前触发的定时器

```
    [Fiber]              [Timer]
       |                    |
       |                    |
       |                    |
    [Thread]          [TimerManager]
       |                    |
       |                    |
  [Scheduler] --------- [IOManager]
```

## HOOK


socket 相关函数
socket, connect, accept
read, write, send, recv
fcntl, ioctl

## Socket

```
            Socket
               |
            Address ------ [ IPAddress ] --- [ IPv4 ]
               |                            |
        [ Unix Address]                     |- [ IPv6 ]

connect, accept, read, write, close
```

## 序列化

write(int, float, int64, ...)
read(int, float, int64, ...)