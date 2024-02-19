/*
 * Helper for complex numbers handling in Python
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#include <complex.h>
#include <stdint.h>

#include <mm_api.h>
#include <metersim/metersim.h>

#include "complex_py.h"


struct dataVectorPy {
	complexPy_t s[3]; /* complex power */
	complexPy_t u[3]; /* phase voltage */
	complexPy_t i[3]; /* phase current */
	complexPy_t iN;   /* neutral wire current */
};


double _Complex pyToComplex(complexPy_t x)
{
	return x.real + x.imag * _Complex_I;
}


complexPy_t complexToPy(double _Complex x)
{
	return (complexPy_t) { crealf(x), cimagf(x) };
}


int mme_getVectorPy(mm_ctx_T *ctx, struct dataVectorPy *ret)
{
	struct mme_dataVector data;
	int status = mme_getVector(ctx, &data);
	if (status == MM_SUCCESS) {
		for (int i = 0; i < 3; i++) {
			ret->s[i] = complexToPy(data.s[i]);
			ret->u[i] = complexToPy(data.s[i]);
			ret->i[i] = complexToPy(data.s[i]);
		}
		ret->iN = complexToPy(data.in);
	}
	return status;
}


void metersim_getVectorPy(metersim_ctx_t *ctx, struct dataVectorPy *ret)
{
	metersim_vector_t data;
	metersim_getVector(ctx, &data);

	for (int i = 0; i < 3; i++) {
		ret->s[i] = complexToPy(data.complexPower[i]);
		ret->u[i] = complexToPy(data.phaseVoltage[i]);
		ret->i[i] = complexToPy(data.phaseCurrent[i]);
	}
	ret->iN = complexToPy(data.complexNeutral);
}
