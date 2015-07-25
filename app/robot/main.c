/**
 * @file main.c
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

#define VERSION_STR "0.1.0"

FS_COMMAND_DEFINE("/robot/start", robot_cmd_start);
FS_COMMAND_DEFINE("/robot/stop", robot_cmd_stop);
FS_COMMAND_DEFINE("/robot/status", robot_cmd_status);
FS_COMMAND_DEFINE("/robot/mode/set", robot_cmd_mode_set);
FS_COMMAND_DEFINE("/robot/manual/movement/set", robot_cmd_manual_movement_set);

static struct uart_driver_t uart;
static char qinbuf[32];
static struct shell_args_t shell_args;

static struct robot_t robot;
static struct timer_t ticker;
static struct thrd_t *self_p;
static struct emtp_t emtp;

static char shell_stack[350];

/* Modes as strings. */
static FAR const char manual_string[] = "manual";
static FAR const char automatic_string[] = "automatic";

FAR const char FAR *mode_as_string[] = {
    manual_string,
    automatic_string
};

/* States as strings. */
static FAR const char idle_string[] = "idle";
static FAR const char starting_string[] = "starting";
static FAR const char cutting_string[] = "cutting";
static FAR const char searching_for_base_station_string[] = "searching_for_base_station";
static FAR const char in_base_station_string[] = "in_base_station";

FAR const char FAR *state_as_string[] = {
    idle_string,
    starting_string,
    cutting_string,
    searching_for_base_station_string,
    in_base_station_string
};

/* Cutting states as strings. */
static FAR const char cutting_forward_string[] = "forward";
static FAR const char cutting_backwards_string[] = "backwards";
static FAR const char cutting_rotating_string[] = "rotating";

FAR const char FAR *cutting_state_as_string[] = {
    cutting_forward_string,
    cutting_backwards_string,
    cutting_rotating_string
};

/* Searching for base station states as strings. */
static FAR const char searching_state_searching_for_perimeter_wire_string[] =
    "searching_for_perimeter_wire";
static FAR const char searching_state_following_perimeter_wire_string[] =
    "following_perimeter_wire";

FAR const char FAR *searching_state_as_string[] = {
    searching_state_searching_for_perimeter_wire_string,
    searching_state_following_perimeter_wire_string,
};

int robot_cmd_start(int argc,
		    const char *argv[],
		    void *out_p,
		    void *in_p)
{
    robot_start(&robot);

    return (0);
}

int robot_cmd_stop(int argc,
		   const char *argv[],
		   void *out_p,
		   void *in_p)
{
    robot_stop(&robot);

    return (0);
}

int robot_cmd_status(int argc,
		     const char *argv[],
		     void *out_p,
		     void *in_p)
{
    UNUSED(in_p);
    struct time_t time;

    std_fprintf(out_p, FSTR("mode = "));
    std_fprintf(out_p, mode_as_string[robot.mode]);
    std_fprintf(out_p, FSTR("\r\nstate = /"));
    std_fprintf(out_p, state_as_string[robot.state.current]);

    switch (robot.state.current) {
    case ROBOT_STATE_CUTTING:
	std_fprintf(out_p, FSTR("/"));
	std_fprintf(out_p, cutting_state_as_string[robot.substate.cutting.state]);
	break;
    case ROBOT_STATE_SEARCHING_FOR_BASE_STATION:
	std_fprintf(out_p, FSTR("/"));
	std_fprintf(out_p, searching_state_as_string[robot.substate.searching.state]);
	break;
    }

    time.seconds = 0;
    time.nanoseconds = PROCESS_PERIOD_NS;
    std_fprintf(out_p,
		FSTR("\r\nperiod = %d ticks\r\n"
		     "execution time = %d ticks\r\n"
		     "perimeter signal level = %d\r\n"
		     "energy level = %d\r\n"
                     "watchdog count = %d\r\n"),
		(int)T2ST(&time),
		robot.debug.tick_time,
		(int)perimeter_wire_rx_get_cached_signal(&robot.perimeter),
		(int)power_get_cached_stored_energy_level(&robot.power),
                robot.watchdog.count);

    if (robot.mode == ROBOT_MODE_MANUAL) {
	std_fprintf(out_p,
		    FSTR("speed = %d.%u m/s\r\n"
			 "omega = %d.%u rad/s\r\n"),
		    (int)(robot.manual.speed),
		    ((unsigned int)(robot.manual.speed * 100.0f)) % 100,
		    (int)(robot.manual.omega),
		    ((unsigned int)(robot.manual.omega * 100.0f)) % 100);
    }

    return (0);
}

int robot_cmd_mode_set(int argc,
		       const char *argv[],
		       void *out_p,
		       void *in_p)
{
    UNUSED(in_p);

    if (argc != 2) {
	std_fprintf(out_p, FSTR("Usage: set {manual,automatic}\r\n"));
	return (1);
    }

    if (std_strcmp(argv[1], FSTR("manual")) == 0) {
	robot.mode = ROBOT_MODE_MANUAL;
    } else if (std_strcmp(argv[1], FSTR("automatic")) == 0) {
	robot.mode = ROBOT_MODE_AUTOMATIC;
    } else {
	std_fprintf(out_p, FSTR("Bad mode '%s'\r\n"), argv[1]);
    }

    return (0);
}

int robot_cmd_manual_movement_set(int argc,
				  const char *argv[],
				  void *out_p,
				  void *in_p)
{
    UNUSED(in_p);

    long speed;
    long omega;

    std_strtol(argv[1], &speed);
    std_strtol(argv[2], &omega);

    robot.manual.speed = ((float)speed) / 100.0f;
    robot.manual.omega = ((float)omega) / 100.0f;

    return (0);
}

static void timer_callback(void *arg_p)
{
    struct thrd_t *thrd_p = arg_p;

    thrd_resume_irq(thrd_p, 0);
}

static int init()
{
    sys_start();
    uart_module_init();
    adc_module_init();

    /* Setup UART. */
    uart_init(&uart, &uart_device[0], 38400, qinbuf, sizeof(qinbuf));
    uart_start(&uart);
    sys_set_stdout(&uart.chout);

    emtp_init(&emtp,
	      NULL,
	      NULL,
	      NULL,
              (int (*)(void *,
                       struct emtp_t *,
                       struct emtp_message_header_t *))robot_handle_emtp_message,
	      &robot);

    robot_init(&robot);

    self_p = thrd_self();
    thrd_set_name("robot");

    /* Start the shell. */
    shell_args.chin_p = &uart.chin;
    shell_args.chout_p = &uart.chout;
    thrd_spawn(shell_entry,
               &shell_args,
               20,
               shell_stack,
               sizeof(shell_stack));

    return (0);
}

int main()
{
    struct time_t timeout;
    struct time_t start_time;

    init();

    std_printf(FSTR("RoboMower robot v" VERSION_STR "\r\n"));

    /* Start the robot periodic timer with a 50ms period. */
    timeout.seconds = 0;
    timeout.nanoseconds = PROCESS_PERIOD_NS;

    timer_set(&ticker,
	      &timeout,
	      timer_callback,
	      self_p,
	      TIMER_PERIODIC);

    /* Robot main loop. */
    while (1) {
	/* Timer callback resumes this thread. */
	thrd_suspend(NULL);

        time_get(&start_time);

        robot_tick(&robot);

        /* Handle any emtp message or stream data. */
        //emtp_process(&robot_p->emtp);

        time_get(&timeout);
        robot.debug.tick_time = (timeout.seconds - start_time.seconds);
    }

    return (0);
}
