/*
 * Unit tests of the cfgparser
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include <metersim/metersim_types.h>
#include "cfgparser.h"
#include <unity.h>


static struct {
	metersim_scenario_t scenario;
	metersim_update_t upd;
} common;


void setUp(void)
{
}


void tearDown(void)
{
}


static void compareEnergy(metersim_energy_t *expected, metersim_energy_t *actual, const char *msg)
{
	TEST_ASSERT_EQUAL_INT64_MESSAGE(expected->activePlus.value, actual->activePlus.value, msg);
	TEST_ASSERT_EQUAL_INT64_MESSAGE(expected->activeMinus.value, actual->activeMinus.value, msg);
	TEST_ASSERT_EQUAL_INT64_MESSAGE(expected->reactive[0].value, actual->reactive[0].value, msg);
	TEST_ASSERT_EQUAL_INT64_MESSAGE(expected->reactive[0].value, actual->reactive[0].value, msg);
	TEST_ASSERT_EQUAL_INT64_MESSAGE(expected->reactive[2].value, actual->reactive[2].value, msg);
	TEST_ASSERT_EQUAL_INT64_MESSAGE(expected->reactive[3].value, actual->reactive[3].value, msg);
	TEST_ASSERT_EQUAL_INT64_MESSAGE(expected->apparentPlus.value, actual->apparentPlus.value, msg);
	TEST_ASSERT_EQUAL_INT64_MESSAGE(expected->apparentMinus.value, actual->apparentMinus.value, msg);
}


static void compareUpdates(metersim_update_t *expected, metersim_update_t *actual)
{
	TEST_ASSERT_EQUAL_INT(expected->timestamp, actual->timestamp);
	TEST_ASSERT_EQUAL_UINT8(expected->currentTariff, actual->currentTariff);
	TEST_ASSERT_EQUAL_DOUBLE(expected->instant.frequency, actual->instant.frequency);
	TEST_ASSERT_EQUAL_DOUBLE(expected->instant.voltage[0], actual->instant.voltage[0]);
	TEST_ASSERT_EQUAL_DOUBLE(expected->instant.voltage[1], actual->instant.voltage[1]);
	TEST_ASSERT_EQUAL_DOUBLE(expected->instant.voltage[2], actual->instant.voltage[2]);
	TEST_ASSERT_EQUAL_DOUBLE(expected->instant.current[0], actual->instant.current[0]);
	TEST_ASSERT_EQUAL_DOUBLE(expected->instant.current[1], actual->instant.current[1]);
	TEST_ASSERT_EQUAL_DOUBLE(expected->instant.current[2], actual->instant.current[2]);
	TEST_ASSERT_EQUAL_DOUBLE(expected->instant.uiAngle[0], actual->instant.uiAngle[0]);
	TEST_ASSERT_EQUAL_DOUBLE(expected->instant.uiAngle[1], actual->instant.uiAngle[1]);
	TEST_ASSERT_EQUAL_DOUBLE(expected->instant.uiAngle[2], actual->instant.uiAngle[2]);
	TEST_ASSERT_EQUAL_FLOAT(expected->thd.thdU[0], actual->thd.thdU[0]);
	TEST_ASSERT_EQUAL_FLOAT(expected->thd.thdU[1], actual->thd.thdU[1]);
	TEST_ASSERT_EQUAL_FLOAT(expected->thd.thdU[2], actual->thd.thdU[2]);
	TEST_ASSERT_EQUAL_FLOAT(expected->thd.thdI[0], actual->thd.thdI[0]);
	TEST_ASSERT_EQUAL_FLOAT(expected->thd.thdI[1], actual->thd.thdI[1]);
	TEST_ASSERT_EQUAL_FLOAT(expected->thd.thdI[2], actual->thd.thdI[2]);
}


static void compareScenarios(metersim_scenario_t *expected, metersim_scenario_t *actual)
{
	TEST_ASSERT_EQUAL_STRING(expected->cfg.serialNumber, actual->cfg.serialNumber);
	TEST_ASSERT_EQUAL_UINT8(expected->cfg.tariffCount, actual->cfg.tariffCount);
	TEST_ASSERT_EQUAL_UINT8(expected->cfg.phaseCount, actual->cfg.phaseCount);
	TEST_ASSERT_EQUAL_UINT32(expected->cfg.meterConstant, actual->cfg.meterConstant);
	TEST_ASSERT_EQUAL_INT(expected->cfg.speedup, actual->cfg.speedup);

	if (expected->cfg.tariffCount != actual->cfg.tariffCount) {
		return;
	}

	for (int i = 0; i < expected->cfg.tariffCount; i++) {
		compareEnergy(&expected->energy[i][0], &actual->energy[i][0], "");
		compareEnergy(&expected->energy[i][1], &actual->energy[i][1], "");
		compareEnergy(&expected->energy[i][2], &actual->energy[i][2], "");
	}
}


static void testScenario(void)
{
	metersim_energy_t scenario1Energy[12][3] = {
		[0][2] = {
			.activePlus.value = 1236,
		},
		[2][1] = {
			.activePlus.value = 42949672950, /* This is outside uint32 range */
		},
		[5][0] = {
			.activePlus.value = 1235,
		},
		[5][2] = {
			.reactive[3].value = 78,
		},
		[10][0] = {
			.reactive[0].value = 11,
		},
		[11][0] = {
			.activeMinus.value = 7,
		},
	};


	metersim_scenario_t scenario1 = {
		.cfg = {
			.serialNumber = "abcde54321",
			.tariffCount = 12,
			.phaseCount = 2,
			.meterConstant = 7200,
			.speedup = 4,
		},
		.energy = scenario1Energy,
	};

	compareScenarios(&scenario1, &common.scenario);
}


