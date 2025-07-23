#include <stdio.h>
#include "add.h"

int Add() {
    int a, b;
    printf("Введите два целых числа: ");
    scanf("%d %d", &a, &b);
    return a + b;
}