#include <stdio.h>

void dummy(int *x) {}

void simple(void)
{
    int x;
    int y = x + 1;
}

void unsure(void)
{
    int x;
    int z = x;
    dummy(&x);
    int y = x + 1;
}

void paths(void)
{
    int x;
    int y = 0;

    while (y == 1) {
        x = 3;
    }

    y += x;
}

void paths_unsure(void)
{
    int x;
    int y = 0;

    while (y == 1) {
        dummy(&x); 
    }

    y += x;
}

int main(int argc, const char **argv)
{
    return 0;
}
