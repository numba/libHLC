from ctypes import *
from hlc import HLC


hlc = HLC()

empty_mod = hlc.parse_assembly(r"""
; ModuleID = "empty"
target triple = "hsail64-pc-unknown-amdopencl"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n32"
""")

with open("test_input.ll", 'rb') as fin:
    mod_buf = fin.read()

module = hlc.parse_assembly(mod_buf)
hlc.optimize(module)

hlc.link(module, empty_mod)

hlc.optimize(module)

print(hlc.to_string(module))

print(hlc.to_hsail(module))
print(hlc.to_brig(module))

hlc.destroy_module(module)
hlc.destroy_module(empty_mod)

print('ok')
