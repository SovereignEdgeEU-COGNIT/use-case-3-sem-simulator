/*
 * Simulator of energy meter
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "cfgparser.h"
#include "simulator.h"
#include <metersim/metersim_types.h>
#include "metersim_types_int.h"
#include "log.h"
#include "calculator.h"
#include "devicemgr.h"


#define LOG_TAG "simulator : "


static const metersim_scenario_t defaultScenario = {
	.cfg = {
		.serialNumber = "",
		.speedup = 1,
		.phaseCount = 3,
		.tariffCount = 1,
		.startTime = -1,
	},
	.energy = NULL,
};


static inline int32_t min(int32_t a, int32_t b)
{
	return a < b ? a : b;
}


static inline int32_t max(int32_t a, int32_t b)
{
	return a > b ? a : b;
}


static void getValidUpdate(simulator_ctx_t *sctx)
{
	int status = 0;
	metersim_update_t next;

	for (;;) {
		next = sctx->nextUpdate;

		status = cfgparser_getUpdate(&sctx->cfgparserCtx, &next);

		if (status < 0) {
			continue;
		}

		if (status == 1) {
			sctx->nextConfigUpdateTime = METERSIM_NO_UPDATE_SCHEDULED;
			break;
		}

		if (next.timestamp > sctx->now && next.currentTariff < sctx->state.cfg.tariffCount) {
			sctx->nextUpdate = next;
			sctx->nextConfigUpdateTime = next.timestamp;
			break;
		}
	}
}


int32_t simulator_getNextUpdateTime(simulator_ctx_t *sctx)
{
	int32_t res;

	res = min(devicemgr_getNextUpdateTime(sctx->devmgrCtx), sctx->nextConfigUpdateTime);
	res = max(res, sctx->now);
	return res;
}


static void simulator_updateDevices(simulator_ctx_t *sctx, metersim_infoForDevice_t *info)
{
	calculator_bias_t bias = { 0 };
	info->now = sctx->now;
	info->nowUtc = sctx->now + sctx->state.cfg.startTime;
	devicemgr_updateDevices(sctx->devmgrCtx, &bias, info);

	sctx->bias = bias;
}


void simulator_stepForward(simulator_ctx_t *sctx, int32_t seconds)
{
	const int32_t end = sctx->now + seconds;
	int32_t next, nextDeviceUpdateTime;

	metersim_infoForDevice_t info;

	assert(end >= sctx->now);

	pthread_mutex_lock(&sctx->lock);
	do {
		next = min(simulator_getNextUpdateTime(sctx), end);
		nextDeviceUpdateTime = devicemgr_getNextUpdateTime(sctx->devmgrCtx);

		calculator_accumulateEnergy(&sctx->state, next - sctx->now);
		sctx->now = next;
		if (sctx->now == sctx->nextConfigUpdateTime) {
			sctx->currUpdate = sctx->nextUpdate;
			calculator_prepareInfoForDevice(&sctx->currUpdate, &info);
			getValidUpdate(sctx);

			simulator_updateDevices(sctx, &info);
			calculator_handleUpdate(&sctx->state, &sctx->currUpdate, &sctx->bias);
		}
		else if (nextDeviceUpdateTime != METERSIM_NO_UPDATE_SCHEDULED && sctx->now >= nextDeviceUpdateTime) {
			calculator_prepareInfoForDevice(&sctx->currUpdate, &info);
			simulator_updateDevices(sctx, &info);
			calculator_handleUpdate(&sctx->state, &sctx->currUpdate, &sctx->bias);
		}
		else {
			assert(end == sctx->now);
		}
	} while (end > sctx->now);
	pthread_mutex_unlock(&sctx->lock);

	assert(end == sctx->now);
}


simulator_ctx_t *simulator_init(const char *dir)
{
	simulator_ctx_t *sctx;
	int ret = 0;

	sctx = malloc(sizeof(simulator_ctx_t));
	if (sctx == NULL) {
		return NULL;
	}

	*sctx = (simulator_ctx_t) {
		.cfgparserCtx.updateFile = NULL,
		.nextConfigUpdateTime = 0,
		.now = -1
	};

	ret += cfgparser_init(&sctx->cfgparserCtx, dir);
	if (ret < 0) {
		free(sctx);
		return NULL;
	}

	ret += pthread_mutex_init(&sctx->lock, NULL);
	if (ret < 0) {
		cfgparser_close(&sctx->cfgparserCtx);
		free(sctx);
		return NULL;
	}

	/* Read Config */
	metersim_scenario_t scenario = defaultScenario;
	if (cfgparser_getScenario(&scenario, dir) != 0) {
		/* If reading scenario failed, continue anyway with default scenario */
		log_warning("Reading scenario config failed. Proceeding with default config.");
		scenario = defaultScenario;
		scenario.energy = calloc(scenario.cfg.tariffCount, sizeof(metersim_energy_t[3]));
		if (scenario.energy == NULL) {
			pthread_mutex_destroy(&sctx->lock);
			cfgparser_close(&sctx->cfgparserCtx);
			free(sctx);
			return NULL;
		}
	}
	if (scenario.cfg.startTime == -1) {
		scenario.cfg.startTime = (int64_t)time(NULL);
	}
	calculator_initScenario(&sctx->state, &scenario);

	sctx->devmgrCtx = devicemgr_init();
	if (sctx->devmgrCtx == NULL) {
		free(scenario.energy);
		pthread_mutex_destroy(&sctx->lock);
		cfgparser_close(&sctx->cfgparserCtx);
		free(sctx);
		return NULL;
	}

	getValidUpdate(sctx);
	sctx->now = 0;
	simulator_stepForward(sctx, 0); /* Calculate update at timestamp 0 */

	return sctx;
}


