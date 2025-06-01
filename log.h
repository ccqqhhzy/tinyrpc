// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef __LOG_H__
#define __LOG_H__


#include <string>
#include <string.h>
#include <vector>
#include <syslog.h>
#include <sys/time.h>
#include <stdarg.h>


namespace tinyrpc {


enum LogLevel {
    Debug = 0,
    Info,
    Notice,
    Warn,
    Error,
    LogLevelMax,
};


class Logger {
public:
    enum {
        LOG_LINE_LEN_MAX = 1024,
    };

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    ~Logger() {
        if (isOpen_) {
            closelog();
        }
    }

    void init(const std::string& ident) {
        openlog(ident.c_str(), LOG_PID | LOG_NDELAY | LOG_NOWAIT, LOG_USER);
        isOpen_ = true;        
    }

    // __attribute__((format(printf,N,M)))
    // 其中N表示第几个参数是格式化字符串，M指明从第几个参数开始做检查
    // C++类中如使用，则要注意参数位置，默认隐藏有this指针，所以N、M的值要加1。
    // 但是类的静态成员没有this指针
    void log(LogLevel level, const std::string& file, int line, 
             const char* fmt, ...) __attribute__((format(printf,5,6))) {
        if (level < Debug || level >= LogLevelMax) {
            level = Info;
        }
		
        char msg[LOG_LINE_LEN_MAX] = {0};

        va_list	ap;
        va_start(ap,fmt);
        int ret = vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);

        if (ret < 0)
            return;

        struct timeval tv;
        long long sec, mSec;
        gettimeofday(&tv, NULL);
        sec = ((long long)tv.tv_sec);
        mSec = ((long long)tv.tv_usec) / 1000;

        syslog(level_[level], "[%lld.%lld][%s:%d] %s", 
            sec, mSec, file.c_str(), line, msg);
    }

private:
    Logger() : isOpen_(false) {
    	// level : LOG_DEBUG  LOG_INFO  LOG_NOTICE LOG_WARNING LOG_ERR
        level_.resize(LogLevelMax);
        level_[Debug] = LOG_DEBUG;
        level_[Info] = LOG_INFO;
        level_[Notice] = LOG_NOTICE;
        level_[Warn] = LOG_WARNING;
        level_[Error] = LOG_ERR;
    }

    bool isOpen_;

    std::vector<int> level_;
};


#define LOG(level, ...) do {\
    std::string file = std::string(__FILE__) + ":" + std::string(__FUNCTION__);\
    Logger::getInstance().log(level, file, __LINE__, __VA_ARGS__); \
} while(0)


} // namespace tinyrpc

#endif // __LOG_H__

/**

1.
查看 rsyslog 后台进程是否在运行
ps -ef | grep rsyslog

查看日志
tail -f /var/log/syslog

2.设置日志输出到指定文件

 /etc/init.d/rsyslog restart
 

https://www.rsyslog.com/doc/master/configuration/templates.html

template(name="FileFormat" type="string"
         string= "%TIMESTAMP% %HOSTNAME% %syslogtag%%msg:::sp-if-no-1st-sp%%msg:::drop-last-lf%\n"
        )

template(name="FileFormat" type="list") {
    property(name="timestamp" dateFormat="rfc3339")
    constant(value=" ")
    property(name="hostname")
    constant(value=" ")
    property(name="syslogtag")
    property(name="msg" spifno1stsp="on" )
    property(name="msg" droplastlf="on" )
    constant(value="\n")
    }

template (name="DynFile" type="string" string="/var/log/system-%HOSTNAME%.log")

"*" action(type="omfile" file="/var/log/all-messages.log" template="Name-of-your-template")

user.* action(type="omfile" file="/var/log/test/xxx.log" template="FileFormat")

sudo vim /etc/rsyslog.d/50-default.conf
$template log_tmp,"%timegenerated% %PRI-text% %syslogtag%%msg%\r\n"
$template dynamic_file,"/var/log/test/exeR_%$year%-%$month%-%$day%.log"
user.*  ?dynamic_file;log_tmp

 */