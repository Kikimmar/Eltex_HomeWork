#include <stdio.h>
#include "func.h"

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

void addAbonent(int* count, struct abonent* call_book) {
	if((*count) >= 100) {
		printf("Справочник переполнен!\n");
		return;
	}

	printf("=========================\n");
	printf("Введите данные абонента.\n");
	printf("Имя: ");
	scanf("%s", call_book[*count].name);
	getchar();

	printf("Фамилия: ");
	scanf("%s", call_book[*count].second_name);
	getchar();

	printf("Телефон: ");
	scanf("%s", call_book[*count].tel);

	(*count)++;
}

void deleteAbonent(int* count, struct abonent* call_book) {
	int delete_number;
				printf("Введите номер удаляемого абонента: ");
				scanf("%d", &delete_number);
				if (delete_number <= 0 || delete_number > (*count)) {
					printf("Неверный номер!\n");
					return;
				}

				for (int i = 0; i < 10; i++) {
					call_book[delete_number - 1].name[i] = '\0';
					call_book[delete_number - 1].second_name[i] = '\0';
					call_book[delete_number - 1].tel[i] = '\0';
				}

				for (int i = delete_number - 1; i < (*count); i++)
					call_book[i] = call_book[i + 1];

				(*count)--;
}

void findAbonent(int* count, struct abonent* call_book) {
	char find[10];
				printf("Введите имя абонента: ");
				scanf("%9s", find);
				getchar();

				printf("\nРезультат поиска: \n");
				int found = 0;

				for (int i = 0; i < (*count); i++) {
					if (searchName(call_book[i].name, find)) {
						printf("# %d\tИмя:  %s\tФамилия: %s\tТел: %s\n", 
							i + 1,
							call_book[i].name,
							call_book[i].second_name,
							call_book[i].tel);
						found = 1;
					}
				}
				if (!found) printf("Ничего не найдено.\n");
}

void showAllAbonent(int count ,struct abonent* call_book) {
					printf("\nВсе абоненты: \n");
				for (int i = 0; i < count; i++){
					printf("# %d\tName: %s\tSecond Name: %s\tTel: %s\n",
						i + 1,
						call_book[i].name, 
						call_book[i].second_name,
						call_book[i].tel);
				}
}