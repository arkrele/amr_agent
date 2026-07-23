void Ozone_rate(double T,double *c1,double *c2,double *c3,double **kvt,double **re_kvt,double **E){


	double **a,**b;
	double temp, temp1;

	int i,j,v;

	a =mat(20,20);
	b =mat(20,20);

	for(i=0;i<10;i++)for(j=0;j<10;j++)b[i][j]=a[i][j]=0.0;
	for(i=0;i<10;i++)kvt[5][i]=re_kvt[5][i]=0.0;

	for(j=1;j<=5;j++)b[1][j]=c1[j];
	for(j=1;j<=5;j++)b[2][j]=c2[j];
	for(j=1;j<=5;j++)b[3][j]=c3[j];

	for(v=1;v<=10;v++){
		temp = (double)v;
		for(i=1;i<=3;i++){
			a[v][i]=b[i][1]+b[i][2]*log(temp)+(b[i][3]+b[i][4]*temp+b[i][5]*temp*temp)/(1e21+exp(temp));
		}
	}

	temp = (1.0/27.0)*1e-6;
	temp1= log(T);
	for(v=0;v<10;v++){
		kvt[5][v+1]=temp*exp(a[v+1][1]+a[v+1][2]/(temp1)+a[v+1][3]*temp1);
	}
//Billing uses DegF factor as 1.0/(3.0*(5.0+3.0*exp(-227.6/T)+exp(-325.9/T)))

//for(v=1;v<20;v++)printf("%e\n",kvt[5][v]);
//exit(0);

	temp = 1.0/T;
	for(v=0;v<10;v++)re_kvt[5][v+1]=kvt[5][v+1]*exp(-(E[0][v+1]-E[0][v])*temp); 

//for(v=0;v<10;v++)printf("%e\n",kvt[5][v]);
//exit(0);

	free_mat(a,20,20),free_mat(b,20,20);

}
