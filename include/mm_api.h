/*
 * meter messaging - user API with public access
 *
 * Copyright 2022 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * %LICENSE%
 */

#ifndef MM_API
#define MM_API

#include <stddef.h>


/*
 * meter messaging common context
 */
typedef struct mm_ctx_S mm_ctx_T;


/*
 * General result codes
 */
#define MM_SUCCESS  0
#define MM_ERROR    -1
#define MM_NOMEM    -2
#define MM_REFUSED  -3
#define MM_TRYAGAIN -4

/*
 * API entry and exit point
 */

/* Allocate, create and initialize */
mm_ctx_T *mm_init(void);

/* Release resources */
int mm_free(mm_ctx_T *);

/*
 * Connection to e-metering server
 */

#define MM_ADDRTYP_PHX 0

struct mm_address {
	int typ;
	const char *addr;
};

/* Connect to server (open session) */
int mm_connect(mm_ctx_T *, struct mm_address *, void *);

/* Disconnect from server (close session) */
int mm_disconnect(mm_ctx_T *);

/* Synchronize cached data (optional) */
int mm_sync(mm_ctx_T *);


/* Abort pending transaction (thread) */
int mm_transactAbort(mm_ctx_T *);


/*
 * Generic informations (may be cached)
 */

/* Get number of available tariffs */
int mm_getTariffCount(mm_ctx_T *, int *retCount);

/* Get index of the current tariff */
int mm_getTariffCurrent(mm_ctx_T *, int *retIndex);

/* Get serial numbers of the meter as string */
int mm_getSerialNumber(mm_ctx_T *, int idx, char *dstBuf, size_t dstBufLen);

/* Get current timestamp from RTC */
int mm_getTimeUTC(mm_ctx_T *, int64_t *retTime);

/* Get e-metering server uptime (seconds) */
int mm_getUptime(mm_ctx_T *, int32_t *retSeconds);

/* Get ambient temperature (deg.C)*/
int mm_getTemperature(mm_ctx_T *, float *retTemp);

#define MM_DOMAIN_ELECTRICITY (1uL << 0)
#define MM_DOMAIN_WATER       (1uL << 1)
#define MM_DOMAIN_GAS         (1uL << 2)
#define MM_DOMAIN_MASK        (MM_DOMAIN_ELECTRICITY | MM_DOMAIN_WATER | MM_DOMAIN_MASK)
#define MM_DOMAIN_CAP_MASK    (0xffffffffuL & ~(MM_DOMAIN_MASK))

/* Get metering domain and capabilities */
int mm_getMeterDomain(mm_ctx_T *, unsigned int *retDomainCapFlags);


/*
 * meter metering electricity (domain 0)
 */

#define MME_DOMAIN_CAP_VALVE    (1uL << 3)
#define MME_DOMAIN_CAP_NEUTRAL  (1uL << 4)
#define MME_DOMAIN_CAP_PREPAID  (1uL << 5)
#define MME_DOMAIN_CAP_USERGPIO (1uL << 6)


/* Get number of phases */
int mme_getPhaseCount(mm_ctx_T *, int *retCount);

/* Get mains frequency (Hz) */
int mme_getFrequency(mm_ctx_T *, float *retFreq);

/* Get meter constant (Ws) */
int mme_getMeterConstant(mm_ctx_T *, unsigned int *ret);


/*
 * Actual electricity metering readouts and coefficients
 */

struct mme_dataInstant {
	float u[3];       /* phase voltage */
	float i[3];       /* phase current */
	float in;         /* neutral wire current */
	float uiAngle[3]; /* angle between ui */
	float ppAngle[2]; /* phase-phase angle */
};

/* Get instantaneous values of Urms, Irms, and UI and phase-phase angles */
int mme_getInstant(mm_ctx_T *, struct mme_dataInstant *ret);


struct mme_dataEnergy {
	int64_t activePlus;
	int64_t activeMinus;
	int64_t reactive[4];
	int64_t apparentPlus;
	int64_t apparentMinus;
};

/* Get energy registers grand total (all phases, all tariffs) */
int mme_getEnergyTotal(mm_ctx_T *, struct mme_dataEnergy *ret);

/* Get energy registers particularly per tariff */
int mme_getEnergyTariff(mm_ctx_T *, struct mme_dataEnergy ret[3], int idxTariff);


struct mme_dataPower {
	float p[3];   /* true power */
	float q[3];   /* reactive power */
	float s[3];   /* apparent power */
	float phi[3]; /* angle between ui */
};

/* Get power triangle (P, Q, S, phi angle) */
int mme_getPower(mm_ctx_T *, struct mme_dataPower *);


struct mme_dataVector {
	float _Complex s[3]; /* complex power */
	float _Complex u[3]; /* phase voltage */
	float _Complex i[3]; /* phase current */
	float _Complex in;   /* neutral wire current */
};

/* Get vector data on complex number plane (fundamental freq) */
int mme_getVector(mm_ctx_T *, struct mme_dataVector *);

/* Get total harmonic distortion for current per phase */
int mme_getThdI(mm_ctx_T *, float retThdI[3]);

/* Get total harmonic distortion for voltage per phase */
int mme_getThdU(mm_ctx_T *, float retThdU[3]);


#endif /* end of MM_API */
