void Helmholtz(int NR,int NZ,int *A,double *gamma,double pO2,double **pphi,
			int **flag,int **iflag,int **jflag,int **otuflag,double *rhalf,double **rho,
                		double **P1,double **P2,double **P3,double **P4,double **P5,int pnum){//,double **P1,double **P2,double **P3,double **P4){

double Conv  = 1.0e-5;    /* 収束と判定する前回ループとの差 1.0e-6で十分   */
double OMEGA=1.9;   //SOR法の緩和係数

int i,j,loop;
double MaxPhi;            /* 最大電位                         */
double MaxErr;
double Prev_phi;
double CurErr;
double **gsphi;

gsphi=mat(NR,NZ);



    /* 繰り返し計算 */
  loop   = 0;         /* 別に無くても良いのだが，目安としてループをカウントする．              */
  MaxPhi = 1.0e-10;   /* 系内の最大の電位を入れる変数．ある有限の値を入れておく(ゼロ割り防止)．*/

  do {
		MaxErr = CurErr = 0.0;

		for (i = 0; i < NR; i++) {      /* 領域端を除く全ての点をループ                */
			for (j = 0; j < NZ; j++) {  /*                                             */

				Prev_phi = pphi[i][j];    // 前回ループのphiをPrev_phiにいれておいて，   

				if(flag[i][j]);
				else{

					//回転軸上(i=0)の点について
					if(i == 0){
						gsphi[i][j] = P1[i][j]*(-(pow(gamma[pnum]*pO2,2)*gsphi[i][j] - rho[i][j]*pow(pO2,2)*A[pnum])*0.5 
									- P2[i][j]*pphi[i][j+1] - P4[i][j]*pphi[i+1][j] - P5[i][j]*pphi[i][j-1]);

					if(j==0)gsphi[i][j] = P1[i][j]*(-(pow(gamma[pnum]*pO2,2)*gsphi[i][j] - rho[i][j]*pow(pO2,2)*A[pnum])*0.5 
									- P2[i][j]*pphi[i][j+1] - P4[i][j]*pphi[i+1][j] - P5[i][j]*0.0); //軸上の平板点について,平板電位0
					if(jflag[i][j])gsphi[i][j] = P1[i][j]*(-(pow(gamma[pnum]*pO2,2)*gsphi[i][j] - rho[i][j]*pow(pO2,2)*A[pnum])*0.5 
									- P2[i][j]*pphi[i][j] - P4[i][j]*pphi[i+1][j] - P5[i][j]*pphi[i][j-1]);

					} else if( j==0){  //平板の電位を考慮

						gsphi[i][j] = P1[i][j]*(-(pow(gamma[pnum]*pO2,2)*gsphi[i][j] - rho[i][j]*pow(pO2,2)*A[pnum])*rhalf[i] 
									- P2[i][j]*pphi[i][j+1] - P3[i][j]*pphi[i-1][j] - P4[i][j]*pphi[i+1][j] - P5[i][j]*0.0); 

					} else if( i==NR-1){  //右端
						gsphi[i][j]=gsphi[i-1][j];

					}else if( j == NZ - 1  ){  //針元の扱い

						gsphi[i][j]=gsphi[i][j-1];//なにか良い方法はないか。

					}else if(jflag[i][j]){ 
						gsphi[i][j] = P1[i][j]*(-(pow(gamma[pnum]*pO2,2)*gsphi[i][j] - rho[i][j]*pow(pO2,2)*A[pnum])*rhalf[i] 
									- P2[i][j]*pphi[i][j] - P3[i][j]*pphi[i-1][j] - P4[i][j]*pphi[i+1][j] - P5[i][j]*pphi[i][j-1]);           
					}else if(iflag[i][j]){ 
						gsphi[i][j] = P1[i][j]*(-(pow(gamma[pnum]*pO2,2)*gsphi[i][j] - rho[i][j]*pow(pO2,2)*A[pnum])*rhalf[i] 
									- P2[i][j]*pphi[i][j+1] - P3[i][j]*pphi[i][j] - P4[i][j]*pphi[i+1][j] - P5[i][j]*pphi[i][j-1]);           

					}else if(otuflag[i][j]){ 
						gsphi[i][j] = P1[i][j]*(-(pow(gamma[pnum]*pO2,2)*gsphi[i][j] - rho[i][j]*pow(pO2,2)*A[pnum])*rhalf[i] 
									- P2[i][j]*pphi[i][j] - P3[i][j]*pphi[i][j] - P4[i][j]*pphi[i+1][j] - P5[i][j]*pphi[i][j-1]);           


					}else{ //それ以外の点は普通のポアソン方程式

						gsphi[i][j] = P1[i][j]*(-(pow(gamma[pnum]*pO2,2)*gsphi[i][j] - rho[i][j]*pow(pO2,2)*A[pnum])*rhalf[i] 
									- P2[i][j]*pphi[i][j+1] - P3[i][j]*pphi[i-1][j] - P4[i][j]*pphi[i+1][j] - P5[i][j]*pphi[i][j-1]);           

					}
				}            

				pphi[i][j]=pphi[i][j]+OMEGA*(gsphi[i][j]-pphi[i][j]);   //SOR法、緩和係数OMEGAを用いる。

				if (MaxPhi < fabs(pphi[i][j]))MaxPhi = pphi[i][j];// 電位最大が更新されたらMaxPhiを書き換え      
				CurErr = (fabs(pphi[i][j] - Prev_phi));// 前回ループと新しい答えの差を，MaxPhiで規格化

				if (MaxErr < CurErr) MaxErr = CurErr;// 誤差の最大を常にMaxErrに持つようにする                      
			}
		}
  loop++;
if(loop%100==0)printf("%d\t%e\t%e\n",loop,MaxErr/MaxPhi,MaxPhi);
  } while (MaxErr/MaxPhi>Conv);               /* 領域全ての点の誤差がConvを下回ったらおしまい*/


printf("---Finish_Helmholtz_equation---No.%d--",pnum);
printf("%05d  %e\n", loop, MaxPhi);

free_mat(gsphi,NR,NZ);
}
