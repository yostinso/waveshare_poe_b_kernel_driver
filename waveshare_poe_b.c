#include "linux/version.h"
#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/resource.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/i2c.h>

#include "waveshare_poe_b.h"
#include "bcm_consts.h"

MODULE_AUTHOR("E.O. Stinson <yostinso@gmail.com>");
MODULE_DESCRIPTION("Waveshare Raspberry Pi 4 PoE Hat B fan and OLED driver");
MODULE_LICENSE("GPL v2");

#define COOLING_DEVICE_NAME "waveshare-poe-b"
#define MAX_STATE 1
#define PCF8574_Address 0x20
#define FAN_MIN 0x01
#define FAN_MAX 0xfe

// this is /dev/i2c-1 which is GPIO on Raspberry Pi
#define I2C_ADAPTER_NR 1

static struct waveshare_poe_b_cooling devdata = {
    .throttle_state = 0
};

static int get_max_state(struct thermal_cooling_device *dev, unsigned long *state) {
    *state = MAX_STATE;
    return 0;
}
static int get_cur_state(struct thermal_cooling_device *dev, unsigned long *state) {
    struct waveshare_poe_b_cooling *hat = dev->devdata;
    mutex_lock(&hat->conf_mutex);
    *state = hat->throttle_state;
    mutex_unlock(&hat->conf_mutex);
    return 0;
}
static int set_cur_state(struct thermal_cooling_device *dev, unsigned long state) {
    struct waveshare_poe_b_cooling *hat = dev->devdata;
    mutex_lock(&hat->conf_mutex);
    hat->throttle_state = state;
    mutex_unlock(&hat->conf_mutex);
    // TODO: I2C
    return 0;
}

static const struct thermal_cooling_device_ops ops = {
    .get_max_state = get_max_state,
    .get_cur_state = get_cur_state,
    .set_cur_state = set_cur_state,
};

static void register_hat_fan() {

    devdata.cdev = thermal_cooling_device_register(
        COOLING_DEVICE_NAME,
        &devdata,
        &ops
    );
}

static void unregister_hat_fan() {
    thermal_cooling_device_unregister(devdata.cdev);
}

struct resource* get_i2c_mem_range() {
    // Find the existing memory mapped by the bmc2835_i2c driver
    struct resource *res = 0;
    struct resource *p = &iomem_resource;
    printk("IOMEM: %s\n", p->name);
    for (p = p->child; p; p = p->sibling) {
        if (strncmp(p->name, "fe804000.i2c ", 13) == 0) {
            res = p;
            break;
        }
    }
    return res;
}

static int i2cdev_check_mux_parents(struct i2c_adapter *adapter, int addr);
static int i2cdev_check_mux_children(struct device *dev, void *addrp);
static int i2cdev_check(struct device *dev, void *addrp);

static int i2c_check_addr_busy(struct i2c_adapter *adapter, unsigned int addr) {
	struct i2c_adapter *parent = i2c_parent_is_i2c_adapter(adapter);
	int result = 0;

	if (parent)
		result = i2cdev_check_mux_parents(parent, addr);

	if (!result)
		result = device_for_each_child(&adapter->dev, &addr, i2cdev_check_mux_children);

	return result;
}
static int i2cdev_check_mux_parents(struct i2c_adapter *adapter, int addr) {
	struct i2c_adapter *parent = i2c_parent_is_i2c_adapter(adapter);
	int result;

	result = device_for_each_child(&adapter->dev, &addr, i2cdev_check);
	if (!result && parent)
		result = i2cdev_check_mux_parents(parent, addr);

	return result;
}

/* recurse down mux tree */
static int i2cdev_check_mux_children(struct device *dev, void *addrp) {
	int result;

	if (dev->type == &i2c_adapter_type)
		result = device_for_each_child(dev, addrp,
						i2cdev_check_mux_children);
	else
		result = i2cdev_check(dev, addrp);

	return result;
}

static int i2cdev_check(struct device *dev, void *addrp) {
	struct i2c_client *client = i2c_verify_client(dev);

	if (!client || client->addr != *(unsigned int *)addrp)
		return 0;

	return dev->driver ? -EBUSY : 0;
}


static int dev_print_name(struct device *dev, void *data) {
    struct i2c_adapter *adapter;
    struct i2c_client *client;
    const char write_byte[] = { 0x01 };
    //const char write_byte[] = { 0xfe };

    if (i2c_verify_adapter(dev)) {
        printk("  Found an adapter\n");
        adapter = to_i2c_adapter(dev);

        // Allocate an anonymous client
        client = kzalloc(sizeof(*client), GFP_KERNEL);
        if (!client) { return -ENOMEM; }
        snprintf(client->name, I2C_NAME_SIZE, "i2c-dev %d", adapter->nr);
        client->adapter = adapter;

        // Assign client device address
        if (i2c_check_addr_busy(adapter, PCF8574_Address)) {
            kfree(client);
            return -EBUSY;
        }

        client->addr = PCF8574_Address;

        // Send a message!
        printk("SEND A MESSAGE\n");
        i2c_master_send(client, write_byte, 1);


        kfree(client);
    }
    return 0;
}

static int drv_print_name(struct device_driver *drv, void *data) {
    struct i2c_driver *driver;
    struct i2c_client *client, *n;
    int i = 0;
    driver = to_i2c_driver(drv);
    list_for_each_entry_safe(client, n, &driver->clients, detected) {
        i++;
    }
    if (i > 0) {
        printk("  Driver had %d clients\n", i);
    }
    return 0;
}

static void get_i2c_adapter_name(void) {
    int level = 1;
    printk("Iterating devices...\n");
    bus_for_each_dev(&i2c_bus_type, 0, &level, dev_print_name);
    printk("Iterating drivers...\n");
    bus_for_each_drv(&i2c_bus_type, 0, &level, drv_print_name);
}

// TODO: Reimplement bcm driver

int init_module() {
    printk("------STARTING\n");
    //get_i2c_mem_range();
    get_i2c_adapter_name();
    register_hat_fan();
    printk("------DONE\n");
    return 0;
}

void cleanup_module() {
    unregister_hat_fan();
}