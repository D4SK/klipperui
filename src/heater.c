// Heating of heaters.
//
// Copyright (C) 2020  Konstantin Vogel <konstantin.vogel@gmx.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "autoconf.h" // CONFIG_*
#include "basecmd.h" // oid_alloc
#include "board/gpio.h" // gpio_out_write
#include "board/irq.h" // irq_disable
#include "board/misc.h" // timer_is_before
#include "command.h" // DECL_COMMAND
#include "sched.h" // struct timer

struct heater
{
    float kp;
    float ki;
    float kd;
    float max_power;
    // itegrator power contribution is limited config pid_integral_max /ki
    float temp_integ_max;
    float prev_temp = AMBIENT_TEMP;
    uint32_t prev_temp_time = 0;
    float prev_temp_deriv = 0;
    float prev_temp_integ = 0;
};



void pid_temperature_control(struct *heater, float temp)
{
    uint32_t time_delta = read_time - self.prev_temp_time;
    //TODO add filtering, feed-forward, power~pwm relationship (probably quadratic)
    // Calculate change of temperature
    float temp_delta = temp - heater->prev_temp;
    float temp_deriv = temp_delta / time_delta;
    // Calculate accumulated temperature "error"
    float temp_err = heater->target_temp - temp;
    temp_integ = heater->prev_temp_integ + temp_err * time_delta;
    temp_integ = max(0., min(self.temp_integ_max, temp_integ));
    // Calculate output
    power_setpoint = heater->kp*temp_err + heater->ki*temp_integ - heater->kd*temp_deriv;
    bounded_power_setpoint = (power_setpoint > heater->max_power) ? heater->max_power : ((power_setpoint < 0) ? 0 : power_setpoint);
    self.heater.set_pwm(read_time, bounded_co);
    // Store state for next measurement
    heater->prev_temp = temp;
    heater->prev_temp_time = read_time;
    heater->prev_temp_deriv = temp_deriv;
    sendf("pid update");
    if (power_setpoint == bounded_power_setpoint)
        self.prev_temp_integ = temp_integ;
};
