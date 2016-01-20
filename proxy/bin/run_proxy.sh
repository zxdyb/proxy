#!/bin/bash 
if [ $# -lt 1 ]
then
    echo "usage: $0 portnumber";
    exit;
fi

td=2;
if [ $# -eq 2 ]
then
    td=$2;
fi

#echo $#
#echo $2
#echo $td

ulimit -HSn 65536
ulimit -c unlimited
chmod a+x ./proxy
setsid ./proxy $1 $td > /dev/null &
pidof proxy
