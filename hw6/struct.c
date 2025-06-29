#include <stdio.h>
#include <stdlib.h>

#include "struct.h"

void PrintMenu() {
		printf("\nMenu: \n");
	printf("1. Добавить абонента\n");
	printf("2. Удалить абонента\n");
	printf("3. Поиск абонентов по имени\n");
	printf("4. Вывод всех записей\n");
	printf("5. Выход\n");
	printf("Ваш выбор: ");
}

struct Node* CreateNode() {
		struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
		if (!new_node) {
			printf("Ошибка выделения памяти!\n");
			exit(1);
		}

		printf("Введите данные абонента.\n");
		printf("Имя: ");
		scanf("%9s", new_node->data.name);
		printf("Введите фамилию: \n");
		scanf("%9s", new_node->data.second_name);
		printf("Введите номер телефона: \n");
		scanf("%9s", new_node->data.tel);
		new_node->prev = NULL;
		new_node->next = NULL;

		return new_node;
}

void Append(struct Node** head_ref) {
	struct Node* new_node = CreateNode();

	if (*head_ref == NULL) {
		*head_ref = new_node;
		return;
	}
	
	struct Node* last = *head_ref;
	while (last->next != NULL) last = last->next;
	last->next = new_node;
	new_node->prev = last;
}

		


