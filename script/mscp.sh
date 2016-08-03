#!/bin/bash
Usage(){  
    echo "Usage: sh $0 [ip_list file] [source_file] [dest_dir]"  
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

pscp -A -h $1 $2 $3
