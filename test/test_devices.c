/*
 * Unit tests of the device mgr
 *
 * Copyright 2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>


#include <metersim/metersim_types.h>
#include <metersim/metersim.h>
#include <unity.h>


#define MAX_DEVICE_NUM 32


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


/* Device1 with constant current */
static void cb1(metersim_infoForDevice_t *info, metersim_deviceResponse_t *res, void *ctx)
{
	(void)ctx;
	(void)info;
	res->current[0] = 1;
	res->current[1] = 2 * cexp(225.0 * M_PI / 180.0 * _Complex_I);
	res->current[2] = 3 * cexp(275.0 * M_PI / 180.0 * _Complex_I);
	res->nextUpdateTime = METERSIM_NO_UPDATE_SCHEDULED;
}


/* Device2 with current that changes over time */
static void cb2(metersim_infoForDevice_t *info, metersim_deviceResponse_t *res, void *ctx)
{
	(void)ctx;
	int32_t now = info->now;
	if (now < 60) {
		res->current[0] = (double)now; /* Here current is set as the current timestamp for the sake of testing */
		res->current[1] = cexp(225.0 * M_PI / 180.0 * _Complex_I) * (double)now;
		res->current[2] = cexp(275.0 * M_PI / 180.0 * _Complex_I) * (double)now;
		res->nextUpdateTime = now + 1;
	}
	else {
		res->nextUpdateTime = METERSIM_NO_UPDATE_SCHEDULED;
	}
}


/* Device3 with dynamic switching */
typedef struct {
	int switchOn;
	pthread_mutex_t lock;
} ctx3_t;


static void cb3(metersim_infoForDevice_t *info, metersim_deviceResponse_t *res, void *ctx)
{
	(void)info;
	ctx3_t *ctx3 = (ctx3_t *)ctx;

	pthread_mutex_lock(&ctx3->lock);
	if (ctx3->switchOn == 1) {
		res->current[0] = 10;
	}
	else {
		res->current[0] = -15;
	}
	pthread_mutex_unlock(&ctx3->lock);
	res->nextUpdateTime = METERSIM_NO_UPDATE_SCHEDULED;
}


static void compareCurrent(double *expected, double *actual, char *msg)
{
	TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(expected[0], actual[0], msg);
	TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(expected[1], actual[1], msg);
	TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(expected[2], actual[2], msg);
}


static void testConstantCurrent(void)
{
	double expectedCurrent[3] = { 11, 22, 33 };

	metersim_instant_t instant;

	metersim_createRunner(common.ctx, 0);
	metersim_newDevice(common.ctx, &cb1, NULL);

	metersim_resume(common.ctx);
	metersim_getInstant(common.ctx, &instant);
	compareCurrent(expectedCurrent, instant.current, "");

	metersim_destroyRunner(common.ctx);
}


static void testChangingCurrent(void)
{
	double expectedCurrent1[3] = { 69, 79, 89 };
	double expectedCurrent2[3] = { 10, 20, 30 };

	metersim_instant_t instant;

	metersim_newDevice(common.ctx, &cb2, NULL);

	metersim_stepForward(common.ctx, 59);
	metersim_getInstant(common.ctx, &instant);
	compareCurrent(expectedCurrent1, instant.current, "Test 1");

	metersim_stepForward(common.ctx, 1);
	metersim_getInstant(common.ctx, &instant);
	compareCurrent(expectedCurrent2, instant.current, "Test 2");

	metersim_stepForward(common.ctx, 1000);
	metersim_getInstant(common.ctx, &instant);
	compareCurrent(expectedCurrent2, instant.current, "Test 3");
}


static void testDynamicSwitching(void)
{
	int status;
	metersim_instant_t instant;
	metersim_vector_t vector;

	ctx3_t cbCtx;

	status = pthread_mutex_init(&cbCtx.lock, NULL);
	TEST_ASSERT(status == 0);
	if (status < 0) {
		return;
	}

	cbCtx.switchOn = 1;

	metersim_newDevice(common.ctx, &cb3, &cbCtx);

	metersim_stepForward(common.ctx, 15);
	metersim_getInstant(common.ctx, &instant);
	TEST_ASSERT_EQUAL_DOUBLE(20, instant.current[0]);

	metersim_stepForward(common.ctx, 15);

	pthread_mutex_lock(&cbCtx.lock);
	cbCtx.switchOn = 0;
	pthread_mutex_unlock(&cbCtx.lock);
	metersim_notifyDevicemgr(common.ctx);

	metersim_stepForward(common.ctx, 15);
	metersim_getInstant(common.ctx, &instant);
	TEST_ASSERT_EQUAL_DOUBLE(5, instant.current[0]);

	metersim_getVector(common.ctx, &vector);
	TEST_ASSERT(vector.phaseCurrent[0] == -5);

	pthread_mutex_lock(&cbCtx.lock);
	cbCtx.switchOn = 1;
	pthread_mutex_unlock(&cbCtx.lock);
	metersim_notifyDevicemgr(common.ctx);

	metersim_stepForward(common.ctx, 10);
	metersim_getInstant(common.ctx, &instant);
	TEST_ASSERT_EQUAL_DOUBLE(20, instant.current[0]);

	metersim_getVector(common.ctx, &vector);
	TEST_ASSERT(vector.phaseCurrent[0] == 20);
}


void testDestroyingDevices(void)
{
	int devIds[MAX_DEVICE_NUM];
	for (int i = 0; i < MAX_DEVICE_NUM; i++) {
		devIds[i] = metersim_newDevice(common.ctx, &cb1, NULL);
	}

	metersim_stepForward(common.ctx, 10);

	TEST_ASSERT(metersim_newDevice(common.ctx, &cb1, NULL) == METERSIM_ERROR);

	metersim_destroyDevice(common.ctx, devIds[7]);
	TEST_ASSERT(metersim_newDevice(common.ctx, &cb2, NULL) == devIds[7]);
}


int main(int argc, char **args)
{
	if (argc < 2) {
		printf("Error! Please specify scenario directory.\n");
		return 1;
	}

	strcpy(common.inputPath, args[1]);

	RUN_TEST(testConstantCurrent);
	RUN_TEST(testChangingCurrent);
	RUN_TEST(testDynamicSwitching);
	RUN_TEST(testDestroyingDevices);
}
