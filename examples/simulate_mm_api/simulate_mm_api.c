/*
 * Basic simulation app with mm_api
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <mm_api.h>


static void printEnergy(struct mme_dataEnergy *energy)
{
	printf(
		"+A  [Ws]:     %lu\n"
		"-A  [Ws]:     %lu\n"
		"+Ri [vars]:   %lu\n"
		"+Rc [vars]:   %lu\n"
		"-Ri [vars]:   %lu\n"
		"-Rc [vars]:   %lu\n"
		"+S  [VAs]:    %lu\n"
		"-S  [VAs]:    %lu\n"
		"\n",
		energy->activePlus,
		energy->activeMinus,
		energy->reactive[0],
		energy->reactive[1],
		energy->reactive[2],
		energy->reactive[3],
		energy->apparentPlus,
		energy->apparentMinus);
}


int main(int argc, char **args)
{
	int phases, currentTariff;
	int32_t uptime;
	struct mme_dataEnergy energy;
	mm_ctx_T *mctx = NULL;

	if (argc < 2) {
		printf("Error! Please specify scenario directory.\n");
		return EXIT_FAILURE;
	}

	/* Allocate memory for the simulator */
	mctx = mm_init();
	if (mctx == NULL) {
		printf("Unable to create mm instance.\n");
		return EXIT_FAILURE;
	}

	struct mm_address addr = {
		.addr = args[1], /* Passing scenario directory as address */
		.typ = 1
	};

	/* Configure the scenario directory and start the simulation */
	if (mm_connect(mctx, &addr, NULL) != MM_SUCCESS) {
		printf("Unable to start simulation.\n");
		return EXIT_FAILURE;
	}

	/* Get the number of phases */
	mme_getPhaseCount(mctx, &phases);

	/* Print energy registers for current tariff for each phase every second */
	for (;;) {
		mm_getUptime(mctx, &uptime);
		mm_getTariffCurrent(mctx, &currentTariff);
		mme_getEnergyTotal(mctx, &energy);

		printf("Total energy at time %d\n"
			   "Current tariff is %d\n\n",
			uptime, currentTariff);
		printEnergy(&energy);
		sleep(1);
	}

	/* This call may be required in a real situatiuon, but does nothing in a simulation */
	(void)mm_sync(mctx);

	/* Finish simulation */
	(void)mm_disconnect(mctx);

	/* Free the memory */
	(void)mm_free(mctx);

	return EXIT_SUCCESS;
}
