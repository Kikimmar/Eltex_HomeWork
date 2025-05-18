#include <stdio.h>



int searchName(char* s1, char* s2) {
	int i = 0;
	while (s1[i] != '\0' || s2[i] != '\0') {
		if (s1[i] != s2[i]) return 0;
		i++;
	}
	return 1;
}

void printMenu() {
	printf("\nMenu: \n");
	printf("1. Добавить абонента\n");
	printf("2. Удалить абонента\n");
	printf("3. Поиск абонентов по имени\n");
	printf("4. Вывод всех записей\n");
	printf("5. Выход\n");
	printf("Ваш выбор: ");
}

