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

#include <metersim/metersim_types.h>
#include <metersim/metersim.h>


static void printEnergy(metersim_energy_t *energy)
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
		energy->activePlus.value,
		energy->activeMinus.value,
		energy->reactive[0].value,
		energy->reactive[1].value,
		energy->reactive[2].value,
		energy->reactive[3].value,
		energy->apparentPlus.value,
		energy->apparentMinus.value);
}


int main(int argc, char **args)
{
	int phases, currentTariff;
	metersim_energy_t energy;
	metersim_ctx_t *mctx = NULL;
	int32_t uptime;

	if (argc < 2) {
		printf("Error! Please specify scenario directory.\n");
		return EXIT_FAILURE;
	}

	/* Allocate memory for the simulator */
	mctx = metersim_init(args[1]); /* Pass scenario directory */
	if (mctx == NULL) {
		printf("Unable to create metersim instance.\n");
		return EXIT_FAILURE;
	}

	/* Create runner, but do not start it */
	metersim_createRunner(mctx, 0);

	/* Set speedup */
	metersim_setSpeedup(mctx, 10);

	/* Get the number of phases */
	metersim_getPhaseCount(mctx, &phases);

	/* Schedule a pause at timestamp 100 */
	metersim_pause(mctx, 100);

	/* Start runner */
	metersim_resume(mctx);


	/* Print energy registers for current tariff for each phase every 100 milisecond */
	for (int i = 0; i < 120; i++) {
		metersim_getTariffCurrent(mctx, &currentTariff);
		metersim_getEnergyTotal(mctx, &energy);
		metersim_getUptime(mctx, &uptime);

		printf("Total energy at time %d\n"
			   "Current tariff is %d\n\n",
			uptime, currentTariff);
		printEnergy(&energy);
		usleep(100 * 1000);
	}

	/* Now simulation is paused, so we can step forward by 100 seconds */
	metersim_stepForward(mctx, 100);

	metersim_getTariffCurrent(mctx, &currentTariff);
	metersim_getEnergyTotal(mctx, &energy);
	metersim_getUptime(mctx, &uptime);

	printf("Total energy at time %d\n"
		   "Current tariff is %d\n\n",
		uptime, currentTariff);
	printEnergy(&energy);

	metersim_destroyRunner(mctx);
	metersim_free(mctx);

	return EXIT_SUCCESS;
}
