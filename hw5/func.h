#ifndef FUNC_H
#define FUNC_H

struct abonent {
	char name[10];
	char second_name[10];
	char tel[10];
};

int searchName(char* s1, char* s2);
void printMenu();
void addAbonent(int* count, struct abonent* call_book);
void deleteAbonent(int* count, struct abonent* call_book);
void findAbonent(int* count, struct abonent* call_book);
void showAllAbonent(int count ,struct abonent* call_book);

#endif
