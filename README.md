# ubouch

Undefined behavior, ouch

A common example of something a compiler is permitted to do when it encounters
undefined behavior is "deleting your hard drive". Ubouch is a LLVM pass which
implements this for local uses of uninitialized primitive stack variables.

## demo

(slightly out of date)

[![asciicast](https://asciinema.org/a/1mxq4b0kfdx48djx6wk7mg8d9.png)](https://asciinema.org/a/1mxq4b0kfdx48djx6wk7mg8d9)

## building

Install LLVM 3.8.

To build the test program and pass.

```
make
```

To run the pass on the test program.

```
make check
```

## misc

- "SURE" means there is a code path between allocation of a stack variable, and
  a read of it, before any writes to it
- "UNSURE" means that in between the code path between allocation of a stack
  variable, and a read of it, a pointer to that variable is passed into at
  least one function
- in some cases, "SURE" warnings might be misclassified as "UNSURE". this
  occurs when the analysis encounters an "UNSURE" path before a "SURE"
  path involving the same variable and memory read

