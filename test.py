from ctypes import *

hlc = CDLL('libHLC.so')
hlc.libHLC_Initialize()
hlc.libHLC_Finalize()
