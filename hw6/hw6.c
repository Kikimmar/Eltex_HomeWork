#include <stdio.h>
#include "struct.h"

int main() {
	struct Node* list = NULL;
	int choice;

	do {
		PrintMenu();
		scanf("%d", &choice);

		switch(choice) {

		case 1:
			//добавление абонента
			Append(&list);
			break;
		case 2:
			//удаление абонента
			break;
		case 3:
			//поиск абонента по имени
			break;
		case 4:
			//вывод всех записей
			ShowAll(list);
			break;
		case 5:
			//выход
			break;
		default:
			printf("Вы ввели неверное значение, попробуйте еще.\n");

		}
	} while (choice != 5);

	return 0;
}
