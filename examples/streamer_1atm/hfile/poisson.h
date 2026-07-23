void discretization(int N,int M,double *rhalf,double *zhalf,
			double **P1,double **P2,double **P3,double **P4,double **P5,
				double a,double b,int **iflag,int **jflag,int **otuflag,int **flag){

  int i,j;
  double **h1,**h2,**h3,**h4;

  h1=mat(N,M),h2=mat(N,M),h3=mat(N,M),h4=mat(N,M);

  for(i=0;i<N;i++){
	for(j=0;j<M;j++){
		if(i==0){
			if(j==M-1)h1[i][j] = h1[i][j-1];
			else h1[i][j] = zhalf[j+1]-zhalf[j];

			h2[i][j] = rhalf[i];
			h3[i][j] = rhalf[i+1]-rhalf[i];

			if(j==0)h4[i][j]=zhalf[j];
			else h4[i][j] = zhalf[j]-zhalf[j-1];

      		}else if(j==0){

			h1[i][j] = zhalf[j+1]-zhalf[j];
			h2[i][j] = rhalf[i]-rhalf[i-1];

			if(i==N-1)h3[i][j] = h3[i-1][j];
			else h3[i][j] = rhalf[i+1]-rhalf[i];

			h4[i][j] = zhalf[j];
      		}else if(j==M-1){
			
			h1[i][j] = h1[i][j-1];  //上
			h2[i][j] = rhalf[i]-rhalf[i-1];  //左

			if(i==N-1)h3[i][j]=h3[i-1][j];
			else h3[i][j] = rhalf[i+1]-rhalf[i];  //右

			h4[i][j] = zhalf[j]-zhalf[j-1];  //下

      		}else if(i==N-1){
			
			if(j==M-1)h1[i][j] = h1[i][j-1];
			else h1[i][j] = zhalf[j+1]-zhalf[j];  //上

			h2[i][j] = rhalf[i]-rhalf[i-1];  //左
			h3[i][j] = h3[i-1][j];  //右
			h4[i][j] = zhalf[j]-zhalf[j-1];  //下
		}else{
			h1[i][j] = zhalf[j+1]-zhalf[j];  //上
			h2[i][j] = rhalf[i]-rhalf[i-1];  //左
			h3[i][j] = rhalf[i+1]-rhalf[i];  //右
			h4[i][j] = zhalf[j]-zhalf[j-1];  //下
		}
	}
  }

double hh1,hh2,hh3,hh4,r0_double;

		for (i = 0; i < N; i++) {      // poissonを解くための準備 
			for (j = 0; j < M; j++) { 
				if (flag[i][j]) {
					P1[i][j]=P2[i][j]=P3[i][j]=P4[i][j] = 0.0;
				}else{

					if(otuflag[i][j]==1 || jflag[i][j]==1)hh1 = b*sqrt(1.0+pow(rhalf[i]/a,2)) - zhalf[j];
					else hh1 = h1[i][j];

					if(iflag[i][j]==1 || otuflag[i][j]==1)hh2 = rhalf[i] - a*sqrt(pow(zhalf[j]/b,2)-1.0);
					else hh2 = h2[i][j];

					hh3 = h3[i][j];

					if(j==0)hh4 = zhalf[j];
					else hh4 = h4[i][j];

					r0_double =   2.0*rhalf[i];

					if(i==0){
						P1[i][j] = (hh1*hh4*hh3*hh3)/(hh3*hh3+2.0*hh1*hh4);
//						P1[i][j] =  1.0/(hh1*hh4) + 2.0/(hh3*hh3);
						P2[i][j] = -1.0/(hh1*(hh1+hh4));
						P3[i][j] = 0.0;
						P4[i][j] = -2.0/(hh3*hh3);
						P5[i][j] = -1.0/(hh4*(hh1+hh4));
					}else{
						P1[i][j] = hh1*hh2*hh3*hh4/(r0_double*hh2*hh3 + (r0_double +hh2-hh3)*hh1*hh4);
//						P1[i][j] =    r0_double/(hh1*hh4) + (r0_double+hh2-hh3)/(hh2*hh3);
						P2[i][j] =   -r0_double/(hh1*(hh1+hh4));
						P3[i][j] =  (-r0_double+hh3)/(hh2*(hh2+hh3));
						P4[i][j] =  -(r0_double+hh2)/(hh3*(hh2+hh3));
						P5[i][j] =  -r0_double/(hh4*(hh1+hh4));
					}

				}            
			}
		}


  free_mat(h1,N,M),free_mat(h2,N,M),free_mat(h3,N,M),free_mat(h4,N,M);

}

