from ctypes import *
from llvmlite import binding as llvm

hlc = CDLL('libHLC.so')
hlc.HLC_Initialize()

with open("test_input.ll") as fin:
    modtext = fin.read()

# print(modtext)
hlc.HLC_ParseModule.restype = c_void_p

opt_llvm = c_char_p(0)
buf = create_string_buffer(modtext.encode("ascii"))

module = hlc.HLC_ParseModule(buf)

hlc.HLC_ModulePrint(module, byref(opt_llvm))
print(opt_llvm.value.decode('ascii'))

hlc.HLC_ModuleOptimize(module)

hlc.HLC_ModulePrint(module, byref(opt_llvm))
print(opt_llvm.value.decode('ascii'))



bufempty = create_string_buffer(r"""
; ModuleID = "empty"
target triple = "hsail64-pc-unknown-amdopencl"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n32"


""".encode("ascii"))

empty = hlc.HLC_ParseModule(bufempty)

hlc.HLC_ModuleLinkIn(empty, module)

hsail = c_char_p(0)
hlc.HLC_ModuleEmitHSAIL(empty, byref(hsail))
print(hsail.value.decode('ascii'))

brigptr = c_void_p(0)

hlc.HLC_ModuleEmitBRIG.restype = c_size_t
size = hlc.HLC_ModuleEmitBRIG(empty, byref(brigptr))
print(size)
brig = (c_byte * size).from_address(brigptr.value)
print(bytes(brig))

print('ok')
