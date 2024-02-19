/*
 * Logging utils for SEM simulator
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#ifndef LOG_H
#define LOG_H

#include <stdio.h>


#define LOG_LVL_SILENT  0
#define LOG_LVL_ERROR   1
#define LOG_LVL_WARNING 2
#define LOG_LVL_INFO    3
#define LOG_LVL_DEBUG   4


#ifndef LOG_LVL
#define LOG_LVL LOG_LVL_ERROR
#endif


#if LOG_LVL >= LOG_LVL_DEBUG
#define log_debug(fmt, ...) \
	do { \
		fprintf(stderr, LOG_TAG fmt "\n", ##__VA_ARGS__); \
	} while (0)
#else
#define log_debug(fmt, ...) (void)0
#endif


#if LOG_LVL >= LOG_LVL_INFO
#define log_info(fmt, ...) \
	do { \
		fprintf(stderr, LOG_TAG fmt "\n", ##__VA_ARGS__); \
	} while (0)
#else
#define log_info(fmt, ...) (void)0
#endif


#if LOG_LVL >= LOG_LVL_WARNING
#define log_warning(fmt, ...) \
	do { \
		fprintf(stderr, LOG_TAG fmt "\n", ##__VA_ARGS__); \
	} while (0)
#else
#define log_warning(fmt, ...) (void)0
#endif


#if LOG_LVL >= LOG_LVL_ERROR
#define log_error(fmt, ...) \
	do { \
		fprintf(stderr, LOG_TAG fmt "\n", ##__VA_ARGS__); \
	} while (0)
#else
#define log_error(fmt, ...) (void)0
#endif


#endif /* LOG_H */
