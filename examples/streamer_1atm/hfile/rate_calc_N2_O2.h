void N2_O2_rate(double T, double **kvt,double **re_kvt, double **kvv2,
                     double **re_kvv2,double *dEv, double *ddEv,double **E){

double Xe=0.00758;     /*anharmonicity of the molecule*/   /*O2,‰»Šw•Ö——‚æ‚è*/ 
double Ye=0.006073171; /*anharmonicity of the molecule*/   /*N2,‰»Šw•Ö——‚æ‚è*/
double LL=0.18;                                            /*VVrate‚ÉŽg—p*/
double u= 14.93333;  /*reduced mass*/  /*N2=28,O2=36*/

double DE=17.24;   /*‰»Šw•Ö——‚ÌO2‚Ìwexe‚Ì’l‚ðƒPƒ‹ƒrƒ“‚É’¼‚µ‚½‚à‚Ì*/

	int n,m,s,v,w,Tvv;
	double **y,**F,**Gs,*Zv,*Zw;
	double temp1, temp2, temp3;

	int N=10;

	y   =mat(N+1,N+1),F   =mat(N+1,N+1),Gs  =mat(N+1,N+1),Zv =vec(N+1),Zw =vec(N+1);

	for(n=0;n<=N-1;n++){ //‰Šú‰»
		Zv [n]=Zw [n]=kvt[2][n]=re_kvt[2][n]=0.0;
		for(s=0;s<=N-1;s++){
			kvv2[n][s]=re_kvv2[n][s]=y[n][s]=F[n][s]=Gs[n][s]=0.0;
		}
	}

/////////////ŒW”y[w][v]‚ÌŽZo/////////////////////////////

	temp1 = 0.32*LL*sqrt(u*T)/T;
	for(w=0;w<N;w++){
		for(v=0;v<N;v++){
			y[w][v+1]=fabs(ddEv[v+1]-dEv[w+1])*temp1;  /*dEv=O2‚Ì‡™v=1‚ÌƒGƒlƒ‹ƒM[AddEv[]=N2‚Ì‡™v=1‚ÌƒGƒl*/
		}
	}

///////////////////////////////////////////////////////////

///////////ŒW”F[w][v]‚ÌŽZo/////////////////////

	for(w=0;w<N;w++){
		for(v=0;v<N;v++){
			if(y[w][v+1]<=20.0){
				temp1 = exp(-2.0*y[w][v+1]/3.0);
				F[w][v+1]=0.5*(3.0-temp1)*temp1;
			}else{
				temp1 = cbrt(y[w][v+1]);
				F[w][v+1]=8.0*sqrt(3.141592/3.0)*temp1*temp1*temp1*temp1*temp1*temp1*temp1*exp(-3.0*temp1*temp1); 
			}
		}
	}



//////////////////////////////////////////////////

	for(v=0;v<N;v++)Zv[v+1]=(v+1)*(1-Ye)/(1-(v+1)*Ye); 
	for(w=0;w<N;w++)Zw[w+1]=(w+1)*(1-Xe)/(1-(w+1)*Xe);

///////////////Gs[w][v]‚ÌŽZo////////////////////////

	temp1 = 1.0/F[0][1];
	for(w=0;w<N;w++)for(v=0;v<N;v++)Gs[w+1][v+1]=Zv[v+1]*Zw[w+1]*F[w][v+1]*temp1;

////////////////////////////////////////////////

////////////////////////////////////////////////

	kvv2[0][1]=3.69e-12*T/300.0*exp(-104.0/cbrt(T))*1e-6;

	for(w=0;w<=N;w++){
		for(v=0;v<N;v++){
			kvv2[w][v+1]=kvv2[0][1]*Gs[w+1][v+1];  //³”½‰ž
		}
	}

	temp1 = 1.0/T;
	for(w=0;w<=N;w++){
		for(v=0;v<N;v++){
			re_kvv2[w][v+1]=kvv2[w][v+1]*exp(-((E[1][v+1]-E[1][v])+(E[0][w]-E[0][w+1]))*temp1); //‹t”½‰ž
		}
	}


//////////ŽŸ‚ÉVTrate(O2(v)+N2(0)¨O2(v-1)+N2(0))‚àŒvŽZ‚·‚é//////////////

	for(v=0;v<N;v++)kvt[2][v+1]=kvt[0][v+1]; //O2-O2‚Ìkvt‚Æ“¯‚¶B

	for(v=0;v<N;v++)re_kvt[2][v+1]=kvt[2][v+1]*exp(-(E[0][v+1]-E[0][v])*temp1); //‹t”½‰ž‚ÍÚ×’Þ‡

//////////ŽŸ‚ÉVTrate(N2(v)+O2(0)¨N2(v-1)+O2(0))‚àŒvŽZ‚·‚é//////////////

	for(v=0;v<N;v++)kvt[4][v+1]=kvt[1][v+1]; //N2-N2‚Ìkvt‚Æ“¯‚¶B

	for(v=0;v<N;v++)re_kvt[4][v+1]=kvt[4][v+1]*exp(-(E[1][v+1]-E[1][v])*temp1); //‹t”½‰ž‚ÍÚ×’Þ‡

	free_mat(y,N+1,N+1),free_mat(F ,N+1,N+1),free_mat(Gs ,N+1,N+1);
	free_vec(Zv,N+1), free_vec(Zw,N+1);

}
