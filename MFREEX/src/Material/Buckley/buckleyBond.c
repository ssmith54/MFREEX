/*
 * =====================================================================================
 *
 *       Filename:  buckleyBond.c
 *
 *    Description:  Finds the bond stress component of the Buckley model
 *
 *        Version:  1.0
 *        Created:  04/05/18 11:09:36
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  STEPHEN SMITH
 *   Organization:  QUEEN'S UNIVERSITY BELFAST 
 *
 * =====================================================================================
 */
#include "buckleyBond.h"
#include "trace.h"

int buckleyBond(MAT * Sb_n_1, State stateOld, VEC * para, double dt){



	// material constants
	double Gb = para->ve[10];
	double H0 = para->ve[7];
	double R = para->ve[8];
	double star_T = para->ve[4];
	double temperature = para->ve[11];
	double Vs = para->ve[0];
	double Vp = para->ve[1];
	double star_mu0 = para->ve[2];
	double vogel_T = para->ve[3];
	double Cv = para->ve[6];




	double tauOCT = sqrt (  (1.000/3.00) * contraction(stateOld->Sb,stateOld->Sb)  ) ;
	double alpha_sig = 1;
	
	double sigma_m = stateOld->mSigma;


		 if (sigma_m < 0)
	 {
	 	sigma_m = 0;
 	}

	if ( tauOCT == 0){
		alpha_sig = 1;
	}else{
		alpha_sig = ( Vs*tauOCT/(2*R*temperature) ) * exp ( -Vp * sigma_m/(R*temperature) ) / ( sinh ( Vs * tauOCT/(2*R*temperature)) ) ; 
	}

	// alpha_s
	double alpha_s = exp ( Cv / ( temperature - vogel_T) - Cv / ( star_T - vogel_T) );	
	// alpha_T
	double alpha_T = exp ( (H0/R) * ( 1/temperature - 1/star_T) );


	// tau = tau_s * alpha_s * alpha_T * alpha_sig ;
	double tau = (star_mu0/(2*Gb))*alpha_sig*alpha_s*alpha_T;


	MAT * deltaSb = m_get(3,3);
	MAT * mat1 = m_get(3,3) ;
	MAT * mat2 = m_get(3,3) ;
	MAT * mat3 = m_get(3,3);
	MAT * mat4 = m_get(3,3);
	MAT * l = m_get(3,3);


	m_add(stateOld->Dbar,stateOld->Wbar,l);

	double trace_l = l->me[0][0] + l->me[1][1] + l->me[2][2]; 

	double sbFactor = 1;


	sbFactor = 1 - exp(-dt/tau);


	// mat1 = 2Gb*D*tau
	sm_mlt(2*Gb*tau,stateOld->Dbar,mat1);

	// 2Gb*D*tau - Sb_n
	m_sub(mat1,stateOld->Sb,deltaSb);

	// deltaSb = ( 1- exp(-dt/tau)) * ( 2G*D*tau - Sb_n)
	sm_mlt(sbFactor,deltaSb, deltaSb);

	// Sb_n_1 = Sb_n + deltaSb + (WSb_n - Sb_n W)dt
	m_add(stateOld->Sb,deltaSb,Sb_n_1);

	// Spin component of stress
	// l*s
	m_mlt(l,stateOld->Sb,mat1);
	// s*l'
	mmtr_mlt(stateOld->Sb,l,mat2);
	// trace(l)*S
	sm_mlt(trace_l,stateOld->Sb,mat3);
	// l+s + s*l'
	m_add(mat1,mat2,mat4);
	// l*s + s*l' + trace(l)s
	m_sub(mat4,mat3,mat1);
	// mat1 = (mat1)*dt;
	sm_mlt(dt,mat1,mat2);

	// finally update Sb_n_1
	m_add(Sb_n_1,mat2,Sb_n_1);


	

	M_FREE(l);
	M_FREE(deltaSb); 
	M_FREE(mat1);
	M_FREE(mat2);
	M_FREE(mat3);
	M_FREE(mat4);



	return 0;
}
