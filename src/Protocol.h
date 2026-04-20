#ifndef PROTOCOL_H
#define PROTOCOL_H

struct CAN_IMU_Frame
{
    float v1;
    float v2;
};

enum CAN_IMU_ID{
    ID_X = 0x01,
    ID_Y = 0x02,
    ID_Z = 0x03

};

#endif
