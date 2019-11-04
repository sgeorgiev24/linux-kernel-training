all: export ARCH=arm
all: export CROSS_COMPILE=arm-linux-gnueabihf-

KERNEL = ../../linux/.
ifneq ($(KERNELRELEASE),)
obj-m := tmp100.o
else
KDIR := $(KERNEL)
all:
	$(MAKE) -C $(KDIR) M=$(PWD)
endif
