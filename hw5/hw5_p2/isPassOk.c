#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int isPassOk(void);

int main(void) {
	int pwStatus;

	puts("Enter password: ");
	pwStatus = isPassOk();

	if(pwStatus == 0) {
		printf("Bad password!\n");
		exit(1);
	} else {
		printf("Access granted!\n");
	}

	return 0;
}

int isPassOk(void) {
	char Pass[12];
	gets(Pass);
	return 0 == strcmp(Pass, "test");
}
