# Nvidia backend implementation notes

The Nvidia backend for DNNL can be exposed to the user via the `dnnl::engine::kind::gpu` engine kind. Currently, for the case when user's system has both Intel and Nvidia GPUs, 
`DNNL_GPU_VENDOR=NVIDIA` Flag is used in CMake, since the devices are clustered based on the device vendor ID and index pattern can not be used to distinguish between Intel GPU and Nvidia GPU.

## Requirements for Building from source
- oneDNN Nvidia backend requires Intel oneAPI DPC++ Compiler, details of aquiring and installing it is available https://github.com/mehdi-goli/oneDNN/blob/codeplay-onednn-nvidia-support/README.md


The Nvidia backend requires 
- Nvidia CUDA driver version  418.87.01 or 440.33.01
- cuBLAS library version  10.1 or 1.2,  
- cuDNN library version 7.6.5.
- `LD_LIBRARY_PATH` should be set to `/Path/to/dpcpp/lib`

## Commands to run after DPC++ setup

### Setup

```
$ export CXX=/Path/to/dpcpp/bin/clang++
$ export CC=/Path/to/dpcpp/bin/clang
$ export LD_LIBRARY=/Path/to/dpcpp/lib
```

### Build

```
$ mkdir build
$ cd build
$ cmake -DDNNL_CPU_RUNTIME=DPCPP -DDNNL_GPU_RUNTIME=DPCPP -DDNNL_GPU_VENDOR=NVIDIA -G Ninja .. -DOPENCLROOT=/Path/to/OpenCL-ICD-Loader -DOPENCLHEADERS=/Path/to/OpenCL-Headers
$ ninja

```

### Running

```
$ ./run_model.sh
```
