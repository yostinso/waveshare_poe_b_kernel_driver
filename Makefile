obj-m := waveshare_poe_b.o

all:
	make -C /lib/modules/5.4.0-1052-raspi/build M=$(PWD) modules

clean:
	make -C /lib/modules/5.4.0-1052-raspi/build M=$(PWD) clean