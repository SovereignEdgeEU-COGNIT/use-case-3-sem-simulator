/*
 * Unit tests of the calculations
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <complex.h>
#include <stdint.h>
#include <stdlib.h>

#include <unity.h>
#include <metersim/metersim_types.h>

#include "metersim_types_int.h"
#include "calculator.h"

#define TEST_DOUBLE_WITH_EPSILON_MESSAGE(expected, actual, epsilon, msg) TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(expected *UNITY_DOUBLE_PRECISION + epsilon, expected, actual, msg)
#define TEST_DOUBLE_WITH_EPSILON(expected, actual, epsilon)              TEST_ASSERT_DOUBLE_WITHIN(expected *UNITY_DOUBLE_PRECISION + epsilon, expected, actual)


static struct {
	metersim_state_t state;
} common;


static metersim_update_t upd[3] = {
	[0] = {
		.instant = {
			.current = { 50, 50, 50 },
			.voltage = { 220, 220, 220 },
			.uiAngle = { 0, 30, 90 },
		},
		.currentTariff = 0,
	},
	[1] = {
		.instant = {
			.current = { 50, 50, 40 },
			.voltage = { 220, 220, 220 },
			.uiAngle = { 315, 135, 225 },
		},
		.currentTariff = 0,
	},
	[2] = {
		.instant = {
			.current = { METERSIM_MAX_CURRENT, METERSIM_MAX_CURRENT, METERSIM_MAX_CURRENT },
			.voltage = { METERSIM_MAX_VOLTAGE, METERSIM_MAX_VOLTAGE, METERSIM_MAX_VOLTAGE },
			.uiAngle = { 0, 0, 35 },
		},
		.currentTariff = 0,
	},
};


static calculator_bias_t bias = { 0 };


void setUp(void)
{
	common.state = (metersim_state_t) { 0 };
	common.state.energy = calloc(1, sizeof(metersim_energy_t[3]));

	common.state.cfg = (metersim_config_t) {
		.tariffCount = 1,
		.phaseCount = 3,
	};
}


void tearDown(void)
{
	free(common.state.energy);
}


void clearState(void)
{
	for (int i = 0; i < 3; i++) {
		common.state.energy[0][i] = (metersim_energy_t) { 0 };
	}
}


static double getDouble(metersim_eregister_t reg)
{
	return (double)reg.value + (double)reg.fraction;
}


static void comparePower(metersim_power_t *expected, metersim_power_t *actual, double epsilon)
{
	TEST_DOUBLE_WITH_EPSILON(expected->truePower[0], actual->truePower[0], epsilon);
	TEST_DOUBLE_WITH_EPSILON(expected->reactivePower[0], actual->reactivePower[0], epsilon);
	TEST_DOUBLE_WITH_EPSILON(expected->apparentPower[0], actual->apparentPower[0], epsilon);

	TEST_DOUBLE_WITH_EPSILON(expected->truePower[1], actual->truePower[1], epsilon);
	TEST_DOUBLE_WITH_EPSILON(expected->reactivePower[1], actual->reactivePower[1], epsilon);
	TEST_DOUBLE_WITH_EPSILON(expected->apparentPower[1], actual->apparentPower[1], epsilon);

	TEST_DOUBLE_WITH_EPSILON(expected->truePower[2], actual->truePower[2], epsilon);
	TEST_DOUBLE_WITH_EPSILON(expected->reactivePower[2], actual->reactivePower[2], epsilon);
	TEST_DOUBLE_WITH_EPSILON(expected->apparentPower[2], actual->apparentPower[2], epsilon);
}


static void compareEnergy(double *expected, metersim_energy_t *actual, const char *msg, double epsilon)
{
	TEST_DOUBLE_WITH_EPSILON_MESSAGE(expected[0], getDouble(actual->activePlus), epsilon, msg);
	TEST_DOUBLE_WITH_EPSILON_MESSAGE(expected[1], getDouble(actual->activeMinus), epsilon, msg);
	TEST_DOUBLE_WITH_EPSILON_MESSAGE(expected[2], getDouble(actual->reactive[0]), epsilon, msg);
	TEST_DOUBLE_WITH_EPSILON_MESSAGE(expected[3], getDouble(actual->reactive[1]), epsilon, msg);
	TEST_DOUBLE_WITH_EPSILON_MESSAGE(expected[4], getDouble(actual->reactive[2]), epsilon, msg);
	TEST_DOUBLE_WITH_EPSILON_MESSAGE(expected[5], getDouble(actual->reactive[3]), epsilon, msg);
	TEST_DOUBLE_WITH_EPSILON_MESSAGE(expected[6], getDouble(actual->apparentPlus), epsilon, msg);
	TEST_DOUBLE_WITH_EPSILON_MESSAGE(expected[7], getDouble(actual->apparentMinus), epsilon, msg);
}


static void testPower(void)
{
	metersim_power_t expectedPower[2] = {
		[0] = {
			.truePower = {
				50 * 220,
				50 * 220 * sqrt(3) / 2,
				50 * 220 * cos(90 * M_PI / 180.0), /* this should be 0, but is -0.0004808253 */
			},
			.reactivePower = {
				0,
				50 * 220 / 2,
				50 * 220,
			},
			.apparentPower = {
				50 * 220,
				50 * 220,
				50 * 220,
			},
			.phi = {
				0,
				30,
				90,
			},
		},
		[1] = {
			.truePower = {
				50 * 220 * sqrt(2) / 2,
				-50 * 220 * sqrt(2) / 2,
				-40 * 220 * sqrt(2) / 2,
			},
			.reactivePower = {
				-50 * 220 * sqrt(2) / 2,
				50 * 220 * sqrt(2) / 2,
				-40 * 220 * sqrt(2) / 2,
			},
			.apparentPower = {
				50 * 220,
				50 * 220,
				40 * 220,
			},
			.phi = {
				315,
				135,
				225,
			},
		},
	};

	calculator_handleUpdate(&common.state, &upd[0], &bias);
	comparePower(&expectedPower[0], &common.state.power, 0);
	clearState();

	calculator_handleUpdate(&common.state, &upd[1], &bias);
	comparePower(&expectedPower[1], &common.state.power, 0);
	clearState();
}