static void testUpdate(void)
{
	metersim_update_t upd1 = {
		.timestamp = 0,
		.currentTariff = 11,
		.instant = {
			.frequency = 50.81,
			.voltage = { 310, 320, 330 },
			.current = { 30, 20, 10 },
			.uiAngle = { 15.88, 25.999, 35.1234 },
		},
		.thd = {
			.thdU = { 0.5, 0.512, 0.589 },
			.thdI = { 0.689, 0.45, 0.25 },
		},
	};

	compareUpdates(&upd1, &common.upd);
}


static void testUpdate2(void)
{
	metersim_update_t upd2 = {
		.timestamp = 200,
		.currentTariff = 12,
		.instant = {
			.frequency = 50.81,
			.voltage = { 310, 320, 356 },
			.current = { 30, 70, 10 },
			.uiAngle = { 15.88, 89, 97 },
		},
		.thd = {
			.thdU = { 0.95, 0.512, 0.165 },
			.thdI = { 0.2689, 0.45, 0.25 },
		},
	};

	compareUpdates(&upd2, &common.upd);
}


int main(int argc, char **args)
{
	FILE *fd;
	char line[cfgparser_BUFFER_LENGTH];

	if (argc < 3) {
		printf("Error! Please specify inputs.\n");
		return 1;
	}

	/* Test readScenario */
	if (cfgparser_readScenario(&common.scenario, args[1]) != 0) {
		printf("Error when reading scenario.\n");
		return 1;
	}

	RUN_TEST(testScenario);

	/* Test readUpdate */
	fd = fopen(args[2], "r");
	if (fd == NULL) {
		printf("Error! Could not open updates file.\n");
		return 1;
	}

	if (fgets(line, cfgparser_BUFFER_LENGTH, fd) == NULL) {
		printf("Error! Could not read line from updates file.\n");
		return 1;
	}

	if (cfgparser_readLine(&common.upd, line) != 0) {
		printf("Error! Could not parse update line.\n");
		return 1;
	}
	RUN_TEST(testUpdate);

	if (fgets(line, cfgparser_BUFFER_LENGTH, fd) == NULL) {
		printf("Error! Could not read line from updates file.\n");
		return 1;
	}

	if (cfgparser_readLine(&common.upd, line) != 0) {
		printf("Error! Could not parse update line.\n");
		return 1;
	}
	RUN_TEST(testUpdate2);

	return 0;
}
