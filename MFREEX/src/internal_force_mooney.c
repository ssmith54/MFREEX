
#include "internal_force_mooney.h"



static double epsilon_penalty = -5000;



void internal_force_mooney(void *threadarg)


{
	// Internal forces
	double P11,P12,P13,P21,P22,P23,P31,P32,P33;

	MAT * F ;
	MAT * Fdot;
	VEC * stressVoigt ;
	MAT * B ;
	IVEC * neighbours;
	MAT * F_r;
	VEC * fInt;
	double Xp_n, Xp_n_1, Yp_n, Yp_n_1;
	int i = 0;

	internal_force_args * internal_force_struct = (internal_force_args*) threadarg;

	// Get inputs from struct 
	VEC * FINT = internal_force_struct->FINT;
	VEC * NODAL_MASS = internal_force_struct->NODAL_MASS;
	VEC * RPEN = internal_force_struct->RPEN;
	IVEC * mat_points = internal_force_struct->mat_points;
	VEC * materialParameters = internal_force_struct->materialParameters;
	VEC * inc_disp = internal_force_struct->inc_disp;
	CELLS * cells = internal_force_struct->cells;
	MATERIAL_POINTS * material_points = internal_force_struct->material_points;
	MAT * XI_n = internal_force_struct->XI_n;
	MAT * XI_n_1 = internal_force_struct->XI_n_1;


	__zero__(FINT->ve,FINT->max_dim);
	__zero__(NODAL_MASS->ve,NODAL_MASS->max_dim);
	__zero__(RPEN->ve,RPEN->max_dim);


	for ( int k = 0 ; k < mat_points->max_dim; k++)
	{

		i = mat_points->ive[k];
		F = material_points->MP[i]->stateNew->F;
		Fdot = material_points->MP[i]->stateNew->Fdot;
		B = material_points->MP[i]->B;
		neighbours = material_points->MP[i]->neighbours;
		F_r = material_points->MP[i]->F_n;
		stressVoigt = material_points->MP[i]->stressVoigt;
		fInt = material_points->MP[i]->fInt;
		int num_neighbours = material_points->MP[i]->num_neighbours;


		// Compute incremental deformation gradient
		defgrad_m(material_points->MP[i]->inc_F, B, neighbours, num_neighbours, inc_disp);
			

		// compute total deformation gradient 
		m_mlt(material_points->MP[i]->inc_F,
			material_points->MP[i]->F_n,material_points->MP[i]->stateNew->F);
	
	
		// Integration factor
		double intFactor = material_points->MP[i]->volume*
		material_points->MP[i]->INTEGRATION_FACTOR;


		// Material law 
		mooneyRivlin(stressVoigt,material_points->MP[i]->stateNew,materialParameters);
		

		__zero__(fInt->ve,fInt->max_dim);


		// push forward piola kirchoff stress to Omega_n configuration
		sv_mlt(1.00/material_points->MP[i]->Jn,stressVoigt,stressVoigt);

		P11 = stressVoigt->ve[0];
		P22 = stressVoigt->ve[1];
		P12 = stressVoigt->ve[2];
		P21 = stressVoigt->ve[3];

		stressVoigt->ve[0] = P11*F_r->me[0][0] + P12*F_r->me[0][1];
		stressVoigt->ve[1] = P21*F_r->me[1][0] + P22*F_r->me[1][1];
		stressVoigt->ve[2] = P11*F_r->me[1][0] + P12*F_r->me[1][1];
		stressVoigt->ve[3] = P21*F_r->me[0][0] + P22*F_r->me[0][1];


		vm_mlt(B,stressVoigt,fInt);



		// Assemble internal force vector
		for ( int k = 0 ; k < num_neighbours; k++){

			int index = neighbours->ive[k];

			FINT->ve[2*index] += intFactor * fInt->ve[2*k];
			FINT->ve[2*index + 1] += intFactor *fInt->ve[2*k+1];


		}


		// update_material point neighbours
		material_points->MP[i] = update_material_point(material_points->MP[i], cells,
			 XI_n_1, NODAL_MASS);

		// Find error in displacment field
		Xp_n_1 = material_points->MP[i]->coords_n_1[0];
		Xp_n = material_points->MP[i]->coords_n[0];
		Yp_n_1 = material_points->MP[i]->coords_n_1[1];
		Yp_n = material_points->MP[i]->coords_n[1];


		num_neighbours = material_points->MP[i]->num_neighbours;
		neighbours = material_points->MP[i]->neighbours;




		for ( int k = 0 ; k < num_neighbours; k++){

			int index = neighbours->ive[k];

			// Find vectors dX_n and dX_n_1
			double delta_x_n[2] = {XI_n->me[index][0] - Xp_n,XI_n->me[index][1] - Yp_n};
			double delta_x_n_1[2] = {XI_n_1->me[index][0] - Xp_n_1,XI_n_1->me[index][1] - Yp_n_1};

			// Find error in displacement 
			double x_tilde[2] = {material_points->MP[i]->inc_F->me[0][0]*(delta_x_n[0])
					+ material_points->MP[i]->inc_F->me[0][1]*(delta_x_n[1]),
					material_points->MP[i]->inc_F->me[1][0]*delta_x_n[0]
					+ material_points->MP[i]->inc_F->me[1][1]*delta_x_n[1]};

			double norm_x = sqrt(pow(delta_x_n[0],2) + pow(delta_x_n[1],2));
			double e_x  = (delta_x_n_1[0] - x_tilde[0])/norm_x;
			double e_y =  (delta_x_n_1[1] - x_tilde[1])/norm_x ;
		
			// assemble forces 
			RPEN->ve[2*index] += epsilon_penalty*material_points->MP[i]->shape_function->phi->ve[k]*e_x;
			RPEN->ve[2*index+1] += epsilon_penalty*material_points->MP[i]->shape_function->phi->ve[k]*e_y;

			}




		} //end of loop over points




		return ;
	}





		