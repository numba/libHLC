from ctypes import *
from llvmlite import binding as llvm

hlc = CDLL('libHLC.so')
hlc.HLC_Initialize()

with open("test_input.ll") as fin:
    modtext = fin.read()

# print(modtext)


opt_llvm = c_char_p(0)
buf = create_string_buffer(modtext.encode("ascii"))


# Emit Optimize
print(hlc.HLC_Optimize(buf, byref(opt_llvm)))

print(opt_llvm.value.decode("ascii"))


bufempty = create_string_buffer(r"""
; ModuleID = "empty"
target triple = "hsail64-pc-unknown-amdopencl"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n32"


""".encode("ascii"))


linked_llvm = c_char_p(0)

hlc.HLC_LinkModules(opt_llvm, bufempty, byref(linked_llvm))

print(linked_llvm.value.decode("ascii"))

# Emit HSAIL
emit_hsail = c_char_p(0)
print(hlc.HLC_EmitHSAIL(opt_llvm, byref(emit_hsail)))

print(emit_hsail.value.decode("ascii"))

hlc.HLC_Finalize()

print("ok")
