#ifndef PROTOCOL_H
#define PROTOCOL_H

struct CAN_IMU_Frame
{
    float v1;
    float v2;
};



enum CAN_ID{
    ID_IMU_X = 0x101,
    ID_IMU_Y = 0x102,
    ID_IMU_Z = 0x103,
    ID_GPS_ = 0x20

};

#endif
