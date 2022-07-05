#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    // Test out dereferencing a null pointer
    char* mybuf = ((void*)0);
    write(1, mybuf, 8096);

    int a, b;
    int *pi;
    a = 5;
    pi = &a;
    a = *pi;
    pi = ((void*)0);
    b = *pi;
    printf(1, "Null Pointer value %p\n", *pi);
    printf(1, "value of b %d", b);
    exit();
}