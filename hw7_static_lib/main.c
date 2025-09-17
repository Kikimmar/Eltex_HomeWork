#include <stdio.h>
#include "add.h"
#include "sub.h"
#include "mul.h"
#include "div.h"

void PrintMenu();

int main() {
    int choise;
    do {
        PrintMenu();
        scanf("%d", &choise);

        switch (choise)
        {
            case 1:
                printf("Результат: %d", Add());
                break;
            case 2:
                printf("Результат: %d", Sub());
                break;
            case 3:
                printf("Результат: %d", Mul());
                break;
            case 4:
                printf("Результат: %d", Div());
                break;        
            case 5:
                printf("Выход из программы...\n");
                break;
            default:
                printf("\nВы ввели неверное значение, попробуйте еще.\n");
                break;
        }

    } while (choise != 5);
    
}

void PrintMenu() {
    printf("\nMenu: \n");
	printf("1. Сложение\n");
	printf("2. Вычитание\n");
	printf("3. Умножение\n");
	printf("4. Деление\n");
	printf("5. Выход\n");
	printf("Ваш выбор: ");
}