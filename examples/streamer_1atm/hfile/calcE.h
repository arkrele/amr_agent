void calc_E(int NR,int NZ,double **phi,double **absE,double **Ey,double **Ex,
                double air_kg,int **totuflag,
                 int **otuflag,int **iflag,int **jflag,int **flag,
                     double a,double b,double *rhalf,double *zhalf,double **Mol,int num, int Prinstp){

	int i,j;
	double hzm,hrm,**mol;
	double Avo=  6.022141e+23;
	double **LEx;

	LEx = mat(NR,NZ);
	mol=mat(NR,NZ);

	for(i=0;i<NR;i++)for(j=0;j<NZ;j++)mol[i][j]=Mol[i][j]/(air_kg/Avo);//kg/m^3 → コ/m^3
	for(i=0;i<NR;i++)for(j=0;j<NZ;j++)LEx[i][j] = Ex[i][j];

	for(i=1;i<NR;i++){
		for(j=1;j<NZ;j++){
			if(jflag[i][j-1]==1){
				hzm=b*sqrt(1.0+pow(rhalf[i]/a,2)) - zhalf[j-1];

				Ex[i][j]=-1e+21*(phi[i][j]-phi[i-1][j])/((rhalf[i]-rhalf[i-1])*mol[i][j]);
				Ey[i][j]=-1e+21*(phi[i][j]-phi[i][j-1])/((hzm)*mol[i][j]);    //y方向中心差分

			}else if(iflag[i][j]==1 && j != NZ-1){
				hrm=rhalf[i] - a*sqrt(pow(zhalf[j]/b,2)-1.0);

				Ex[i][j]=-1e+21*(phi[i][j]-phi[i-1][j])/(hrm*mol[i][j]);
				Ey[i][j]=-1e+21*(phi[i][j]-phi[i][j-1])/((zhalf[j]-zhalf[j-1])*mol[i][j]);    //y方向中心差分

			}else if(otuflag[i][j]==1){
				hrm=rhalf[i] - a*sqrt(pow(zhalf[j]/b,2)-1.0);

				Ex[i][j]=-1e+21*(phi[i][j]-phi[i-1][j])/(hrm*mol[i][j]);
				Ey[i][j]=-1e+21*(phi[i][j]-phi[i][j-1])/((zhalf[j]-zhalf[j-1])*mol[i][j]);    //y方向中心差分

			} else { //それ以外の点は普通のポアソン方程式
				Ex[i][j]=-1e+21*(phi[i][j]-phi[i-1][j])/((rhalf[i]-rhalf[i-1])*mol[i][j]);    //x方向中心差分、Ex=-dφ/dx, Ex[i][j+1/2]
				Ey[i][j]=-1e+21*(phi[i][j]-phi[i][j-1])/((zhalf[j]-zhalf[j-1])*mol[i][j]);    //y方向中心差分              Ey[i+1/2][j]
			}     
     
			if(otuflag[i][j-1]){    //この点だけ特殊(flag=1の領域に値が入る)
				hzm=b*sqrt(1.0+pow(rhalf[i]/a,2)) - zhalf[j-1];

//          Ex[i][j]=-1e+21*(phi[i][j]-phi[i-1][j])/(hrm*Mol);
				Ey[i][j]=-1e+21*(phi[i][j]-phi[i][j-1])/((hzm)*mol[i][j]);    //y方向中心差分
			}
		}
	}

	i=0;
	for(j=1;j<NZ;j++){
		if(flag[i][j]==0)Ey[i][j]=-1e+21*(phi[i][j]-phi[i][j-1])/((zhalf[j]-zhalf[j-1])*mol[i][j]);

		if(jflag[i][j-1]==1){
			hzm=b*sqrt(1.0+pow(rhalf[i]/a,2)) - zhalf[j-1];

			Ey[i][j]=-1e+21*(phi[i][j]-phi[i][j-1])/((hzm)*mol[i][j]);   
		}

		Ex[i][j]=0.0;
	}


	j=0;
	for(i=1;i<NR;i++){
		Ex[i][j]=-1e+21*(phi[i][j]-phi[i-1][j])/((rhalf[i]-rhalf[i-1])*mol[i][j]);
		Ey[i][j]=-1e+21*(phi[i][j]-0.0        )/((zhalf[j]-0.0          )*mol[i][j]);
	}

	i=0,j=0;
	Ex[i][j]=0.0;
	Ey[i][j]=-1e+21*(phi[i][j]-0.0        )/((zhalf[j]-0.0          )*mol[i][j]);


double sigm;
double zmm;
for(i=0;i<NR;i++){
	for(j=0;j<NZ;j++){
		zmm = zhalf[j]*1000-12.7;
		sigm = (2.0*zmm/(1+fabs(2.0*zmm))+1.0)*0.5;
		Ex[i][j] = Ex[i][j] - 0.5*sigm*LEx[i][j];
	}
}

	// absEの算出

	double aEx, aEy, aExEy;
	double Ezlim = -30.0;	//ストリーマヘッドをとらえるためのもの

	for(i=0;i<NR-1;i++){
		for(j=0;j<NZ-1;j++){
				aExEy = sqrt(Ex[i][j]*Ex[i][j]+Ey[i][j]*Ey[i][j]);
			if(aExEy==0.0){
				absE[i][j] = 0.0;
			}else{
				aEx = (Ex[i][j] + Ex[i+1][j])*0.5;
				if(Ey[i][j+1]==0.0){
					aEy = Ey[i][j];
				}else{
					if(Ey[i][j]<Ey[i][j+1] + Ezlim && num < Prinstp + 10000){ // 1次ストリーマ到達+10000step目まで有効 // 1次ストリーマが到達したら通常の平均値をとる
						aEy = Ey[i][j];
					}else{
						aEy = (Ey[i][j] + Ey[i][j+1])*0.5;
					}
				}
				absE[i][j]=sqrt(aEx*aEx + aEy*aEy);
			}
			//if(i==0)printf("%d\t%d\t%e\t%e\n",i,j,aExEy,absE[i][j]);
		}
	}


	i=NR-1;
	for(j=0;j<NZ-1;j++){
		aEx = Ex[i][j];
		aEy = (Ey[i][j] + Ey[i][j+1])*0.5;
		absE[i][j]=sqrt(aEx*aEx + aEy*aEy);
	}

	j=NZ-1;
	for(i=0;i<NR-1;i++){
		aEx = (Ex[i][j] + Ex[i+1][j])*0.5;
		aEy = Ey[i][j];
		absE[i][j]=sqrt(aEx*aEx + aEy*aEy);
	}
	i=NR-1;j=NZ-1;
	aEx = Ex[i][j];
	aEy = Ey[i][j];
	absE[i][j]=sqrt(aEx*aEx + aEy*aEy);



	///////////////////////// (ブランチング対策) ////////////////)
/*
	if(num > Prinstp + 10000){ // 1次ストリーマ到達+10000step目以降有効

		double max, Elim;
		max=0.0;
		i=0;
		for(j=600;j<1421;j++){
			if(max<absE[i][j])Elim = absE[i][j] + 10.0;
		}
		if(Elim>200.0)Elim = 200.0;
		if(Elim<120.0)Elim = 120.0;

		double Eratio;
		for(i=0;i<NR-1;i++){
			for(j=600;j<NZ-1;j++){
				if(absE[i][j]>Elim){
					Eratio = Elim/absE[i][j];
					absE[i][j] = Elim;
					Ex[i][j] = Ex[i][j]*Eratio;
					Ey[i][j] = Ey[i][j]*Eratio;
				
				}
			}
		}
	}
*/
	////////////////////////////////////

	free_mat(mol,NR,NZ);
	free_mat(LEx,NR,NZ);

}
