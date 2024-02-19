/*
 * Devices manager for SEM Simulator
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#ifndef DEVICEMGR_H
#define DEVICEMGR_H

#include <pthread.h>
#include <stdint.h>

#include <metersim/metersim_types.h>

#include "calculator.h"

#define DEVICEMGR_MAX_DEVICES_COUNT 32


struct metersim_deviceCtx_s {
	void (*callback)(metersim_infoForDevice_t *, metersim_deviceResponse_t *, void *);
	void *callbackCtx;
};


typedef struct {
	int deviceNum;
	metersim_deviceCtx_t *devices[DEVICEMGR_MAX_DEVICES_COUNT];
	int32_t nextUpdateTime;

	pthread_mutex_t lock;
} devicemgr_ctx_t;


int devicemgr_newDevice(devicemgr_ctx_t *ctx, void (*callback)(metersim_infoForDevice_t *, metersim_deviceResponse_t *, void *), void *callbackCtx);


int devicemgr_destroyDevice(devicemgr_ctx_t *ctx, int id);


void devicemgr_notify(devicemgr_ctx_t *ctx);


int32_t devicemgr_getNextUpdateTime(devicemgr_ctx_t *ctx);


void devicemgr_updateDevices(devicemgr_ctx_t *ctx, calculator_bias_t *bias, metersim_infoForDevice_t *info);


devicemgr_ctx_t *devicemgr_init(void);


void devicemgr_destroy(devicemgr_ctx_t *ctx);

#endif /* DEVICEMGR_H */
