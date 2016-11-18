#!/bin/bash
Usage(){  
    echo "Usage: sh $0 command"  
}  
 
couchbase_dir=../couchbase/bin/
if [ $# -lt 1 ]  
then  
    Usage
	${couchbase_dir}couchbase-cli -h 
    exit 1
elif [ $1 == "-h" ]
then 
	${couchbase_dir}couchbase-cli -h 
    exit 1 
fi


command=$1

case $2 in
	176) host=172.16.101.176;;
	213) host=172.16.101.213;;
	224) host=172.16.101.224;;
	224) host=172.16.101.246;;
	169) host=172.16.102.169;;
	* ) echo "error host" >&2;;
 esac

${couchabse_dir}couchbase-cli ${command} -c ${host}:8091 -u Administrator -p cbtest
