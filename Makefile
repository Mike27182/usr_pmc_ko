obj-m += usrpmc.o #obj-m tell it to make .ko files

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) NO_BTF=1 modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

