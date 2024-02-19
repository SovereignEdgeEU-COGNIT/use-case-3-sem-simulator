/*
 * Helper for devices handling in Python
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <complex.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>

#include <metersim/metersim.h>
#include <metersim/metersim_types.h>

#include "complex_py.h"


typedef struct {
	complexPy_t current[3];
	int32_t nextUpdateTime;
} metersim_deviceResponsePy_t;


typedef struct {
	complexPy_t voltage[3];
	int32_t now;
} metersim_infoForDevicePy_t;


typedef struct {
	int deviceId;

	bool ready;
	bool responseNeeded;
	bool shutdownFlag;
	metersim_deviceResponse_t res;
	metersim_infoForDevice_t info;

	metersim_ctx_t *metersimCtx;

	pthread_cond_t cond;
	pthread_mutex_t lock;
} metersim_devicePy_ctx_t;


static void callback(metersim_infoForDevice_t *info, metersim_deviceResponse_t *res, void *ptr)
{
	metersim_devicePy_ctx_t *ctx = (metersim_devicePy_ctx_t *)ptr;

	assert(ctx != NULL);
	pthread_mutex_lock(&ctx->lock);
	ctx->ready = false;
	ctx->responseNeeded = true;
	ctx->info = *info;
	ctx->res = (metersim_deviceResponse_t) { 0 };
	pthread_cond_broadcast(&ctx->cond);

	while (!ctx->ready && !ctx->shutdownFlag) {
		pthread_cond_wait(&ctx->cond, &ctx->lock); /* TODO: consider timedwait */
	}

	if (!ctx->shutdownFlag) {
		*res = ctx->res;
	}

	pthread_mutex_unlock(&ctx->lock);
}


void devicePy_waitForWakeup(metersim_devicePy_ctx_t *ctx, metersim_infoForDevicePy_t *info)
{
	pthread_mutex_lock(&ctx->lock);

	while (!ctx->responseNeeded && !ctx->shutdownFlag) {
		pthread_cond_wait(&ctx->cond, &ctx->lock);
	}

	if (ctx->shutdownFlag) {
		pthread_mutex_unlock(&ctx->lock);
		return;
	}

	ctx->responseNeeded = false;

	info->now = ctx->info.now;
	for (int i = 0; i < 3; i++) {
		info->voltage[i] = complexToPy(ctx->info.voltage[i]);
	}
	pthread_mutex_unlock(&ctx->lock);
}


void devicePy_setResponse(metersim_devicePy_ctx_t *ctx, metersim_deviceResponsePy_t *res)
{
	pthread_mutex_lock(&ctx->lock);
	ctx->res.nextUpdateTime = res->nextUpdateTime;
	for (int i = 0; i < 3; i++) {
		ctx->res.current[i] = pyToComplex(res->current[i]);
	}
	ctx->ready = true;
	pthread_cond_broadcast(&ctx->cond);
	pthread_mutex_unlock(&ctx->lock);
}


void devicePy_notify(metersim_devicePy_ctx_t *ctx)
{
	metersim_notifyDevicemgr(ctx->metersimCtx);
}


metersim_devicePy_ctx_t *devicePy_init(metersim_ctx_t *metersimCtx)
{
	int res = 0;
	metersim_devicePy_ctx_t *ctx = malloc(sizeof(metersim_devicePy_ctx_t));
	if (ctx == NULL) {
		return NULL;
	}

	ctx->metersimCtx = metersimCtx;

	pthread_mutex_init(&ctx->lock, NULL);

	pthread_condattr_t attr;
	res += pthread_condattr_init(&attr);
	res += pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
	if (res < 0) {
		free(ctx);
		return NULL;
	}

	res = pthread_cond_init(&ctx->cond, &attr);
	pthread_condattr_destroy(&attr);
	if (res < 0) {
		free(ctx);
		return NULL;
	}

	ctx->ready = false;
	ctx->responseNeeded = false;
	ctx->shutdownFlag = false;

	ctx->deviceId = metersim_newDevice(metersimCtx, &callback, ctx);

	return ctx;
}


void devicePy_finish(metersim_devicePy_ctx_t *ctx)
{
	/* NOTE: DeviceMgr thread gets shut down after the runner thread */
	pthread_mutex_lock(&ctx->lock);
	ctx->shutdownFlag = true;
	metersim_destroyDevice(ctx->metersimCtx, ctx->deviceId);
	pthread_cond_broadcast(&ctx->cond);
	pthread_mutex_unlock(&ctx->lock);
}


void devicePy_destroy(metersim_devicePy_ctx_t *ctx)
{
	pthread_mutex_destroy(&ctx->lock);
	pthread_cond_destroy(&ctx->cond);
	free(ctx);
}
