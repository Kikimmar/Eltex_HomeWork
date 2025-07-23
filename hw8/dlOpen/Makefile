.PHONY: all clean

all: main

# Сборка исполняемого файла main, линкуем с библиотекой libcalc.so
main: main.o libcalc.so
	gcc main.o -L. -lcalc -o main

# Сборка статической библиотеки из объектных файлов
libcalc.so: add.o sub.o mul.o div.o
	gcc -shared -o libcalc.so add.o sub.o mul.o div.o

# Компиляция объектных файлов из исходников библиотеки
%.o: %.c
	gcc -fPIC -c $< -o $@

# Компиляция main.o
main.o: main.c add.h sub.h mul.h div.h
	gcc -c main.c -o main.o -ldl

clean:
	rm -f *.o libcalc.so main
