#!/bin/bash
Usage(){  
    echo "Usage: sh $0 thread_file log_file log_prefix"  
}  
  
if [ $# -lt 3 ]  
then  
    Usage
    exit 1
elif [ $1 == "-h" ]
then 
    Usage 
    exit 1 
fi 

threadfile=$1
logfile=$2
log_prefix=$3   

rm -rf ${logfile}


echo log4j.rootLogger=INFO,fa>>${logfile}

echo log4j.appender.ca=org.apache.log4j.ConsoleAppender>>${logfile}
echo log4j.appender.ca.layout=org.apache.log4j.PatternLayout>>${logfile}
echo log4j.appender.ca.layout.ConversionPattern=%d[%t] %-5p %l %.16c - %m%n>>${logfile}
echo >> ${logfile}

echo log4j.appender.fa=org.apache.log4j.DailyRollingFileAppender>>${logfile}
echo log4j.appender.fa.File=${log_prefix}'${JPROCID}'.log>>${logfile}
echo log4j.appender.fa.DatePattern='.'yyyyMMdd-HH>>${logfile}
echo log4j.appender.fa.layout=org.apache.log4j.PatternLayout>>${logfile}
echo log4j.appender.fa.layout.ConversionPattern=%d[%t] %-5p %l %.16c - %m%n>>${logfile}
echo >> ${logfile}

for thread_name in $(cat ${threadfile})        
do
    handle=th_${thread_name}
    echo log4j.logger.${thread_name}=INFO,${handle} >> ${logfile}
    echo log4j.additivity.${thread_name}=false >> ${logfile}

    echo log4j.appender.${handle}=org.apache.log4j.DailyRollingFileAppender >> ${logfile}
    echo log4j.appender.${handle}.File=${log_prefix}'${JPROCID}'_${thread_name}.log >>${logfile}
    echo log4j.appender.${handle}.DatePattern='.'yyyyMMdd-HH >> ${logfile}
    echo log4j.appender.${handle}.layout=org.apache.log4j.PatternLayout >> ${logfile}
    echo log4j.appender.${handle}.layout.ConversionPattern=%d[%t] %-5p %l %.16c - %m%n >> ${logfile}
    echo >> ${logfile}
done
