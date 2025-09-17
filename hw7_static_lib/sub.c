#include <stdio.h>
#include "sub.h"

int Sub() {
    int a, b;
    printf("Введите два целых числа: ");
    scanf("%d %d", &a, &b);
    return a - b;
}