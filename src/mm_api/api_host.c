/*
 * meter messaging - user API implementation for the SEM simulator
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <mm_api.h>
#include <metersim/metersim.h>
#include <metersim/metersim_types.h>


struct mm_ctx_S { /* TODO: Consider multiple clients per simulator */
	metersim_ctx_t *msCtx;
	bool connected;
};


static int checkStatus(mm_ctx_T *ctx)
{
	if (ctx == NULL || !ctx->connected) {
		return MM_ERROR;
	}

	return MM_SUCCESS;
}


mm_ctx_T *mm_init(void)
{
	mm_ctx_T *ctx;
	ctx = malloc(sizeof(mm_ctx_T));
	if (ctx != NULL) {
		ctx->connected = false;
	}

	return ctx;
}


int mm_free(mm_ctx_T *ctx)
{
	if (ctx == NULL || ctx->connected) {
		return MM_ERROR;
	}

	free(ctx);
	return MM_SUCCESS;
}


int mm_connect(mm_ctx_T *ctx, struct mm_address *addr, void *)
{
	if (ctx == NULL || ctx->connected) {
		return MM_ERROR;
	}

	ctx->msCtx = metersim_init(addr->addr);
	if (ctx->msCtx == NULL) {
		return MM_ERROR;
	}

	if (metersim_createRunner(ctx->msCtx, 1) < 0) {
		return MM_ERROR;
	}

	ctx->connected = true;

	return MM_SUCCESS;
}


int mm_disconnect(mm_ctx_T *ctx)
{
	if (ctx == NULL || !ctx->connected) {
		return MM_ERROR;
	}

	metersim_destroyRunner(ctx->msCtx);
	metersim_free(ctx->msCtx);
	ctx->connected = false;

	return MM_SUCCESS;
}


int mm_sync(mm_ctx_T *ctx)
{
	/* TODO: Adjust this function to its usage on an actual SEM */
	(void)ctx;
	return MM_SUCCESS;
}


int mm_transactAbort(mm_ctx_T *ctx)
{
	/* TODO: Adjust this function to its usage on an actual SEM */
	(void)ctx;
	return MM_SUCCESS;
}


int mm_getTariffCount(mm_ctx_T *ctx, int *retCount)
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	metersim_getTariffCount(ctx->msCtx, retCount);
	return MM_SUCCESS;
}


int mm_getTariffCurrent(mm_ctx_T *ctx, int *retIndex)
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	metersim_getTariffCurrent(ctx->msCtx, retIndex);
	return MM_SUCCESS;
}


int mm_getSerialNumber(mm_ctx_T *ctx, int idx, char *dstBuf, size_t dstBufLen)
{
	int status;
	int serialNumberLen;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	serialNumberLen = metersim_getSerialNumber(ctx->msCtx, idx, dstBuf, dstBufLen);

	return serialNumberLen >= 0 ? serialNumberLen : MM_ERROR; /* Negative values on invalid idx */
}


int mm_getTimeUTC(mm_ctx_T *ctx, int64_t *retTime)
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	metersim_getTimeUTC(ctx->msCtx, retTime);
	return MM_SUCCESS;
}


int mm_getUptime(mm_ctx_T *ctx, int32_t *retSeconds)
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	metersim_getUptime(ctx->msCtx, retSeconds);
	return MM_SUCCESS;
}


int mm_getTemperature(mm_ctx_T *ctx, float *retTemp)
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	*retTemp = 26.0;
	return MM_SUCCESS;
}


int mm_getMeterDomain(mm_ctx_T *ctx, unsigned int *retDomainCapFlags)
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	*retDomainCapFlags = MM_DOMAIN_ELECTRICITY;
	return MM_SUCCESS;
}


int mme_getPhaseCount(mm_ctx_T *ctx, int *retCount)
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	metersim_getPhaseCount(ctx->msCtx, retCount);
	return MM_SUCCESS;
}


int mme_getFrequency(mm_ctx_T *ctx, float *retFreq)
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	metersim_getFrequency(ctx->msCtx, retFreq);
	return MM_SUCCESS;
}


