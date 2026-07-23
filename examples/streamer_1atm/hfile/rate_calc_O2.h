void O2_rate(double Tt,double **kvt,double **re_kvt ,double **kvv0,double **re_kvv0
                                ,double *dEv,double **E){

double Xe=0.00758; /*anharmonicity of the molecule*/
double Ye=0.00758; /*anharmonicity of the molecule*/
double L= 0.16;    /*short-rangerepulsive potential*/
double u= 16.0;    /*reduced mass*/
double A= 1.35e-12;
double B= 137.9;
double C= 0;
double D= 1;

double DE=17.24078; /*VT‚Ì‡™E‚Ì’l(K)*/

	int n,m,s,v,w;
	double **y,**F,**Gs,*Zv,*Zw,*G,*gamma;
	double temp;

	int N=10;

	y  =mat(N+1,N+1),F  =mat(N+1,N+1),Gs =mat(N+1,N+1);
	Zv =vec(N+1),Zw =vec(N+1),G  =vec(N+1),gamma=vec(N+1);

	for(n=0;n<N;n++){
		Zv[n]=Zw[n]=G[n]=kvt[0][n]=re_kvt[0][n]=0.0;
		for(s=0;s<N;s++)kvv0[n][s]=re_kvv0[n][s]=y[n][s]=F[n][s]=Gs[n][s]=0.0;
	}

/////////////ŒW”y[w][v]‚ÌŽZo/////////////////////////////

	for(w=0;w<N;w++){
		for(v=0;v<N;v++){
			y[w][v+1]=0.32*fabs((E[0][v+1]-E[0][v])-(E[0][w+1]-E[0][w]))*L*sqrt(u*Tt)/Tt;
		}
	}
///////////////////////////////////////////////////////////

///////////ŒW”F[w][v]‚ÌŽZo/////////////////////

	for(w=0;w<N;w++){
		for(v=0;v<N;v++){
			if(y[w][v+1]<=20.0){
				temp = exp(-2.0*y[w][v+1]/3.0);
				F[w][v+1]=0.5*(3.0-temp)*temp;
			} else {
				temp = cbrt(y[w][v+1]);
				F[w][v+1]=8.0*sqrt(3.141592/3.0)*temp*temp*temp*temp*temp*temp*temp*exp(-3.0*temp*temp);
    			}
		}
	}

//////////////////////////////////////////////////

	for(v=0;v<N;v++)Zv[v+1]=(v+1)*(1-Xe)/(1-(v+1)*Xe); 
	for(w=0;w<N;w++)Zw[w+1]=(w+1)*(1-Xe)/(1-(w+1)*Xe);

///////////////Gs[w][v]‚ÌŽZo////////////////////////

	temp = 1.0/F[0][1];
	for(w=0;w<N;w++)for(v=0;v<N;v++)Gs[w+1][v+1]=Zv[v+1]*Zw[w+1]*F[w][v+1]*temp;

////////////////////////////////////////////////

////////////////////////////////////////////////

	kvv0[0][1]=2e-20*(Tt/300.0);

	for(w=0;w<=N;w++)for(v=0;v<N;v++)kvv0[w][v+1]=kvv0[0][1]*Gs[w+1][v+1];

	temp = 1.0/Tt;
	for(w=0;w<=N;w++)for(v=0;v<N;v++)re_kvv0[w][v+1]=kvv0[w][v+1]*exp(-((E[0][v+1]-E[0][v])+(E[0][w]-E[0][w+1]))*temp);

	//”ñ’²˜a«‚ðl—¶‚·‚éê‡
	if(1)for(w=0;w<N;w++)for(v=w+1;v<=N;v++)kvv0[w][v+1]=re_kvv0[w][v+1]=0.0;

//exit(0);
/*
for(w=0;w<8;w++){
  for(v=0;v<8;v++){
    printf("O2[%d] + O2[%d]-->O2[%d] + O2[%d]  %e\t%e\n",v+1,w,v,w+1,kvv0[w][v+1],re_kvv0[w][v+1]);
  }
printf("----------------------------------------------------------------------\n\n");
}
exit(0);
*/

////////////////////ŽŸ‚ÉVTrate‚àŒvŽZ‚·‚é////////////////////

	kvt[0][1]=A*Tt*exp(-B/cbrt(Tt)+C)/((1-D*exp(-dEv[1]/Tt)))*1e-6;

	for(v=0;v<N;v++)gamma[v]=0.32*dEv[v+1]*L*sqrt(u/Tt);

	for(v=0;v<N;v++){
		if(gamma[v]<20)G[v+1]=(v+1)*(1-Ye)/(1-(v+1)*Ye)*exp(v*4.0*DE*gamma[0]/3.0/dEv[1]);
		else G[v+1]=(v+1)*(1-Ye)/(1-(v+1)*Ye)*exp(v*4.0*DE*cbrt(gamma[0])*cbrt(gamma[0])/dEv[1]);
	}

	for(v=0;v<N;v++)kvt[0][v+1]=kvt[0][1]*G[v+1]; //³”½‰ž
	for(v=0;v<N;v++)re_kvt[0][v+1]=kvt[0][v+1]*exp(-(E[0][v+1]-E[0][v])/Tt);//‹t”½‰ž 

//for(v=0;v<8;v++)printf("%e\t%e\n",kvt[0][v+1],re_kvt[0][v+1]);
//exit(0);

	free_mat(y,N+1,N+1), free_mat(F,N+1,N+1),free_mat(Gs,N+1,N+1);
	free_vec(Zv,N+1),free_vec(Zw,N+1),free_vec(G  ,N+1),free_vec(gamma,N+1);
}
