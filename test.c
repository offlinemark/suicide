#include <stdio.h>

void dummy(int *x) {}

void func(void)
{
    int x;
    int y = x + 1;
    puts("harmless function");
}

void func2(void)
{
    int x;
    int z = x;
    dummy(&x);
    int y = x + 1;
}

int main(int argc, const char **argv)
{
    func();
    return 0;
}
