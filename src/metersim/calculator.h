/*
 * Calculations for the SEM simulator
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <metersim/metersim_types.h>
#include "metersim_types_int.h"

typedef struct {
	double _Complex current[3];
} calculator_bias_t;


void calculator_initScenario(metersim_state_t *state, metersim_scenario_t *scenario);


void calculator_prepareInfoForDevice(metersim_update_t *upd, metersim_infoForDevice_t *info);


void calculator_handleUpdate(metersim_state_t *state, metersim_update_t *upd, calculator_bias_t *bias);


void calculator_accumulateEnergy(metersim_state_t *state, int32_t dt);


void calculator_accumulateBias(calculator_bias_t *bias, metersim_deviceResponse_t *res);

#endif /* CALCULATOR_H */
