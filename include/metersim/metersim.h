/*
 * Smart Energy Meter Simulator API
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#ifndef METERSIM_H
#define METERSIM_H


#include <metersim/metersim_types.h>


/* Simulator context */
typedef struct metersim_ctx_s metersim_ctx_t;


/*
 * Allocate, create and initialize the simulator
 * Returns pointer to the context of the simulation.
 */
metersim_ctx_t *metersim_init(const char *dir);


/* Release simulator resources */
void metersim_free(metersim_ctx_t *ctx);


/* SIMULATION WITH RUNNER */

/*
 * Create simulation runner. Returns status code.
 * If `start` is set, runner is initialized starts automatically
 */
int metersim_createRunner(metersim_ctx_t *ctx, int start);


/*
 * Release runner resources
 * (Function stops the runner if it is still running)
 */
void metersim_destroyRunner(metersim_ctx_t *ctx);


/* Resume the runner. Returns status code. */
int metersim_resume(metersim_ctx_t *ctx);


/* Stop the runner. Returns status code. */
int metersim_pause(metersim_ctx_t *ctx, int32_t when);


/* Check whether runner is running. Returns status code. */
int metersim_isRunning(metersim_ctx_t *ctx);


/* Set runner speedup. Returns status code. */
int metersim_setSpeedup(metersim_ctx_t *ctx, uint16_t speedup);


/* SIMULATION WITHOUT RUNNER */

/* Simulate the passage of time */
int metersim_stepForward(metersim_ctx_t *ctx, uint32_t seconds);


/* DEVICES API */

/* Create new device by providing callback and its context. Returns nonnegative id of the created device, or -1 on error. */
int metersim_newDevice(metersim_ctx_t *ctx, void (*callback)(metersim_infoForDevice_t *, metersim_deviceResponse_t *, void *), void *callbackCtx);


/* Destroys device with given id. Returns status code. */
int metersim_destroyDevice(metersim_ctx_t *ctx, int id);


/* Notify the Simulator that some device has updated it's state */
void metersim_notifyDevicemgr(metersim_ctx_t *ctx);


/* DATA API */

/* Get number of available tariffs */
void metersim_getTariffCount(metersim_ctx_t *ctx, int *retCount);


/* Get index of the current tariff */
void metersim_getTariffCurrent(metersim_ctx_t *ctx, int *retIndex);


/* Get serial numbers of the meter as string. Returns length of the serial number. */
int metersim_getSerialNumber(metersim_ctx_t *ctx, int idx, char *dstBuf, size_t dstBufLen);


/* Get current absolute timestamp */
void metersim_getTimeUTC(metersim_ctx_t *ctx, int64_t *retTime);


/* Set current UTC time */
void metersim_setTimeUTC(metersim_ctx_t *ctx, int64_t time);


/* Get simulator uptime (seconds) */
void metersim_getUptime(metersim_ctx_t *ctx, int32_t *retSeconds);


/* Get number of phases */
void metersim_getPhaseCount(metersim_ctx_t *ctx, int *retCount);


/* Get mains frequency (Hz) */
void metersim_getFrequency(metersim_ctx_t *ctx, float *retFreq);


/* Get meter constant (Ws) */
void metersim_getMeterConstant(metersim_ctx_t *ctx, unsigned int *ret);


/* Get instantaneous values of Urms, Irms, and UI and phase-phase angles */
void metersim_getInstant(metersim_ctx_t *ctx, metersim_instant_t *ret);


/* Get energy registers grand total (all phases, all tariffs) */
void metersim_getEnergyTotal(metersim_ctx_t *ctx, metersim_energy_t *ret);


/* Get energy registers particularly per tariff. Returns status code. */
int metersim_getEnergyTariff(metersim_ctx_t *ctx, metersim_energy_t ret[3], int idxTariff);


/* Get power triangle (P, Q, S, phi angle) */
void metersim_getPower(metersim_ctx_t *ctx, metersim_power_t *ret);


/* Get vector data on complex number plane (fundamental freq) */
void metersim_getVector(metersim_ctx_t *ctx, metersim_vector_t *ret);


/* Get total harmonic distortion for voltage and current per phase */
void metersim_getThd(metersim_ctx_t *ctx, metersim_thd_t *ret);


#endif /* METERSIM_H */
