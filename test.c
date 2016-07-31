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

// this code causes a bug where an "UNSURE" is reported, but there is an
// alternate path the is a "SURE". this may occur when the graph traversal
// encounters a call -> load before the alternative path which goes straight
// from alloca to load. this is fundamentally caused by the fact that the
// traversal visits each basic block exactly once.
void paths_mislabeling(void)
{
    int x;
    int y = 0;

    if (y == 1) {
        int z;
    } else {
        dummy(&x);
    }

    y += x;
}

int main(int argc, const char **argv)
{
    return 0;
}
