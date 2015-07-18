/**
 * @file robomower/perimeter_wire_rx.h
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

#ifndef __ROBOMOWER_PERIMITER_WIRE_RX_H__
#define __ROBOMOWER_PERIMITER_WIRE_RX_H__

#include "simba.h"

struct perimeter_wire_rx_t {
    uint8_t state;
    uint8_t pos;
    struct pin_driver_t pin_in1;
    struct pin_driver_t pin_in2;
};

int perimeter_wire_rx_module_init(void);

/**
 * Initialize a perimeter wire instance from given data.
 * @param[out] pwire_p Instance to be initialised.
 * @param[in] pin_dev_in1_p Pin device for in1.
 * @param[in] pin_dev_in2_p Pin device for in2.
 * @return zero(0) or negative error code.
 */
int perimeter_wire_rx_init(struct perimeter_wire_rx_t *pwire_p,
                           struct pin_device_t *pin_dev_in1_p,
                           struct pin_device_t *pin_dev_in2_p);

/**
 * Start transmitting the signal on the perimeter wire.
 * @param[out] pwire_p Perimiter wire instance.
 * @return zero(0) or negative error code.
 */
int perimeter_wire_rx_start(struct perimeter_wire_rx_t *pwire_p);

#endif