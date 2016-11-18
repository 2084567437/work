#!/bin/bash
Usage(){  
    echo "Usage: sh $0 [ip_list file] [cmd] [passwd]"  
}  
  
if [ $# -lt 2 ]  
then  
    Usage
    exit 1
elif [ $1 == "-h" ]
then 
    Usage 
    exit 1 
fi 

passwd='KKpush_123$%^'
if [ $# -gt 2 ]
then
	passwd='Zhaojun@0715'
fi

/usr/bin/sshpass -p ${passwd} pssh -A -O StrictHostKeyChecking=no -t 0 -h $1 -i $2
