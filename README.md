# Build Instruction


## Install libHSAIL

Build https://github.com/HSAFoundation/HSAIL-Tools/tree/master/libHSAIL by
folloing the instruction in the README.

Running `make install` in libHSAIL build directory will put some C++ header
files and a static library (libhsail.a) into `/usr/local/lib`.  They will be
used in the LLVM build later.  **NOTE**: Failure to install these files will
not cause any error during LLVM installation.  Instead, it will cause hard
error at runtime.


## Install HLC

Obtain HLC from https://github.com/HSAFoundation/HLC-HSAIL-Development-LLVM .
Build with:

```bash
cmake <hlc_source> -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_RTTI=ON  -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=HSAIL
make -j
```

## Build libHLC

```bash
LLVMCONFIG=<path-to-hlc-llvm-config-binary> conda build condarecipe
```
