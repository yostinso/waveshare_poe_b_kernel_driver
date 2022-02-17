#ifndef _WAVESHARE_POE_B_
#define _WAVESHARE_POE_B_

struct waveshare_poe_b_cooling {
    struct thermal_cooling_device *cdev;
    unsigned long throttle_state;
};

#endif