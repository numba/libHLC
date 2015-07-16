from ctypes import *
from llvmlite import binding as llvm

hlc = CDLL('libHLC.so')
hlc.HLC_Initialize()

with open("test_input.ll") as fin:
    modtext = fin.read()

# print(modtext)

out = c_char_p(0)
buf = create_string_buffer(modtext.encode("ascii"))

print(hlc.HLC_Optimize(buf, byref(out)))

print(out.value.decode("ascii"))

hlc.HLC_Finalize()

print("ok")
