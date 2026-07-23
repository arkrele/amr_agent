void H2O_O2_rate(double T, double **kvt,double **re_kvt,
                      double **kvv3,double **re_kvv3,double *dEv,double **E){

	double Xe=0.00758; /*anharmonicity of the molecule*/
	double L =0.18; /*short-rangerepulsive potential*/
	double uu=18.0;/*reduced mass of H2O-H2O */
	double A =3.21e-8;
	double B =228.2;
	double C =930.5;

	double DE=17.24; //O2Ç∆ìØÇ∂Ç…ÇµÇƒÇ¢ÇÈÅB

	int v,w;
	double *G,*gamma;
	double inv_T,inv_cbrtT;

	int N=10;

	G  =vec(N+1),gamma=vec(N+1);

	for(v=0;v<N+1;v++){  /*èâä˙âª*/
		G[v]=kvt[3][v]=re_kvt[3][v]=0.0;
		for(w=0;w<N;w++)kvv3[v][w]=re_kvv3[v][w]=0.0;
	}

	inv_T = 1.0/T;
	inv_cbrtT = 1.0/cbrt(T);
///////////////////O2-H2OÇÃVVrate////////////////////

	kvv3[0][1]=5.5e-19*sqrt(T/300.0); //O2-H2OÇÃVVîΩâûåWêî

	w=0, v=1;
	re_kvv3[w][v]=kvv3[w][v]*exp(-((E[0][v]-E[0][v-1])+(E[3][w]-E[3][w+1]))*inv_T);//è⁄ç◊íﬁçáÇÊÇËãtîΩâûåWêî

////////////////////éüÇ…H2O-H2OÇÃVTrateÇ‡åvéZÇ∑ÇÈ////////////////////

	kvt[3][1]=A*300.0*exp(-B*inv_cbrtT+C*inv_cbrtT*inv_cbrtT)*1e-6;

	for(v=0;v<N;v++)gamma[v]=0.32*dEv[v+1]*L*sqrt(uu*inv_T);

	for(v=0;v<N;v++){
		if(gamma[v]<20)G[v+1]=(v+1)*(1-Xe)/(1-(v+1)*Xe)*exp(v*4.0*DE*gamma[0]/3.0/dEv[1]);
		else G[v+1]=(v+1)*(1-Xe)/(1-(v+1)*Xe)*exp(v*4.0*DE*cbrt(gamma[0])*cbrt(gamma[0])/dEv[1]);
	}

	for(v=0;v<N;v++)kvt[3][v+1]=kvt[3][1]*G[v+1];//ê≥îΩâû
	for(v=0;v<N;v++)re_kvt[3][v+1]=kvt[3][v+1]*exp(-(E[3][v+1]-E[3][v])*inv_T);//ãtîΩâû 

	free_vec(G,N+1);
	free_vec(gamma,N+1);
}
