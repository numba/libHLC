CXX=g++
LLVMCONFIG=~/dev/HLC-HSAIL-Development-LLVM/build/bin/llvm-config
CXXFLAGS=`$(LLVMCONFIG) --cxxflags`
LDFLAGS=`$(LLVMCONFIG) --system-libs --ldflags --libs all` -lhsail -lLLVMHSAILUtil

all:
	$(CXX) $(CXXFLAGS) -shared -o libHLC.so test.cpp $(LDFLAGS)
