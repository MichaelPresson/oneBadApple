#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {

	int k;

	printf("Enter number of proesses: \n");
	if (scanf("%d", &k) != 1 || k < 1) {
        	fprintf(stderr, "Invalid input! Please enter a positive integer.\n");
        	return 1;
    	}
	printf("%d\n", k);
	return 0;

}
