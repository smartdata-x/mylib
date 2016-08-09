## glog使用整理 ##

### 安装 ###
./configure && make && make install

### 日志类型 ###
>LOG    //内置日志

>VLOG    //自定义日志

>DLOG    //DEBUG模式可输出的日志

>DVLOG    //DEBUG模式可输出的自定义日志

>SYSLOG    //系统日志，同时通过 syslog() 函数写入到/var/log/message 文件

>PLOG    //perror风格日志，设置errno状态并输出到日志中

>RAW_LOG   //线程安全的日志，需要#include "glog/raw_logging.h"


### 使用 ###
<pre><code>
#include <stdio.h>
#include "glog/logging.h"
int main(int argc, char* argv[]) {
    //初始化日志名
    google::InitGoogleLogging(argv[0]);

    //初始化info级别日志存储位置以及日志文件开头 ./info/info_20160808-105851.4743
    google::SetLogDestination(google::INFO, "./info/info_");
    google::SetLogDestination(google::WARNING, "./warning/waring_");
    google::SetLogDestination(google::ERROR, "./error/error_");

    //配置输出到标准输出的级别 INFO级别及以上输出到标准输出
    google::SetStderrLogging(google::INFO);

    //设置输出到屏幕的日志显示相应颜色
    FLAGS_colorlogtostderr=true;   

    //实时输出日志
    FLAGS_logbufsecs =0; 

    //最大日志大小（MB） 
    FLAGS_max_log_size =100;

    //当磁盘被写满时，停止日志输出
    FLAGS_stop_logging_if_full_disk = true;
    
    //捕捉 core dumped
    google::InstallFailureSignalHandler();     

    //输出info
    LOG(INFO) << "this is info";

    //输出warning
    LOG(WARNING) << "this is warning";

    //输出error
    LOG(ERROR) << "this is error";

    //LOG_IF(INFO, true) << "log if is true";
    //LOG_EVERY_N(INFO, 10) << "log every 10 counts";
    //LOG_IF_EVERY_N(INFO, true, 10) << "";
    //LOG_FIRST_N(INFO, 10) << ""log first 10 times;

    //资源释放
    google::ShutdownGoogleLogging();
}

</code></pre>





