#include <stdio.h>

void func(void)
{
    int x;
    int y = x + 1;
    puts("harmless function");
}

int main(int argc, const char **argv)
{
    func();
    return 0;
}
