#!/bin/bash

shopt -s nullglob
for file in *.ll; do
  opt -load ../build/LoopFusion/libLoopFusion.so -loopfusion -enable-new-pm=0 -S $file > "opt_$file"
  llc "opt_$file" -o test.s
  clang -c test.s -o test.o
  clang test.o -o test
  ./test
  echo "OUTPUT for $file: $?"
  rm -f $file test.s test.o test
done
