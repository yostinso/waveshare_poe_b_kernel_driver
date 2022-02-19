#include "linux/version.h"
#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/resource.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/i2c.h>

#include "waveshare_poe_b.h"

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
    if (client) {
        if (client->addr != PCF8574_Address)
            set_i2c_addr(client, PCF8574_Address);

        if (state) {
            set_i2c_byte(client, 0xfe);
        } else {
            set_i2c_byte(client, 0x01);
        }
    }
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

/* Busy checks borrowed from i2c-dev.c */
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

static int setup_i2c_client(struct i2c_adapter *adapter, struct i2c_client **client) {
    struct i2c_client *c;
    c = kzalloc(sizeof(*c), GFP_KERNEL);
    if (!c) { return -ENOMEM; }

    snprintf(c->name, I2C_NAME_SIZE, "i2c-dev %d", adapter->nr);
    c->adapter = adapter;

    *client = c;
    return 0;
}

static int set_i2c_addr(struct i2c_client *client, unsigned short addr) {
    if (i2c_check_addr_busy(client->adapter, addr)) {
        return -EBUSY;
    }

    client->addr = addr;
    return 0;
}

static int find_i2c_client(struct device *dev, void *data) {
    struct i2c_adapter *adapter;
    struct i2c_client *client;
    struct i2c_client **client_ptr = data;
    int res = 0;
    *client_ptr = NULL;
    if (i2c_verify_adapter(dev)) {
        adapter = to_i2c_adapter(dev);
        if (adapter && adapter->nr == I2C_ADAPTER_NR) {
            printk("waveshare_poe_b: Found I2C adapter %d\n", I2C_ADAPTER_NR);
            if ((res = setup_i2c_client(adapter, &client)) == 0) {
                *client_ptr = client;
                return 1;
            }
        }
    }
    return 0;
}

static int set_i2c_byte(struct i2c_client *client, char byte) {
    const char write_byte[] = { byte };

    if (!client) { return -EINVAL; }

    // Send a message!
    printk("waveshare_poe_b: Sending message 0x%x to address 0x%x on I2C bus %d\n", byte, client->addr, client->adapter->nr);
    return i2c_master_send(client, write_byte, 1);
}

static char get_i2c_byte(struct i2c_client *client) {
    char read_byte[1];
    i2c_master_recv(client, read_byte, 1);

    return read_byte[0];
}

static void get_i2c_client(struct i2c_client **client) {
    bus_for_each_dev(&i2c_bus_type, 0, client, find_i2c_client);
}

static void free_i2c_client(struct i2c_client *client) {
    kfree(client);
}


int init_module() {
    printk("waveshare_poe_b: Initializing...\n");
    get_i2c_client(&client);


    register_hat_fan();

    // Get the current fan state
    if (client) {
        set_i2c_addr(client, PCF8574_Address);

        devdata.throttle_state = (get_i2c_byte(client) == 0) ? 0 : 1;
    }

    printk("waveshare_poe_b: Initialized with client %p...\n", client);
    return 0;
}

void cleanup_module() {
    printk("waveshare_poe_b: unloading\n");
    if (client) {
        free_i2c_client(client);
    }

    unregister_hat_fan();
    printk("waveshare_poe_b: done unloading\n");
}