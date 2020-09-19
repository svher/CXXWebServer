
# 日志系统
```
log4j
logger (Define Journal Type)
   |  
   |---------- formatter (Journal Type)
   |
appender (Journal Output Detination)
```

# 配置文件

template<T, FromStr, ToStr>
class ConfigVar

template<F, T>
class LexicalCast


key 相同，类型不同的不会报错

自定义类型，需要实现偏特化
实现后，就可以支持 config 解析自定义类型，自定义类型可以和常规 STL 容器一起使用。

# 配置的事件机制

当一个配置项发生修改，可以通知运行中的代码

# 日志系统整合
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

# 协程库封装

# socket 函数库