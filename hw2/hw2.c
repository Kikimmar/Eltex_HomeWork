#include <stdio.h>

#define N 5

void firstTask();
void secondTask();
void thirdTask();
void fourthTask();

int main() {
	// 1. Вывести квадратную матрицу по заданому N.
		
	firstTask();

	// 2. Вывести заданный массив размером N
	// в обратном порядке. 

	secondTask();

	// 3. Заполнить верхний треугольник матрицы 1, а 
	// нижний 0.	

	thirdTask();

	// 4. Заполнить матрицу числами от 1 до N^2 улиткой.
	
	fourthTask();

	return 0;

}

void firstTask() {
	int value = 1;
	int arr[N][N];
	printf("Размерность матрицы N = %d\n", N);
	printf("Квадратная матрица: \n");
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			arr[i][j] = value;
			printf("%3d", arr[i][j]);
			value++;
		}
		printf("\n");
	}
	printf("\n");
}

void secondTask() {
        int arr[N];
        printf("Введите числа от 0 до %d: \n", N);
        for (int i = 0; i < N; i++)
		scanf("%d", &arr[i]);
        
        for (int i = 0; i < N/2; i++) {
                int temp = arr[i];
                arr[i]= arr[N - 1 - i];
                arr[N - 1 - i] = temp;
        }

        for (int i = 0; i < N; i++)
  		printf("%d ", arr[i]);
        printf("\n\n");
}

void thirdTask() {
	int arr[N][N];
	printf("Матрица заполненная \"1\" в верхнем треугольнике: \n");
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			arr[i][j] = (i + j >= N - 1) ? 0 : 1;
			printf("%d ", arr[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

void fourthTask() {
	int matrix[N][N];
	int value = 1;
	int rowStart = 0, rowEnd = N - 1;
	int colStart = 0, colEnd = N - 1;

	while (rowStart <= rowEnd && colStart <= colEnd){
		// Верхняя строка слева направо
		for (int i = colStart; i <= colEnd; i++)
			matrix[rowStart][i] = value++;
		rowStart++;

		// Правый столбец сверху вниз
		for (int i = rowStart; i <= rowEnd; i++)
			matrix[i][colEnd] = value++;
		colEnd--;

		// Нижняя строка справа налево
		if  (rowStart <= rowEnd) {
			for (int i = colEnd; i >= colStart; i--)
				matrix[rowEnd][i] = value++;
			rowEnd--;
		}

		// Левый стобец снизу вверх
		if (colStart <= colEnd) {
			for (int i = rowEnd; i >= rowStart; i--)
				matrix[i][colStart] = value++;
			colStart++;
		}
	}

	printf("Матрица-улитка:\n");
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++)
			printf("%3d ", matrix[i][j]);
		printf("\n");
	}
}
