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
