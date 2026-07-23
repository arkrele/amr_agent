void multi_poiseq(int NR,int NZ,double **pphi,int **flag,double **rho,
                double **P1,double **P2,double **P3,double **P4,double **P5
			,int iter,int mf,double *res){

double OMEGA=1.9;   //SOR法の緩和係数

int i,j,loop;
double MaxPhi,MaxErr,Prev_phi,CurErr,**gsphi;

gsphi=mat(NR,NZ);

    /* 繰り返し計算 */
  loop   = 0;         /* 別に無くても良いのだが，目安としてループをカウントする．              */
  MaxPhi = 1.0e-20;   /* 系内の最大の電位を入れる変数．ある有限の値を入れておく(ゼロ割り防止)．*/

for(loop=0;loop<iter;loop++){
		MaxErr = CurErr = 0.0;

		for (i = 0; i < NR; i++) {      /* 領域端を除く全ての点をループ                */
			for (j = 0; j < NZ; j++) {  /*                                             */

				Prev_phi = pphi[i][j];    // 前回ループのphiをPrev_phiにいれておいて，   

				if (flag[i][j]) {
					gsphi[i][j] = 0.0;
				}else{


					//回転軸上(i=0)の点について
					if(i == 0){

						gsphi[i][j] = P1[i][j]*(rho[i][j] 
										- P2[i][j]*pphi[i][j+1] 
											- P4[i][j]*pphi[i+1][j] 
												- P5[i][j]*pphi[i][j-1]);
					if(j==0)gsphi[i][j] = P1[i][j]*(rho[i][j] 
										- P2[i][j]*pphi[i][j+1] 
											- P4[i][j]*pphi[i+1][j] 
												- P5[i][j]*0.0); //軸上の平板点について,平板電位0

					} else if( j==0){  //平板の電位を考慮

						gsphi[i][j] = P1[i][j]*(rho[i][j] 
										- P2[i][j]*pphi[i][j+1] 
											- P3[i][j]*pphi[i-1][j] 
												- P4[i][j]*pphi[i+1][j] 
													- P5[i][j]*0.0); 

					} else if( i == NR-1){  //右端

						gsphi[i][j]=gsphi[i-1][j];

					} else if( j == NZ - 1  ){  //針元の扱い

						gsphi[i][j]=gsphi[i][j-1];//なにか良い方法はないか。

					} else { //それ以外の点は普通のポアソン方程式

						gsphi[i][j] = P1[i][j]*(rho[i][j] 
										- P2[i][j]*pphi[i][j+1] 
											- P3[i][j]*pphi[i-1][j] 
												- P4[i][j]*pphi[i+1][j] 
													- P5[i][j]*pphi[i][j-1]);           

					}
				}

				pphi[i][j]=pphi[i][j]+OMEGA*(gsphi[i][j]-pphi[i][j]);   //SOR法、緩和係数OMEGAを用いる。

				if (MaxPhi < fabs(pphi[i][j]))MaxPhi = fabs(pphi[i][j]);// 電位最大が更新されたらMaxPhiを書き換え      

				CurErr = (fabs(pphi[i][j] - Prev_phi));// 前回ループと新しい答えの差を，MaxPhiで規格化

				if (MaxErr < CurErr) MaxErr = CurErr;// 誤差の最大を常にMaxErrに持つようにする                      

			}
		}
}

	if(mf==0)(*res)=MaxErr/MaxPhi;
free_mat(gsphi,NR,NZ);
}

void Error(int NR,int NZ,double **pphi,int **flag,double **rho,
                double **P1,double **P2,double **P3,double **P4,double **P5,double **gsphi){

	int i,j,ijb,ijbb;
	double left;

		for (i = 0; i < NR; i++) {      /* 領域端を除く全ての点をループ                */
			for (j = 0; j < NZ; j++) {  /*                                             */
				if (flag[i][j]) {
					gsphi[i][j]=0.0;
				}else{
					//回転軸上(i=0)の点について
					if(i == 0){
						if(j==0)left = pphi[i][j]/P1[i][j] 
									+ P2[i][j]*pphi[i][j+1] 
										+ P4[i][j]*pphi[i+1][j] 
											+ P5[i][j]*0.0; //軸上の平板点について,平板電位0
						else left = pphi[i][j]/P1[i][j] 
									+ P2[i][j]*pphi[i][j+1] 
										+ P4[i][j]*pphi[i+1][j] 
											+ P5[i][j]*pphi[i][j-1];

					} else if( j==0){  //平板の電位を考慮
						
						if(i == NR-1){
							ijb = i-1;
							left = pphi[ijb][j]/P1[ijb][j] 
									+ P2[ijb][j]*pphi[ijb][j+1] 
										+ P3[ijb][j]*pphi[ijb-1][j] 
											+ P4[ijb][j]*pphi[ijb+1][j] 
												+ P5[ijb][j]*pphi[ijb][j-1];
						}else left = pphi[i][j]/P1[i][j] 
									+ P2[i][j]*pphi[i][j+1] 
										+ P3[i][j]*pphi[i-1][j] 
											+ P4[i][j]*pphi[i+1][j] 
												+ P5[i][j]*0.0; 
						
					} else if( i == NR-1){  //右端
						if(j == NZ - 1){
							ijbb = j-1;
							ijb = i-1;
							left = pphi[ijb][ijbb]/P1[ijb][ijbb] 
									+ P2[ijb][ijbb]*pphi[ijb][ijbb+1] 
										+ P3[ijb][ijbb]*pphi[ijb-1][ijbb] 
											+ P4[ijb][ijbb]*pphi[ijb+1][ijbb] 
												+ P5[ijb][ijbb]*pphi[ijb][ijbb-1];
						}else{
							ijb = i-1;
							left = pphi[ijb][j]/P1[ijb][j] 
									+ P2[ijb][j]*pphi[ijb][j+1] 
										+ P3[ijb][j]*pphi[ijb-1][j] 
											+ P4[ijb][j]*pphi[ijb+1][j] 
												+ P5[ijb][j]*pphi[ijb][j-1];
						}
					} else if( j == NZ - 1  ){  //針元の扱い
						ijb = j-1;
						left = pphi[i][ijb]/P1[i][ijb] 
									+ P2[i][ijb]*pphi[i][ijb+1] 
										+ P3[i][ijb]*pphi[i-1][ijb] 
											+ P4[i][ijb]*pphi[i+1][ijb] 
												+ P5[i][ijb]*pphi[i][ijb-1];

					} else { //それ以外の点は普通のポアソン方程式

						left = pphi[i][j]/P1[i][j] 
									+ P2[i][j]*pphi[i][j+1] 
										+ P3[i][j]*pphi[i-1][j] 
											+ P4[i][j]*pphi[i+1][j] 
												+ P5[i][j]*pphi[i][j-1];           

					}
						gsphi[i][j]=rho[i][j] - left;
				}
			}
		}

}

void Interpolation(int NR, int NZ, int **flag, double **Cphi, double **Cphi2){

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

void Restruction(int NR2,int NZ2,double **Cres,double **Cres2){

	int i,j;

	for(i=0;i<NR2;i++)for(j=0;j<NZ2;j++)Cres2[i][j] = Cres[i*2][j*2];
}

