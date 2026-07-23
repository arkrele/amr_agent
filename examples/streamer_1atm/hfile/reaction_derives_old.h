void reaction_derivs(int ii,int jj,double *y,double *yout){

	double k1,k2,k3,k4,k5,k6,k7,k8,k9,k10,k11,k12,k13,dpht,DA0,DA1,DA2,DA3,DA4;
	double N2,ne,N2p,N2C,Nb1,Natm,N2A,Na1,O2,O2p,O2m,Oatm,Om;
	double O2v1,O2v2,O2v3,O2v4,N2v1,N2v2,N2v3,N2v4,N2v5,N2v6,N2v7,N2v8;
	double dN2,de_dens,dN2p,dN2C,dNb1,dNatm,dna3,dNa1,dO2,dO2p,dO2m,dOatm,dOm;
	double dO2v1,dO2v2,dO2v3,dO2v4,dN2v1,dN2v2,dN2v3,dN2v4,dN2v5,dN2v6,dN2v7,dN2v8;
	double r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13;
	double rn2v1,rn2v2,rn2v3,rn2v4,rn2v5,rn2v6,rn2v7,rn2v8,rnat,rn2c,rn2pb,rn2b1,rna3,rn2bq;
	double ro2v1,ro2v2,ro2v3,ro2v4;
	double N2B,dN2B;
	double rN2meta1,rN2meta2;
	double DEM0,DEM1,DEM2,DEM3,DEM4,DEM5,DEM6,DEM7,DEM8;


	ne     = y[0];
	N2p    = y[1];
	O2p    = y[2];
	O2m    = y[3];
	Om     = y[4];
	N2     = y[5];
	N2C    = y[6];
	Nb1    = y[7];
	Natm   = y[8];
	N2A    = y[9];
	Na1    = y[10];
	N2v1   = y[11];
	N2v2   = y[12];
	N2v3   = y[13];
	N2v4   = y[14];
	N2v5   = y[15];
	N2v6   = y[16];
	N2v7   = y[17];
	N2v8   = y[18];
	N2B    = y[19];

	O2     = y[20];
	Oatm   = y[21];
	O2v1   = y[22];
	O2v2   = y[23];
	O2v3   = y[24];
	O2v4   = y[25];

	k1 = krate[13][ii][jj];
	k2 = krate[23][ii][jj];
	k3 = krate[16][ii][jj];	 //ko2at2;
	k4 = krate[15][ii][jj]*MOL*pO2*1e-6;
	if(TeV[ii][jj])k5=1.8e-13*exp(0.39*log(300.0/TeV[ii][jj])); //Mintoussov
	k6 =4.0e-18;
	k7 =4.0e-18;
	if(TeV[ii][jj])k8=1.95e-13*exp(0.7*log(300.0/TeV[ii][jj])); //Mintoussov
	k9 =4.0e-13;
	k10=1.6e-13;
	k11=9.6e-14;
	k12=4.2e-13;
	k13=( krate[21][ii][jj] + krate[22][ii][jj] );
	dpht=phot_num[ii][jj];

	r1  = k1*ne*N2;			// N2 + e     => N2p + 2e
	r2  = k2*ne*O2;			// O2 + e     => O2p + 2e
	r3  = k3*ne*O2;			// O2 + e     => Om  + O
	r4  = k4*ne*O2;			// O2 + O2+ e => O2m + O2
	r5  = k5*ne*N2p;		// N2p+ e     => N   + N
	r6  = k6*ne*N2p;		// N2p+ e     => N2
	r7  = k7*ne*O2p;		// O2p+ e     => O2
	r8  = k8*ne*O2p;		// O2p+ e     => O   + O
	r9  = k9*Om*N2p;		// N2p+ Om    => N2  + O
	r10 = k10*O2m*N2p;		// N2p+ O2m   => N2  + O2
	r11 = k11*Om*O2p;		// O2p+ Om    => O2  + O
	r12 = k12*O2p*O2m;		// O2p+ O2m   => 2O2
	r13 = k13*ne*O2;		// O2 + e     => 2O + e

	rN2meta1 = 3.0e-16*N2A*N2A;	// N2(A) + N2(A) ¨ N2 + N2(B) 
	rN2meta2 = 1.5e-16*N2A*N2A;	// N2(A) + N2(A) ¨ N2 + N2(C) 

	rn2v1 = N2*ne*krate[0][ii][jj];
	rn2v2 = N2*ne*krate[1][ii][jj];
	rn2v3 = N2*ne*krate[2][ii][jj];
	rn2v4 = N2*ne*krate[3][ii][jj];	// Vibrational excitation
	rn2v5 = N2*ne*krate[4][ii][jj];	// N2 + e => N2(v) + e
	rn2v6 = N2*ne*krate[5][ii][jj];
	rn2v7 = N2*ne*krate[6][ii][jj];
	rn2v8 = N2*ne*krate[7][ii][jj];

	rna3  = N2*ne*krate[8][ii][jj];		// N2 + e => N2(A) + e
	rn2b1 = N2*ne*krate[9][ii][jj];		// N2 + e => N2(B) + e
	rn2c  = N2*ne*krate[11][ii][jj];	// N2 + e => N2(C) + e
	rnat  = N2*ne*krate[12][ii][jj];		// N2 + e => 2N + e
	rn2pb = N2*ne*krate[14][ii][jj];		// N2 + e => N2+(B) + e

	ro2v1 = O2*ne*krate[17][ii][jj];
	ro2v2 = O2*ne*krate[18][ii][jj];
	ro2v3 = O2*ne*krate[19][ii][jj];	//Vibrational excitation
	ro2v4 = O2*ne*krate[20][ii][jj];	// O2 + e => O2(v) + e

	DA0 = krate[24][ii][jj]*ne*O2;	//Dissociative attachment
	DA1 = krate[25][ii][jj]*ne*O2v1;	// O2(v) + e => O + O^-
	DA2 = krate[26][ii][jj]*ne*O2v2;
	DA3 = krate[27][ii][jj]*ne*O2v3;
	DA4 = krate[28][ii][jj]*ne*O2v4;

	DEM0 = krate[29][ii][jj]*ne*N2;
	DEM1 = krate[30][ii][jj]*ne*N2v1;
	DEM2 = krate[31][ii][jj]*ne*N2v2;
	DEM3 = krate[32][ii][jj]*ne*N2v3;
	DEM4 = krate[33][ii][jj]*ne*N2v4;
	DEM5 = krate[34][ii][jj]*ne*N2v5;
	DEM6 = krate[35][ii][jj]*ne*N2v6;
	DEM7 = krate[36][ii][jj]*ne*N2v7;
	DEM8 = krate[37][ii][jj]*ne*N2v8;


	rn2bq = 0.95*3e-17*N2*Nb1;	// N2(B) + N2 => N2(A) + N2 

	de_dens =  dt*(r1 + r2 - (DA0 + DA1 + DA2 + DA3 + DA4) - r4 - r5 - r6 - r7 - r8 + dpht );

/////////////////////////////////////////////////////////////////////////////////////////////////////

	dN2     = dt*(- rn2v1 - rn2v2 - rn2v3 - rn2v4 - rn2v5 - rn2v6 - rn2v7 - rn2v8
			 - DEM0 - rnat - r1 + r6 + r9 + r10 + rN2meta1 + rN2meta2
					-rn2c - rn2pb - rn2b1 - rna3);


	dN2p    =  dt*(r1 - r5 - r6 - r9 -r10);
	dN2C    =  -dt*N2C*(2.38e7 + 3.0e-16*O2+ 0.13e-16*N2) + dt*(rn2c + rN2meta2);
	dN2B    =  -dt*N2B*(1.6e7 + 5.1e-16*O2+ 2.1e-16*N2) + dt*rn2pb;
	dNb1    =  dt*(rn2b1 + rN2meta1 - rn2bq);
	dNatm   =  dt*( 2*(DEM0+DEM1+DEM2+DEM3+DEM4+DEM5+DEM6+DEM7+DEM8) + 2.0*rnat + 2.0*r5);
	dna3    =  dt*(rna3 - 2.0*rN2meta1 - 2.0*rN2meta2 + rn2bq);
	dNa1    =  0.0;//dt*kna1[ii][jj]*N2*ne;
 
	dN2v1	=  dt*(rn2v1 - DEM1);
	dN2v2	=  dt*(rn2v2 - DEM2);
	dN2v3	=  dt*(rn2v3 - DEM3);
	dN2v4	=  dt*(rn2v4 - DEM4);
	dN2v5	=  dt*(rn2v5 - DEM5);
	dN2v6	=  dt*(rn2v6 - DEM6);
	dN2v7	=  dt*(rn2v7 - DEM7);
	dN2v8	=  dt*(rn2v8 - DEM8);

	dN2 = - dt*(rn2v1 + rn2v2 + rn2v3 + rn2v4 + rn2v5 + rn2v6 + rn2v7 + rn2v8);  

/////////////////////////////////////////////////////////////////////////////////////////////////////

	dO2     =  dt*( -ro2v1 - ro2v2 - ro2v3 - ro2v4 -r2 - DA3  - r4 + r7 + r10 + r11 +2.0*r12 - r13 - dpht );

	dO2p    =  dt*(  r2 - r7  - r8 - r11 - r12 + dpht);
	dO2m    =  dt*(  r4 - r10 - r12);
	dOm     =  dt*(  DA0 + DA1 + DA2 + DA3 + DA4 - r9  - r11);
	dOatm   =  dt*(  DA0 + DA1 + DA2 + DA3 + DA4 + 2.0*r8 + r9 + r11 + 2.0*r13);

	dO2v1   =  dt*(  ro2v1 - DA1);
	dO2v2   =  dt*(  ro2v2 - DA2);
	dO2v3   =  dt*(  ro2v3 - DA3);
	dO2v4   =  dt*(  ro2v4 - DA4);
	dO2	=  dO2 - dO2v1 - dO2v2 - dO2v3 - dO2v4;

//////////////////////////////////////////////////////////////////////////////////////////////////

        yout[0]	= de_dens;
        yout[1]	= dN2p;
        yout[2]	= dO2p;
        yout[3]	= dO2m;
        yout[4]	= dOm;
        yout[5]	= dN2;
        yout[6]	= dN2C;
        yout[7]	= dNb1;
        yout[8]	= dNatm;
        yout[9]	= dna3;
        yout[10]= 0.0;//dNa1;

        yout[11]= dN2v1;
        yout[12]= dN2v2;
        yout[13]= dN2v3;
        yout[14]= dN2v4;
        yout[15]= dN2v5;
        yout[16]= dN2v6;
        yout[17]= dN2v7;
        yout[18]= dN2v8;
	yout[19]= dN2B;

        yout[20]= dO2;
        yout[21]= dOatm;

        yout[22]= dO2v1;
        yout[23]= dO2v2;
        yout[24]= dO2v3;
        yout[25]= dO2v4;

}
