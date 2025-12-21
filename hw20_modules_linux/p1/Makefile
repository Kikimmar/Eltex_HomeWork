obj-m += test1.o
#ccflags-y += -g -DEBUG

all:
	make -C /lib/modules/6.14.0-37-generic/build M=$(PWD) modules
clean:
	make -C /lib/modules/6.14.0-37-generic/build M=$(PWD) clean
