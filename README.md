# Loop Fusion

Implementation of a [loop fusion](https://en.wikipedia.org/wiki/Loop_fission_and_fusion) transform pass within LLVM.

## Build

```shell
mkdir build
cd build
cmake ..
make
cd ..
```

## Run

```shell
clang -S -emit-llvm <filename>.cpp
opt -load build/LoopFusion/libLoopFusion.so -loopfusion --enable-new-pm=0 -S <filename>.ll
```
