from ctypes import (c_size_t, byref, c_char_p, c_void_p, Structure, CDLL,
                    POINTER, create_string_buffer)
from llvmlite import binding as llvm

class OpaqueModuleRef(Structure):
    pass

moduleref_ptr = POINTER(OpaqueModuleRef)

hlc = CDLL('libHLC.so')
hlc.HLC_ParseModule.restype = moduleref_ptr
hlc.HLC_ModuleEmitBRIG.restype = c_size_t


class Error(Exception):
    pass


class HLC(object):
    _singleton = None

    def __new__(cls):
        if cls._singleton is None:
            cls._singleton = object.__new__(cls)
        return cls._singleton

    def __init__(self):
        hlc.HLC_Initialize()

    def __del__(self):
        hlc.HLC_Finalize()

    def parse_assembly(self, ir):
        if isinstance(ir, str):
            ir = ir.encode("latin1")
        buf = create_string_buffer(ir)
        mod = hlc.HLC_ParseModule(buf)
        if not mod:
            raise Error("Failed to parse assembly")
        return mod

    def optimize(self, mod, opt=3, size=0, verify=1):
        buf = c_char_p(0)
        if not hlc.HLC_ModuleOptimize(mod, int(opt), int(size), int(verify)):
            raise Error("Failed to optimize module")

    def link(self, dst, src):
        if not hlc.HLC_ModuleLinkIn(dst, src):
            raise Error("Failed to link modules")

    def to_hsail(self, mod, opt=3):
        buf = c_char_p(0)
        if not hlc.HLC_ModuleEmitHSAIL(mod, int(opt), byref(buf)):
            raise Error("Failed to emit HSAIL")
        ret = buf.value.decode("latin1")
        hlc.HLC_DisposeString(buf)
        return ret

    def to_brig(self, mod, opt=3):
        bufptr = c_void_p(0)
        size = hlc.HLC_ModuleEmitBRIG(mod, int(opt), byref(bufptr))
        if not size:
            raise Error("Failed to emit BRIG")
        buf = (c_char_p * size).from_address(bufptr.value)
        ret = bytes(buf)
        hlc.HLC_DisposeString(buf)
        return ret

    def to_string(self, mod):
        buf = c_char_p(0)
        if not hlc.HLC_ModulePrint(mod, byref(buf)):
            raise Error("Failed to print module")
        ret = buf.value.decode("latin1")
        hlc.HLC_DisposeString(buf)
        return ret

    def destroy_module(self, mod):
        hlc.HLC_ModuleDestroy(mod)