void poiseq(int num_x,int num_y,double **pphi,int **flag,
               int **iflag,int **jflag,int **otuflag,double *rhalf,double *zhalf,double **rho,
                double **P1,double **P2,double **P3,double **P4,double **P5){//,double **P1,double **P2,double **P3,double **P4){

double Conv  = 1.0e-5;    /* 収束と判定する前回ループとの差 1.0e-6で十分   */
double OMEGA=1.9;   //SOR法の緩和係数

int i,j,loop;
double MaxPhi;            /* 最大電位                         */
double MaxErr;
double Prev_phi;
double CurErr;
double **gsphi;

gsphi=mat(num_x,num_y);

    /* 繰り返し計算 */
  loop   = 0;         /* 別に無くても良いのだが，目安としてループをカウントする．              */
  MaxPhi = 1.0e-20;   /* 系内の最大の電位を入れる変数．ある有限の値を入れておく(ゼロ割り防止)．*/

  do {
//	if(loop!=0 && (loop%1000==0)) printf("%05d  %e  (%d,%d)\n", loop, MaxPhi,memoi,memoj); /* 10000ループ毎に経過表示 */
		MaxErr = CurErr = 0.0;

		for (i = 0; i < num_x; i++) {      /* 領域端を除く全ての点をループ                */
			for (j = 0; j < num_y; j++) {  /*                                             */

				Prev_phi = pphi[i][j];    // 前回ループのphiをPrev_phiにいれておいて，   

				if (flag[i][j]) {
					gsphi[i][j] = 0.0;
				}else{


					//回転軸上(i=0)の点について
					if(i == 0){

						gsphi[i][j] = P1[i][j]*(rho[i][j] - P2[i][j]*pphi[i][j+1] - P4[i][j]*pphi[i+1][j] - P5[i][j]*pphi[i][j-1]);
					if(j==0)gsphi[i][j] = P1[i][j]*(rho[i][j] - P2[i][j]*pphi[i][j+1] - P4[i][j]*pphi[i+1][j] - P5[i][j]*0.0); //軸上の平板点について,平板電位0

					} else if( j==0){  //平板の電位を考慮

						gsphi[i][j] = P1[i][j]*(rho[i][j] - P2[i][j]*pphi[i][j+1] - P3[i][j]*pphi[i-1][j] - P4[i][j]*pphi[i+1][j] - P5[i][j]*0.0); 

					} else if( i == num_x-1){  //右端

						gsphi[i][j]=gsphi[i-1][j];

					} else if( j == num_y - 1  ){  //針元の扱い

						gsphi[i][j]=gsphi[i][j-1];//なにか良い方法はないか。

					} else { //それ以外の点は普通のポアソン方程式

						gsphi[i][j] = P1[i][j]*(rho[i][j] - P2[i][j]*pphi[i][j+1] - P3[i][j]*pphi[i-1][j] - P4[i][j]*pphi[i+1][j] - P5[i][j]*pphi[i][j-1]);           

					}
				}

				pphi[i][j]=pphi[i][j]+OMEGA*(gsphi[i][j]-pphi[i][j]);   //SOR法、緩和係数OMEGAを用いる。

				if (MaxPhi < fabs(pphi[i][j]))MaxPhi = fabs(pphi[i][j]);// 電位最大が更新されたらMaxPhiを書き換え      

				CurErr = (fabs(pphi[i][j] - Prev_phi));// 前回ループと新しい答えの差を，MaxPhiで規格化

				if (MaxErr < CurErr) MaxErr = CurErr;// 誤差の最大を常にMaxErrに持つようにする                      

			}
		}
  loop++;
if(loop%100==0)printf("%d\t%e\t%e\n",loop,MaxErr/MaxPhi,MaxPhi);
  } while (MaxErr/MaxPhi>Conv);               /* 領域全ての点の誤差がConvを下回ったらおしまい*/

printf("---Finish_poisson_equation---");
printf("%05d  %e\n", loop, MaxPhi);

free_mat(gsphi,num_x,num_y);
}

