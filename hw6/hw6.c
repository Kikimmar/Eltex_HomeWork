#include <stdio.h>

#include "struct.h"

int main() {
	struct Node* head = NULL;
	int choice;

	do {
		PrintMenu();
		scanf("%d", &choice);

		switch(choice)
		case 1:
			Append(&head);
			break;

	} while (choice != 5);

	return 0;
}
