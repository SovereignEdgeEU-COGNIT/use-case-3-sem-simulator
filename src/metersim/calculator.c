/*
 * Calculations for the SEM simulator
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <pthread.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#include <stdbool.h>

#include "calculator.h"
#include <metersim/metersim_types.h>
#include "metersim_types_int.h"


static inline int64_t calculateApparent(int64_t active, int64_t reactive)
{
	return (int64_t)sqrt(active * active + reactive * reactive);
}


static void addEnergyRegisters(metersim_eregister_t *dst, metersim_eregister_t *src, int sign)
{
	dst->value += sign * src->value;
	dst->fraction += sign * src->fraction;
	if (fabs(dst->fraction) >= 1) {
		dst->value += (int64_t)floor(dst->fraction);
		dst->fraction -= floor(dst->fraction);
	}
	else if (dst->fraction < 0) {
		dst->value--;
		dst->fraction += 1;
	}
}


static void energyRegFromDouble(metersim_eregister_t *dst, double v)
{
	dst->value = (int64_t)floor(v);
	dst->fraction = v - floor(v);
}


void calculator_initScenario(metersim_state_t *state, metersim_scenario_t *scenario)
{
	state->energy = scenario->energy;

	for (int tariff = 0; tariff < scenario->cfg.tariffCount; tariff++) {
		for (int i = 0; i < scenario->cfg.phaseCount; i++) {
			state->energy[tariff][i].apparentPlus.value = calculateApparent(state->energy[tariff][i].activePlus.value, state->energy[tariff][i].reactive[0].value + state->energy[tariff][i].reactive[3].value);
			state->energy[tariff][i].apparentMinus.value = calculateApparent(state->energy[tariff][i].activeMinus.value, state->energy[tariff][i].reactive[1].value + state->energy[tariff][i].reactive[2].value);
		}
	}

	state->cfg = scenario->cfg;
}


void calculator_prepareInfoForDevice(metersim_update_t *upd, metersim_infoForDevice_t *info)
{
	for (int i = 0; i < 3; i++) {
		info->voltage[i] = upd->instant.voltage[i] * cexp(120.0 * i * M_PI / 180.0 * _Complex_I);
	}
}


void calculator_handleUpdate(metersim_state_t *state, metersim_update_t *upd, calculator_bias_t *bias)
{
	metersim_instant_t instant = { 0 };
	metersim_power_t power = { 0 };
	metersim_vector_t vector = { 0 };

	/* Calculate instant */
	instant = upd->instant;

	/* We assume phase-phase angles to be 120 degrees */
	instant.ppAngle[0] = 120;
	instant.ppAngle[1] = 120;

	double i_angle;
	vector.complexNeutral = 0;
	for (int i = 0; i < state->cfg.phaseCount; i++) {
		vector.phaseVoltage[i] = instant.voltage[i] * cexp(120.0 * i * M_PI / 180.0 * _Complex_I);

		i_angle = 120.0 * i + instant.uiAngle[i];
		vector.phaseCurrent[i] = instant.current[i] * cexp(i_angle * M_PI / 180.0 * _Complex_I);
		vector.phaseCurrent[i] += bias->current[i];

		vector.complexNeutral -= vector.phaseCurrent[i];
	}

	for (int i = 0; i < state->cfg.phaseCount; i++) {
		instant.current[i] = cabs(vector.phaseCurrent[i]);

		/* Get the ui angle as degrees in interval [0, 360) */
		if (instant.current[i] < (1e-10)) {
			/* If current is negligibly small, set it and uiAngle to zero */
			instant.current[i] = 0;
			instant.uiAngle[i] = 0;
		}
		else {
			instant.uiAngle[i] = carg(vector.phaseCurrent[i]) * 180.0 / M_PI - 120.0 * i;
			while (instant.uiAngle[i] < 0) {
				instant.uiAngle[i] += 360.0;
			}
		}
	}
	instant.currentNeutral = cabs(vector.complexNeutral);

	for (int i = 0; i < state->cfg.phaseCount; i++) {
		power.apparentPower[i] = instant.voltage[i] * instant.current[i];
		power.truePower[i] = cos(instant.uiAngle[i] * M_PI / 180.0) * power.apparentPower[i];
		power.reactivePower[i] = sin(instant.uiAngle[i] * M_PI / 180.0) * power.apparentPower[i];
		power.phi[i] = instant.uiAngle[i];
	}

	for (int i = 0; i < state->cfg.phaseCount; i++) {
		vector.complexPower[i] = power.apparentPower[i] * cexp(instant.uiAngle[i] * M_PI / 180.0 * _Complex_I);
	}

	instant.currentNeutral = cabs(vector.complexNeutral);

	state->currentTariff = upd->currentTariff;
	state->instant = instant;
	state->thd = upd->thd;
	state->power = power;
	state->vector = vector;
}


void calculator_accumulateEnergy(metersim_state_t *state, int32_t dt)
{
	int quadrant;

	bool isPositiveEreactive;
	metersim_eregister_t eapparent;
	metersim_eregister_t ereactive;
	metersim_eregister_t eactive;

	for (int i = 0; i < state->cfg.phaseCount; i++) {
		energyRegFromDouble(&eapparent, (double)dt * state->power.apparentPower[i]);
		energyRegFromDouble(&ereactive, (double)dt * state->power.reactivePower[i]);
		energyRegFromDouble(&eactive, (double)dt * state->power.truePower[i]);

		isPositiveEreactive = ereactive.value >= 0;

		if (eactive.value < 0) {
			quadrant = isPositiveEreactive ? 2 : 3;
			addEnergyRegisters(&state->energy[state->currentTariff][i].activeMinus, &eactive, -1);
			addEnergyRegisters(&state->energy[state->currentTariff][i].apparentMinus, &eapparent, 1);
		}
		else {
			quadrant = isPositiveEreactive ? 1 : 4;
			addEnergyRegisters(&state->energy[state->currentTariff][i].activePlus, &eactive, 1);
			addEnergyRegisters(&state->energy[state->currentTariff][i].apparentPlus, &eapparent, 1);
		}

		addEnergyRegisters(&state->energy[state->currentTariff][i].reactive[quadrant - 1], &ereactive, isPositiveEreactive ? 1 : -1);
	}
}


void calculator_accumulateBias(calculator_bias_t *bias, metersim_deviceResponse_t *res)
{
	for (int i = 0; i < 3; i++) {
		bias->current[i] += res->current[i];
	}
}
