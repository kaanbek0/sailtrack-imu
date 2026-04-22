#ifndef PROTOCOL_H
#define PROTOCOL_H

struct CAN_IMU_Frame
{
    float v1;
    float v2;
};

struct CAN_GPS_POS
{
    int32_t lat;
    int32_t lng;
};

struct CAN_GPS_MOTION
{
    float kmph;
    float headMot;
};

struct CAN_GPS_INFO
{
    uint32_t time;
    uint8_t sats;
};


enum CAN_ID{
    ID_IMU_X = 0x101,
    ID_IMU_Y = 0x102,
    ID_IMU_Z = 0x103,
    ID_GPS_POS = 0x201,
    ID_GPS_MOTION = 0x202,
    ID_GPS_INFO = 0x203
};

#endif
