# suicide cc

A common illustrative example of something undefined behavior in C can do
is "format your hard drive". Suicide CC is a
LLVM pass which implements this. When compiling with this pass enabled,
each function is checked for 1 type of undefined behavior. If it is found,
the pass emits code which executes an arbitrary shell command using `system()`.

The scope of detected undefined behavior is simply local uses of uninitialized
stack variables of primitive type. It should support most POSIX like operating
systems.

This project is named for its similarity to [Suicide
Linux](https://qntm.org/suicide).

> Note: For the safety of my dev machine, the pass in this source tree is
declawed such that it will only emit code to delete a very specific file from
the system executing the program.  However, to achieve the fully advertised
functionality, simply reassign `Suicide::SYSTEM_CMD` in `Suicide.cc` to your
choice of hard drive erasing shell command.

## demo

(slightly out of date)

[![asciicast](https://asciinema.org/a/42649.png)](https://asciinema.org/a/42649)

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
  occurs when the analysis encounters an "UNSURE" path before a "SURE" path
  involving the same variable and memory read
