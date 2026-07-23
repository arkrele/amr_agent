void N2_rate(double T,double **kvt,double **re_kvt,double **kvv1,double **re_kvv1,
                      double *dEv,double **E){

	double Xe=0.0060732; /*anharmonicity of the molecule*/
	double Ye=0.0060732; /*anharmonicity of the molecule*/
	double L =0.16;      /*short-rangerepulsive potential*/
	double u =14.0;      /*reduced mass*/
	double A =7.8e-12;
	double B =218.0;
	double C =690.0;
	double D =1.0;

	double Avogadro=6.022141e+23;   		//ÉAÉ{ÉKÉhÉçíËêî ÉR/mol
	double massN2 = 28.0*1e-3/Avogadro; 		// kg/ÉR
	double N2CS = 4.536e-19;  			//N2-N2 cross section N2CS = pi*É–^2 = 3.14*(3.8e-10)^2
	double VMcoeff = 4.0*sqrt(kb/(M_PI*massN2));	//Vm=sqrt(8kbT/(pi*u))=4*sqrt(kbT/(pi*m))=VMcoeff*sqrt(T)
	double N2Z = N2CS*VMcoeff;
	double Lst=0.185e-10;
	double CC=270.0;
	double aT,bT;
	double temp;

	int N=10;

	double DE=20.61;   /*(K)*/

	int v,w;
	double **y,**F,**Gs,*Zv,*Zw,*G,*gamma;

	y  =mat(N+1,N+1),F=mat(N+1,N+1),Gs =mat(N+1,N+1);
	Zv =vec(N+1),Zw =vec(N+1),G  =vec(N+1),gamma=vec(N+1);

	for(v=0;v<N;v++){
		Zv[v]=Zw[v]=G[v]=kvt[1][v]=re_kvt[1][v]=0.0;
		for(w=0;w<N;w++)kvv1[v][w]=re_kvv1[v][w]=y[v][w]=F[v][w]=Gs[v][w]=0.0;
	}


//Plasma sources Sci.Technol.(2010) 045015
	for(w=0;w<N;w++){
		for(v=0;v<N;v++){
			y[w][v+1]=Lst*pow(M_PI,2.0)
				*fabs((E[1][v+1]-E[1][v])-(E[1][w+1]-E[1][w]))*sqrt(2.0*0.5*massN2*kb/(pow(h,2.0)*T));
		}
	}

///////////////////////////////////////////////////////////

///////////åWêîF[w][v]ÇÃéZèo/////////////////////
//ã≥â»èëÇ‡ò_ï∂Ç‡ìØÇ∂
	for(w=0;w<N;w++){
		for(v=0;v<N;v++){
			if(y[w][v+1]<=21.6){
				temp = exp(-2.0*y[w][v+1]/3.0);
				F[w][v+1]=0.5*(3.0-temp)*temp;
			}else{
				temp = cbrt(y[w][v+1]);
				F[w][v+1]=8.0*sqrt(3.141592/3.0)*temp*temp*temp*temp*temp*temp*temp*exp(-3.0*temp*temp);
			}
		}
	}

//////////////////////////////////////////////////

//ò_ï∂
	for(v=0;v<=N;v++){
		Zv[v]=(double)v/(1.0-v*Xe); 
		Zw[v]=(double)v/(1.0-v*Xe);
	}

///////////////Gs[w][v]ÇÃéZèo////////////////////////

	aT = 12.2e-8;
	bT = 9.1e-3;

	for(w=0;w<=N;w++){
		for(v=0;v<N;v++){
			kvv1[w][v+1]=1.0e-6*0.2*1e6*N2Z*sqrt(T)*Zv[v+1]*Zw[w+1]
				*exp((fabs((E[1][v+1]-E[1][v])-(E[1][w+1]-E[1][w])))/(2.0*T))
				*(aT*T*F[w][v+1] + bT/T*exp(-pow(fabs((E[1][v+1]-E[1][v])-(E[1][w+1]-E[1][w])),2.0))/(CC*T));
		}
	}

////////////////////////////////////////////////

	temp = 1.0/T;
	for(w=0;w<=N;w++){
		for(v=0;v<N;v++){
			re_kvv1[w][v+1]=kvv1[w][v+1]*exp(-((E[1][v+1]-E[1][v])+(E[1][w]-E[1][w+1]))*temp);
		}
	}

	if(1){  //îÒí≤òaê´Ççló∂Ç∑ÇÈèÍçá
		for(w=0;w<N;w++){
			for(v=w+1;v<=N;v++){
				kvv1[w][v+1]=re_kvv1[w][v+1]=0.0;
			}
		}
	}




////////////////////éüÇ…VTrateÇ‡åvéZÇ∑ÇÈ////////////////////

	temp= 1.0/T;
	kvt[1][1]=A*T*exp(-B/cbrt(T)+C*temp)/((1-D*exp(-dEv[1]*temp)))*1e-6;

	for(v=0;v<N;v++)gamma[v]=0.32*dEv[v+1]*L*sqrt(u*temp);

	temp = cbrt(gamma[0]);
	for(v=0;v<N;v++){
		if(gamma[v]<20)G[v+1]=(v+1)*(1-Ye)/(1-(v+1)*Ye)*exp(v*4.0*DE*gamma[0]/3.0/dEv[1]);
		else G[v+1]=(v+1)*(1-Ye)/(1-(v+1)*Ye)*exp(v*4.0*DE*temp*temp/dEv[1]);
	}

	//ê≥îΩâû
	for(v=0;v<N;v++)kvt[1][v+1]=kvt[1][1]*G[v+1];   

	temp= 1.0/T;
	for(v=0;v<N;v++)re_kvt[1][v+1]=kvt[1][v+1]*exp(-(E[1][v+1]-E[1][v])*temp);   //ãtîΩâû

//for(v=0;v<8;v++)printf("%e\t%e\n",kvt[1][v+1],re_kvt[1][v+1]);
//exit(0);
	free_mat(y,N+1,N+1),free_mat(F,N+1,N+1),free_mat(Gs,N+1,N+1);
	free_vec(Zv,N+1),free_vec(Zw,N+1),free_vec(G,N+1),free_vec(gamma,N+1);
}
