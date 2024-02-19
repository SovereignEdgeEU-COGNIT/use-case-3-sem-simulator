/*
 * Scenario files parser
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>

#include <toml.h>

#include "cfgparser.h"
#include <metersim/metersim_types.h>
#include "metersim_types_int.h"
#include "log.h"


#define LOG_TAG "cfgparser : "


static void handleEnergyRegister(toml_table_t *phase, const char *regName, metersim_eregister_t *reg)
{
	int64_t valInt;
	toml_datum_t val = toml_int_in(phase, regName);
	if (val.ok) {
		valInt = val.u.i;
		if (valInt >= 0 && valInt <= METERSIM_MAX_INIT_ENERGY_REG) {
			reg->value = valInt;
		}
		else {
			log_error("Parsed invalid energy register: %s", regName);
		}
	}
}


int cfgparser_readScenario(metersim_scenario_t *scenario, const char *filename)
{
	FILE *fd;
	char errbuf[256];
	toml_datum_t val;
	int64_t valInt;

	fd = fopen(filename, "r");
	if (fd == NULL) {
		log_error("Cannot open %s", filename);
		return -1;
	}

	toml_table_t *conf = toml_parse_file(fd, errbuf, sizeof(errbuf));
	fclose(fd);

	if (conf == NULL) {
		log_error("Cannot parse config.toml: %s", errbuf);
		return -1;
	}

	toml_datum_t serialNumber = toml_string_in(conf, "serialNumber");
	if (serialNumber.ok) {
		if (strlen(serialNumber.u.s) > METERSIM_MAX_SERIAL_NUMBER_LENGTH - 1) {
			log_error("Serial number too long.");
		}
		else {
			strcpy(scenario->cfg.serialNumber, serialNumber.u.s);
		}
	}
	free(serialNumber.u.s);

	val = toml_int_in(conf, "speedup"); /* TODO: Find a better way of setting speedup then through config.toml */
	if (val.ok) {
		valInt = val.u.i;
		if (valInt > 0 && valInt <= METERSIM_MAX_SPEEDUP) {
			scenario->cfg.speedup = valInt;
		}
		else {
			log_error("Parsed invalid speedup");
		}
	}

	val = toml_int_in(conf, "tariffCount");
	if (val.ok) {
		valInt = val.u.i;
		if (valInt > 0 && valInt <= METERSIM_MAX_TARIFF_COUNT) {
			scenario->cfg.tariffCount = valInt;
		}
		else {
			log_error("Parsed invalid tariff count");
		}
	}

	val = toml_int_in(conf, "phaseCount");
	if (val.ok) {
		valInt = val.u.i;
		if (valInt > 0 && valInt <= 3) {
			scenario->cfg.phaseCount = valInt;
		}
		else {
			log_error("Parsed invalid phase count");
		}
	}

	val = toml_int_in(conf, "meterConstant");
	if (val.ok) {
		valInt = val.u.i;
		if (valInt >= 0 && valInt <= METERSIM_MAX_METERCONSTANT) {
			scenario->cfg.meterConstant = valInt;
		}
		else {
			log_error("Parsed invalid meter constant");
		}
	}

	val = toml_timestamp_in(conf, "startTimestamp");
	if (val.ok) {
		struct tm s;
		s.tm_year = *val.u.ts->year - 1900;
		s.tm_mon = *val.u.ts->month - 1;
		s.tm_mday = *val.u.ts->day;
		s.tm_hour = *val.u.ts->hour;
		s.tm_min = *val.u.ts->minute;
		s.tm_sec = *val.u.ts->second;
		s.tm_isdst = 0;
		valInt = (int64_t)mktime(&s);
		scenario->cfg.startTime = valInt;
	}

	scenario->energy = malloc(scenario->cfg.tariffCount * sizeof(metersim_energy_t[3]));
	if (scenario->energy == NULL) {
		log_error("Could not allocate memory for energy registers");
		return -1;
	}
	for (int i = 0; i < scenario->cfg.tariffCount; i++) {
		for (int j = 0; j < 3; j++) {
			scenario->energy[i][j] = (metersim_energy_t) { 0 };
		}
	}

	toml_array_t *tariffArray = toml_array_in(conf, "tariff");
	if (tariffArray == NULL) {
		log_debug("Provided no tariff info");
	}
	else {
		for (int i = 0; i < scenario->cfg.tariffCount; i++) {
			toml_table_t *tariff = toml_table_at(tariffArray, i);
			if (tariff == NULL) {
				continue;
			}

			for (int j = 0; j < 3; j++) {
				char phaseName[sizeof("phaseN")];
				sprintf(phaseName, "phase%d", j + 1);
				toml_table_t *phase = toml_table_in(tariff, phaseName);
				if (phase == NULL) {
					continue;
				}

				handleEnergyRegister(phase, "activePlus", &scenario->energy[i][j].activePlus);
				handleEnergyRegister(phase, "activeMinus", &scenario->energy[i][j].activeMinus);
				handleEnergyRegister(phase, "reactive1", &scenario->energy[i][j].reactive[0]);
				handleEnergyRegister(phase, "reactive2", &scenario->energy[i][j].reactive[1]);
				handleEnergyRegister(phase, "reactive3", &scenario->energy[i][j].reactive[2]);
				handleEnergyRegister(phase, "reactive4", &scenario->energy[i][j].reactive[3]);
			}
		}
	}

	toml_free(conf);
	return 0;
}


