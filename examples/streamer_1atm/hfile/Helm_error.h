void multi_Helmholtz(int NR,int NZ,int *A,double *gamma,double pO2,double **pphi,
			int **flag,int **iflag,int **jflag,int **otuflag,double *rh,double **rho,
		                double **P1,double **P2,double **P3,double **P4,double **P5
					,int pnum,int iter,int set,double OMEGA){

int i,j,ijb,loop;
double MaxPhi;            /* 最大電位                         */
double MaxErr;
double Prev_phi;
double CurErr;
double a,b,c,d,e,f,new_field;



    /* 繰り返し計算 */
  MaxPhi = 1.0e-30;   /* 系内の最大の電位を入れる変数．ある有限の値を入れておく(ゼロ割り防止)．*/


	for(loop=0;loop<iter;loop++){	
		MaxErr = CurErr = 0.0;

		for (i = 0; i < NR; i++) {      /* 領域端を除く全ての点をループ                */
			for (j = 0; j < NZ; j++) {  /*                                             */

				Prev_phi = pphi[i][j];    // 前回ループのphiをPrev_phiにいれておいて，   

				if(flag[i][j]);
				else{

					if(i==0){
						b = 0.5;
						d = P3[i][j]*0.0;
						e = P4[i][j]*pphi[i+1][j];
	
						if(j==0){
							c = P2[i][j]*pphi[i][j+1];
							f = P5[i][j]*0.0;
						} else if(jflag[i][j]){
							c = P2[i][j]*pphi[i][j];
							f = P5[i][j]*pphi[i][j-1];
						} else {
							c = P2[i][j]*pphi[i][j+1];
							f = P5[i][j]*pphi[i][j-1];
						}
					} else if(j==0){
						b = rh[i];
						c = P2[i][j]*pphi[i][j+1];
						d = P3[i][j]*pphi[i-1][j];

						if(i==NR-1)e = P4[i][j]*pphi[i][j];
						else e = P4[i][j]*pphi[i+1][j];

						f = P5[i][j]*pphi[i][j];

					} else if( i==NR-1){  //右端
						ijb=i-1;

						b = rh[ijb];

						if(j==NZ-1)c = P2[ijb][j]*pphi[ijb][j];
						else c = P2[ijb][j]*pphi[ijb][j+1];

						d = P3[ijb][j]*pphi[ijb-1][j];
						e = P4[ijb][j]*pphi[ijb+1][j];
						f = P5[ijb][j]*pphi[ijb][j-1];

					}else if( j == NZ - 1  ){  //針元の扱い
						ijb=j-1;

						b = rh[i];
						c = P2[i][ijb]*pphi[i][ijb+1];
						d = P3[i][ijb]*pphi[i-1][ijb];
						e = P4[i][ijb]*pphi[i+1][ijb];
						f = P5[i][ijb]*pphi[i][ijb-1];

					}else if(jflag[i][j]){ 
						b = rh[i];
						c = P2[i][j]*pphi[i][j];
						d = P3[i][j]*pphi[i-1][j];
						e = P4[i][j]*pphi[i+1][j];
						f = P5[i][j]*pphi[i][j-1];

					}else if(iflag[i][j]){ 
						b = rh[i];
						c = P2[i][j]*pphi[i][j+1];
						d = P3[i][j]*pphi[i][j];
						e = P4[i][j]*pphi[i+1][j];
						f = P5[i][j]*pphi[i][j-1];
					}else if(otuflag[i][j]){ 
						b = rh[i];
						c = P2[i][j]*pphi[i][j];
						d = P3[i][j]*pphi[i][j];
						e = P4[i][j]*pphi[i+1][j];
						f = P5[i][j]*pphi[i][j-1];
					}else{ //それ以外の点は普通のポアソン方程式
						b = rh[i];
						c = P2[i][j]*pphi[i][j+1];
						d = P3[i][j]*pphi[i-1][j];
						e = P4[i][j]*pphi[i+1][j];
						f = P5[i][j]*pphi[i][j-1];

					}
				}
				a = -(pow(gamma[pnum]*pO2,2)*pphi[i][j] - rho[i][j]*pow(pO2,2)*A[pnum]);

				new_field = P1[i][j] * ( a*b - c - d - e - f );

			  	pphi[i][j] = (1.0 - OMEGA)*Prev_phi + OMEGA*new_field;

				if (MaxPhi < fabs(pphi[i][j]))MaxPhi = pphi[i][j];// 電位最大が更新されたらMaxPhiを書き換え      
				CurErr = (fabs(pphi[i][j] - Prev_phi));// 前回ループと新しい答えの差を，MaxPhiで規格化

				if (MaxErr < CurErr) MaxErr = CurErr;// 誤差の最大を常にMaxErrに持つようにする                      
			}
		}
//printf("%d\n",loop);
	}
 
//printf("---Finish_Helmholtz_equation---No.%d--",pnum);
//if(set==1)printf("%05d  %e\n", loop, MaxErr/MaxPhi);
if(set==1)printf("%05d  %e\n", loop, MaxErr/MaxPhi);
}

