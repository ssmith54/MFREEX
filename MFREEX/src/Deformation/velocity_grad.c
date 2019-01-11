#include "Deformation/velocity_grad.h"



int velocity_grad(MAT * L, MAT * D, MAT * W, MAT * Fdot, MAT * invF)
{

	L = m_mlt(Fdot,invF,L);


	int dim = L->m;


	if ( dim == 2)
	{
		D->me[0][0] = 0.5*(L->me[0][0] + L->me[0][0]);
		D->me[0][1] = 0.5*(L->me[0][1] + L->me[1][0]);
		D->me[1][0] = 0.5*(L->me[1][0] + L->me[0][1]);
		D->me[1][1] = 0.5*(L->me[1][1] + L->me[1][1]);

		// row
		W->me[0][0] = 0.5*(L->me[0][0] - L->me[0][0]);
		W->me[0][1] = 0.5*(L->me[0][1] - L->me[1][0]);
		// row2
		W->me[1][0] = 0.5*(L->me[1][0] - L->me[0][1]);
		W->me[1][1] = 0.5*(L->me[1][1] - L->me[1][1]);
	}

	if ( dim == 3)
	{
		// Row 1
		D->me[0][0] = 0.5*(L->me[0][0] + L->me[0][0]);
		D->me[0][1] = 0.5*(L->me[0][1] + L->me[1][0]);
		D->me[0][2] = 0.5*(L->me[0][2] + L->me[2][0]);
		// Row 2
		D->me[1][0] = 0.5*(L->me[1][0] + L->me[0][1]);
		D->me[1][1] = 0.5*(L->me[1][1] + L->me[1][1]);
		D->me[1][2] = 0.5*(L->me[1][2] + L->me[2][1]);
		// Row 3
		D->me[2][0] = 0.5*(L->me[2][0] + L->me[0][2]);
		D->me[2][1] = 0.5*(L->me[2][1] + L->me[1][2]);
		D->me[2][2] = 0.5*(L->me[2][2] + L->me[2][2]);


		W->me[0][0] = 0.5*(L->me[0][0] - L->me[0][0]);
		W->me[0][1] = 0.5*(L->me[0][1] - L->me[1][0]);
		W->me[0][2] = 0.5*(L->me[0][2] - L->me[2][0]);

		W->me[1][0] = 0.5*(L->me[1][0] - L->me[0][1]);
		W->me[1][1] = 0.5*(L->me[1][1] - L->me[1][1]);
		W->me[1][2] = 0.5*(L->me[1][2] - L->me[2][1]);

		W->me[2][0] = 0.5*(L->me[2][0] - L->me[0][2]);
		W->me[2][1] = 0.5*(L->me[2][1] - L->me[1][2]);
		W->me[2][2] = 0.5*(L->me[2][2] - L->me[2][2]);
		
	}

	return 0;
}
