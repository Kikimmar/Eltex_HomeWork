#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    const char* filename = "outputfile.txt";
    const char* text = "String from file";

    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        perror("Ошибка при открытии файла");
        return 1;
    }
    fprintf(file, "%s", text);
    fclose(file);


    file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Ошибка при открытии файла");
        return 1;
    }
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    if (filesize <= 0) {
        printf("Файл пустой или ошибка при определении размера\n");
        fclose(file);
        return 1;
    }

    char* buffer = malloc(filesize + 1);
    if (buffer == NULL) {
        printf ("Ошибка выделения паммяти.");
        fclose(file);
        return 1;
    }

    fseek(file, 0, SEEK_SET);
    fread(buffer, 1, filesize, file);
    buffer[filesize] = '\n';
    fclose(file);

    printf("Строка считанная с конца:\n");
    for (long i = filesize - 1; i >= 0; i--) {
        putchar(buffer[i]);
    }
    putchar('\n');

    free(buffer);
    return 0;
} 