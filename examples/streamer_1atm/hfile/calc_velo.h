
void calc_e_velo(int num_x,int num_y,double **Ex,double **Ey,double **velox,double **veloy){

	int i,j;
	double A1,B1,A2,B2,log_10;

	A1 = 5.5236702;
	B1 = 0.7822439;

	A2 = 5.8692884;
	B2 = 0.4375671;

	log_10=log(10.0);


	for(i=0;i<num_x;i++){
		for(j=0;j<num_y;j++){

/*
//BOLSIGのデータを用いる場合
        	splint(TdE,v_elec1,v_elec2,boltNum,fabs(Ex[i][j]),&v_elec);
        	if(Ex[i][j]<0){
        		velox[i][j]= v_elec;//*Ex[i][j]*1e-21;   //velo_eに電子速度格納、単位は[m/s]
        	}else{
        		velox[i][j]= -v_elec;
	        }

        	splint(TdE,v_elec1,v_elec2,boltNum,fabs(Ey[i][j]),&v_elec);

		if(Ey[i][j]<0){
			veloy[i][j] = v_elec;//*Ey[i][j]*1e-21;   //velo_eに電子速度格納、単位は[m/s]
		} else {
			veloy[i][j] = -v_elec;//*Ey[i][j]*1e-21;   //velo_eに電子速度格納、単位は[m/s]
		}
*/

//Ducasse:2007 IEEE trans.vol35,no5,pp1287　のデータ
			if(fabs(Ex[i][j]) > 9.8){
				if(Ex[i][j]<0)velox[i][j] =  0.01*exp((A1+B1*log10(fabs(Ex[i][j])))*log_10);
				else          velox[i][j] = -0.01*exp((A1+B1*log10(fabs(Ex[i][j])))*log_10);
			}else{
				if(Ex[i][j]<0)velox[i][j] =  0.01*exp((A2+B2*log10(fabs(Ex[i][j])))*log_10);   //velo_eに電子速度格納、単位は[m/s]
				else          velox[i][j] = -0.01*exp((A2+B2*log10(fabs(Ex[i][j])))*log_10); //velo_eに電子速度格納、単位は[m/s]
			}

			if(fabs(Ey[i][j]) > 9.8){
				if(Ey[i][j]<0)veloy[i][j] =  0.01*exp((A1+B1*log10(fabs(Ey[i][j])))*log_10);   //velo_eに電子速度格納、単位は[m/s]
				else          veloy[i][j] = -0.01*exp((A1+B1*log10(fabs(Ey[i][j])))*log_10); //velo_eに電子速度格納、単位は[m/s]
			}else{
				if(Ey[i][j]<0)veloy[i][j] =  0.01*exp((A2+B2*log10(fabs(Ey[i][j])))*log_10);   //velo_eに電子速度格納、単位は[m/s]
				else          veloy[i][j] = -0.01*exp((A2+B2*log10(fabs(Ey[i][j])))*log_10);
			}

/*
//Morrow 1997 J.Phys.D ,vol.30,614-627　のデータ
			if(fabs(Ex[i][j]) > 200.0){
				if(Ex[i][j]<0)velox[i][j] =  (7.4e4*fabs(Ex[i][j]) + 7.1e6)*0.01 ;
				else          velox[i][j] = -(7.4e4*fabs(Ex[i][j]) + 7.1e6)*0.01 ;
			}else  if(fabs(Ex[i][j]) > 10.0){
				if(Ex[i][j]<0)velox[i][j] =  (1.03e5*fabs(Ex[i][j]) + 1.3e6)*0.01 ;   //velo_eに電子速度格納、単位は[m/s]
				else          velox[i][j] = -(1.03e5*fabs(Ex[i][j]) + 1.3e6)*0.01 ; //velo_eに電子速度格納、単位は[m/s]
			}else  if(fabs(Ex[i][j]) > 2.6){
				if(Ex[i][j]<0)velox[i][j] =  (7.2973e4*fabs(Ex[i][j]) + 1.63e6)*0.01 ;   //velo_eに電子速度格納、単位は[m/s]
				else          velox[i][j] = -(7.2973e4*fabs(Ex[i][j]) + 1.63e6)*0.01 ; //velo_eに電子速度格納、単位は[m/s]
			}else{
				if(Ex[i][j]<0)velox[i][j] =  (6.87e5*fabs(Ex[i][j]) + 3.38e4)*0.01 ;   //velo_eに電子速度格納、単位は[m/s]
				else          velox[i][j] = -(6.87e5*fabs(Ex[i][j]) + 3.38e4)*0.01 ; //velo_eに電子速度格納、単位は[m/s]
			}

			if(fabs(Ey[i][j]) > 200.0){
				if(Ey[i][j]<0)veloy[i][j] =  (7.4e4*fabs(Ey[i][j]) + 7.1e6)*0.01 ;
				else          veloy[i][j] = -(7.4e4*fabs(Ey[i][j]) + 7.1e6)*0.01 ;
			}else  if(fabs(Ey[i][j]) > 10.0){
				if(Ey[i][j]<0)veloy[i][j] =  (1.03e5*fabs(Ey[i][j]) + 1.3e6)*0.01 ;   //velo_eに電子速度格納、単位は[m/s]
				else          veloy[i][j] = -(1.03e5*fabs(Ey[i][j]) + 1.3e6)*0.01 ; //velo_eに電子速度格納、単位は[m/s]
			}else  if(fabs(Ey[i][j]) > 2.6){
				if(Ey[i][j]<0)veloy[i][j] =  (7.2973e4*fabs(Ey[i][j]) + 1.63e6)*0.01 ;   //velo_eに電子速度格納、単位は[m/s]
				else          veloy[i][j] = -(7.2973e4*fabs(Ey[i][j]) + 1.63e6)*0.01 ; //velo_eに電子速度格納、単位は[m/s]
			}else{
				if(Ey[i][j]<0)veloy[i][j] =  (6.87e5*fabs(Ey[i][j]) + 3.38e4)*0.01 ;   //velo_eに電子速度格納、単位は[m/s]
				else          veloy[i][j] = -(6.87e5*fabs(Ey[i][j]) + 3.38e4)*0.01 ; //velo_eに電子速度格納、単位は[m/s]
			}
*/

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

				vx[i][j] = keisu*Ex[i][j];
				vy[i][j] = keisu*Ey[i][j];   //Tochikubo et al J.Appl.Phys,Vol41(2002)、単位は[m/s]

			}
		}
	}
	if(mp==1){  //-イオンだったら
		for(i=0;i<num_x;i++){
			for(j=0;j<num_y;j++){

				vx[i][j] = -keisu*Ex[i][j];
				vy[i][j] = -keisu*Ey[i][j];   //Tochikubo et al J.Appl.Phys,Vol41(2002)、単位は[m/s]

			}
		}
	}

}

