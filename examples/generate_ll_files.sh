#!/bin/bash

shopt -s nullglob

for file in *.ll; do
  rm $file
done

for file in *.cpp; do
  clang -S -emit-llvm -fno-discard-value-names $file
done
