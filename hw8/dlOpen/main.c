#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>


void PrintMenu();

int main() {
    int choise;

    void* handle = dlopen("./libcalc.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        return 1;
    }
    dlerror();

    do {
        PrintMenu();
        scanf("%d", &choise);

        switch (choise)
        {
            case 1: {
                int (*Add_func)() = dlsym(handle, "Add");
                char* error = dlerror(); 
                if (error != NULL) {
                    fprintf(stderr, "dlsym error: %s\n", error);
                    dlclose(handle);
                    return 1;
                }
                printf("Результат: %d", Add_func());
                break;
            }
            case 2: {
                int (*Sub_func)() = dlsym(handle, "Sub");
                char* error = dlerror(); 
                if (error != NULL) {
                    fprintf(stderr, "dlsym error: %s\n", error);
                    dlclose(handle);
                    return 1;
                }            
                printf("Результат: %d", Sub_func());
                break;
            }
            case 3: {
                int (*Mul_func)() = dlsym(handle, "Mul");
                char* error = dlerror(); 
                if (error != NULL) {
                    fprintf(stderr, "dlsym error: %s\n", error);
                    dlclose(handle);
                    return 1;
                }     
                printf("Результат: %d", Mul_func());
                break;
            }
            case 4: {
                int (*Div_func)() = dlsym(handle, "Div");
                char* error = dlerror(); 
                if (error != NULL) {
                    fprintf(stderr, "dlsym error: %s\n", error);
                    dlclose(handle);
                    return 1;
                }    
                printf("Результат: %d", Div_func());
                break;        
            }
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