void simulator_destroy(simulator_ctx_t *sctx)
{
	devicemgr_destroy(sctx->devmgrCtx);
	free(sctx->state.energy);
	pthread_mutex_destroy(&sctx->lock);
	cfgparser_close(&sctx->cfgparserCtx);
	free(sctx);
}


void simulator_getTariffCount(simulator_ctx_t *sctx, int *retCount)
{
	pthread_mutex_lock(&sctx->lock);
	*retCount = sctx->state.cfg.tariffCount;
	pthread_mutex_unlock(&sctx->lock);
}


void simulator_getTariffCurrent(simulator_ctx_t *sctx, int *retIndex)
{
	pthread_mutex_lock(&sctx->lock);
	*retIndex = sctx->state.currentTariff;
	pthread_mutex_unlock(&sctx->lock);
}


int simulator_getSerialNumber(simulator_ctx_t *sctx, int idx, char *dstBuf, size_t dstBufLen)
{
	size_t serialNumberLen;

	if (idx != 0) {
		return METERSIM_ERROR;
	}

	pthread_mutex_lock(&sctx->lock);

	serialNumberLen = strlen(sctx->state.cfg.serialNumber);
	strncpy(dstBuf, sctx->state.cfg.serialNumber, dstBufLen);

	pthread_mutex_unlock(&sctx->lock);

	return (int)serialNumberLen;
}


void simulator_getPhaseCount(simulator_ctx_t *sctx, int *retCount)
{
	pthread_mutex_lock(&sctx->lock);
	*retCount = sctx->state.cfg.phaseCount;
	pthread_mutex_unlock(&sctx->lock);
}


void simulator_getFrequency(simulator_ctx_t *sctx, float *retFreq)
{
	pthread_mutex_lock(&sctx->lock);
	*retFreq = sctx->state.instant.frequency;
	pthread_mutex_unlock(&sctx->lock);
}


void simulator_getMeterConstant(simulator_ctx_t *sctx, unsigned int *ret)
{
	pthread_mutex_lock(&sctx->lock);
	*ret = sctx->state.cfg.meterConstant;
	pthread_mutex_unlock(&sctx->lock);
}


void simulator_getInstant(simulator_ctx_t *sctx, metersim_instant_t *ret)
{
	pthread_mutex_lock(&sctx->lock);
	*ret = sctx->state.instant;
	pthread_mutex_unlock(&sctx->lock);
}


void simulator_getEnergyTotal(simulator_ctx_t *sctx, metersim_energy_t *ret)
{
	*ret = (metersim_energy_t) { 0 };

	pthread_mutex_lock(&sctx->lock);
	for (int tariff = 0; tariff < sctx->state.cfg.tariffCount; tariff++) {
		for (int phase = 0; phase < sctx->state.cfg.phaseCount; phase++) {
			ret->activeMinus.value += sctx->state.energy[tariff][phase].activeMinus.value;
			ret->activePlus.value += sctx->state.energy[tariff][phase].activePlus.value;
			ret->apparentMinus.value += sctx->state.energy[tariff][phase].apparentMinus.value;
			ret->apparentPlus.value += sctx->state.energy[tariff][phase].apparentPlus.value;
			for (int i = 0; i < 4; i++) {
				ret->reactive[i].value += sctx->state.energy[tariff][phase].reactive[i].value;
			}
		}
	}
	pthread_mutex_unlock(&sctx->lock);
}


int simulator_getEnergyTariff(simulator_ctx_t *sctx, metersim_energy_t ret[3], int idxTariff)
{
	int status;

	memset(ret, 0, 3 * sizeof(metersim_energy_t));

	pthread_mutex_lock(&sctx->lock);

	if (idxTariff < sctx->state.cfg.tariffCount) {
		memcpy(ret, sctx->state.energy[idxTariff], sctx->state.cfg.phaseCount * sizeof(metersim_energy_t));
		status = METERSIM_SUCCESS;
	}
	else {
		status = METERSIM_ERROR;
	}
	pthread_mutex_unlock(&sctx->lock);

	return status;
}


void simulator_getPower(simulator_ctx_t *sctx, metersim_power_t *ret)
{
	pthread_mutex_lock(&sctx->lock);
	*ret = sctx->state.power;
	pthread_mutex_unlock(&sctx->lock);
}


void simulator_getVector(simulator_ctx_t *sctx, metersim_vector_t *ret)
{
	pthread_mutex_lock(&sctx->lock);
	*ret = sctx->state.vector;
	pthread_mutex_unlock(&sctx->lock);
}


void simulator_getThd(simulator_ctx_t *sctx, metersim_thd_t *ret)
{
	pthread_mutex_lock(&sctx->lock);
	*ret = sctx->state.thd;
	pthread_mutex_unlock(&sctx->lock);
}
