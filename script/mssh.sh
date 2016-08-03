#!/bin/bash
Usage(){  
    echo "Usage: sh $0 [ip_list file] [cmd]"  
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

pssh -A -O StrictHostKeyChecking=no -t 0 -h $1 -i $2
