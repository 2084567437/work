#!/bin/bash

#yum-utils
sudo yum -y install yum-utils

#epel
sudo yum -y install http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm

#ICE 3.5
sudo wget -O /etc/yum.repos.d/zeroc-ice-rhel6.repo http://www.zeroc.com/download/Ice/3.5/el6/zeroc-ice-el6.repo

# erlang repo
sudo yum install -y http://packages.erlang-solutions.com/erlang-solutions-1.0-1.noarch.rpm

# erlang packages
sudo yum install -y esl-erlang

# ice packages
sudo yum install -y ice-*

#DB53
sudo yum install -y db53 db53-devel 

#mcpp
sudo yum install -y mcpp-devel

#bzip2
sudo yum install -y bzip2-devel

#expat
sudo yum install -y expat-devel

#cmake
sudo yum install -y cmake28

#boost
sudo yum install -y boost-*

#couchbase
sudo perl ./couchbase-csdk-setup
sudo yum install -y libcouchbase-devel libcouchbase2*

#rabbitmq
sudo yum install -y librabbitmq-devel

#mysql
sudo yum -y install mysql-devel

#protobuf
sudo yum install -y protobuf*

#gtest
sudo yum install -y gtest-devel

#json-c
sudo yum install -y json-c-devel

#redis
sudo yum install -y hiredis-devel

#curl
sudo yum install -y curl-devel

#zookeeper
sudo yum install -y ../thirdparty/rpms/zookeeper-lib-3.4.4-1.x86_64.rpm 

#rpmbuild
sudo yum install -y rpm-build

#rpm-config
sudo yum install -y redhat-rpm-config

#sqlite
sudo yum install -y sqlite-devel

#mysql c++
sudo yum install -y ../thirdparty/rpms/mysql-connector-c++-1.1.6-linux-el6-x86-64bit.rpm

#log4cxx
sudo yum install -y ../thirdparty/rpms/log4cxx-0.10.0-16.el6.x86_64.rpm ../thirdparty/rpms/log4cxx-devel-0.10.0-16.el6.x86_64.rpm