enum {
	updateColumn_timestamp = 0,
	updateColumn_currentTariff,
	updateColumn_frequency,
	updateColumn_u1,
	updateColumn_u2,
	updateColumn_u3,
	updateColumn_i1,
	updateColumn_i2,
	updateColumn_i3,
	updateColumn_uiAngle1,
	updateColumn_uiAngle2,
	updateColumn_uiAngle3,
	updateColumn_thdU1,
	updateColumn_thdU2,
	updateColumn_thdU3,
	updateColumn_thdI1,
	updateColumn_thdI2,
	updateColumn_thdI3,
};


static int assignValueDouble(double value, double minVal, double maxVal, double *reg)
{
	if (value >= minVal && value <= maxVal) {
		*reg = value;
		return 0;
	}
	else {
		return -1;
	}
}


static int assignValueFloat(float value, float minVal, float maxVal, float *reg)
{
	if (value >= minVal && value <= maxVal) {
		*reg = value;
		return 0;
	}
	else {
		return -1;
	}
}


/* TODO: use proper approach to CSV parsing */
int cfgparser_readLine(metersim_update_t *upd, char *line)
{
	char *curr = line;
	char *next;
	int pos = 0;

	double valD = 0;
	long long int valLL = 0;

	if (isdigit(line[0]) == 0) {
		return -1;
	}

	for (;;) {
		errno = 0;

		/* Get value */
		if (pos >= 2 && pos < 18) {
			valD = strtod(curr, &next);
		}
		else {
			valLL = strtoll(curr, &next, 10);
		}

		if (errno != 0) {
			log_error("Error when parsing update at position %d", pos);
			return -1;
		}

		if (*curr == '\n') {
			break;
		}

		if (curr == next) {
			curr++;
			pos++;
			continue;
		}

		switch (pos) {
			case updateColumn_timestamp:
				if (valLL >= 0 && valLL <= INT32_MAX) {
					upd->timestamp = valLL;
				}
				else {
					log_error("Parsed invalid timestamp");
					return -1;
				}
				break;

			case updateColumn_currentTariff:
				if (valLL >= 0 && valLL < METERSIM_MAX_TARIFF_COUNT) {
					upd->currentTariff = valLL;
				}
				else {
					log_error("Parsed invalid current tariff id");
					return -1;
				}
				break;

			case updateColumn_frequency:
				if (assignValueFloat(valD, 0, METERSIM_MAX_FREQUENCY, &upd->instant.frequency) < 0) {
					log_error("Parsed invalid frequency");
					return -1;
				}
				break;

			case updateColumn_u1:
				if (assignValueDouble(valD, 0, METERSIM_MAX_VOLTAGE, &upd->instant.voltage[0]) < 0) {
					log_error("Parsed invalid voltage");
					return -1;
				}
				break;

			case updateColumn_u2:
				if (assignValueDouble(valD, 0, METERSIM_MAX_VOLTAGE, &upd->instant.voltage[1]) < 0) {
					log_error("Parsed invalid voltage");
					return -1;
				}
				break;

			case updateColumn_u3:
				if (assignValueDouble(valD, 0, METERSIM_MAX_VOLTAGE, &upd->instant.voltage[2]) < 0) {
					log_error("Parsed invalid voltage");
					return -1;
				}
				break;

			case updateColumn_i1:
				if (assignValueDouble(valD, 0, METERSIM_MAX_CURRENT, &upd->instant.current[0]) < 0) {
					log_error("Parsed invalid current");
					return -1;
				}
				break;

			case updateColumn_i2:
				if (assignValueDouble(valD, 0, METERSIM_MAX_CURRENT, &upd->instant.current[1]) < 0) {
					log_error("Parsed invalid current");
					return -1;
				}
				break;

			case updateColumn_i3:
				if (assignValueDouble(valD, 0, METERSIM_MAX_CURRENT, &upd->instant.current[2]) < 0) {
					log_error("Parsed invalid current");
					return -1;
				}
				break;

			case updateColumn_uiAngle1:
				if (assignValueDouble(valD, 0, 360, &upd->instant.uiAngle[0]) < 0) {
					log_error("Parsed invalid ui_angle");
					return -1;
				}
				break;

			case updateColumn_uiAngle2:
				if (assignValueDouble(valD, 0, 360, &upd->instant.uiAngle[1]) < 0) {
					log_error("Parsed invalid ui_angle");
					return -1;
				}
				break;

			case updateColumn_uiAngle3:
				if (assignValueDouble(valD, 0, 360, &upd->instant.uiAngle[2]) < 0) {
					log_error("Parsed invalid ui_angle");
					return -1;
				}
				break;

			case updateColumn_thdU1:
				if (assignValueFloat(valD, 0, METERSIM_MAX_THDU, &upd->thd.thdU[0]) < 0) {
					log_error("Parsed invalid thdU");
					return -1;
				}

				break;
			case updateColumn_thdU2:
				if (assignValueFloat(valD, 0, METERSIM_MAX_THDU, &upd->thd.thdU[1]) < 0) {
					log_error("Parsed invalid thdU");
					return -1;
				}
				break;

			case updateColumn_thdU3:
				if (assignValueFloat(valD, 0, METERSIM_MAX_THDU, &upd->thd.thdU[2]) < 0) {
					log_error("Parsed invalid thdU");
					return -1;
				}
				break;

			case updateColumn_thdI1:
				if (assignValueFloat(valD, 0, METERSIM_MAX_THDI, &upd->thd.thdI[0]) < 0) {
					log_error("Parsed invalid thdI");
					return -1;
				}
				break;

			case updateColumn_thdI2:
				if (assignValueFloat(valD, 0, METERSIM_MAX_THDI, &upd->thd.thdI[1]) < 0) {
					log_error("Parsed invalid thdI");
					return -1;
				}
				break;

			case updateColumn_thdI3:
				if (assignValueFloat(valD, 0, METERSIM_MAX_THDI, &upd->thd.thdI[2]) < 0) {
					log_error("Parsed invalid thdI");
					return -1;
				}
				break;

			default:
				log_error("Too many columns");
				return -1;
		}

		if (*next == '\n') {
			break;
		}

		pos++;
		curr = next + 1;
	}

	return 0;
}


