// 4. Напишите программу, которая ищет в введенной строке (с клавиатуры)
//  введеную подстроку (с клавиатуры) и возвращает указатель на 
//  начало подстроки, если подстрока не найдена в указатель 
//  записывается NULL. В качестве строк использовать 
//  статические массивы.

#include <stdio.h>

char* find_substr(const char* str, const char* sub) {
	for (int i = 0; str[i]; i++) {
		for (int j = 0; ; j++) {
			if (!sub[j]) return (char*)&str[i];
			if (str[i + j] != sub[j]) break;
		}
	}
	return NULL;
}

int main(){
	char str[80];
	char buffer[80];

	printf("Введите первоначальную строку: ");
	scanf("%99[^\n]", str);
	getchar();

	printf("Введите искомую подстроку: ");
	scanf("%99[^\n]", buffer);
	getchar();

	char* result = find_substr(str, buffer);

	if (result) printf("Позиция: %ld\n", result - str);
	else printf("Не найдено\n");

	return 0;
}
