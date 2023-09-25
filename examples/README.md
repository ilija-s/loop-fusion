# Folder containing the example code

## Run

Script `generate_ll_files.sh` will create `.ll` files for all of the `.cpp` files inside of this directory.

Script `run_loop_fusion.sh` will run the optimization on all `.ll` files, then take the optimized `.ll` files,
compile them, run them and write their outputs.

```shell
$ chmod +x generate_ll_files.sh
$ chmod +x run_loop_fusion.sh
$ ./generate_ll_files.sh
$ ./run_loop_fusion.sh
```

## Examples

### How does the fusion algorithm work?

The algorithm consist of six key steps:

1) Moving instructions from the L2 Exit block to L1 Preheader
  ![image](https://github.com/ilija-s/loop-fusion/assets/46342896/d5d4e01f-94f3-489b-9ce3-a70fe4ae3248)
2) Redirecting edge from L1 Header to point to L2 Exit block
  ![image](https://github.com/ilija-s/loop-fusion/assets/46342896/8d2aaa0c-7f7e-491f-baf8-a5164db5fae0)
3) Deleting L1 Exit block
  ![image](https://github.com/ilija-s/loop-fusion/assets/46342896/9d618fb8-3117-4546-b390-48e3a436b761)
4) Redirecting edge from L1 Latch to point to L2 Header and L2 Latch to point to L1 Header
  ![image](https://github.com/ilija-s/loop-fusion/assets/46342896/cd51ea7e-b330-4cf3-b094-500f0b17d4bc)
5) Merging L1 Latch into L2 Latch and Merging L2 Header into L1 Latch
  ![image](https://github.com/ilija-s/loop-fusion/assets/46342896/f140d9dc-3edf-4ee6-8be2-204358bb5c79)
6) Merging L2 into L1
  ![image](https://github.com/ilija-s/loop-fusion/assets/46342896/3be386cd-f2e8-4fc8-9961-ab27f3edf384)

#### Control-flow graph example

Loop fusion for `test_fusable_2.cpp`:

![image](https://github.com/ilija-s/loop-fusion/assets/46342896/352aed3f-ce3e-4161-b624-24455a5c2c84)

![image](https://github.com/ilija-s/loop-fusion/assets/46342896/5eaaad8d-3705-480e-bf14-eaa77fd3033e)

![image](https://github.com/ilija-s/loop-fusion/assets/46342896/04f8ea9e-0a4b-4c90-924c-40fd47e70f3a)

![image](https://github.com/ilija-s/loop-fusion/assets/46342896/80e8e5a4-0c85-4c54-a38b-a7f97d762203)

![image](https://github.com/ilija-s/loop-fusion/assets/46342896/01519ef7-a0a3-4c28-9fb2-21665af62d9b)

![image](https://github.com/ilija-s/loop-fusion/assets/46342896/45ecc20f-4114-42dd-800b-0954f54ee8e5)

### Other examples

#### `test_fusable_1.cpp`

![opt_test_fusable_1](https://github.com/ilija-s/loop-fusion/assets/46342896/c73105c8-cc9e-4bf7-9e88-0824a4ba9812)

#### `test_fusable_4.cpp`

![opt_test_fusable_4](https://github.com/ilija-s/loop-fusion/assets/46342896/46b34103-aa76-496a-9a6f-bf8023134e53)

#### `test_fusable_5.cpp`

![opt_test_fusable_5](https://github.com/ilija-s/loop-fusion/assets/46342896/fa60656a-0a95-42bb-952e-6e73cf8327c5)

#### `test_fusable_6.cpp`

![opt_test_fusable_6](https://github.com/ilija-s/loop-fusion/assets/46342896/37d715d6-6e14-4786-b4c3-04de05fb5ca4)