static void testEnergy(void)
{
	double sin45 = sin(45.0 * M_PI / 180.0);
	double cos45 = cos(45.0 * M_PI / 180.0);

	double expected1[3][8] = {
		[0] = {
			3 * 50 * 220, /* activePlus */
			0,            /* activeMinus */
			0,            /* reactive I */
			0,            /* reactive II */
			0,            /* reactive III */
			0,            /* reactive IV */
			3 * 50 * 220, /* apparentPlus */
			0,            /* apparentMinus */
		},
		[1] = {
			3 * 50 * 110 * sqrt(3), /* activePlus */
			0,                      /* activeMinus */
			3 * 50 * 110,           /* reactive I */
			0,                      /* reactive II */
			0,                      /* reactive III */
			0,                      /* reactive IV */
			3 * 50 * 220,           /* apparentPlus */
			0,                      /* apparentMinus */
		},
		[2] = {
			0,            /* activePlus */
			0,            /* activeMinus */
			3 * 50 * 220, /* reactive I + reactive II */
			0,            /* placeholder */
			0,            /* reactive III */
			0,            /* reactive IV */
			3 * 50 * 220, /* apparentPlus + apparentMinus */
			0,            /* placeholder */
		},
	};

	double expected2[3][8] = {
		[0] = {
			7 * 50 * 220 * cos45, /* activePlus */
			0,                    /* activeMinus */
			0,                    /* reactive I */
			0,                    /* reactive II */
			0,                    /* reactive III */
			7 * 50 * 220 * sin45, /* reactive IV */
			7 * 50 * 220,         /* apparentPlus */
			0,                    /* apparentMinus */
		},
		[1] = {
			0,                    /* activePlus */
			7 * 50 * 220 * cos45, /* activeMinus */
			0,                    /* reactive I */
			7 * 50 * 220 * sin45, /* reactive II */
			0,                    /* reactive III */
			0,                    /* reactive IV */
			0,                    /* apparentPlus */
			7 * 50 * 220,         /* apparentMinus */
		},
		[2] = {
			0,                    /* activePlus */
			7 * 40 * 220 * cos45, /* activeMinus */
			0,                    /* reactive I */
			0,                    /* reactive II */
			7 * 40 * 220 * sin45, /* reactive III */
			0,                    /* reactive IV */
			0,                    /* apparentPlus */
			7 * 40 * 220,         /* apparentMinus */
		},

	};

	/* TESTING UPD1 */
	calculator_handleUpdate(&common.state, &upd[0], &bias);
	calculator_accumulateEnergy(&common.state, 3);

	compareEnergy(expected1[0], &common.state.energy[0][0], "phase 1", 0);
	compareEnergy(expected1[1], &common.state.energy[0][1], "phase 2", 0);

	/* NOTE: Because ui_angle[2] = 90, it is at the border of quadrant 1 and 2 and we should not expect it to fall into a specific one of these two */
	TEST_DOUBLE_WITH_EPSILON(expected1[2][0], common.state.energy[0][2].activePlus.value, 0);
	TEST_DOUBLE_WITH_EPSILON(expected1[2][1], common.state.energy[0][2].activeMinus.value, 0);
	TEST_DOUBLE_WITH_EPSILON(expected1[2][2], common.state.energy[0][2].reactive[0].value + common.state.energy[0][2].reactive[1].value, 0);
	TEST_DOUBLE_WITH_EPSILON(expected1[2][4], common.state.energy[0][2].reactive[2].value, 0);
	TEST_DOUBLE_WITH_EPSILON(expected1[2][5], common.state.energy[0][2].reactive[3].value, 0);
	TEST_DOUBLE_WITH_EPSILON(expected1[2][6], common.state.energy[0][2].apparentPlus.value + common.state.energy[0][2].apparentMinus.value, 0);
	clearState();

	/* TESTING UPD2 */
	calculator_handleUpdate(&common.state, &upd[1], &bias);

	/* Testing composition of multiple accumulations */
	calculator_accumulateEnergy(&common.state, 4);
	calculator_accumulateEnergy(&common.state, 3);

	compareEnergy(expected2[0], &common.state.energy[0][0], "phase 1", 0);
	compareEnergy(expected2[1], &common.state.energy[0][1], "phase 2", 0);
	compareEnergy(expected2[2], &common.state.energy[0][2], "phase 3", 0);
	clearState();
}


