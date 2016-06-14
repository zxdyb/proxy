#!/bin/bash 
if [ $# -lt 1 ]
then
    echo "usage: $0 portnumber";
    exit;
fi

td=2;
if [ $# -ge 2 ]
then
    td=$2;
fi

#echo $#
#echo $2
#echo $td

ulimit -HSn 65536
ulimit -c unlimited
chmod a+x ./proxy
if [ $# -le 2 ]
then
  setsid ./proxy $1 $td > /dev/null &
else
  ./proxy $1 $td $3
fi
pidof proxy
