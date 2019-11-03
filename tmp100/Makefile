ifneq ($(KERNELRELEASE),)
obj-m := tmp100.o
else
KDIR := ../../../../../../linux/.
all:
	$(MAKE) -C $(KDIR) M=$$PWD
endif
