/*
################
TEAM INFO
################
Names1: Kevin Nguyen
EID1: kdn433
CS Login: kxnguyen
Email: kxnguyen60@gmail.com
Unique Number: 51075

Name2: Sangwon Lee
EID2: sl34645
CS Login: bearbox
Email: fhqksl@hotmail.com
Unique Number: 51100

Slip days used: 0

################
*/

#ifndef DEVICES_TIMER_H
#define DEVICES_TIMER_H

#include <round.h>
#include <stdint.h>

/* Number of timer interrupts per second. */
#define TIMER_FREQ 100

void timer_init (void);
void timer_calibrate (void);
void timer_wakeup(int64_t);

int64_t timer_ticks (void);
int64_t timer_elapsed (int64_t);

/* Sleep and yield the CPU to other threads. */
void timer_sleep (int64_t ticks);
void timer_msleep (int64_t milliseconds);
void timer_usleep (int64_t microseconds);
void timer_nsleep (int64_t nanoseconds);

/* Busy waits. */
void timer_mdelay (int64_t milliseconds);
void timer_udelay (int64_t microseconds);
void timer_ndelay (int64_t nanoseconds);

void timer_print_stats (void);

#endif /* devices/timer.h */