int cfgparser_getScenario(metersim_scenario_t *scenario, const char *dir)
{
	char *filename;
	int dirPathLength;
	int ret;

	dirPathLength = strlen(dir);

	filename = malloc((dirPathLength + sizeof("/config.toml") * sizeof(char)));
	if (filename == NULL) {
		return -1;
	}
	sprintf(filename, "%s/config.toml", dir);

	ret = cfgparser_readScenario(scenario, filename);

	free(filename);

	return ret;
}


int cfgparser_getUpdate(cfgparser_ctx_t *ctx, metersim_update_t *upd)
{
	char *ret;

	ret = fgets(ctx->buffer, cfgparser_BUFFER_LENGTH, ctx->updateFile);

	if (ret == NULL) { /* If EOF then no more updates are available */
		return 1;
	}

	return cfgparser_readLine(upd, ctx->buffer);
}


int cfgparser_init(cfgparser_ctx_t *ctx, const char *dir)
{
	char *filename;
	int dirPathLength;

	dirPathLength = strlen(dir);

	filename = malloc((dirPathLength + sizeof("/updates.csv")) * sizeof(char));
	if (filename == NULL) {
		return -1;
	}

	sprintf(filename, "%s/updates.csv", dir);
	ctx->updateFile = fopen(filename, "r");
	if (ctx->updateFile == NULL) {
		log_error("Error while trying to open %s", filename);
		free(filename);
		return -1;
	}
	free(filename);

	return 0;
}


int cfgparser_close(cfgparser_ctx_t *ctx)
{
	if (ctx->updateFile == NULL) {
		return 0;
	}
	else {
		return fclose(ctx->updateFile);
	}
}
