#ifndef FUNC_H
#define FUNC_H

struct abonent {
	char name[10];
	char second_name[10];
	char tel[10];
};

int searchName(char* s1, char* s2);
void printMenu();

#endif