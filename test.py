from ctypes import *
from llvmlite import binding as llvm

hlc = CDLL('libHLC.so')
hlc.HLC_Initialize()

with open("test_input.ll") as fin:
    modtext = fin.read()

# print(modtext)

opt_llvm = c_char_p(0)
buf = create_string_buffer(modtext.encode("ascii"))

print(hlc.HLC_Optimize(buf, byref(opt_llvm)))

print(opt_llvm.value.decode("ascii"))

emit_hsail = c_char_p(0)
print(hlc.HLC_EmitHSAIL(opt_llvm, byref(emit_hsail)))

print(emit_hsail.value.decode("ascii"))

hlc.HLC_Finalize()

print("ok")