void multi_Helmholtz2(int NR,int NZ,int *A,double *gamma,double pO2,double **pphi,
			int **flag,int **iflag,int **jflag,int **otuflag,double *rh,double **rho,
		                double **P1,double **P2,double **P3,double **P4,double **P5
					,int pnum,int iter,int set ,double OMEGA){

int i,j,ijb,loop;
double MaxPhi;            /* 最大電位                         */
double MaxErr;
double Prev_phi;
double CurErr;
double **gsphi;
double a,b,c,d,e,f,new_field;

gsphi=mat(NR,NZ);



    /* 繰り返し計算 */
  MaxPhi = 1.0e-30;   /* 系内の最大の電位を入れる変数．ある有限の値を入れておく(ゼロ割り防止)．*/


	for(loop=0;loop<iter;loop++){	
		MaxErr = CurErr = 0.0;

		for (i = 0; i < NR; i++) {      /* 領域端を除く全ての点をループ                */
			for (j = 0; j < NZ; j++) {  /*                                             */

				Prev_phi = pphi[i][j];    // 前回ループのphiをPrev_phiにいれておいて，   

				if(flag[i][j]);
				else{

					if(i==0){
						b = 0.5;
						d = P3[i][j]*0.0;
						e = P4[i][j]*gsphi[i+1][j];
	
						if(j==0){
							c = P2[i][j]*gsphi[i][j+1];
							f = P5[i][j]*0.0;
						} else if(jflag[i][j]){
							c = P2[i][j]*gsphi[i][j];
							f = P5[i][j]*gsphi[i][j-1];
						} else {
							c = P2[i][j]*gsphi[i][j+1];
							f = P5[i][j]*gsphi[i][j-1];
						}
					} else if(j==0){
						b = rh[i];
						c = P2[i][j]*gsphi[i][j+1];
						d = P3[i][j]*gsphi[i-1][j];

						if(i==NR-1)e = P4[i][j]*gsphi[i][j];
						else e = P4[i][j]*gsphi[i+1][j];

						f = P5[i][j]*gsphi[i][j];

					} else if( i==NR-1){  //右端
						ijb=i-1;

						b = rh[ijb];

						if(j==NZ-1)c = P2[ijb][j]*gsphi[ijb][j];
						else c = P2[ijb][j]*gsphi[ijb][j+1];

						d = P3[ijb][j]*gsphi[ijb-1][j];
						e = P4[ijb][j]*gsphi[ijb+1][j];
						f = P5[ijb][j]*gsphi[ijb][j-1];

					}else if( j == NZ - 1  ){  //針元の扱い
						ijb=j-1;

						b = rh[i];
						c = P2[i][ijb]*gsphi[i][ijb+1];
						d = P3[i][ijb]*gsphi[i-1][ijb];
						e = P4[i][ijb]*gsphi[i+1][ijb];
						f = P5[i][ijb]*gsphi[i][ijb-1];

					}else if(jflag[i][j]){ 
						b = rh[i];
						c = P2[i][j]*gsphi[i][j];
						d = P3[i][j]*gsphi[i-1][j];
						e = P4[i][j]*gsphi[i+1][j];
						f = P5[i][j]*gsphi[i][j-1];

					}else if(iflag[i][j]){ 
						b = rh[i];
						c = P2[i][j]*gsphi[i][j+1];
						d = P3[i][j]*gsphi[i][j];
						e = P4[i][j]*gsphi[i+1][j];
						f = P5[i][j]*gsphi[i][j-1];
					}else if(otuflag[i][j]){ 
						b = rh[i];
						c = P2[i][j]*gsphi[i][j];
						d = P3[i][j]*gsphi[i][j];
						e = P4[i][j]*gsphi[i+1][j];
						f = P5[i][j]*gsphi[i][j-1];
					}else{ //それ以外の点は普通のポアソン方程式
						b = rh[i];
						c = P2[i][j]*gsphi[i][j+1];
						d = P3[i][j]*gsphi[i-1][j];
						e = P4[i][j]*gsphi[i+1][j];
						f = P5[i][j]*gsphi[i][j-1];

					}
				}
//				a = -(pow(gamma[pnum]*pO2,2)*pphi[i][j] - rho[i][j]*pow(pO2,2)*A[pnum]);
				a = rho[i][j];

				new_field = P1[i][j] * ( a - c - d - e - f );

			  	pphi[i][j] = (1.0 - OMEGA)*Prev_phi + OMEGA*new_field;

				if (MaxPhi < fabs(pphi[i][j]))MaxPhi = pphi[i][j];// 電位最大が更新されたらMaxPhiを書き換え      
				CurErr = (fabs(pphi[i][j] - Prev_phi));// 前回ループと新しい答えの差を，MaxPhiで規格化

				if (MaxErr < CurErr) MaxErr = CurErr;// 誤差の最大を常にMaxErrに持つようにする                      
			}
		}
//printf("a%d\n",loop);
	}
 
//printf("---Finish_Helmholtz_equation---No.%d--",pnum);
//if(set==1)printf("%05d  %e\n", loop, MaxErr/MaxPhi);
if(set==1)printf("%05d  %e\n", loop, MaxErr/MaxPhi);

free_mat(gsphi,NR,NZ);
}


