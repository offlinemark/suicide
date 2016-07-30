CLANG=clang-3.8
OPT=opt-3.8
CXX=clang++-3.8
LLVMDIS=llvm-dis-3.8
LLVMCONFIG=llvm-config-3.8
CFLAGS += -Wall -fno-rtti
OUT = test

all: bc ll pass

bc: test.bc

ll: test.ll

pass: Ubouch.so

%.bc: %.c
	$(CLANG) -emit-llvm -c $< -o $@

%.ll: %.bc
	$(LLVMDIS) $<

%.so: %.cc
	$(CXX) -fPIC -shared $< -o $@ -std=c++11 `$(LLVMCONFIG) --cppflags` $(CFLAGS)

check:
	$(OPT) -load ./Ubouch.so -ubouch test.bc -o out.bc
	$(CXX) -o test out.bc
	read x
	@echo === ORIGINAL ===================
	@cat test.ll
	read x
	@echo === OPTIMIZED ===================
	@$(LLVMDIS) out.bc
	@cat out.ll
	@rm out.ll

clean:
	rm -rf *.so *.bc *.ll $(OUT)
