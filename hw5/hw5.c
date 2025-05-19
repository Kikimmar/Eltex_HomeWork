#include <stdio.h>
#include "func.h"

struct abonent call_book[100];

int main() {

	int choice;
	int count = 0;

	do {
		printMenu();
		scanf("%d", &choice);

		switch(choice) {
			case 1:
				addAbonent(&count, call_book);
				break;
			case 2:
				// Удалить абонента
				deleteAbonent(&count, call_book);
				break;
			case 3:
				// Поиск абонентов по имени
				findAbonent(&count, call_book);
				break;
			case 4:
				// Вывод всех записей
				showAllAbonent(count ,call_book);
				break;
			case 5:
				// Выход
				break;
			default:
				printf("\nВы ввели неверное значение, попробуйте еще.\n");
		}
	} while (choice != 5);

	return 0;
}

