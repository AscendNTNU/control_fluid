/**
 * @file type_mask.h
 */

#ifndef TYPE_MASK_H
#define TYPE_MASK_H

#include <cstdint>

/**
 * @brief Represents bitmasks for different kind of control within PX4 when setting a setpoint.
 */
class TypeMask {
   public:
    static constexpr uint16_t IGNORE_PX = (1 << 0);
    static constexpr uint16_t IGNORE_PY = (1 << 1);
    static constexpr uint16_t IGNORE_PZ = (1 << 2);
    static constexpr uint16_t IGNORE_VX = (1 << 3);
    static constexpr uint16_t IGNORE_VY = (1 << 4);
    static constexpr uint16_t IGNORE_VZ = (1 << 5);
    static constexpr uint16_t IGNORE_AFX = (1 << 6);
    static constexpr uint16_t IGNORE_AFY = (1 << 7);
    static constexpr uint16_t IGNORE_AFZ = (1 << 8);
    static constexpr uint16_t FORCE = (1 << 9);
    static constexpr uint16_t IGNORE_YAW = (1 << 10);
    static constexpr uint16_t IGNORE_YAW_RATE = (1 << 11);
    static constexpr uint16_t IDLE = 0x4000;

    static constexpr uint16_t POSITION =
        IGNORE_VX | IGNORE_VY | IGNORE_VZ | IGNORE_AFX | IGNORE_AFY | IGNORE_AFZ | IGNORE_YAW_RATE;

    static constexpr uint16_t VELOCITY =
        IGNORE_PX | IGNORE_PY | IGNORE_PZ | IGNORE_AFX | IGNORE_AFY | IGNORE_AFZ | IGNORE_YAW_RATE;

    static constexpr uint16_t POSITION_AND_VELOCITY =
        IGNORE_AFX | IGNORE_AFY | IGNORE_AFZ | IGNORE_YAW_RATE;
    
    static constexpr uint16_t ACCELERATION =
        IGNORE_PX | IGNORE_PY | IGNORE_PZ | IGNORE_VX | IGNORE_VY | IGNORE_VZ | IGNORE_YAW_RATE;

};

#endif