void Helm_Error(int NR,int NZ,int *A,double *gamma,double pO2,double **pphi,int **flag,int **iflag,int **jflag,int **otuflag,double *rh,double **rho,
                double **P1,double **P2,double **P3,double **P4,double **P5,double **gsphi,int pnum){//,double **P1,double **P2,double **P3,double **P4){

int i,j,ijb,ijbb;
double left;
double a,b,c,d,e,f;



		for (i = 0; i < NR; i++) {      /* 領域端を除く全ての点をループ                */
			for (j = 0; j < NZ; j++) {  /*                                             */
				if(flag[i][j])gsphi[i][j]=0.0;
				else{
					if(i==0){
						b = 0.5;
						d = P3[i][j]*0.0;
						e = P4[i][j]*pphi[i+1][j];
	
						if(j==0){
							c = P2[i][j]*pphi[i][j+1];
							f = P5[i][j]*0.0;
						} else if(jflag[i][j]){
							c = P2[i][j]*pphi[i][j];
							f = P5[i][j]*pphi[i][j-1];
						} else {
							c = P2[i][j]*pphi[i][j+1];
							f = P5[i][j]*pphi[i][j-1];
						}
					} else if(j==0){
						b = rh[i];
						c = P2[i][j]*pphi[i][j+1];
						d = P3[i][j]*pphi[i-1][j];

						if(i==NR-1)e = P4[i][j]*pphi[i][j];
						else e = P4[i][j]*pphi[i+1][j];

						f = P5[i][j]*pphi[i][j];

					} else if( i==NR-1){  //右端
						ijb=i-1;

						b = rh[ijb];

						if(j==NZ-1)c = P2[ijb][j]*pphi[ijb][j];
						else c = P2[ijb][j]*pphi[ijb][j+1];

						d = P3[ijb][j]*pphi[ijb-1][j];
						e = P4[ijb][j]*pphi[ijb+1][j];
						f = P5[ijb][j]*pphi[ijb][j-1];

					}else if( j == NZ - 1  ){  //針元の扱い
						ijb=j-1;

						b = rh[i];
						c = P2[i][ijb]*pphi[i][ijb+1];
						d = P3[i][ijb]*pphi[i-1][ijb];
						e = P4[i][ijb]*pphi[i+1][ijb];
						f = P5[i][ijb]*pphi[i][ijb-1];

					}else if(jflag[i][j]){ 
						b = rh[i];
						c = P2[i][j]*pphi[i][j];
						d = P3[i][j]*pphi[i-1][j];
						e = P4[i][j]*pphi[i+1][j];
						f = P5[i][j]*pphi[i][j-1];

					}else if(iflag[i][j]){ 
						b = rh[i];
						c = P2[i][j]*pphi[i][j+1];
						d = P3[i][j]*pphi[i][j];
						e = P4[i][j]*pphi[i+1][j];
						f = P5[i][j]*pphi[i][j-1];
					}else if(otuflag[i][j]){ 
						b = rh[i];
						c = P2[i][j]*pphi[i][j];
						d = P3[i][j]*pphi[i][j];
						e = P4[i][j]*pphi[i+1][j];
						f = P5[i][j]*pphi[i][j-1];
					}else{ //それ以外の点は普通のポアソン方程式
						b = rh[i];
						c = P2[i][j]*pphi[i][j+1];
						d = P3[i][j]*pphi[i-1][j];
						e = P4[i][j]*pphi[i+1][j];
						f = P5[i][j]*pphi[i][j-1];

					}

					a = -(pow(gamma[pnum]*pO2,2)*pphi[i][j] - rho[i][j]*pow(pO2,2)*A[pnum]);

					left = pphi[i][j]/P1[i][j] + c + d + e + f;
					gsphi[i][j] = a*b -left;
				}
			}
		}


}

