#include <stdio.h>

void dummy(int *x) {}

void func(void)
{
    int x;
    int y = x + 1;
}

void func2(void)
{
    int x;
    int z = x;
    dummy(&x);
    int y = x + 1;
}

void fun(void)
{
    int x;
    int y = 0;

    while (y == 1) {
        x = 3;
    }

    y += x;
}

int main(int argc, const char **argv)
{
    fun();
    return 0;
}
