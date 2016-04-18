# ubouch

Undefined behavior, ouch

A common example of something a compiler is permitted to do when it encounters
undefined behavior is "deleting your hard drive". Ubouch is a LLVM pass which
implements this.

## demo

[![asciicast](https://asciinema.org/a/1mxq4b0kfdx48djx6wk7mg8d9.png)](https://asciinema.org/a/1mxq4b0kfdx48djx6wk7mg8d9)

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
