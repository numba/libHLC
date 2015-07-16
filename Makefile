CXX=g++
LLVMCONFIG=~/dev/HLC-HSAIL-Development-LLVM/build/bin/llvm-config
CXXFLAGS=`$(LLVMCONFIG) --cxxflags`
LDFLAGS=`$(LLVMCONFIG) --system-libs --ldflags --libs`

all:
	$(CXX) $(CXXFLAGS) -o test test.cpp $(LDFLAGS)


