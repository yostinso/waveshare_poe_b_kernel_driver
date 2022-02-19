#ifndef _WAVESHARE_POE_B_
#define _WAVESHARE_POE_B_

struct waveshare_poe_b_cooling {
    struct mutex conf_mutex;
    struct thermal_cooling_device *cdev;
    unsigned long throttle_state;
};

static void register_hat_fan(void);
static void unregister_hat_fan(void);
struct resource* get_i2c_mem_range(void);

#endif