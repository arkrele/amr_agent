void H2O_N2_rate(double T, double **kvv4,double **re_kvv4,double *TN2TH2O,
                          double *kN2H2O,double *nkN2H2O,double **E){

	int i,j;
	double kkout;

	for(i=0;i<15;i++)for(j=0;j<15;j++)kvv4[i][j]=re_kvv4[i][j]=0.0;

	splint(TN2TH2O,kN2H2O,nkN2H2O,4,T,&kkout);
	kvv4[0][1]=kkout*1e-6;                                                          //³”½‰ž

	re_kvv4[0][1]=kvv4[0][1]*exp(-((E[1][1]-E[1][0])+(E[3][0]-E[3][1]))/T);    //‹t”½‰ž

}
