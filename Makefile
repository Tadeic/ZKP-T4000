TARGET_MODULE:=eon

obj-m := $(TARGET_MODULE).o
$(TARGET_MODULE)-objs := eon_drv.o test_dev.o hdma.o iatu.o
CURRENT_PATH := $(shell pwd)
LINUX_KERNEL := $(shell uname -r)
LINUX_KERNEL_PATH := /lib/modules/$(shell uname -r)/build
all:
	$(MAKE) -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean
