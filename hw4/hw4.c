#include <stdio.h>

void showMenu();

int main() {

	int choice;

	do {
		showMenu();
		scanf("%d", &choice);

		switch(choice) {
			case 1:
				printf(" ");
				break;
			case 2:
				printf(" ");
				break;
			case 3:
				printf(" ");
				break;
			case 4:
				printf(" ");
				break;
			case 5:
				printf(" ");
				break;
			default:
				printf("\nВы ввели неверное значение, попробуйте еще.\n");
		}
	} while (choice != 0);

	return 0;
}

void showMenu() {
	printf("\nMenu: \n");
	printf("1. Добавить абонента\n");
	printf("2. Удалить абонента\n");
	printf("3. Поиск абонентов по имени\n");
	printf("4. Вывод всех записей\n");
	printf("5. Выход\n");
	printf("Ваш выбор: ");
}
