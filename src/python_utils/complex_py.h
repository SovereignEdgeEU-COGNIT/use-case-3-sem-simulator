/*
 * Helper for complex numbers handling in Python
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Kobak
 *
 * %LICENSE%
 */

#ifndef COMPLEX_PY
#define COMPLEX_PY

typedef struct ComplexPy {
	double real;
	double imag;
} complexPy_t;

double _Complex pyToComplex(complexPy_t x);


complexPy_t complexToPy(double _Complex x);

#endif /* COMPLEX_PY */
