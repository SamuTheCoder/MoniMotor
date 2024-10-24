/* REAL TIME DATABASE header file


*/

#pragma once

#include <stdlib.h>
#include <stdint.h>

#ifndef RT_DB_H
#define RT_DB_H

typedef struct {
    uint32_t motor_speed = 0;
    double highest_amplitude = 0;
    uint8_t has_bearing_issues = 0;
} rt_db_t;

#endif