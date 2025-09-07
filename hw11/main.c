#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h> // для sleep

#define SHOP_COUNT 5
#define BUYER_COUNT 3

int shop[SHOP_COUNT];                                     // Массив магазинов
pthread_mutex_t shop_mutex[SHOP_COUNT];                   // Мьютексы для магазинов
int buyers_finished = 0;                                  // Счётчик завершившихся покупателей
pthread_mutex_t finish_mutex = PTHREAD_MUTEX_INITIALIZER; // Мьютекс для счетчика

typedef struct {
    int id;      // ID покупателя
    int demand;  // Покупательская потребность
} buyer_t;

// Функция потока покупателей
void* buyer_func(void* arg);

// Функция потока погрузчика
void* loader_func(void* arg);

int main() {
    srand(time(NULL));

    // Инициализация магазинов товарами от 400 до 600
    for (int i = 0; i < SHOP_COUNT; i++) 
    {
        shop[i] = rand() % (600 - 400 + 1) + 400;
        pthread_mutex_init(&shop_mutex[i], NULL);
        printf("Магазин %d инициализирован с товаром %d.\n", i + 1, shop[i]);
    }

    pthread_t buyers[BUYER_COUNT];
    pthread_t loader;

    // Инициализируем покупателей
    for (int i = 0; i < BUYER_COUNT; i++) 
    {
        buyer_t* buyer = malloc(sizeof(buyer_t));
        if (buyer == NULL) 
        {
            fprintf(stderr, "Ошибка выделения памяти\n");
            exit(EXIT_FAILURE);
        }
        buyer->id = i + 1;
        buyer->demand = rand() % (11000 - 9000 + 1) + 9000;

        if (pthread_create(&buyers[i], NULL, buyer_func, buyer) != 0) 
        {
            fprintf(stderr, "Ошибка создания потока покупателя %d\n", i + 1);
            free(buyer);
            exit(EXIT_FAILURE);
        }
    }

    // Создаем поток погрузчика
    if (pthread_create(&loader, NULL, loader_func, NULL) != 0) 
    {
        fprintf(stderr, "Ошибка создания потока погрузчика\n");
        exit(EXIT_FAILURE);
    }

    // Ждем завершения покупателей
    for (int i = 0; i < BUYER_COUNT; i++) 
        pthread_join(buyers[i], NULL);

    // Ждем завершения погрузчика
    pthread_join(loader, NULL);

    // Уничтожаем мьютексы
    for (int i = 0; i < SHOP_COUNT; i++) 
        pthread_mutex_destroy(&shop_mutex[i]);
    
    pthread_mutex_destroy(&finish_mutex);

    printf("Программа завершена!\n");

    return 0;
}

void* buyer_func(void* arg) {
    buyer_t* buyer = (buyer_t*)arg;
    printf("#Покупатель %d, потребность: %d.\n", buyer->id, buyer->demand);

    while (buyer->demand > 0) {
        int bought = 0;
        for (int i = 0; i < SHOP_COUNT; i++) {
            pthread_mutex_lock(&shop_mutex[i]);

            if (shop[i] > 0) {
                int can_buy = shop[i];
                if (buyer->demand < can_buy)
                    can_buy = buyer->demand;

                shop[i] -= can_buy;
                buyer->demand -= can_buy;

                printf("#Покупатель %d, зашел в магазин %d, там было %d товаров, купил %d, осталась потребность %d.\n",
                    buyer->id, i + 1, can_buy, can_buy, buyer->demand);

                bought = 1;

                pthread_mutex_unlock(&shop_mutex[i]);
                break; // После покупки выходим и спим
            }

            pthread_mutex_unlock(&shop_mutex[i]);
        }

        if (!bought) {
            printf("#Покупатель %d, не нашёл товара, жду...\n", buyer->id);
        }

        printf("#Покупатель %d уснул...\n", buyer->id);
        sleep(2);
        printf("#Покупатель %d проснулся, потребность %d.\n", buyer->id, buyer->demand);
    }

    printf("#Покупатель %d исчерпал свою потребность, я завершаюсь...\n", buyer->id);

    pthread_mutex_lock(&finish_mutex);
    buyers_finished++;
    pthread_mutex_unlock(&finish_mutex);

    free(buyer);
    return NULL;
}

void* loader_func(void* arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&finish_mutex);
        if (buyers_finished == BUYER_COUNT) {
            pthread_mutex_unlock(&finish_mutex);
            break; // Все покупатели завершились — погрузчик завершается
        }
        pthread_mutex_unlock(&finish_mutex);

        for (int i = 0; i < SHOP_COUNT; i++) {
            pthread_mutex_lock(&shop_mutex[i]);
            shop[i] += 500;
            printf("@Погрузчик зашел в магазин %d, добавил 500 товаров, теперь товаров: %d.\n", i + 1, shop[i]);
            pthread_mutex_unlock(&shop_mutex[i]);
            sleep(1);
        }
    }

    printf("@Погрузчик завершается...\n");
    return NULL;
}