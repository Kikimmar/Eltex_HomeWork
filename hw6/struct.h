#ifndef STRUCT_H
#define STRUCT_H

struct Abonent {
	char name[10];
	char second_name[10];
	char tel[10];
};

struct Node {
	struct Abonent data;
	struct Node* prev;
	struct Node* next;
};

void PrintMenu();
struct Node* CreateNode();
void Append(struct Node** head_ref);
void DeleteAbonent(struct Node** head_ref);
void SearchByName(struct Node* head);
void ShowAll(struct Node* head);
void FreeList(struct Node* head);
void ExitProgram(struct Node** head_ref);

#endif
