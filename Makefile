
MY_TARGET := ehci
obj-m += $(MY_TARGET).o


CFLAGS_ehci.o := -O0

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

