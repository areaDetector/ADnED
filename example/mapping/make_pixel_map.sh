#!/bin/bash

# Create an example pixel mapping file.

echo 1024 > pixel.map

for i in `seq 0 1023`
do
    echo $i >> pixel.map
done

echo Done.