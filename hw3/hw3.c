// 1. Поменять в целом положительном числе (типа int) значение третьего байта
// на введенное пользователем число (изначально число так же вводится с клавиатуры)
// через указатель (не применяя битовые операции).

#include <stdio.h>

int main(){
	int num;
	unsigned char new_byte;

	printf("Введите целое положительное число: ");
	scanf("%d", &num);

	printf("Введите новое значение третьего байта (0-255): ");
	scanf("%hhu", &new_byte);

	unsigned char* byte_ptr = (unsigned char *)&num;
	byte_ptr[2] = new_byte;

	printf("Измененное число: %d\n", num);

	return 0;
}
