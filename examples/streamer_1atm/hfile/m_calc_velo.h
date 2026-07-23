void calc_e_velo(int num_x,int num_y,double **absE, double **Ex,double **Ey,double **velox,double **veloy, double **ne, double *rh, double *zh,
			double *TdE, double *v_elec1, double *v_elec2, int boltNum){

	int i,j,k;
	double A1,B1,A2,B2,log_10,Nm,coeff,temp,EE,D, v_elec;
	double mEx, mEy, NN;


	A1 = 5.5236702;
	B1 = 0.7822439;

	A2 = 5.8692884;
	B2 = 0.4375671;

	log_10=log(10.0);

	Nm = ((6.02e+23*1000.0)/(0.0820578*300))*1e-6;
	NN = ((6.02e+23*1000.0)/(0.0820578*300));
	coeff = 3.74e22/Nm;


	for(i=0;i<num_x;i++){
		for(j=0;j<num_y;j++){

//BOLSIGのデータを用いる場合
	        	splint(TdE,v_elec1,v_elec2,boltNum,fabs(absE[i][j]),&v_elec);  //v_elec = mu * N
	        	if(Ex[i][j]<0){
	        		velox[i][j]= v_elec*fabs(Ex[i][j])*1e-21;   //velo_e
	        	}else{
	        		velox[i][j]= -v_elec*fabs(Ex[i][j])*1e-21;
		        }

	        	splint(TdE,v_elec1,v_elec2,boltNum,fabs(absE[i][j]),&v_elec);

			if(Ey[i][j]<0){
				veloy[i][j] =  v_elec*fabs(Ey[i][j])*1e-21;   //velo_eに電子速度格納、単位は[m/s]
			} else {
				veloy[i][j] = -v_elec*fabs(Ey[i][j])*1e-21;   //velo_eに電子速度格納、単位は[m/s]
			}

		}
	}

}

void calc_ie_velo(int num_x,int num_y,double **vx,double **vy,double **Ex,double **Ey,int mp){

	int i,j;
	double MOL=(6.02e+23*1000.0)/(0.0820578*300.0);
	double keisu=2.2e-4*MOL*1e-21;

	if(mp==0){  //+イオンだったら
		for(i=0;i<num_x;i++){
			for(j=0;j<num_y;j++){

				vx[i][j] = 0.0;//keisu*Ex[i][j];
				vy[i][j] = 0.0;//keisu*Ey[i][j];   //Tochikubo et al J.Appl.Phys,Vol41(2002)、単位は[m/s]

			}
		}
	}
	if(mp==1){  //-イオンだったら
		for(i=0;i<num_x;i++){
			for(j=0;j<num_y;j++){

				vx[i][j] = -0.0;//keisu*Ex[i][j];
				vy[i][j] = -0.0;//keisu*Ey[i][j];   //Tochikubo et al J.Appl.Phys,Vol41(2002)、単位は[m/s]

			}
		}
	}

}

