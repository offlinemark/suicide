# ubouch

Undefined behavior, ouch

A common example of something a compiler is permitted to do when it encounters
undefined behavior is "deleting your hard drive". Ubouch is a LLVM pass which
implements this.

## building

Install LLVM 3.7.

To build the test program and pass.

```
make
```

To run the pass on the test program.

```
make check
```
