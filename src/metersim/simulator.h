/*
 * Simulator of energy meter
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <pthread.h>
#include <stdbool.h>
#include <limits.h>

#include "cfgparser.h"
#include "time_machine.h"
#include <metersim/metersim_types.h>
#include "devicemgr.h"


typedef struct {
	metersim_state_t state;
	cfgparser_ctx_t cfgparserCtx;
	int32_t now; /* virtual seconds elapsed from the beginning of the simulation */
	int32_t nextConfigUpdateTime;
	devicemgr_ctx_t *devmgrCtx;

	metersim_update_t currUpdate;
	metersim_update_t nextUpdate;

	calculator_bias_t bias;

	pthread_mutex_t lock;
} simulator_ctx_t;


void simulator_stepForward(simulator_ctx_t *sctx, int32_t seconds);


int32_t simulator_getNextUpdateTime(simulator_ctx_t *sctx);


simulator_ctx_t *simulator_init(const char *dir);


void simulator_destroy(simulator_ctx_t *sctx);


void simulator_getTariffCount(simulator_ctx_t *sctx, int *retCount);


void simulator_getTariffCurrent(simulator_ctx_t *sctx, int *retIndex);


int simulator_getSerialNumber(simulator_ctx_t *sctx, int idx, char *dstBuf, size_t dstBufLen);


void simulator_getPhaseCount(simulator_ctx_t *sctx, int *retCount);


void simulator_getFrequency(simulator_ctx_t *sctx, float *retFreq);


void simulator_getMeterConstant(simulator_ctx_t *sctx, unsigned int *ret);


void simulator_getInstant(simulator_ctx_t *sctx, metersim_instant_t *ret);


void simulator_getEnergyTotal(simulator_ctx_t *sctx, metersim_energy_t *ret);


int simulator_getEnergyTariff(simulator_ctx_t *sctx, metersim_energy_t ret[3], int idxTariff);


void simulator_getPower(simulator_ctx_t *sctx, metersim_power_t *ret);


void simulator_getVector(simulator_ctx_t *sctx, metersim_vector_t *ret);


void simulator_getThd(simulator_ctx_t *sctx, metersim_thd_t *ret);

#endif /* SIMULATOR_H */
