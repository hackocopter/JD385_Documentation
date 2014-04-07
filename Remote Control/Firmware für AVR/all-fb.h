#ifndef ALLFB_H
#define ALLFB_H

typedef struct payload {
    uint8_t throttle;
    uint8_t rudder;
    uint8_t elevator;
    uint8_t aileron;
    uint8_t trimm_yaw;
    uint8_t trimm_pitch;
    uint8_t trimm_roll;
    uint8_t tx_id[3];
    uint8_t empty[4];
    uint8_t flags;                      // 0xc0 = bind, 0x04 = flip, Rest nicht genutzt
    uint8_t chksum;
    } PAYLOAD;

#define BIND    0
#define RUN     1


#endif
