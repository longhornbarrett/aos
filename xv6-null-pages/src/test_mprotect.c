#include "types.h"
#include "user.h"
int *p;


int main(void)
{
 	p = (int *) sbrk(1);
 	mprotect((void *)p, 1);
    munprotect((void *)p, 1);
 	*p=100;
 	printf(1, "COMPLETED: value is %d, expecting 100!\n", *p);
 	

 	exit();
}
