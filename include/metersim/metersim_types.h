/*
 * Type definitions for the SEM simulator
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#ifndef METERSIM_TYPES_H
#define METERSIM_TYPES_H


#include <stdint.h>
#include <complex.h>
#include <limits.h>


/* Status codes */
#define METERSIM_SUCCESS 0
#define METERSIM_ERROR   (-1)
#define METERSIM_REFUSE  (-2)


#define METERSIM_MAX_TARIFF_COUNT         16
#define METERSIM_MAX_SERIAL_NUMBER_LENGTH 32

#define METERSIM_MAX_SPEEDUP         10000
#define METERSIM_MAX_METERCONSTANT   UINT_MAX
#define METERSIM_MAX_INIT_ENERGY_REG ((int64_t)100 * 1000 * 1000 * 1000 * 1000) /* (Ws) */
#define METERSIM_MAX_VOLTAGE         ((double)400)                              /* (V) */
#define METERSIM_MAX_CURRENT         ((double)100)                              /* (A) */
#define METERSIM_MAX_THDU            ((double)1)
#define METERSIM_MAX_THDI            ((double)1)
#define METERSIM_MAX_FREQUENCY       ((double)1000) /* (Hz) */


#define METERSIM_NO_UPDATE_SCHEDULED (INT32_MAX)
#define METERSIM_UPDATE_NEEDED_NOW   (0)


typedef struct {
	int64_t value;
	double fraction;
} metersim_eregister_t;


typedef struct {
	metersim_eregister_t activePlus;    /* (Ws) */
	metersim_eregister_t activeMinus;   /* (Ws) */
	metersim_eregister_t reactive[4];   /* (vars) */
	metersim_eregister_t apparentPlus;  /* (VAs) */
	metersim_eregister_t apparentMinus; /* (VAs) */
} metersim_energy_t;


typedef struct {
	float frequency;       /* (Hz) */
	double voltage[3];     /* (V) */
	double current[3];     /* (A) */
	double currentNeutral; /* (A) */
	double uiAngle[3];     /* (degrees) */
	double ppAngle[2];     /* (degrees) */
} metersim_instant_t;


typedef struct {
	double truePower[3];     /* W */
	double reactivePower[3]; /* var */
	double apparentPower[3]; /* VA */
	double phi[3];           /* degrees */
} metersim_power_t;


typedef struct {
	double _Complex complexPower[3];
	double _Complex phaseVoltage[3];
	double _Complex phaseCurrent[3];
	double _Complex complexNeutral;
} metersim_vector_t;


typedef struct {
	float thdU[3];
	float thdI[3];
} metersim_thd_t;


typedef struct {
	double _Complex voltage[3];
	int32_t now;
	int64_t nowUtc;
} metersim_infoForDevice_t;


typedef struct {
	double _Complex current[3];
	int32_t nextUpdateTime;
} metersim_deviceResponse_t;


typedef struct metersim_deviceCtx_s metersim_deviceCtx_t;

#endif /* METERSIM_TYPES_H */