void Helm_Interpolation(int NR, int NZ, int **flag, double **Cphi, double **Cphi2){

	int i,j,ijb;

	//Cphi2 を fine-grid に補間
	for(i=0;i<NR;i++){
		for(j=0;j<NZ;j++){
			if(flag[i][j])Cphi[i][j]=0.0;
			else{
				if(i==0){
					if(j%2==0)Cphi[i][j] += Cphi2[0][j/2];
					if(j%2!=0)Cphi[i][j] += (Cphi2[0][(j-1)/2] + Cphi2[0][(j+1)/2])/2.0;		
					
				}else if(j==0){
					if(i%2==0 )Cphi[i][j] += Cphi2[i/2][0];
					if(i%2!=0 )Cphi[i][j] += (Cphi2[(i-1)/2][0] + Cphi2[(i+1)/2][0])/2.0;
				}else if(i==NR-1)Cphi[i][j]=Cphi[i-1][j];
				 else if(j==NZ-1)Cphi[i][j]=Cphi[i][j-1];
				 else{
					if(i%2==0 && j%2==0)Cphi[i][j] += Cphi2[i/2][j/2];
					if(i%2!=0 && j%2==0)Cphi[i][j] += (Cphi2[(i-1)/2][j/2] + Cphi2[(i+1)/2][j/2])/2.0;
					if(i%2==0 && j%2!=0)Cphi[i][j] += (Cphi2[i/2][(j-1)/2] + Cphi2[i/2][(j+1)/2])/2.0;			
					if(i%2!=0 && j%2!=0)Cphi[i][j] += (Cphi2[(i-1)/2][(j-1)/2] 
										+ Cphi2[(i+1)/2][(j-1)/2]
											+Cphi2[(i-1)/2][(j+1)/2] 
												+ Cphi2[(i+1)/2][(j+1)/2])/4.0;
				}
			}
		}
	}
}
