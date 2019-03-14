KVERS = $(shell uname -r)

# Kernel modules
obj-m += epaper_spi.o

# Specify flags for module compilation
#EXTRA_CFLAGS=-g -O0

build: kernel_modules

kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

modules_install:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules_install

clean:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean