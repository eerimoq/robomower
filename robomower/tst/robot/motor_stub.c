/**
 * @file motor_stub.c
 * @version 0.1
 *
 * @section License
 * Copyright (C) 2015, Erik Moqvist
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * This file is part of the RoboMower project.
 */

#include "simba.h"
#include "robomower.h"

extern struct queue_t motor_queue;

int motor_init(struct motor_t *motor_p,
               struct pin_device_t *in1_p,
               struct pin_device_t *in2_p,
               struct pwm_device_t *enable_p)
{
    return (0);
}

/**
 * Set motor direction.
 * @param[in] direction 0 for CW, 1 for CCW
 * @return zero(0) or negative error code
 */
int motor_set_direction(struct motor_t *motor_p,
                        int direction)
{
    chan_write(&motor_queue, &direction, sizeof(direction));

    return (0);
}

int motor_set_omega(struct motor_t *motor_p,
                    float omega)
{
    chan_write(&motor_queue, &omega, sizeof(omega));

    return (0);
}