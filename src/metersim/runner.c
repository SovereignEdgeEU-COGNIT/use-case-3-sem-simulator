/*
 * SEM simulator runner
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <pthread.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#include <metersim/metersim_types.h>
#include "metersim_types_int.h"
#include "time_machine.h"
#include "simulator.h"
#include "runner.h"
#include "log.h"

#define RUNNER_STACKSIZE 1024
#define LOG_TAG          "runner : "


static inline int32_t min(int32_t a, int32_t b)
{
	return a < b ? a : b;
}


void runner_update(runner_ctx_t *rctx)
{
	pthread_mutex_lock(&rctx->lock);
	if (rctx->running) {
		rctx->updating = true;
		pthread_cond_broadcast(&rctx->cond);
		while (rctx->updating) {
			pthread_cond_wait(&rctx->cond, &rctx->lock);
		}
	}
	pthread_mutex_unlock(&rctx->lock);
}


void runner_setSpeedup(runner_ctx_t *rctx, uint16_t speedup)
{
	if (rctx->type != runner_typeTimeMachine) {
		return;
	}

	pthread_mutex_lock(&rctx->lock);
	timeMachine_setSpeedup(&rctx->tmCtx, speedup);
	pthread_mutex_unlock(&rctx->lock);
}


static void _pauseAndWait(runner_ctx_t *rctx)
{
	log_debug("Pausing");
	rctx->running = false;
	rctx->updating = false; /* Thread might stop, while another is waiting for update */
	pthread_cond_broadcast(&rctx->cond);
	while (!rctx->running && !rctx->shutdownFlag) {
		pthread_cond_wait(&rctx->cond, &rctx->lock);
	}
	log_debug("Resuming");
}


static void _sleepOnCond(runner_ctx_t *rctx, int16_t sleepMilisec)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts.tv_nsec += (long)(sleepMilisec % 1000) * 1000 * 1000;
	if (ts.tv_nsec >= 1000 * 1000 * 1000) {
		ts.tv_nsec -= 1000 * 1000 * 1000;
		ts.tv_sec++;
	}
	ts.tv_sec += sleepMilisec / 1000;
	pthread_cond_timedwait(&rctx->cond, &rctx->lock, &ts);
}


static void *runnerCustomeGetTimeThread(void *arg)
{
	int32_t nowMono = 0;
	uint64_t nowUtc = 0;
	runner_ctx_t *rctx = (runner_ctx_t *)arg;
	simulator_ctx_t *sctx = rctx->sctx;

	pthread_mutex_lock(&rctx->lock);

	nowUtc = rctx->getTimeCb(rctx->cbArgs);
	uint64_t startUtc = nowUtc;

	rctx->running = true;

	log_debug("Starting custom time runner");
	for (;;) {
		nowUtc = rctx->getTimeCb(rctx->cbArgs);
		nowMono = (int32_t)(nowUtc - startUtc);
		simulator_stepForward(sctx, nowMono - sctx->now);

		if (rctx->shutdownFlag) {
			break;
		}

		rctx->updating = false;
		pthread_cond_broadcast(&rctx->cond);
		_sleepOnCond(rctx, 100);
	}
	rctx->running = false;
	pthread_mutex_unlock(&rctx->lock);
	log_debug("Finishing runner");

	return NULL;
}


static void *runnerTimeMachineThread(void *arg)
{
	int32_t now = 0;
	int32_t nextWakeupTime = 0;
	runner_ctx_t *rctx = (runner_ctx_t *)arg;
	simulator_ctx_t *sctx = rctx->sctx;

	pthread_mutex_lock(&rctx->lock);

	rctx->running = true;

	log_debug("Starting time machine runner");
	for (;;) {
		now = timeMachine_gettime(&rctx->tmCtx);
		simulator_stepForward(sctx, now - sctx->now);

		if (rctx->shutdownFlag) {
			break;
		}
		else if (now == rctx->stopTime) {
			assert(timeMachine_isStopped(&rctx->tmCtx));
			_pauseAndWait(rctx);
			if (rctx->shutdownFlag) {
				break;
			}
		}
		else {
			assert(rctx->stopTime == METERSIM_NO_UPDATE_SCHEDULED || now < rctx->stopTime);

			nextWakeupTime = min(simulator_getNextUpdateTime(sctx), rctx->stopTime);

			rctx->updating = false;
			pthread_cond_broadcast(&rctx->cond);
			if (nextWakeupTime == METERSIM_NO_UPDATE_SCHEDULED) {
				pthread_cond_wait(&rctx->cond, &rctx->lock);
			}
			else {
				struct timespec ts = { 0 };
				timeMachine_getWaitTime(&rctx->tmCtx, nextWakeupTime, &ts);
				pthread_cond_timedwait(&rctx->cond, &rctx->lock, &ts);
			}
		}
	}
	rctx->running = false;
	pthread_mutex_unlock(&rctx->lock);
	log_debug("Finishing runner");

	return NULL;
}


void runner_resume(runner_ctx_t *rctx)
{
	if (rctx->type != runner_typeTimeMachine) {
		return;
	}

	pthread_mutex_lock(&rctx->lock);
	rctx->running = true;
	if (rctx->stopTime <= rctx->sctx->now) {
		rctx->stopTime = METERSIM_NO_UPDATE_SCHEDULED;
	}
	timeMachine_start(&rctx->tmCtx, rctx->sctx->now);

	pthread_cond_broadcast(&rctx->cond);
	pthread_mutex_unlock(&rctx->lock);
}


