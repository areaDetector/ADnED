#!/bin/bash

# Create an example TOF transformation file for 1024 pixel IDs.

echo 1024 > tof.trans

for i in `seq 0 1023`
do
    val=`echo "$RANDOM%100+1" | bc`
    result=`echo "scale=6; 1/$val" | bc`
    echo $result >> tof.trans
done

echo Done.