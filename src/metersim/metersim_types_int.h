/*
 * Definitions of internal types of SEM simulator
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#ifndef METERSIM_TYPES_INT_H
#define METERSIM_TYPES_INT_H

#include <stdint.h>
#include <pthread.h>

#include <metersim/metersim_types.h>

typedef struct {
	char serialNumber[METERSIM_MAX_SERIAL_NUMBER_LENGTH];
	int64_t startTime;
	uint8_t tariffCount;
	uint8_t phaseCount;
	unsigned int meterConstant;
	uint16_t speedup;
} metersim_config_t;


typedef struct {
	metersim_config_t cfg;
	metersim_energy_t (*energy)[3];
} metersim_scenario_t;


typedef struct {
	int32_t timestamp;
	uint8_t currentTariff;

	/* Data */
	metersim_instant_t instant;
	metersim_thd_t thd;
} metersim_update_t;


typedef struct {
	metersim_config_t cfg;

	uint8_t currentTariff;

	/* Data */
	metersim_instant_t instant;
	metersim_power_t power;
	metersim_vector_t vector;
	metersim_thd_t thd;

	metersim_energy_t (*energy)[3];
} metersim_state_t;


#endif /* METERSIM_TYPES_INT_H */
