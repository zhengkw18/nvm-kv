#!/bin/bash

bench=('bench.cc' 'bench_range.cc' 'bench_snapshot.cc')

rm -rf /tmp/ramdisk/data/test-*
for f in ${bench[@]}; do
    exe=$(echo $f | cut -d . -f1)
    echo $f
    g++ -std=c++11 -o $exe -g -I.. $f  -L../lib -lengine -lpthread -DMOCK_NVM
done
