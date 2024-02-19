/*
 * Scenario files parser
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#ifndef CFGPARSER_H
#define CFGPARSER_H

#include <stdlib.h>
#include <stdio.h>


#include <metersim/metersim_types.h>
#include "metersim_types_int.h"

#define cfgparser_BUFFER_LENGTH 1024

typedef struct {
	FILE *updateFile;
	char buffer[cfgparser_BUFFER_LENGTH];
} cfgparser_ctx_t;


/*
 * Context independent functions
 */

int cfgparser_readScenario(metersim_scenario_t *cfg, const char *filename);


int cfgparser_readLine(metersim_update_t *upd, char *line);


/*
 * Context dependent functions
 */

int cfgparser_getScenario(metersim_scenario_t *scenario, const char *dir);


int cfgparser_getUpdate(cfgparser_ctx_t *ctx, metersim_update_t *upd);


int cfgparser_init(cfgparser_ctx_t *ctx, const char *dir);


int cfgparser_close(cfgparser_ctx_t *ctx);

#endif /* CFGPARSER_H */