/* Test maximal values of current and voltage */
void testMaxValues(void)
{
	int32_t dt = 24 * 3600;
	double expected[3][8] = {
		[0] = {
			METERSIM_MAX_CURRENT * METERSIM_MAX_VOLTAGE * dt, /* activePlus */
			0,                                                /* activeMinus */
			0,                                                /* reactive I */
			0,                                                /* reactive II */
			0,                                                /* reactive III */
			0,                                                /* reactive IV */
			METERSIM_MAX_CURRENT * METERSIM_MAX_VOLTAGE * dt, /* apparentPlus */
			0,                                                /* apparentMinus */
		},
		[1] = {
			METERSIM_MAX_CURRENT * METERSIM_MAX_VOLTAGE * dt, /* activePlus */
			0,                                                /* activeMinus */
			0,                                                /* reactive I */
			0,                                                /* reactive II */
			0,                                                /* reactive III */
			0,                                                /* reactive IV */
			METERSIM_MAX_CURRENT * METERSIM_MAX_VOLTAGE * dt, /* apparentPlus */
			0,                                                /* apparentMinus */
		},
		[2] = {
			METERSIM_MAX_CURRENT * METERSIM_MAX_VOLTAGE * cos(35.0 * M_PI / 180.0) * dt, /* activePlus */
			0,                                                                           /* activeMinus */
			METERSIM_MAX_CURRENT * METERSIM_MAX_VOLTAGE * sin(35.0 * M_PI / 180.0) * dt, /* reactive I */
			0,                                                                           /* reactive II */
			0,                                                                           /* reactive III */
			0,                                                                           /* reactive IV */
			METERSIM_MAX_CURRENT * METERSIM_MAX_VOLTAGE * dt,                            /* apparentPlus */
			0,                                                                           /* apparentMinus */
		},

	};

	metersim_power_t expectedPower = {
		.truePower = {
			METERSIM_MAX_CURRENT * METERSIM_MAX_VOLTAGE,
			METERSIM_MAX_CURRENT * METERSIM_MAX_VOLTAGE,
			METERSIM_MAX_CURRENT * METERSIM_MAX_VOLTAGE * cos(35.0 * M_PI / 180.0),
		},
		.reactivePower = {
			0,
			0,
			METERSIM_MAX_CURRENT * METERSIM_MAX_VOLTAGE * sin(35.0 * M_PI / 180.0),
		},
		.apparentPower = {
			METERSIM_MAX_CURRENT * METERSIM_MAX_VOLTAGE,
			METERSIM_MAX_CURRENT * METERSIM_MAX_VOLTAGE,
			METERSIM_MAX_CURRENT * METERSIM_MAX_VOLTAGE,
		},
		.phi = {
			0,
			0,
			35,
		},
	};

	calculator_handleUpdate(&common.state, &upd[2], &bias);

	/*
	 * Using non-zero epsilon here as otherwise test fails for reactive power (expected 0, is ca. 8e-12).
	 * Happens after introducing devices to the calculator.
	 * This inaccuracy is caused by transferring current (with the angle) from double to complex and then back to double.
	 * Same with reactive energy in this test.
	 */
	comparePower(&expectedPower, &common.state.power, (1e-11));

	calculator_accumulateEnergy(&common.state, dt);
	compareEnergy(expected[0], &common.state.energy[0][0], "phase 1", (1e-6));
	compareEnergy(expected[1], &common.state.energy[0][1], "phase 2", (1e-6));
	compareEnergy(expected[2], &common.state.energy[0][2], "phase 3", (1e-6));
}


int main(void)
{
	RUN_TEST(testPower);
	RUN_TEST(testEnergy);
	RUN_TEST(testMaxValues);

	return 0;
}
