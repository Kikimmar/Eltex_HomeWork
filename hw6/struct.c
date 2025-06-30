#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
	
	struct Node* last_node = *head_ref;
	while (last_node->next != NULL) last_node = last_node->next;
	last_node->next = new_node;
	new_node->prev = last_node;
}

void DeleteAbonent(struct Node** head_ref) {
	printf("Введите номер абонента для удаления: ");
   	int index;
    scanf("%d", &index);

if (*head_ref == NULL || index <= 0) {
        printf("Список пуст или неверный индекс.\n");
        return;
    }

    struct Node* current = *head_ref;
    int count = 1;

    // Ищем узел с номером index
    while (current != NULL && count < index) {
        current = current->next;
        count++;
    }

    if (current == NULL) {
        printf("Абонент с таким номером не найден.\n");
        return;
    }

    // Если удаляемый узел — голова списка
    if (current == *head_ref) {
        // Обновляем голову списка на следующий узел
        *head_ref = current->next;
        if (*head_ref != NULL) {
            // Новый голова не имеет предыдущего узла
            (*head_ref)->prev = NULL;
        }
    } else {
        // Обновляем указатель next предыдущего узла
        if (current->prev != NULL)
            current->prev->next = current->next;

        // Обновляем указатель prev следующего узла
        if (current->next != NULL)
            current->next->prev = current->prev;
    }

    free(current);
    printf("Абонент с номером %d удалён.\n", index);
}

void SearchByName(struct Node* head) {
	char search_name[10];
    printf("Введите имя для поиска: ");
    scanf("%9s", search_name);

	 struct Node* current = head;
    int found = 0;
    int index = 1;

    while (current != NULL) {
        if (strcmp(current->data.name, search_name) == 0) {
            printf("Найден абонент #%d: %s %s, Телефон: %s\n",
                   index,
                   current->data.name,
                   current->data.second_name,
                   current->data.tel);
            found = 1;
        }
        current = current->next;
        index++;
    }

    if (!found) {
        printf("Абоненты с именем \"%s\" не найдены.\n", search_name);
    }
}

void ShowAll(struct Node* head) {
	int i = 1;
	while (head != NULL) {
		printf("%d.%s %s %s \n", i, head->data.name, head->data.second_name, head->data.tel);
		head = head->next;
		i++;
	}

}

void FreeList(struct Node* head) {
    struct Node* tmp;
    while (head != NULL) {
        tmp = head;
        head = head->next;
        free(tmp);
    }
}

void ExitProgram(struct Node** head_ref) {
    FreeList(*head_ref);
    *head_ref = NULL;      // Обнуляем указатель на голову
    printf("Память освобождена. Выход из программы.\n");
    exit(0);
}