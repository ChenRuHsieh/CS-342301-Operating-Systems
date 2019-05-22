#include<syscall.h>

int main(void)
{
	int i, j;
	for(i=0; i<5; i++){
		for(j=0; j<50; j++);
		PrintInt(i+50);
	}
}
