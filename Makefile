CLANG=clang-3.8
OPT=opt-3.8
CXX=clang++-3.8
LLVMDIS=llvm-dis-3.8
LLVMCONFIG=llvm-config-3.8
CFLAGS += -Wall -fno-rtti -fcolor-diagnostics
OUT = test

all: bc pass

bc: test.bc

pass: Ubouch.so

%.bc: %.c
	$(CLANG) -emit-llvm -c $< -o $@
	$(LLVMDIS) $@

%.so: %.cc
	$(CXX) -fPIC -shared $< -o $@ -std=c++11 `$(LLVMCONFIG) --cxxflags` $(CFLAGS)

check:
	$(OPT) -load ./Ubouch.so -ubouch test.bc -o out.bc
	$(CXX) -o test out.bc 


# 	read x
# 	@echo === ORIGINAL ===================
# 	@cat test.ll
# 	read x
# 	@echo === OPTIMIZED ===================
# 	@$(LLVMDIS) out.bc
# 	@cat out.ll
# 	@rm out.ll

clean:
	rm -rf *.so *.bc *.ll $(OUT)
