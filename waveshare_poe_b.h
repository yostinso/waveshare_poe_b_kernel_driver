#ifndef _WAVESHARE_POE_B_
#define _WAVESHARE_POE_B_

struct waveshare_poe_b_cooling {
    struct mutex conf_mutex;
    struct thermal_cooling_device *cdev;
    unsigned long throttle_state;
};

void register_hat_fan(void);
void unregister_hat_fan(void);

#endif