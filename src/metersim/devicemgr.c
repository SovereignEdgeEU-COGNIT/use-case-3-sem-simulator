/*
 * Devices manager for SEM Simulator
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <assert.h>

#include <metersim/metersim_types.h>

#include "metersim_types_int.h"
#include "calculator.h"
#include "devicemgr.h"
#include "simulator.h"
#include "log.h"

#define LOG_TAG "devicemgr: "


int devicemgr_newDevice(devicemgr_ctx_t *ctx, void (*callback)(metersim_infoForDevice_t *, metersim_deviceResponse_t *, void *), void *callbackCtx)
{
	int id = -1;
	pthread_mutex_lock(&ctx->lock);
	if (ctx->deviceNum == DEVICEMGR_MAX_DEVICES_COUNT) {
		pthread_mutex_unlock(&ctx->lock);
		return METERSIM_ERROR;
	}

	metersim_deviceCtx_t *dev = malloc(sizeof(metersim_deviceCtx_t));
	if (dev == NULL) {
		pthread_mutex_unlock(&ctx->lock);
		log_error("Could not create new device");
		return METERSIM_ERROR;
	}

	for (int i = 0; i < DEVICEMGR_MAX_DEVICES_COUNT; i++) {
		if (ctx->devices[i] == NULL) {
			ctx->devices[i] = dev;
			id = i;
			break;
		}
	}

	assert(id >= 0);

	ctx->deviceNum++;
	dev->callback = callback;
	dev->callbackCtx = callbackCtx;
	ctx->nextUpdateTime = METERSIM_UPDATE_NEEDED_NOW;
	pthread_mutex_unlock(&ctx->lock);

	return id;
}


int devicemgr_destroyDevice(devicemgr_ctx_t *ctx, int id)
{
	int status;
	if (id < 0 || id >= DEVICEMGR_MAX_DEVICES_COUNT) {
		return METERSIM_ERROR;
	}

	pthread_mutex_lock(&ctx->lock);
	if (ctx->devices[id] == NULL) {
		status = METERSIM_ERROR;
	}
	else {
		free(ctx->devices[id]);
		ctx->devices[id] = NULL;
		ctx->deviceNum--;
		status = METERSIM_SUCCESS;
	}
	pthread_mutex_unlock(&ctx->lock);
	return status;
}


void devicemgr_notify(devicemgr_ctx_t *ctx)
{
	pthread_mutex_lock(&ctx->lock);
	ctx->nextUpdateTime = METERSIM_UPDATE_NEEDED_NOW;
	pthread_mutex_unlock(&ctx->lock);
}


int32_t devicemgr_getNextUpdateTime(devicemgr_ctx_t *ctx)
{
	int32_t ret;
	pthread_mutex_lock(&ctx->lock);
	ret = ctx->nextUpdateTime;
	pthread_mutex_unlock(&ctx->lock);

	return ret;
}


void devicemgr_updateDevices(devicemgr_ctx_t *ctx, calculator_bias_t *bias, metersim_infoForDevice_t *info)
{
	metersim_deviceResponse_t res;

	pthread_mutex_lock(&ctx->lock);

	ctx->nextUpdateTime = METERSIM_NO_UPDATE_SCHEDULED;

	for (int i = 0; i < DEVICEMGR_MAX_DEVICES_COUNT; i++) {
		if (ctx->devices[i] == NULL || ctx->devices[i]->callback == NULL) {
			continue;
		}
		res = (metersim_deviceResponse_t) { 0 };

		ctx->devices[i]->callback(info, &res, ctx->devices[i]->callbackCtx);
		ctx->nextUpdateTime = ctx->nextUpdateTime <= res.nextUpdateTime ? ctx->nextUpdateTime : res.nextUpdateTime;

		/* TODO: consider async callbacks and a wait here */

		calculator_accumulateBias(bias, &res);
	}
	pthread_mutex_unlock(&ctx->lock);
}


devicemgr_ctx_t *devicemgr_init(void)
{
	int status;

	devicemgr_ctx_t *ctx = calloc(1, sizeof(devicemgr_ctx_t));
	if (ctx == NULL) {
		return NULL;
	}

	ctx->nextUpdateTime = METERSIM_NO_UPDATE_SCHEDULED;

	status = pthread_mutex_init(&ctx->lock, NULL);
	if (status != 0) {
		free(ctx);
		return NULL;
	}

	return ctx;
}


void devicemgr_destroy(devicemgr_ctx_t *ctx)
{
	for (int i = 0; i < DEVICEMGR_MAX_DEVICES_COUNT; i++) {
		if (ctx->devices[i] != NULL) {
			free(ctx->devices[i]);
		}
	}
	pthread_mutex_destroy(&ctx->lock);
	free(ctx);
}
