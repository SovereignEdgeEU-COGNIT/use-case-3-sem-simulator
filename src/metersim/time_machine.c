/*
 * Time utils for the SEM simulator
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

#include "time_machine.h"
#include "log.h"

#define LOG_TAG             "timeMachine : "
#define NSEC_PER_SEC        (1000 * 1000 * 1000)
#define PAUSE_NOT_SCHEDULED (INT32_MAX)


static inline int32_t min(int32_t a, int32_t b)
{
	return a < b ? a : b;
}


static inline int32_t max(int32_t a, int32_t b)
{
	return a > b ? a : b;
}


static int32_t getSimulatedSeconds(struct timespec *start, struct timespec *finish, int speedup)
{
	int64_t nfinish = finish->tv_sec * NSEC_PER_SEC + finish->tv_nsec;
	int64_t nstart = start->tv_sec * NSEC_PER_SEC + start->tv_nsec;
	return (nfinish - nstart) * speedup / NSEC_PER_SEC;
}


int32_t timeMachine_gettime(timeMachine_ctx_t *ctx)
{
	struct timespec now;
	int32_t ret;
	clock_gettime(CLOCK_MONOTONIC, &now);

	ret = getSimulatedSeconds(&ctx->lastSwitchReal, &now, ctx->speedup) + ctx->lastSwitch;
	ret = min(ret, ctx->stopTime);
	return ret;
}


void timeMachine_getWaitTime(timeMachine_ctx_t *ctx, int32_t wakeUpTime, struct timespec *ret)
{
	int32_t sec = wakeUpTime - timeMachine_gettime(ctx);

	assert(sec >= 0);
	clock_gettime(CLOCK_MONOTONIC, ret);
	ret->tv_sec += sec / ctx->speedup;
	ret->tv_nsec += (int64_t)(sec % ctx->speedup) * NSEC_PER_SEC / ctx->speedup;
	if (ret->tv_nsec >= NSEC_PER_SEC) {
		ret->tv_nsec -= NSEC_PER_SEC;
		ret->tv_sec += 1;
	}
}


void timeMachine_setSpeedup(timeMachine_ctx_t *ctx, int speedup)
{
	struct timespec realNow;
	int32_t virtualNow;
	clock_gettime(CLOCK_MONOTONIC, &realNow);

	virtualNow = getSimulatedSeconds(&ctx->lastSwitchReal, &realNow, ctx->speedup) + ctx->lastSwitch;
	ctx->lastSwitch = min(virtualNow, ctx->stopTime);
	ctx->lastSwitchReal = realNow;
	ctx->speedup = speedup;
}


void timeMachine_start(timeMachine_ctx_t *ctx, int32_t now)
{
	clock_gettime(CLOCK_MONOTONIC, &ctx->start);
	ctx->lastSwitch = now;
	ctx->lastSwitchReal = ctx->start;

	/* Set stopTime to PAUSE_NOT_SCHEDULED only if there is no pause scheduled for the future */
	if (ctx->stopTime <= now) {
		ctx->stopTime = PAUSE_NOT_SCHEDULED;
	}
}


int32_t timeMachine_setStop(timeMachine_ctx_t *ctx, int32_t stopTime)
{
	ctx->stopTime = max(stopTime, timeMachine_gettime(ctx));
	return ctx->stopTime;
}


bool timeMachine_isStopped(timeMachine_ctx_t *ctx)
{
	return ctx->stopTime == timeMachine_gettime(ctx);
}


void timeMachine_init(timeMachine_ctx_t *ctx, int speedup)
{
	ctx->speedup = speedup;
	ctx->stopTime = 0;
	clock_gettime(CLOCK_MONOTONIC, &ctx->start);
	ctx->lastSwitch = 0;
	ctx->lastSwitchReal = ctx->start;
}
