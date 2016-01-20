#!/bin/bash 
if [ $# -ne 1 ]
then
    echo "usage: $0 portnumber";
    exit;
fi

ulimit -HSn 65536
ulimit -c unlimited
chmod a+x ./proxy
./proxy $1
