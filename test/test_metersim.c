/*
 * Unit tests of the simulator
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
#include <unistd.h>
#include <time.h>

#include <metersim/metersim_types.h>
#include <metersim/metersim.h>
#include <unity.h>


/* Energy register may have value 1 Ws less then expected, as internally casts from double to int64_t are used */
#define TEST_ENERGY_REG(expected, actual) \
	do { \
		TEST_ASSERT_GREATER_OR_EQUAL_INT64(expected - 1, actual); \
		TEST_ASSERT_LESS_OR_EQUAL_INT64(expected, actual); \
	} while (0)


static struct {
	metersim_ctx_t *ctx;
	char inputPath[1024];
} common;


void setUp(void)
{
	common.ctx = metersim_init(common.inputPath);
}


void tearDown(void)
{
	metersim_free(common.ctx);
}


void testStepForward(void)
{
	int tariff;
	metersim_energy_t energy[3];
	metersim_energy_t total;
	metersim_instant_t instant;

	/* Check initial values of energy registers */
	metersim_getEnergyTotal(common.ctx, &total);
	TEST_ENERGY_REG(66, total.activeMinus.value);
	TEST_ENERGY_REG(88, total.apparentMinus.value);

	/* Check that for tariff 0 energy registers are 0 */
	metersim_getEnergyTariff(common.ctx, energy, 0);
	TEST_ENERGY_REG(0, energy[0].activePlus.value);

	metersim_stepForward(common.ctx, 5);

	/* Test energy at timestamp 5 */
	metersim_getEnergyTariff(common.ctx, energy, 0);
	TEST_ENERGY_REG(10500, energy[0].activePlus.value);

	metersim_stepForward(common.ctx, 5);

	/* Test energy at timestamp 10 */
	metersim_getEnergyTariff(common.ctx, energy, 0);
	TEST_ENERGY_REG(21000, energy[0].activePlus.value);

	/* Test that at timestamp 10 the tariff has changed to 4 */
	metersim_getTariffCurrent(common.ctx, &tariff);
	TEST_ASSERT_EQUAL_INT(4, tariff);

	metersim_stepForward(common.ctx, 180);

	/* Test that at timestamp 190 the last update has been introduced */
	metersim_getInstant(common.ctx, &instant);
	TEST_ASSERT_EQUAL_DOUBLE(300, instant.voltage[0]);
}


void testRunner(void)
{
	int tariff;
	int32_t uptime;
	metersim_energy_t energy[3];
	metersim_instant_t instant;

	metersim_createRunner(common.ctx, 0);
	metersim_setSpeedup(common.ctx, 100);
	metersim_pause(common.ctx, 10);
	metersim_resume(common.ctx);

	/* Test current tariff */
	metersim_getTariffCurrent(common.ctx, &tariff);
	TEST_ASSERT_EQUAL_INT(0, tariff);

	/* After this sleep the runner should be paused */
	usleep(150 * 1000);

	/* Runner should be paused at timestamp 10 */
	metersim_getUptime(common.ctx, &uptime);
	TEST_ASSERT_EQUAL_INT32(10, uptime);

	/* Test energy at timestamp 10 */
	metersim_getEnergyTariff(common.ctx, energy, 0);
	TEST_ENERGY_REG(21000, energy[0].activePlus.value);

	/* Test that tariff at timestamp 10 is 4 */
	metersim_getTariffCurrent(common.ctx, &tariff);
	TEST_ASSERT_EQUAL_INT(4, tariff);

	/* Test energy at tariff 4 */
	metersim_getEnergyTariff(common.ctx, energy, 4);
	TEST_ENERGY_REG(0, energy[0].activePlus.value);

	metersim_stepForward(common.ctx, 180);

	/* Test uptime */
	metersim_getUptime(common.ctx, &uptime);
	TEST_ASSERT_EQUAL_INT32(190, uptime);

	/* Test that last update has been already introduced */
	metersim_getInstant(common.ctx, &instant);
	TEST_ASSERT_EQUAL_DOUBLE(300, instant.voltage[0]);

	metersim_destroyRunner(common.ctx);
}


