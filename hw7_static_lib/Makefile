LIB_SOURCES = add.c sub.c mul.c div.c
LIB_OBJECTS = add.o sub.o mul.o div.o

.PHONY: all clean

all: main

# Сборка исполняемого файла main, линкуем с библиотекой libcalc.a
main: main.o libcalc.a
	gcc main.o -L. -lcalc -o main

# Сборка статической библиотеки из объектных файлов
libcalc.a: add.o sub.o mul.o div.o
	ar rc libcalc.a add.o sub.o mul.o div.o

# Компиляция объектных файлов из исходников библиотеки
%.o: %.c
	gcc -c $< -o $@

# Компиляция main.o
main.o: main.c add.h sub.h mul.h div.h
	gcc -c main.c -o main.o

clean:
	rm -f *.o libcalc.a main
