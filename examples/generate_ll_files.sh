#!/bin/bash

shopt -s nullglob

for file in *.ll; do
  rm $file
done

for file in *.cpp; do
  clang -S -emit-llvm -Xclang -disable-O0-optnone -fno-discard-value-names $file
done