int mme_getMeterConstant(mm_ctx_T *ctx, unsigned int *ret)
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	metersim_getMeterConstant(ctx->msCtx, ret);
	return MM_SUCCESS;
}


int mme_getInstant(mm_ctx_T *ctx, struct mme_dataInstant *ret)
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	metersim_instant_t instant;
	metersim_getInstant(ctx->msCtx, &instant);

	for (int i = 0; i < 3; i++) {
		ret->i[i] = (float)instant.current[i];
		ret->u[i] = (float)instant.voltage[i];
		ret->uiAngle[i] = (float)instant.uiAngle[i];
		if (i < 2) {
			ret->ppAngle[i] = (float)instant.ppAngle[i];
		}
	}
	ret->in = (float)instant.currentNeutral;
	return MM_SUCCESS;
}


int mme_getEnergyTotal(mm_ctx_T *ctx, struct mme_dataEnergy *ret)
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	metersim_energy_t energy;
	metersim_getEnergyTotal(ctx->msCtx, &energy);

	for (int i = 0; i < 3; i++) {
		ret->activeMinus = energy.activeMinus.value;
		ret->activePlus = energy.activePlus.value;
		ret->apparentMinus = energy.apparentMinus.value;
		ret->apparentPlus = energy.apparentPlus.value;
		for (int j = 0; j < 4; j++) {
			ret->reactive[j] = energy.reactive[j].value;
		}
	}

	return MM_SUCCESS;
}


int mme_getEnergyTariff(mm_ctx_T *ctx, struct mme_dataEnergy ret[3], int idxTariff)
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	metersim_energy_t energy[3];
	if (metersim_getEnergyTariff(ctx->msCtx, energy, idxTariff) != 0) {
		return MM_ERROR;
	}

	for (int i = 0; i < 3; i++) {
		ret[i].activeMinus = energy[i].activeMinus.value;
		ret[i].activePlus = energy[i].activePlus.value;
		ret[i].apparentMinus = energy[i].apparentMinus.value;
		ret[i].apparentPlus = energy[i].apparentPlus.value;
		for (int j = 0; j < 4; j++) {
			ret[i].reactive[j] = energy[i].reactive[j].value;
		}
	}

	return MM_SUCCESS;
}


int mme_getPower(mm_ctx_T *ctx, struct mme_dataPower *ret)
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	metersim_power_t power;
	metersim_getPower(ctx->msCtx, &power);

	for (int i = 0; i < 3; i++) {
		ret->p[i] = (float)power.truePower[i];
		ret->q[i] = (float)power.reactivePower[i];
		ret->s[i] = (float)power.apparentPower[i];
		ret->phi[i] = (float)power.phi[i];
	}
	return MM_SUCCESS;
}


int mme_getVector(mm_ctx_T *ctx, struct mme_dataVector *ret)
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	metersim_vector_t vector;
	metersim_getVector(ctx->msCtx, &vector);

	for (int i = 0; i < 3; i++) {
		ret->s[i] = (float _Complex)vector.complexPower[i];
		ret->u[i] = (float _Complex)vector.phaseVoltage[i];
		ret->i[i] = (float _Complex)vector.phaseCurrent[i];
	}
	ret->in = (float _Complex)vector.complexNeutral;
	return MM_SUCCESS;
}


int mme_getThdI(mm_ctx_T *ctx, float retThdI[3])
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	metersim_thd_t thd;
	metersim_getThd(ctx->msCtx, &thd);

	for (int i = 0; i < 3; i++) {
		retThdI[i] = thd.thdI[i];
	}
	return MM_SUCCESS;
}


int mme_getThdU(mm_ctx_T *ctx, float retThdU[3])
{
	int status;
	status = checkStatus(ctx);
	if (status != MM_SUCCESS) {
		return status;
	}

	metersim_thd_t thd;
	metersim_getThd(ctx->msCtx, &thd);

	for (int i = 0; i < 3; i++) {
		retThdU[i] = thd.thdU[i];
	}
	return MM_SUCCESS;
}