void runner_pause(runner_ctx_t *rctx, int32_t when)
{
	if (rctx->type != runner_typeTimeMachine) {
		return;
	}

	pthread_mutex_lock(&rctx->lock);
	rctx->stopTime = timeMachine_setStop(&rctx->tmCtx, when);
	pthread_cond_broadcast(&rctx->cond);
	pthread_mutex_unlock(&rctx->lock);
}


bool runner_isRunning(runner_ctx_t *rctx)
{
	bool ret;
	pthread_mutex_lock(&rctx->lock);
	ret = rctx->running;
	pthread_mutex_unlock(&rctx->lock);
	return ret;
}


int32_t runner_getTime(runner_ctx_t *rctx)
{
	int32_t ret;
	pthread_mutex_lock(&rctx->lock);
	ret = rctx->sctx->now;
	pthread_mutex_unlock(&rctx->lock);
	return ret;
}


int64_t runner_getTimeUtc(runner_ctx_t *rctx)
{
	int64_t ret;
	pthread_mutex_lock(&rctx->lock);
	switch (rctx->type) {
		case runner_typeTimeMachine:
			ret = rctx->sctx->state.cfg.startTime + rctx->sctx->now;
			break;

		case runner_typeCustomGetTime:
			ret = (int64_t)rctx->getTimeCb(rctx->cbArgs);
			break;

		default:
			return -1;
	}
	pthread_mutex_unlock(&rctx->lock);
	return ret;
}


void runner_setTimeUtc(runner_ctx_t *rctx, int64_t time)
{
	pthread_mutex_lock(&rctx->lock);
	rctx->sctx->state.cfg.startTime = time - rctx->sctx->now;
	pthread_mutex_unlock(&rctx->lock);
}


int runner_start(runner_ctx_t *rctx)
{
	int ret = 0;
	pthread_attr_t attr;
	ret = pthread_attr_init(&attr);
	if (ret < 0) {
		return ret;
	}

	ret = pthread_attr_setstacksize(&attr, RUNNER_STACKSIZE);
	if (ret < 0) {
		pthread_attr_destroy(&attr);
		return ret;
	}

	if (rctx->type == runner_typeTimeMachine && rctx->stopTime != 0) {
		timeMachine_start(&rctx->tmCtx, rctx->sctx->now);
	}

	void *mainThread = NULL;
	switch (rctx->type) {
		case runner_typeTimeMachine:
			mainThread = runnerTimeMachineThread;
			break;

		case runner_typeCustomGetTime:
			mainThread = runnerCustomeGetTimeThread;
			break;

		default:
			pthread_attr_destroy(&attr);
			return -1;
	}

	ret = pthread_create(&rctx->runnerThread, &attr, mainThread, rctx);
	pthread_attr_destroy(&attr);

	return ret;
}


void runner_finish(runner_ctx_t *rctx)
{
	pthread_mutex_lock(&rctx->lock);
	rctx->shutdownFlag = true;
	rctx->updating = true;
	rctx->running = true;
	pthread_mutex_unlock(&rctx->lock);

	pthread_cond_broadcast(&rctx->cond);
	pthread_join(rctx->runnerThread, NULL);
	rctx->updating = false;
	rctx->running = false;
}


runner_ctx_t *runner_init(simulator_ctx_t *sctx, uint64_t (*getTimeCb)(void *), void *args)
{
	int status = 0;
	runner_ctx_t *rctx = calloc(1, sizeof(runner_ctx_t));
	if (rctx == NULL) {
		return NULL;
	}

	rctx->getTimeCb = getTimeCb;
	rctx->cbArgs = args;

	rctx->type = rctx->getTimeCb == NULL ? runner_typeTimeMachine : runner_typeCustomGetTime;

	rctx->sctx = sctx;
	rctx->shutdownFlag = false;
	rctx->updating = false;
	rctx->stopTime = METERSIM_NO_UPDATE_SCHEDULED;

	status = pthread_mutex_init(&rctx->lock, NULL);
	if (status < 0) {
		free(rctx);
		return NULL;
	}

	pthread_condattr_t condAttr;
	status = pthread_condattr_init(&condAttr);
	if (status < 0) {
		pthread_mutex_destroy(&rctx->lock);
		free(rctx);
		return NULL;
	}

	status = pthread_condattr_setclock(&condAttr, CLOCK_MONOTONIC);
	if (status < 0) {
		pthread_condattr_destroy(&condAttr);
		pthread_mutex_destroy(&rctx->lock);
		free(rctx);
		return NULL;
	}

	status = pthread_cond_init(&rctx->cond, &condAttr);
	pthread_condattr_destroy(&condAttr);
	if (status < 0) {
		pthread_mutex_destroy(&rctx->lock);
		free(rctx);
		return NULL;
	}

	if (rctx->type == runner_typeTimeMachine) {
		timeMachine_init(&rctx->tmCtx, rctx->sctx->state.cfg.speedup);
	}

	return rctx;
}


void runner_destroy(runner_ctx_t *rctx)
{
	pthread_mutex_destroy(&rctx->lock);
	pthread_cond_destroy(&rctx->cond);
	free(rctx);
}
