#!/bin/bash

echo "Init build environment..."

# epel
sudo yum -y install epel-release

# nux-dextop
sudo yum install -y http://li.nux.ro/download/nux/dextop/el7/x86_64/nux-dextop-release-0-5.el7.nux.noarch.rpm

# yum-utils
sudo yum -y install yum-utils

# development tools
sudo yum groupinstall -y "Development Tools"

# nettools
sudo yum install -y net-tools

#ICE 3.6
sudo rpm --import https://zeroc.com/download/GPG-KEY-zeroc-release
sudo wget -O /etc/yum.repos.d/zeroc-ice-el7.repo https://zeroc.com/download/rpm/zeroc-ice-el7.repo
sudo yum install -y ice-all-runtime ice-all-devel

# erlang repo
sudo yum install -y http://packages.erlang-solutions.com/erlang-solutions-1.0-1.noarch.rpm

# erlang packages
#sudo yum install -y erlang-\*

#DB53
sudo yum install -y db53 db53-devel 

#mcpp
sudo yum install -y mcpp-devel

#bzip2
sudo yum install -y bzip2-devel

#expat
sudo yum install -y expat-devel

#cmake
sudo yum install -y cmake

#boost
sudo yum install -y boost-*

#couchbase
sudo ./couchbase.exp

#rabbitmq
#sudo yum install -y librabbitmq-devel

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

#mysql c++
sudo yum install -y ../thirdparty/rpms/mysql-connector-c++-1.1.7-linux-el7-x86-64bit.rpm

#rpmbuild
sudo yum install -y rpm-build

#rpm-config
sudo yum install -y redhat-rpm-config

#sqlite
sudo yum install -y sqlite2-devel

echo "Finished to set up build environment."
