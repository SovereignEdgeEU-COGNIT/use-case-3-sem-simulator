/*
 * SEM simulator runner API
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#ifndef RUNNER_H
#define RUNNER_H

#include <pthread.h>
#include <stdbool.h>

#include <metersim/metersim_types.h>
#include "metersim_types_int.h"
#include "time_machine.h"
#include "simulator.h"


typedef enum {
	runner_typeTimeMachine,
	runner_typeCustomGetTime,
} runner_type_t;


typedef struct {
	runner_type_t type;

	bool updating;
	bool running;
	bool shutdownFlag;

	int32_t stopTime;

	uint64_t (*getTimeCb)(void *args);
	void *cbArgs;

	timeMachine_ctx_t tmCtx;
	simulator_ctx_t *sctx;

	pthread_mutex_t lock;
	pthread_cond_t cond;
	pthread_t runnerThread;
} runner_ctx_t;


void runner_update(runner_ctx_t *rctx);


void runner_setSpeedup(runner_ctx_t *rctx, uint16_t speedup);


void runner_resume(runner_ctx_t *rctx);


void runner_pause(runner_ctx_t *rctx, int32_t when);


bool runner_isRunning(runner_ctx_t *rctx);


int32_t runner_getTime(runner_ctx_t *rctx);


int64_t runner_getTimeUtc(runner_ctx_t *rctx);


void runner_setTimeUtc(runner_ctx_t *rctx, int64_t time);


int runner_start(runner_ctx_t *rctx);


void runner_finish(runner_ctx_t *rctx);


runner_ctx_t *runner_init(simulator_ctx_t *sctx, uint64_t (*getTimeCb)(void *), void *args);


void runner_destroy(runner_ctx_t *rctx);

#endif /* RUNNER_H */
