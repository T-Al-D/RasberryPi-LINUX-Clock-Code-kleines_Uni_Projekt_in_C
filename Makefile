ifneq ($(KERNELVERSION),)
obj-m	:= ds3231_drv.o

else
KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)

default: ds3231_drv

ds3231_drv:
	@echo "Building module ..."
	export CFLAGS='-std=c99'
	@(cd $(KDIR) && make -C $(KDIR) SUBDIRS=$(PWD) modules)

clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c Module.symvers modules.order
	-rm -rf .tmp_versions
	-rm -rf *~

reload: ds3231_drv
	@echo "previous Module out ..."
	-sudo rmmod ds3231_drv
	@echo " insert Mosule in kernel ..."
	sudo insmod ds3231_drv.ko

test: reload
	@echo "read out..."
	cat /dev/ds3231_drv

mat: reload
	@echo "multipe access accomplished !"
	./thread.py
	
endif
