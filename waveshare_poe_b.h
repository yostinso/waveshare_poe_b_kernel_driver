#ifndef _WAVESHARE_POE_B_
#define _WAVESHARE_POE_B_

struct waveshare_poe_b_cooling {
    struct mutex conf_mutex;
    struct thermal_cooling_device *cdev;
    unsigned long throttle_state;
};

static int setup_i2c_client(struct i2c_adapter *adapter, struct i2c_client **client);
static int find_i2c_client(struct device *dev, void *data);
static void get_i2c_client(struct i2c_client **client);
static void free_i2c_client(struct i2c_client *client);

static int i2cdev_check_mux_parents(struct i2c_adapter *adapter, int addr);
static int i2cdev_check_mux_children(struct device *dev, void *addrp);
static int i2cdev_check(struct device *dev, void *addrp);

static int set_i2c_byte(struct i2c_client *client, char byte);

static void register_hat_fan(void);
static void unregister_hat_fan(void);

#endif