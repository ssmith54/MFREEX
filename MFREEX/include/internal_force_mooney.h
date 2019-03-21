#ifndef INTERNAL_FORCE_MOONEY_H_
#define INTERNAL_FORCE_MOONEY_H_

#include "Integration/material_point.h"
#include "Integration/defgrad.h"
#include "Material/material.h"
#include "Force/Internal/internalForce_hyperelastic.h"
#include "thpool.h"

typedef struct internal_force_args
{


	// Thread specific
	VEC * NODAL_MASS;
	VEC * FINT; 
	VEC * RPEN; 
	IVEC * mat_points;

	// Shared
	MATERIAL_POINTS * material_points;
	VEC * inc_disp;
	VEC * materialParameters;
	CELLS * cells;
	MAT * XI_n;
	MAT * XI_n_1;

	
}internal_force_args;


void internal_force_mooney(void *threadarg);


#endif