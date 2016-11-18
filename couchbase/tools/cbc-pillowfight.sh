#!/bin/bash
Usage(){  
    echo "Usage: sh $0 command host [bucket] [bucket_passwd]"  
}
libcb_dir=~/install/libcouchbase/build/bin/ 
if [ $# -lt 1 -o  "$1" == "-h" ]  
then  
    Usage
    ${libcb_dir}cbc-pillowfight -h
    exit 1
fi

options=$1
bucket=ptest
bucket_passwd=ptest

if [ $# -gt 1 ]
then
	host=$2
fi

if [ $# -gt 2 ]
then 
	bucket=$3
fi

if [ $# -gt 3 ]
then
	bucket_passwd=$4
fi

case $2 in
	176) host=172.16.101.176;;
	213) host=172.16.101.213;;
	224) host=172.16.101.224;;
	224) host=172.16.101.246;;
	169) host=172.16.102.169;;
	* ) host=0.0.0.0;;
 esac
echo ${host}
${libcb_dir}cbc-pillowfight ${options} -U couchbase://${host}/${bucket} -u ${bucket} -P ${bucket_passwd}