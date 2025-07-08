/*
 * Implementation of SEM simulator API
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include "time_machine.h"
#include "simulator.h"
#include <metersim/metersim.h>
#include <metersim/metersim_types.h>
#include "metersim_types_int.h"
#include "runner.h"


struct metersim_ctx_s {
	simulator_ctx_t *simulator;
	runner_ctx_t *runner;
};


metersim_ctx_t *metersim_init(const char *dir)
{
	metersim_ctx_t *ctx = malloc(sizeof(metersim_ctx_t));
	if (ctx == NULL) {
		return NULL;
	}

	ctx->simulator = simulator_init(dir);

	if (ctx->simulator == NULL) {
		free(ctx);
		return NULL;
	}

	ctx->runner = NULL;
	return ctx;
}


void metersim_free(metersim_ctx_t *ctx)
{
	simulator_destroy(ctx->simulator);
	free(ctx);
}


int metersim_createRunner(metersim_ctx_t *ctx, int start)
{
	if (ctx->runner != NULL) {
		return METERSIM_ERROR;
	}

	ctx->runner = runner_init(ctx->simulator);
	if (ctx->runner == NULL) {
		return METERSIM_ERROR;
	}

	/* Pause immediately if start == 0 */
	if (start == 0) {
		runner_pause(ctx->runner, 0);
	}

	if (runner_start(ctx->runner) < 0) {
		runner_destroy(ctx->runner);
		return METERSIM_ERROR;
	}

	return METERSIM_SUCCESS;
}


void metersim_destroyRunner(metersim_ctx_t *ctx)
{
	if (ctx->runner == NULL) {
		return;
	}
	runner_finish(ctx->runner);
	runner_destroy(ctx->runner);
	ctx->runner = NULL;
}


int metersim_resume(metersim_ctx_t *ctx)
{
	if (ctx->runner == NULL) {
		return METERSIM_ERROR;
	}
	runner_update(ctx->runner);
	runner_resume(ctx->runner);
	return METERSIM_SUCCESS;
}


int metersim_pause(metersim_ctx_t *ctx, int32_t when)
{
	if (ctx->runner == NULL) {
		return METERSIM_ERROR;
	}
	runner_update(ctx->runner);
	runner_pause(ctx->runner, when);
	return METERSIM_SUCCESS;
}


int metersim_isRunning(metersim_ctx_t *ctx)
{
	if (ctx->runner == NULL) {
		return 0;
	}
	runner_update(ctx->runner);
	return runner_isRunning(ctx->runner) ? 1 : 0;
}


int metersim_setSpeedup(metersim_ctx_t *ctx, uint16_t speedup)
{
	if (ctx->runner == NULL) {
		return METERSIM_ERROR;
	}

	if (speedup < 1 || speedup > METERSIM_MAX_SPEEDUP) {
		return METERSIM_ERROR;
	}

	runner_update(ctx->runner);
	runner_setSpeedup(ctx->runner, speedup);
	runner_update(ctx->runner);
	return METERSIM_SUCCESS;
}


int metersim_stepForward(metersim_ctx_t *ctx, uint32_t seconds)
{
	/* Stepping forward is allowed only when there is no runner and when it is paused */
	if (ctx->runner != NULL && runner_isRunning(ctx->runner)) {
		return METERSIM_REFUSE;
	}

	simulator_stepForward(ctx->simulator, seconds);
	return METERSIM_SUCCESS;
}


int metersim_newDevice(metersim_ctx_t *ctx, void (*callback)(metersim_infoForDevice_t *, metersim_deviceResponse_t *, void *), void *callbackCtx)
{
	int ret;
	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
	}

	ret = devicemgr_newDevice(ctx->simulator->devmgrCtx, callback, callbackCtx);

	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
	}
	return ret;
}


int metersim_destroyDevice(metersim_ctx_t *ctx, int id)
{
	int ret;
	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
	}

	ret = devicemgr_destroyDevice(ctx->simulator->devmgrCtx, id);

	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
	}
	return ret;
}


void metersim_notifyDevicemgr(metersim_ctx_t *ctx)
{
	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
	}

	devicemgr_notify(ctx->simulator->devmgrCtx);

	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
	}
}


void metersim_getTariffCount(metersim_ctx_t *ctx, int *retCount)
{
	simulator_getTariffCount(ctx->simulator, retCount);
}


void metersim_getTariffCurrent(metersim_ctx_t *ctx, int *retIndex)
{
	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
	}
	simulator_getTariffCurrent(ctx->simulator, retIndex);
}


int metersim_getSerialNumber(metersim_ctx_t *ctx, int idx, char *dstBuf, size_t dstBufLen)
{
	return simulator_getSerialNumber(ctx->simulator, idx, dstBuf, dstBufLen);
}


void metersim_getTimeUTC(metersim_ctx_t *ctx, int64_t *retTime)
{
	int32_t uptime;
	metersim_getUptime(ctx, &uptime);
	*retTime = ctx->simulator->state.cfg.startTime + (int64_t)uptime;
}


void metersim_setTimeUTC(metersim_ctx_t *ctx, int64_t time)
{
	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
		runner_setTimeUtc(ctx->runner, time);
	}
	else {
		int32_t uptime;
		metersim_getUptime(ctx, &uptime);
		ctx->simulator->state.cfg.startTime = time - uptime;
	}
}


void metersim_getUptime(metersim_ctx_t *ctx, int32_t *retSeconds)
{
	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
		*retSeconds = (int32_t)runner_getTime(ctx->runner);
	}
	else {
		*retSeconds = (int32_t)ctx->simulator->now;
	}
}


void metersim_getPhaseCount(metersim_ctx_t *ctx, int *retCount)
{
	simulator_getPhaseCount(ctx->simulator, retCount);
}


void metersim_getFrequency(metersim_ctx_t *ctx, float *retFreq)
{
	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
	}
	simulator_getFrequency(ctx->simulator, retFreq);
}


void metersim_getMeterConstant(metersim_ctx_t *ctx, unsigned int *ret)
{
	simulator_getMeterConstant(ctx->simulator, ret);
}


void metersim_getInstant(metersim_ctx_t *ctx, metersim_instant_t *ret)
{
	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
	}
	simulator_getInstant(ctx->simulator, ret);
}


void metersim_getEnergyTotal(metersim_ctx_t *ctx, metersim_energy_t *ret)
{
	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
	}
	simulator_getEnergyTotal(ctx->simulator, ret);
}


int metersim_getEnergyTariff(metersim_ctx_t *ctx, metersim_energy_t ret[3], int idxTariff)
{
	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
	}
	return simulator_getEnergyTariff(ctx->simulator, ret, idxTariff);
}


void metersim_getPower(metersim_ctx_t *ctx, metersim_power_t *ret)
{
	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
	}
	simulator_getPower(ctx->simulator, ret);
}


void metersim_getVector(metersim_ctx_t *ctx, metersim_vector_t *ret)
{
	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
	}
	simulator_getVector(ctx->simulator, ret);
}


void metersim_getThd(metersim_ctx_t *ctx, metersim_thd_t *ret)
{
	if (ctx->runner != NULL) {
		runner_update(ctx->runner);
	}
	simulator_getThd(ctx->simulator, ret);
}
