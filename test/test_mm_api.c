/*
 * Unit tests for the mm_api
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <mm_api.h>
#include <unity.h>


/* Energy register may have value 1 Ws less then expected, as internally casts from double to int64_t are used */
#define TEST_ENERGY_REG(expected, actual) \
	do { \
		TEST_ASSERT_GREATER_OR_EQUAL_INT64(expected - 1, actual); \
		TEST_ASSERT_LESS_OR_EQUAL_INT64(expected, actual); \
	} while (0)


static struct {
	mm_ctx_T *mctx;
	char inputPath[1024];
	struct mm_address addr1;
} common;


void setUp(void)
{
	common.mctx = mm_init();
	if (common.mctx == NULL) {
		printf("Unable to create mm instance\n");
		exit(1);
	}
}


void tearDown(void)
{
	(void)mm_free(common.mctx);
}


static void testNoConnection(void)
{
	float temperature;
	TEST_ASSERT(mm_getTemperature(common.mctx, &temperature) == MM_ERROR);
}


static void testSerialNumber(void)
{
	if (mm_connect(common.mctx, &common.addr1, NULL) != MM_SUCCESS) {
		printf("Unable to start simulation.\n");
		TEST_ASSERT(0);
		return;
	}

	char buf[1024];
	int len = mm_getSerialNumber(common.mctx, 0, NULL, 0);
	TEST_ASSERT_EQUAL_INT(8, len);

	mm_getSerialNumber(common.mctx, 0, buf, 1024);
	TEST_ASSERT_EQUAL_STRING("ABCD1234", buf);

	mm_disconnect(common.mctx);
}


static void testManyTariff(void)
{
	struct mme_dataEnergy *energy;
	int tariffCount;

	if (mm_connect(common.mctx, &common.addr1, NULL) != MM_SUCCESS) {
		printf("Unable to start simulation.\n");
		TEST_ASSERT(0);
		return;
	}

	mm_getTariffCount(common.mctx, &tariffCount);

	TEST_ASSERT_EQUAL_INT(5, tariffCount);

	energy = malloc(3 * tariffCount * sizeof(struct mme_dataEnergy));

	mme_getEnergyTariff(common.mctx, energy, 1);
	TEST_ENERGY_REG(11, energy[0].apparentMinus);

	mme_getEnergyTariff(common.mctx, energy, 2);
	TEST_ENERGY_REG(22, energy[0].apparentMinus);

	mme_getEnergyTariff(common.mctx, energy, 3);
	TEST_ENERGY_REG(55, energy[0].apparentMinus);

	free(energy);

	mm_disconnect(common.mctx);
}


static void testConfig(void)
{
	int serialNumberLen;
	char serialNumber[20];
	unsigned int meterConstant;
	int phaseCount;
	int tariffCount;

	if (mm_connect(common.mctx, &common.addr1, NULL) != MM_SUCCESS) {
		printf("Unable to start simulation.\n");
		TEST_ASSERT(0);
		return;
	}

	/* Test values of the scenario (config.toml file) */

	serialNumberLen = mm_getSerialNumber(common.mctx, 0, NULL, 0);
	mm_getSerialNumber(common.mctx, 0, serialNumber, 20);
	mm_getTariffCount(common.mctx, &tariffCount);
	mme_getPhaseCount(common.mctx, &phaseCount);
	mme_getMeterConstant(common.mctx, &meterConstant);

	TEST_ASSERT_EQUAL_INT(0, serialNumberLen);
	TEST_ASSERT_EQUAL_STRING("", serialNumber);
	TEST_ASSERT_EQUAL_UINT(0, meterConstant);
	TEST_ASSERT_EQUAL_INT(3, phaseCount);
	TEST_ASSERT_EQUAL_INT(1, tariffCount);

	mm_disconnect(common.mctx);
}


static void testEnergy(void)
{
	struct mme_dataEnergy energy[3];

	if (mm_connect(common.mctx, &common.addr1, NULL) != MM_SUCCESS) {
		printf("Unable to start simulation.\n");
		TEST_ASSERT(0);
		return;
	}

	usleep(100 * 1000);

	mme_getEnergyTariff(common.mctx, energy, 0);

	/* From timestamp 4 there should be no power on phases 0 and 1, so energy should be well defined there */

	TEST_ENERGY_REG(22000, energy[0].activePlus);
	TEST_ENERGY_REG(22000, energy[0].reactive[0] + energy[0].reactive[1]);
	TEST_ENERGY_REG(0, energy[0].reactive[2]);
	TEST_ENERGY_REG(44000, energy[0].apparentPlus + energy[0].apparentMinus);
	TEST_ENERGY_REG(11000, energy[1].activeMinus);
	TEST_ENERGY_REG(11000, energy[1].reactive[0]);
	TEST_ENERGY_REG(22000, energy[1].apparentPlus);
	TEST_ENERGY_REG(22000, energy[1].apparentMinus);

	mm_disconnect(common.mctx);
}


int main(int argc, char **args)
{
	if (argc < 3) {
		printf("Error! Please specify scenario directory.\n");
		return EXIT_FAILURE;
	}

	common.mctx = NULL;
	common.addr1 = (struct mm_address) {
		.typ = 1,
		.addr = common.inputPath,
	};

	RUN_TEST(testNoConnection);

	strcpy(common.inputPath, args[1]);
	RUN_TEST(testManyTariff);
	RUN_TEST(testSerialNumber);

	/* Change scenario */
	strcpy(common.inputPath, args[2]);
	RUN_TEST(testConfig);
	RUN_TEST(testEnergy);

	return EXIT_SUCCESS;
}
