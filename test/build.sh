#!/bin/bash

test=('single_thread_test.cc' 'multi_thread_test.cc' 'crash_test.cc' 'multi_thread_linearizability.cc' 'range_test.cc' 'snapshot_test.cc')

rm -rf /tmp/ramdisk/data/test-*
for f in ${test[@]}; do
    exe=$(echo $f | cut -d . -f1)
    echo $f
    g++ -std=c++11 -o $exe -g -I.. $f  -L../lib -lengine -lpthread -DMOCK_NVM
done
