#include <stdlib.h>
#include <stdio.h>
#include "matrix.h"
#include "matrix2.h"
#include "generate_voronoi.h"
#include "meshgrid.h"
#include <sys/time.h>
#include "Integration/SCNI/generate_scni.h"
#include "mls_shapefunction.h"
#include "setDomain.h"
#include "smoothstep.h"
#include "Force/Internal/internalForce_hyperelastic.h"
#include "mat2csv.h"
#include "trigen.h"

int main(void )
{


	/* ------------------------------------------*/
	/* --------------Set up domain---------------*/
	/* ------------------------------------------*/
	
	// Dimension of problem
	int dim = 2;

	/* ------------------------------------------*/
	/* -------------Material & Loading-----------*/
	/* ------------------------------------------*/

	// material parameters and model (St. Venant Kirchoff)
	double nu = 0.0;
	double E = 4.8e3;
	char * material = "SVK";
	double rho = 1000e-9;

	VEC * materialParameters = v_get(2);
	materialParameters->ve[0] = (E*nu)/((1+nu)*(1-2*nu));
	materialParameters->ve[1] =E/(2*(1+nu));


	// tip load
	double P = 9.00;
	// direction of the load
	VEC * dir_load = v_get(dim);
	dir_load->ve[0] = 0;
	dir_load->ve[1] = 1;

	// Beam dimensions 
	double h = 1.00;
	double L = 20.00;

	// Plotting parameters
	double Ixx = 1.000/12.000;
	double yFactor = pow(L,2)/(E*Ixx);
	double xFactor = 1/L;
	double yPoint = P*yFactor;
	double xPoint = 0;


	/* ------------------------------------------*/
	/* --------------Tip Loaded Beam--------------*/
	/* ------------------------------------------*/

	// Number of nodes
	int num_nodes_x = 40;
	int num_nodes_y = 5;


	// Read PLSG 
	char opt[20] = "pDq30a0.05";
	char fileName[30] = "square";
	double * points_out ;
	int * boundaryNodes;
	int numBoundary;
	int * nodalMarkers;
	int  numnodes;
	trigen(&points_out,&boundaryNodes,opt,fileName,&numnodes,&numBoundary,&nodalMarkers,NULL);
	MAT * xI = m_get(numnodes,dim);


	// create nodes
	for ( int i = 0 ; i < numnodes ; i++)
	{
		xI->me[i][0] = points_out[2*i];
		xI->me[i][1] = points_out[2*i+1];

	}

	struct timeval start, end;

	// generate clipped voronoi diagram
	gettimeofday(&start, NULL);
	voronoi_diagram * vor = NULL;
	vor = generate_voronoi(xI->base, boundaryNodes, xI->m, numBoundary, 2);
 	// get time took to run
	gettimeofday(&end, NULL);
	double delta = ((end.tv_sec  - start.tv_sec) * 1000000u + 
         end.tv_usec - start.tv_usec) / 1.e6;

	// get time taken to run
	printf("voronoi took %lf seconds to run\n", delta);


	// Print voronoi diagram to file 
	FILE * fp = fopen("cells.txt","w");
	print_voronoi_diagram(fp,vor);
	fclose(fp);




	/* ------------------------------------------*/
	/* ------------Meshfree Domain---------------*/
	/* ------------------------------------------*/

	// shape function parameters
	double dmax = 2.5;
	int constant_support_size = 1;
	char * basis = "quadratic";
	char * weight = "cubic";
	int compute = 3;
	VEC * dI = v_get(xI->m);

	// meshfree domain
	meshfreeDomain mfree = {.nodes = xI, .di = dI, .num_nodes = xI->m, .dim = dim};
	setDomain(&mfree,constant_support_size, dmax);


	// get transformation matrix at nodal points

	shape_function_container * sf_nodes = mls_shapefunction(mfree.nodes, "linear", "cubic", 2, 1, &mfree);

	MAT * Lambda = m_get(2*mfree.num_nodes, 2*mfree.num_nodes);

	// u = Lambda * u_g
	for ( int i = 0 ; i < mfree.num_nodes ; i++)
	{
		VEC * phi = sf_nodes->sf_list[i]->phi;
		IVEC * neighbours  = sf_nodes->sf_list[i]->neighbours;
		for ( int k = 0 ; k < neighbours->max_dim ; k++)
		{
			Lambda->me[2*i][2*neighbours->ive[k]] += phi->ve[k]; 
			Lambda->me[2*i+1][2*neighbours->ive[k]+1] += phi->ve[k]; 

		}

	}

	/* ------------------------------------------*/
	/* ----------------SCNI CELLS----------------*/
	/* ------------------------------------------*/

	int is_stabalised = 0;
	int is_AXI = 0;
	SCNI_OBJ * _scni_obj = NULL;
	struct timeval start2, end2;
	gettimeofday(&start2, NULL);
	// set up return container
	// generate shape functions at sample points 
	_scni_obj = generate_scni(vor, NULL , is_stabalised, is_AXI, dim, &mfree);
	gettimeofday(&end2, NULL);
	 delta = ((end2.tv_sec  - start2.tv_sec) * 1000000u + 
         end2.tv_usec - start2.tv_usec) / 1.e6;
	printf("scni function took %lf seconds to run\n", delta);
	iv_foutput(stdout, _scni_obj->scni[2]->sfIndex);
	m_foutput(stdout,_scni_obj->scni[2]->B);



	// Test divergence free condition
	v_foutput(stdout,dI);
	int checkPoint = 121;
	IVEC * index ;
	MAT * B; 
	MAT * check_B = m_get(dim*dim,2);
	printf("checking divergence free condition at point %d\n", checkPoint);
	printf("with coordinates %lf %lf\n", mfree.nodes->me[checkPoint][0], mfree.nodes->me[checkPoint][1]);
	printf("cell area = %lf \n", _scni_obj->scni[checkPoint]->area);
	for ( int i = 0 ; i < _scni_obj->num_points ; i++)
	{
		index = _scni_obj->scni[i]->sfIndex;
		B = _scni_obj->scni[i]->B;

		for (int k = 0 ; k < index->max_dim ; k++){
			int indx = index->ive[k];

			if ( indx == checkPoint)
			{
				check_B->me[0][0] += B->me[0][2*k]*_scni_obj->scni[i]->area;
				check_B->me[1][1] += B->me[1][2*k+1]*_scni_obj->scni[i]->area;
				check_B->me[2][0] += B->me[2][2*k]*_scni_obj->scni[i]->area;
				check_B->me[3][1] += B->me[3][2*k+1]*_scni_obj->scni[i]->area;
			}
		}
	}
	m_foutput(stdout, check_B);

	/* ------------------------------------------*/
	/* -------------Boundary Conditions----------*/
	/* ------------------------------------------*/


	// get boundary nodes
	IVEC * eb_nodes = iv_get(num_nodes_y);
	IVEC * traction_nodes = iv_get(num_nodes_y);

	int num_nodes_eb = 0;
	int num_nodes_trac = 0;
	double *x = NULL;
	for ( int i = 0 ; i < mfree.num_nodes ; i++ )
	{
		x = mfree.nodes->me[i];

		if ( x[0] == 0)
		{
			traction_nodes->ive[num_nodes_trac] = i;
			++num_nodes_trac;
		}
		if(  x[0] == L)
		{
			eb_nodes->ive[num_nodes_eb] = i;
			++num_nodes_eb;
		}

	}
	// 
	iv_foutput(stdout,traction_nodes);
	iv_foutput(stdout, eb_nodes);


	// Traction nodes
	MAT * traction_nodes_coords = m_get(num_nodes_trac,dim);
	for ( int i = 0 ; i < num_nodes_trac ; i++)
	{
		traction_nodes_coords->me[i][0] = mfree.nodes->me[traction_nodes->ive[i]][0];
		traction_nodes_coords->me[i][1] = mfree.nodes->me[traction_nodes->ive[i]][1];

	}
	// get shape function and traction nodes 
	shape_function_container * phi_traction = mls_shapefunction(traction_nodes_coords, 
		"linear", "quartic", 2, 1, &mfree);

	/* ------------------------------------------*/
	/* ----------------Mass Vector---------------*/
	/* ------------------------------------------*/

	VEC * nodal_mass = v_get(mfree.num_nodes);

	for ( int i = 0 ; i < mfree.num_nodes ; i++)
	{
		nodal_mass->ve[i] = _scni_obj->scni[i]->area*rho;
	}

	/* ------------------------------------------*/
	/* ----------------Time stepping-------------*/
	/* ------------------------------------------*/
	
	// time parameters
	double t_max = 1.00; // 1s
	double delta_t = 1e-7;
	double t_n = 0;
	double t_n_1 = 0;



	// External Forces
	VEC * Fext_n_1 = v_get(dim*mfree.num_nodes);
	VEC * Fext_n = v_get(dim*mfree.num_nodes);

	// Internal Forces
	VEC * Fint_n_1 = v_get(dim*mfree.num_nodes);
	VEC * Fint_n = v_get(dim*mfree.num_nodes);

	// Net Force
	VEC * Fnet_n_1 = v_get(dim*mfree.num_nodes);


	// Kinematic variables
	// displacement
	VEC * d_n_1 = v_get(dim*mfree.num_nodes);
	VEC * d_n = v_get(dim*mfree.num_nodes);
	VEC * delta_disp = v_get(dim*mfree.num_nodes);
	VEC * nodal_disp = v_get(dim*mfree.num_nodes);
	// Velocity
	VEC * v_n_h = v_get(dim*mfree.num_nodes);
	VEC * v_n_mh = v_get(dim*mfree.num_nodes);
	VEC * v_n_1 = v_get(dim*mfree.num_nodes);
	VEC * v_n = v_get(dim*mfree.num_nodes);
	// Acceleration
	VEC * a_n_1 = v_get(dim*mfree.num_nodes);
	VEC * a_n = v_get(dim*mfree.num_nodes);

	MAT * updatedNodes = m_get(mfree.num_nodes,2);
	// tip load
	VEC * phi;
	IVEC * neighbours;
	double tipLoad = 0;

	// How often to write outputs
	int writeFreq = 1000;
	int fileCounter = 0;

	// time step counter;
	int n = 0;

	// Energy
	double Wext = 0;
	double Wint = 0;
	double Wkin = 0;
	double Wbal = 0;



	//while ( t_n < t_max)
	while ( t_n < t_max)
	{

		// Update time step
		t_n_1 = t_n + delta_t;


		// Find d_n_1 and v_n_h
		// update velocity
		v_mltadd(v_n_mh, a_n, delta_t, v_n_h);
		// update displacements
		v_mltadd(d_n,v_n_h,delta_t,d_n_1);

		// implement boundary conditions

		for ( int i = 0 ; i < num_nodes_eb ; i++)
		{
			d_n_1->ve[eb_nodes->ive[i]*2] = 0;
			d_n_1->ve[eb_nodes->ive[i]*2 + 1] = 0;
			v_n_h->ve[eb_nodes->ive[i]*2] = 0;
			v_n_h->ve[eb_nodes->ive[i]*2 + 1] = 0;
		}

		// find nodal disp
		mv_mlt(Lambda,d_n_1,nodal_disp);

		for ( int i = 0 ; i < mfree.num_nodes ; i++)
		{
			updatedNodes->me[i][0] = mfree.nodes->me[i][0] + nodal_disp->ve[2*i];
			updatedNodes->me[i][1] = mfree.nodes->me[i][1] + nodal_disp->ve[2*i+1];

		}

		// Implement boundary conditions
		// Problem specific code
		// x-y 
		xPoint = nodal_disp->ve[traction_nodes->ive[3]*2+1]*xFactor;
		yPoint = tipLoad * pow(L,2)*(1/E)*(1/Ixx);


		/* ------------------------------------------*/
		/* ------------Find External Force-----------*/
		/* ------------------------------------------*/

		/*  Manually find point load */
		neighbours = phi_traction->sf_list[2]->neighbours;
		phi = phi_traction->sf_list[2]->phi;
		tipLoad = P*smoothstep(t_n_1,t_max,0);
		v_zero(Fext_n_1);
		for ( int i = 0 ; i < neighbours->max_dim; i++){
			// x
			Fext_n_1->ve[2*neighbours->ive[i]] = phi->ve[i]*tipLoad*dir_load->ve[0];
			 // y
			Fext_n_1->ve[2*neighbours->ive[i]+1] = phi->ve[i]*tipLoad*dir_load->ve[1];
		}

		/* ------------------------------------------*/
		/* ------------Find Internal Force-----------*/
		/* ------------------------------------------*/


		internalForce_hyperelastic(Fint_n_1, _scni_obj, d_n_1, materialParameters, "SVK", is_AXI, dim);

		/* ------------------------------------------*/
		/* ---------------Find Net Force-------------*/
		/* ------------------------------------------*/
		v_sub(Fext_n_1, Fint_n_1, Fnet_n_1);

		/* ------------------------------------------*/
		/* ---------------Find Acceleration----------*/
		/* ------------------------------------------*/

		// if ( n % 1000 == 0)

		for ( int i = 0 ; i < mfree.num_nodes; i++)
		{
			a_n_1->ve[2*i] = Fnet_n_1->ve[2*i]/nodal_mass->ve[i]; 
			a_n_1->ve[2*i+1] = Fnet_n_1->ve[2*i+1]/nodal_mass->ve[i]; 

		}


		// update to interger time step velocities
		v_mltadd(v_n_h,a_n_1,0.5*delta_t,v_n_1);	


		for ( int i = 0 ; i < num_nodes_eb ; i++)
		{

			v_n_1->ve[eb_nodes->ive[i]*2] = 0;
			v_n_1->ve[eb_nodes->ive[i]*2 + 1] = 0;
		}



		// save outputs
		if ( n % writeFreq == 0 ){
			char filename[50];
			snprintf(filename, 50, "displacement_%d%s",fileCounter,".txt");
			mat2csv(updatedNodes,"./Displacement",filename);
			fp = fopen("loadDisp.txt","a");
			fprintf(fp,"%lf %lf\n",xPoint,yPoint);
			fclose(fp);
			fileCounter++;	
		}

		/* ------------------------------------------*/
		/* -----------------Find Energy--------------*/
		/* ------------------------------------------*/
		delta_disp = v_sub(d_n_1, d_n, delta_disp);

		Wext += in_prod(delta_disp, Fext_n) + in_prod(delta_disp, Fext_n_1);
		Wint += in_prod(delta_disp, Fint_n) + in_prod(delta_disp, Fint_n_1);
		Wkin = 0;
		for ( int i = 0 ; i < mfree.num_nodes; i++ )
		{
			Wkin += 0.5*(v_n_1->ve[2*i]*nodal_mass->ve[i]*v_n_1->ve[2*i]);
			Wkin += 0.5*(v_n_1->ve[2*i+1]*nodal_mass->ve[i]*v_n_1->ve[2*i+1]);

		}

		double Wmax = max(Wext,Wint);
		Wmax =  max(Wmax,Wkin);
		if ( Wmax == 0)
		{
			Wbal = 0;
		}else{
			Wbal = fabs(Wkin + Wint - Wext)/Wmax;
		}

		// Store previous time step quanities for the kinematic, and force variables.
		v_copy(Fint_n_1,Fint_n);
		v_copy(Fext_n_1,Fext_n);
		v_copy(v_n_h,v_n_mh);
		v_copy(d_n_1,d_n);
		v_copy(v_n_1,v_n);
		v_copy(a_n_1,a_n);

		t_n = t_n_1;

		++n;
		printf("%i  \t  %lf  \t %lf \t  %10.2E \n",n,t_n,tipLoad, Wbal);

	}









	exit(0);


}
