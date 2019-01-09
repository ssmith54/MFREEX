#include <stdlib.h>
#include <stdio.h>
#include "matrix.h"
#include "matrix2.h"
#include "meshgrid.h"
#include "generate_voronoi.h"
#include "meshgrid.h"
#include <sys/time.h>
#include "Material/material.h"
#include "Integration/SCNI/generate_scni.h"
#include "Integration/SCNI/generate_mscni.h"
#include "mls_shapefunction.h"
#include "setDomain.h"
#include "smoothstep.h"
#include "Force/Internal/internalForce_Buckley.h"
#include "Force/Internal/internalForce_hyperelastic.h"

#include "mat2csv.h"
#include "trigen.h"
#include "Boundary/getBoundary.h"
#include "Boundary/iv_addNode.h"
#include "Boundary/Traction/Cavity/cavityVolume.h"
#include "Boundary/Traction/Cavity/flowRate.h"
#include "Boundary/Traction/Pressure/pressure_load.h"
#include <math.h>
#include "Deformation/poldec.h"
#include "Material/Buckley/new_Buckley_State.h"
#include "Boundary/Contact/contactDetection.h"



int main(int argc, char** argv) {

	/*////////////////////////////////////////////////////////// */
	/*                                                           */
	/*			        Initialisation                           */
	/*                                                           */
	/*////////////////////////////////////////////////////////// */

	FILE * fp;


	int dim = 2;
	int is_AXI = 1;

	/*  Material parameters */
	const double rho = 1380e-9;


	double nu = 0.4;
	double E = 10;
	char * material = "SVK";
	VEC * materialParameters = v_get(2);
	materialParameters->ve[0] = (E*nu)/((1+nu)*(1-2*nu));
	materialParameters->ve[1] =E/(2*(1+nu));



	/*  Material struct */
	VEC * matParams = v_get(26);
	matParams->ve[0] = 2.814e-3; // VS
	matParams->ve[1] = 0.526e-3; // VP
	matParams->ve[2] = (1.71e6); // mu*_0
	matParams->ve[3] = (328.76); // Tinf
	matParams->ve[4] = 358.15; // T*
	matParams->ve[5] = matParams->ve[4]; // Tf*
	matParams->ve[6] = (67.47); // Cv
	matParams->ve[7] = 1.23e5; // H0
	matParams->ve[8] = 8.314; // R
	matParams->ve[9] = 1.8e9; // Kb
	matParams->ve[10] = 6e8;// Gb
	matParams->ve[11] = -100000000000;// temperature
	matParams->ve[12] = matParams->ve[11];// Tf
	// conformational constants
	matParams->ve[13] = 0.1553;// alpha_c
	matParams->ve[14] = 0.001;// eta_c
	matParams->ve[15] = 1.8098e17;// Ns_c
	matParams->ve[16] = 1.38e-17;// boltzmann constant kB
	// slippage
	matParams->ve[17] = 100;// lambdaCrit
	matParams->ve[18] = 383.15;// Ts 
	matParams->ve[19] = 0.653e6;// gamma0_ref = 0.653
	matParams->ve[20] = 10612;// Cs 10612
	matParams->ve[21] = 95.48;// Tinf 95.48
	matParams->ve[22] = 0.1565;// C1
	matParams->ve[23] = 39.937;// C2
	matParams->ve[24] = 0.9878;// beta
	matParams->ve[25] = 0.33;// poissons ratio
	VEC * critLambdaParams = v_get(6);
	critLambdaParams->ve[0] = -0.0111; // C1
	critLambdaParams->ve[1] = 3.627; // C2
	critLambdaParams->ve[2] = 0.9856; // BETA
	critLambdaParams->ve[3] = -0.0356; // k
	critLambdaParams->ve[4] = 15.393; // b 
	critLambdaParams->ve[5] = matParams->ve[11]; // temperature 

	/*  Time step variables */

	double tMax = 0.8;





	/* ------------------------------------------*/
	/* --------------Flow restrictor-------------*/
	/* ------------------------------------------*/

	/*  Upstream parameters */
	double P0 = 0;
	double tLine = 304.724;
	double pLine = 0.8; // 0.6 Mpa;
	double molarMass = 29;
	double Rg = 8.314;
	double rLine = Rg/molarMass;
	double gammaLine = 1.4;
	double aReduced = 0.0003924;
	double vDead = (85*1000) ; /*  dead volume in mL -> mm^3 */

	/* ------------------------------------------*/
	/* ---------------Stretch rod----------------*/
	/* ------------------------------------------*/


	// stretch rod polynomial
	double a0 = -2.2264e7;
	double a1 = 2.3704e7;
	double a2 = -9.3769e6;
	double a3 = 1.6212e6;
	double a4 =-9.7380e4;
	double a5 = -1.8801e3;
	double a6 = 559.3131;
	double a7 = 0.2565;


	double stretchRodRad = 5.5;
	int numPointsRod = 15;

	MAT * srNodes = m_get(numPointsRod+1,2);

	MAT * srNodes_O = m_get(numPointsRod+1,2);
	for ( int i = 0 ; i < numPointsRod ; i++){
		double theta = -PI/2.00 + (PI/2/(numPointsRod-1))*i;
		srNodes->me[i][0] = stretchRodRad*cos(theta);
		srNodes->me[i][1] = 9.21+stretchRodRad*sin(theta);
		srNodes_O->me[i][0] = srNodes->me[i][0];
		srNodes_O->me[i][1] = srNodes->me[i][1];
	}
	srNodes->me[numPointsRod][0] = stretchRodRad;
	srNodes->me[numPointsRod][1] = 70;
	srNodes_O->me[numPointsRod][0] = stretchRodRad;
	srNodes_O->me[numPointsRod][1] = 70;

	/*  Stretch rod */
	double dRodStop = 135; /*  130 mm  */
	double tStop = 0.5;
	double tRampRod = 1e-9;
	double vRod = 0;
	const double DISP_ROD_MAX = -1 ; // 132;

	/*////////////////////////////////////////////////////////// */
	/*////////////////////////////////////////////////////////// */


	/*////////////////////////////////////////////////////////// */
	/*                                                           */
	/*			        PREPRCOCESSING STAGE                     */
	/*                                                           */
	/*////////////////////////////////////////////////////////// */



	/*  Create points 
	 *
	 *	Via a 2D triangulation 
	 *
	 *  */
	// Read PLSG 

	char opt[20] = "pDq0a2";
	char fileName[30] = "preform";
	double * points_out ;
	int * boundaryNodes;
	int numBoundary;
	int * nodalMarkers;
	int  numnodes;
	double * temperatures;
	trigen(&points_out,&boundaryNodes,opt,fileName,&numnodes,&numBoundary,&nodalMarkers,&temperatures);	


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
	fp = fopen("cells.txt","w");
	print_voronoi_diagram(fp,vor);
	fclose(fp);



	/* ------------------------------------------*/
	/* ------------Meshfree Domain---------------*/
	/* ------------------------------------------*/

	// shape function parameters
	double dmax = 2;
	int constant_support_size = 1;
	VEC * dI = v_get(xI->m);

	// meshfree domain
	meshfreeDomain mfree = {.nodes = xI, .di = dI, .num_nodes = xI->m, .dim = dim, .IS_AXI = is_AXI};
	setDomain(&mfree,constant_support_size, dmax);


	/* ------------------------------------------*/
	/* ------------Boundaries--------------------*/
	/* ------------------------------------------*/


	// Traction
	IVEC * traction_nodes ;
	getBoundary(&traction_nodes,boundaryNodes,numBoundary,nodalMarkers,numnodes,2);
	pressure_boundary * pB = new_pressure_boundary(traction_nodes, &mfree);
	m_foutput(stdout,pB->coords);

	// /*  EB1  */
	IVEC * eb1_nodes;
	getBoundary(&eb1_nodes,boundaryNodes,numBoundary,nodalMarkers,numnodes,5);
	iv_addNode(eb1_nodes,traction_nodes->ive[0],'s');
	int num_nodes_eb1 = eb1_nodes->max_dim;

	// /*  EB2 */
	IVEC * eb2_nodes;
	getBoundary(&eb2_nodes,boundaryNodes,numBoundary,nodalMarkers,numnodes,4);
	iv_addNode(eb2_nodes,traction_nodes->ive[traction_nodes->max_dim - 1],'e');
	int num_nodes_eb2 = eb2_nodes->max_dim;


	// /*  EB3 */
	int numB3 = 15;
	IVEC * eb3_nodes = iv_get(numB3);
	MAT * contact_nodes_coords = m_get(numB3,dim);

	for ( int i = 0 ; i < numB3 ; i++){
		eb3_nodes->ive[i] = traction_nodes->ive[traction_nodes->max_dim -1 - i];
		contact_nodes_coords->me[i][0] = mfree.nodes->me[eb3_nodes->ive[i]][0];
		contact_nodes_coords->me[i][1] = mfree.nodes->me[eb3_nodes->ive[i]][1];

	}

		// get shape function and contact nodes
	shape_function_container * phi_contact = mls_shapefunction(contact_nodes_coords, 
		"linear", "quartic", 2, 1, &mfree);
	m_foutput(stdout,contact_nodes_coords);

	

	/* ------------------------------------------*/
	/* ------------------SCNI--------------------*/
	/* ------------------------------------------*/

	int is_stabalised = 0;
	SCNI_OBJ * _scni_obj = NULL;
	MSCNI_OBJ * _mscni_obj = NULL;

	struct timeval start2, end2;
	gettimeofday(&start2, NULL);
	// set up return container
	// generate shape functions at sample points 
	_scni_obj = generate_scni(vor, NULL , is_stabalised, is_AXI, dim, &mfree);
	gettimeofday(&end2, NULL);
	delta = ((end2.tv_sec  - start2.tv_sec) * 1000000u + 
         end2.tv_usec - start2.tv_usec) / 1.e6;
	printf("scni function took %lf seconds to run\n", delta);

	// Test divergence free condition
	IVEC * index ;
	MAT * B; 
	MAT * check_B = m_get(dim*dim,2);
	printf("checking scni \n \n");
	int checkPoint = 33;

	m_foutput(stdout,_scni_obj->scni[checkPoint]->B);
	iv_foutput(stdout, _scni_obj->scni[checkPoint]->sfIndex);

	printf("checking divergence free condition at point %d\n", checkPoint);
	printf("with coordinates %lf %lf\n", mfree.nodes->me[checkPoint][0], mfree.nodes->me[checkPoint][1]);
	printf("cell area = %lf \n", _scni_obj->scni[checkPoint]->area);
	printf("and neighbours \n");
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
	// m_foutput(stdout, check_B);


	// m_foutput(stdout,_scni_obj->scni[checkPoint]->B);
	// iv_foutput(stdout, _scni_obj->scni[checkPoint]->sfIndex);
	


	/* ------------------------------------------*/
	/* ----------------Mass Vector---------------*/
	/* ------------------------------------------*/

	VEC * nodal_mass = v_get(mfree.num_nodes);
	VEC * inv_nodal_mass = v_get(mfree.num_nodes);

	for ( int i = 0 ; i < mfree.num_nodes ; i++)
	{
		double volume = _scni_obj->scni[i]->area;
		if ( is_AXI == 1)
		{
			double r = _scni_obj->scni[i]->r;
			volume = volume*r*2*PI;
		}
		nodal_mass->ve[i] = volume*rho;
		inv_nodal_mass->ve[i] = 1.000/nodal_mass->ve[i];

	}

	printf("total mass = %lf (g) \n", v_sum(nodal_mass)*1000);
	//v_foutput(stdout,nodal_mass);
	

	/* ------------------------------------------*/
	/* -----------Transformation matrix----------*/
	/* ------------------------------------------*/


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
	/* --------------Cavity pressure-------------*/
	/* ------------------------------------------*/

	// /*  Cavity pressure paramters */
	double chokedMassRate =  flowRate(0.528, tLine, pLine*1e6, rLine, aReduced, gammaLine);
	double volumeInitial = cavityVolume(traction_nodes,mfree.nodes);
	double massInitial = (P0*volumeInitial)/(rLine*1000*tLine);
	double massAir = massInitial;
	double pRatio;

	// // tst poldec

	// int test_dim = 3;
	// MAT * F = m_get(test_dim,test_dim);
	// F->me[0][0] = 1;
	// F->me[0][1] = 0.495;
	// F->me[0][2] = 0.5;

	// F->me[1][0] = -0.333;
	// F->me[1][1] = 1;
	// F->me[1][2] = -0.247;


	// F->me[2][0] = 0.959;
	// F->me[2][1] = 0;
	// F->me[2][2] = 1.5;


	// MAT * R = m_get(test_dim,test_dim);
	// MAT * U = m_get(test_dim,test_dim);
	// MAT * V = m_get(test_dim,test_dim);


	// poldec(F, R, U, V);
	// printf("F = \n");
	// m_foutput(stdout,F);
	// printf("R = \n");
	// m_foutput(stdout,R);
	// printf("U = \n");
	// m_foutput(stdout,U);
	// printf("V = \n");
	// m_foutput(stdout,V);

	/* ------------------------------------------*/
	/* --------------State storage---------------*/
	/* ------------------------------------------*/
	// store F, D at each time step for each material point for use with buckley model 
	state_Buckley ** state_n = new_Buckley_State(mfree.num_nodes,temperatures,is_AXI,dim);;
	state_Buckley ** state_n_1 = new_Buckley_State(mfree.num_nodes,temperatures,is_AXI,dim);




	// ///////////////////////////////////////////////////////////////

	// /*////////////////////////////////////////////////////////// */
	// /*                                                           */
	// /*			      EXPLICIT TIME STEPPNG                      */
	// /*                                                           */
	// /*////////////////////////////////////////////////////////// */
	
	// time parameters
	double t_max = 0.5; // 1s
	double delta_t = 2e-7;
	double t_n = 0;
	double t_n_1 = 0;
	double t_n_h =  0; 


	VEC * phi;
	IVEC * neighbours;


	int num_dof = dim*mfree.num_nodes;


	// External Forces
	VEC * Fext_n_1 = v_get(num_dof);
	VEC * Fext_n = v_get(num_dof);

	// Internal Forces
	VEC * Fint_n_1 = v_get(num_dof);
	VEC * Fint_n = v_get(num_dof);

	// Conctact Forces
	VEC * Fcont_n_1 = v_get(num_dof);


	// Net Force
	VEC * Fnet_n_1 = v_get(num_dof);

	// Kinematic variables
	// displacement
	VEC * d_n_1 = v_get(num_dof);
	VEC * d_n = v_get(num_dof);
	VEC * delta_disp = v_get(num_dof);
	VEC * nodal_disp = v_get(num_dof);
	// Velocity
	VEC * v_n_h = v_get(num_dof);
	VEC * v_n_mh = v_get(num_dof);
	VEC * v_n_1 = v_get(num_dof);
	VEC * v_n = v_get(num_dof);
	// Acceleration
	VEC * a_n_1 = v_get(num_dof);
	VEC * a_n = v_get(num_dof);
	MAT * updatedNodes = m_get(mfree.num_nodes,2);

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

	// original position of the nodes
	MAT * nodes_X = m_copy(mfree.nodes,MNULL);

	// contact
	double f1Cor = 0;
	double f2Cor = 0; 
	MAT * msNormal = m_get(1,dim);
	MAT * testPoint = m_get(1,dim);
	double distanceProj = 0; 


	//External loading
	double pre_n = 0;
	double pre_n_1 = 0; 
	double volume = 0;
	double volume_t = 0;

	/*  For writing to file */
	fp = fopen("pressureTime.txt","w");
	fprintf(fp,"%lf %lf\n",0.0,0.0);
	fclose(fp);


	// stretch rod displacment 
	double disp_rod = 0;


	// timing parameters
	struct timeval start3, end3;
	gettimeofday(&start3, NULL);



	/*  Explicit Loop */
	//while ( t_n < tMax)
	while ( n < 50000 )
	{

		/*  Update time step */
		t_n_1 = t_n + delta_t;
		t_n_h = (1.00/2.00)*(t_n + t_n_1);

		/*  Make a time step  */ 
		__mltadd__(v_n_h->ve, a_n->ve,delta_t,num_dof);
		__mltadd__(d_n_1->ve,v_n_h->ve,delta_t, num_dof);



		/* ------------------------------------------*/
		/* -----------Contact Conditions-------------*/
		/* ------------------------------------------*/
		__zero__(Fcont_n_1->ve, num_dof);

		if ( disp_rod < DISP_ROD_MAX){
		/*  Update stretch rod */
			disp_rod = a0*pow(t_n_1,7) + a1*pow(t_n_1,6) + a2*pow(t_n_1,5) + a3*pow(t_n_1,4) + a4*pow(t_n_1,3) + a5*pow(t_n_1,2) +
			a6*pow(t_n_1,1) + a7;
			//disp_rod = vRod*t_n_1;

			for ( int i = 0 ; i < srNodes->m ; i++){
	
				srNodes->me[i][1] = srNodes_O->me[i][1] - disp_rod;

			}
		}


		for ( int i = 0 ; i < eb3_nodes->max_dim ; i++){

			neighbours = phi_contact->sf_list[i]->neighbours;
			phi = phi_contact->sf_list[i]->phi;
			testPoint->me[0][0] = updatedNodes->me[eb3_nodes->ive[i]][0];
			testPoint->me[0][1] = updatedNodes->me[eb3_nodes->ive[i]][1];

			distanceProj = contactDetection(testPoint,srNodes,msNormal);

			if (distanceProj > 0){

				f1Cor = 1*(2*distanceProj*msNormal->me[0][0]*nodal_mass->ve[eb3_nodes->ive[i]])/pow(delta_t,2);
				f2Cor = 1*(2*distanceProj*msNormal->me[0][1]*nodal_mass->ve[eb3_nodes->ive[i]])/pow(delta_t,2);



				for ( int k = 0 ; k < neighbours->max_dim ; k++){
					Fcont_n_1->ve[2*neighbours->ive[k]] += phi->ve[k]*f1Cor; 
					Fcont_n_1->ve[2*neighbours->ive[k]+1] += phi->ve[k]*f2Cor; 
				}

			}


		}


		/*  Find a corrective acceleration - method in pronto 3D manual*/
		/*  Make a time step  */ 
		for ( int i = 0 ; i < numnodes  ; i++ )
		{
			a_n->ve[2*i] = Fcont_n_1->ve[2*i]*inv_nodal_mass->ve[i];
			a_n->ve[2*i+1] = Fcont_n_1->ve[2*i+1]*inv_nodal_mass->ve[i];
		}

		/*  Make a time step  */ 
		__mltadd__(v_n_h->ve, a_n->ve,delta_t,num_dof);
		__mltadd__(d_n_1->ve,v_n_h->ve,delta_t, num_dof);



		/* ------------------------------------------*/
		/* -----------Boundary Conditions------------*/
		/* ------------------------------------------*/
		// Fixed top boundary /

		for ( int i = 0 ; i < num_nodes_eb1 ; i++)
		{
			d_n_1->ve[eb1_nodes->ive[i]*2] = 0;
			d_n_1->ve[eb1_nodes->ive[i]*2 + 1] = 0;
			v_n_h->ve[eb1_nodes->ive[i]*2] = 0;
			v_n_h->ve[eb1_nodes->ive[i]*2 + 1] = 0;
		}

		// Symmetry boundary /
		for ( int i = 0 ; i < num_nodes_eb2 ; i++)
		{
			d_n_1->ve[eb2_nodes->ive[i]*2] = 0;
			v_n_h->ve[eb2_nodes->ive[i]*2] = 0;
		}

		// find new nodal positions
		mv_mlt(Lambda,d_n_1,nodal_disp);
		__add__(nodes_X->base, nodal_disp->ve, updatedNodes->base, num_dof);


		// /*  Implement BCs */
		// enforceBC(eb1,d_n_1); 
		// // find velocity correction
		// sv_mlt(1.00/(2*deltaT),eb1->uCorrect1,v_correct);
		// for ( int k = 0 ; k < v_correct->max_dim; k++){
		// 	v_n_h->ve[2*k] += v_correct->ve[k];
		// }

		// sv_mlt(1.000/(2*deltaT),eb1->uCorrect2,v_correct);
		// for ( int k = 0 ; k < v_correct->max_dim; k++){
		// 	v_n_h->ve[2*k+1] += v_correct->ve[k];
		// }


		// // Symmetry boundary /
		// enforceBC(eb2,d_n_1); 
		// sv_mlt(1.00/(2*deltaT),eb2->uCorrect1,v_correct);
		// for ( int k = 0 ; k < v_correct->max_dim; k++){
		// 	v_n_h->ve[2*k] += v_correct->ve[k];
		// }


		/* ------------------------------------------*/
		/* ------------Find External Force-----------*/
		/* ------------------------------------------*/
		__zero__(Fext_n_1->ve,num_dof);
		/*  Find Cavity volume */
		volume = cavityVolume(traction_nodes,updatedNodes);


		/*  Find Cavity pressure */
		pRatio = pre_n/pLine;
		if ( pRatio <= 0.528){
			massAir += chokedMassRate*delta_t;
		}else{
			massAir += flowRate(pRatio,tLine,pLine*1e6, rLine, aReduced,gammaLine)*delta_t;
		}
		pre_n_1 = ((P0*(volume - volumeInitial) + 1000*massAir*rLine*tLine)/(volume+vDead));

		/*  Update pressure load */
		update_pressure_boundary(pB, updatedNodes);
		assemble_pressure_load(Fext_n_1, -pre_n_1, pB);



		/* ------------------------------------------*/
		/* ------------Find Internal Force-----------*/
		/* ------------------------------------------*/

		/*  Internal force */
		//internalForce(Fint_n_1,&scni,d_n_1,matParams,efgBlock->numnode);
		//internalForceBuckley(Fint_n_1,scni,d_n_1,matParams,critLambdaParams,state_n,deltaT,efgBlock->numnode,t_n_1);
		internalForce_ForceBuckley(Fint_n_1, _scni_obj, d_n_1, v_n_h,
		matParams,critLambdaParams, state_n_1, state_n,
		mfree.IS_AXI, dim,delta_t);

		/* ------------------------------------------*/
		/* ---------------Find Net Force-------------*/
		/* ------------------------------------------*/

		/*  Balance of forces */
		__sub__(Fext_n_1->ve, Fint_n_1->ve, Fnet_n_1->ve,num_dof);

		/*  Find acceleration */
		for ( int i = 0 ; i < numnodes  ; i++ )
		{
			a_n_1->ve[2*i] = Fnet_n_1->ve[2*i]*inv_nodal_mass->ve[i];
			a_n_1->ve[2*i+1] = Fnet_n_1->ve[2*i+1]*inv_nodal_mass->ve[i];
		}

		/*  Integer time step velocity */
		v_mltadd(v_n_h,a_n_1,0.5*delta_t,v_n_1);	


		for ( int i = 0 ; i < num_nodes_eb1 ; i++)
		{

			v_n_1->ve[eb1_nodes->ive[i]*2] = 0;
			v_n_1->ve[eb1_nodes->ive[i]*2 + 1] = 0;
		}
		for ( int i = 0 ; i < num_nodes_eb2 ; i++)
		{

			v_n_1->ve[eb2_nodes->ive[i]*2] = 0;
		}


		/* ------------------------------------------*/
		/* --------------Write outputs---------------*/
		/* ------------------------------------------*/

		// update nodal positions
		writeFreq = 100;
		if ( n % writeFreq == 0 ){
			char filename[50];
			snprintf(filename, 50, "displacement_%d%s",fileCounter,".txt");
			mat2csv(updatedNodes,"./Displacement",filename);

			snprintf(filename, 50, "srRod_%d%s",fileCounter,".txt");
			mat2csv(srNodes,"./srRod",filename);

			fp = fopen("pressureTime.txt","a");
			fprintf(fp,"%lf %lf\n",t_n_1,pre_n_1);
			fclose(fp);

			fileCounter++;
		}


		/* ------------------------------------------*/
		/* -----------------Find Energy--------------*/
		/* ------------------------------------------*/
		// find change in displacement 
		__sub__(d_n_1->ve, d_n->ve, delta_disp->ve, num_dof);

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

		/*  Update counters */
		t_n = t_n_1;
		/*  Store previous volume */
		volume_t = volume; 
		// Store previous time step quanities for the kinematic, and force variables.
		v_copy(Fint_n_1,Fint_n);
		v_copy(Fext_n_1,Fext_n);
		v_copy(v_n_h,v_n_mh);
		v_copy(d_n_1,d_n);
		v_copy(v_n_1,v_n);
		v_copy(a_n_1,a_n);
		pre_n = pre_n_1;
		// update iteration counter
		n++	;
		printf("%i  \t  %lf %10.2E %lf %lf %lf \n",n,t_n,Wbal,pre_n_1,disp_rod,volume/1e3);




	}


	// /*////////////////////////////////////////////////////////// */
	// /*////////////////////////////////////////////////////////// */


	// gettimeofday(&endExp, NULL);

	// long elapsedExp = (endExp.tv_sec-endPre.tv_sec) + (endExp.tv_usec-endPre.tv_usec)/1000000.0;


	// printf("Post processing took %ld seconds \n Explicit routine took %ld seconds\n",elapsedPre,elapsedExp);




	// // /*////////////////////////////////////////////////////////// */
	// // /*			      POSTPROCESSING STAGE                       */
	// // /*                                                           */
	// // /*////////////////////////////////////////////////////////// */







	// /*////////////////////////////////////////////////////////// */
	// /*////////////////////////////////////////////////////////// */



	// /*  Program exit */
	exit(0);
}


static inline int heaviside(int x)
{

	if (x > 0){
		return 1;
	}else{
		return 0;
	}
}