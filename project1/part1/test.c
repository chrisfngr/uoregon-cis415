#include <stdlib.h>
#include <stdio.h>

int main() {
	int joe[5];
	joe[0] = 1;
	joe[1] = 1;
	joe[2] = 1;
	joe[3] = 1;
	joe[4] = 1;
	joe[5] = 10;
	fprintf(stderr, "%d\n", joe[5]);
	
}
