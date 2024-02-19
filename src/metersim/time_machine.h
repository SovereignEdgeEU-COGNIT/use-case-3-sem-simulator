/*
 * Time utils for the simulator
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#ifndef TIME_MACHINE_H
#define TIME_MACHINE_H

#include <time.h>
#include <stdint.h>
#include <stdbool.h>


typedef struct {
	struct timespec start;
	int32_t lastSwitch;
	struct timespec lastSwitchReal;
	int speedup;
	int32_t stopTime;
} timeMachine_ctx_t;


int32_t timeMachine_gettime(timeMachine_ctx_t *ctx);


void timeMachine_getWaitTime(timeMachine_ctx_t *ctx, int32_t wakeUpTime, struct timespec *ret);


void timeMachine_setSpeedup(timeMachine_ctx_t *ctx, int speedup);


void timeMachine_start(timeMachine_ctx_t *ctx, int32_t now);


int32_t timeMachine_setStop(timeMachine_ctx_t *ctx, int32_t stopTime);


bool timeMachine_isStopped(timeMachine_ctx_t *ctx);


void timeMachine_init(timeMachine_ctx_t *ctx, int speedup);

#endif /* TIME_MACHINE_H */
