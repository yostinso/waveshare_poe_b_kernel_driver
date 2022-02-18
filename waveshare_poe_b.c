#include "linux/version.h"
#include <linux/module.h>
#include <linux/thermal.h>

#include "waveshare_poe_b.h"

MODULE_LICENSE("GPL");

#define COOLING_DEVICE_NAME "waveshare-poe-b"
#define MAX_STATE 1
#define FAN_I2C_ADDR 0x20
#define FAN_MIN 0x01
#define FAN_MAX 0xfe

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

void register_hat_fan() {

    devdata.cdev = thermal_cooling_device_register(
        COOLING_DEVICE_NAME,
        &devdata,
        &ops
    );
}

void unregister_hat_fan() {
    thermal_cooling_device_unregister(devdata.cdev);
}

int init_module() {
    register_hat_fan();
    return 0;
}

void cleanup_module() {
    unregister_hat_fan();
}