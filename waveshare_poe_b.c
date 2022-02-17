#include "linux/version.h"
#include <linux/module.h>
#include <linux/thermal.h>

#include "waveshare_poe_b.h"

MODULE_LICENSE("GPL");

#define COOLING_DEVICE_NAME "waveshare-poe-b"

static int get_max_state(struct thermal_cooling_device *dev, unsigned long *state) {
    return 0;
}
static int get_cur_state(struct thermal_cooling_device *dev, unsigned long *state) {
    return 0;
}
static int set_cur_state(struct thermal_cooling_device *dev, unsigned long state) {
    return 0;
}

static const struct thermal_cooling_device_ops ops = {
    .get_max_state = get_max_state,
    .get_cur_state = get_cur_state,
    .set_cur_state = set_cur_state,
};

struct thermal_cooling_device* register_hat_fan(void) {
    struct waveshare_poe_b_cooling *devdata = 0;

    return thermal_cooling_device_register(
        COOLING_DEVICE_NAME,
        devdata,
        &ops
    );
}

int init_module(void) {
    register_hat_fan();
    return 0;
}

void cleanup_module(void) {

}