void testUptime(void)
{
	int32_t uptime;
	metersim_instant_t instant;

	metersim_createRunner(common.ctx, 0);
	metersim_setSpeedup(common.ctx, 3000);
	metersim_resume(common.ctx);

	usleep(100 * 1000);

	/* Check if we are in last update (after 180 seconds) */
	metersim_getInstant(common.ctx, &instant);
	TEST_ASSERT_EQUAL_DOUBLE(110, instant.uiAngle[0]);

	metersim_pause(common.ctx, 1000);

	/* After this sleep the runner should be already paused */
	usleep(250 * 1000);

	/* Test if timestamp is equal to the scheduled timestamp of the pause */
	metersim_getUptime(common.ctx, &uptime);
	TEST_ASSERT_EQUAL_INT32(1000, uptime);

	metersim_resume(common.ctx);

	/* Test if speedup as accurate */
	usleep(10 * 1000);
	metersim_getUptime(common.ctx, &uptime);
	TEST_ASSERT_GREATER_THAN_INT32(1015, uptime);
	TEST_ASSERT_LESS_THAN_INT32(1045, uptime);

	metersim_destroyRunner(common.ctx);
}


static uint64_t timeCb(void *arg)
{
	(void)arg;
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec;
}


void testCustomTimeCb(void)
{
	int32_t uptime;
	metersim_instant_t instant;

	metersim_createRunnerWithCb(common.ctx, timeCb, NULL);

	usleep(2000 * 1000);

	metersim_getInstant(common.ctx, &instant);

	metersim_getUptime(common.ctx, &uptime);
	TEST_ASSERT_EQUAL_INT32(2, uptime);
	metersim_destroyRunner(common.ctx);
}


void testIsRunning(void)
{
	int32_t uptime;

	metersim_createRunner(common.ctx, 0);
	metersim_setSpeedup(common.ctx, 1000);
	metersim_pause(common.ctx, 500);
	metersim_resume(common.ctx);

	usleep(50 * 1000);

	/* Test whether running runner prohibits stepping forward */
	TEST_ASSERT_EQUAL_INT(METERSIM_REFUSE, metersim_stepForward(common.ctx, 500));

	metersim_getUptime(common.ctx, &uptime);

	/* Test isRunning function against the scheduled pause */
	while (metersim_isRunning(common.ctx) == 1) {
		TEST_ASSERT_LESS_THAN_INT32(500, uptime);
		usleep(10 * 1000);
		metersim_getUptime(common.ctx, &uptime);
	}

	/* Test if timestamp agrees with the scheduled pause */
	metersim_getUptime(common.ctx, &uptime);
	TEST_ASSERT_EQUAL_INT32(500, uptime);

	/* Test whether non-running runner allows stepping forward */
	TEST_ASSERT_EQUAL_INT(METERSIM_SUCCESS, metersim_stepForward(common.ctx, 500));

	/* The runner should be stopped now */
	TEST_ASSERT_EQUAL_INT(0, metersim_isRunning(common.ctx));

	/* Uptime should be 1000 now */
	metersim_getUptime(common.ctx, &uptime);
	TEST_ASSERT_EQUAL_INT32(1000, uptime);

	metersim_destroyRunner(common.ctx);
}


void testFrequentSpeedupChanges(void)
{
	metersim_createRunner(common.ctx, 0);
	metersim_setSpeedup(common.ctx, 1000);
	metersim_resume(common.ctx);

	/* Check if this does not triggers internal assertions */
	for (int i = 0; i < 100; i++) {
		metersim_setSpeedup(common.ctx, 1000 - 10 * i);
	}

	TEST_ASSERT(metersim_setSpeedup(common.ctx, 0) == METERSIM_ERROR);

	metersim_destroyRunner(common.ctx);
}


void testMaxValues(void)
{
	int32_t dt = 100 * 24 * 3600;
	int32_t dt2 = 4 * 3600;
	metersim_energy_t energy[3];

	metersim_stepForward(common.ctx, dt);
	metersim_getEnergyTotal(common.ctx, energy);

	TEST_ASSERT_EQUAL_INT64((int64_t)dt * 400 * 100 * 3, energy->apparentPlus.value);

	metersim_createRunner(common.ctx, 0);
	metersim_setSpeedup(common.ctx, METERSIM_MAX_SPEEDUP);
	metersim_pause(common.ctx, dt + dt2);
	metersim_resume(common.ctx);

	while (metersim_isRunning(common.ctx)) {
		usleep(100 * 1000);
	}

	int32_t uptime;
	metersim_getUptime(common.ctx, &uptime);
	TEST_ASSERT_EQUAL_INT32(dt + dt2, uptime);
}


int main(int argc, char **args)
{
	if (argc < 3) {
		printf("Error! Please specify scenario directory.\n");
		return 1;
	}

	strcpy(common.inputPath, args[1]);

	RUN_TEST(testStepForward);
	RUN_TEST(testRunner);
	RUN_TEST(testCustomTimeCb);
	RUN_TEST(testUptime);
	RUN_TEST(testIsRunning);
	RUN_TEST(testFrequentSpeedupChanges);

	strcpy(common.inputPath, args[2]);
	RUN_TEST(testMaxValues);
}
