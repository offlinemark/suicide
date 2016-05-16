#include <stdio.h>

/* void dummy(int *x) {} */

/* void func(void) */
/* { */
/*     int x; */
/*     int y = x + 1; */
/*     puts("harmless function"); */
/* } */

/* void func2(void) */
/* { */
/*     int x; */
/*     int z = x; */
/*     dummy(&x); */
/*     int y = x + 1; */
/* } */

void fun(void)
{
    int x = 0;
    int y;

    if (x == 1)
        y = 3;

    x += y;
}

int main(int argc, const char **argv)
{
    fun();
    return 0;
}
