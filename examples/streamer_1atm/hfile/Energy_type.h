

void convert_M(double *O2,double *N2,double *H2O,double *M){

	int i;
	int Q=8;

	for(i=0;i<=Q;i++){
		M[i]=O2[i];
		M[i+(Q+1)]=N2[i];
		M[i+2*(Q+1)]=H2O[i];
	}
}

////////
void convert_ONH2O(double *O2,double *N2,double *H2O,double *M){

	int i;
	int Q=8;
  
	for(i=0;i<=Q;i++){
		O2[i]=M[i];
		N2[i]=M[i+(Q+1)];
		H2O[i]=M[i+2*(Q+1)];
	}
}

void vib_relaxation(int i, int j,double *M,double *dM,double Oz,double **kvt,
           double **kvv0 ,double **kvv1, double **kvv2, double **kvv3,double **kvv4,double **re_kvt,double **re_kvv0,
              double **re_kvv1,double **re_kvv2,double **re_kvv3,double **re_kvv4,double T,double **E){


	int v,w,n;
	double *O2,*N2,*H2O;
	double *O2VT,*N2VT,*H2OVT,*O2OVT,*reO2VT,*reN2VT,*reH2OVT,*reO2OVT,*N2OVT,*reN2OVT;
	double *O2N2VT,*N2O2VT,*reO2N2VT,*reN2O2VT;

	double  *O2VV,*N2VV,*O2N2VV,*N2O2VV,*O2H2OVV,*H2OO2VV,*N2H2OVV,*H2ON2VV;
	double  *reO2VV,*reN2VV,*reO2N2VV,*reN2O2VV,*reO2H2OVV,*reH2OO2VV,*reN2H2OVV,*reH2ON2VV;

	double koz,kN2oz, H2Ov1VT,H2Ov3VT;

	int Q=8;

	koz = 3.2e-12;
	kN2oz = (2.3e-13*exp(-1280.0/T)+2.7e-11*exp(-10840.0/T))*1e-6; 


	O2    =vec(20);
	N2    =vec(20);
	H2O   =vec(20);

	O2VT=vec(Q+5),N2VT=vec(Q+5),H2OVT=vec(Q+5),O2OVT=vec(Q+5),reO2VT=vec(Q+5),reN2VT=vec(Q+5),reH2OVT=vec(Q+5),reO2OVT=vec(Q+5);
	O2N2VT=vec(Q+5),N2O2VT=vec(Q+5),reO2N2VT=vec(Q+5),reN2O2VT=vec(Q+5),N2OVT=vec(Q+5),reN2OVT=vec(Q+5);

	O2VV=vec(Q+5),N2VV=vec(Q+5),O2N2VV=vec(Q+5),N2O2VV=vec(Q+5),O2H2OVV=vec(Q+5),H2OO2VV=vec(Q+5),N2H2OVV=vec(Q+5),H2ON2VV=vec(Q+5);
	reO2VV=vec(Q+5),reN2VV=vec(Q+5),reO2N2VV=vec(Q+5),reN2O2VV=vec(Q+5),reO2H2OVV=vec(Q+5),reH2OO2VV=vec(Q+5),reN2H2OVV=vec(Q+5),reH2ON2VV=vec(Q+5);


	for(w=0;w<=Q;w++){
		O2VT[w]=N2VT[w]=H2OVT[w]=O2OVT[w]=reO2VT[w]=reN2VT[w]=reH2OVT[w]=reO2OVT[w]=0.0;
		O2N2VT[w]=N2O2VT[w]=reO2N2VT[w]=reN2O2VT[w]=N2OVT[w]=reN2OVT[w]=0.0;
		O2VV[w]=N2VV[w]=O2N2VV[w]=N2O2VV[w]=O2H2OVV[w]=H2OO2VV[w]=N2H2OVV[w]=H2ON2VV[w]=0.0;
		reO2VV[w]=reN2VV[w]=reO2N2VV[w]=reN2O2VV[w]=reO2H2OVV[w]=reH2OO2VV[w]=reN2H2OVV[w]=reH2ON2VV[w]=0.0;
	}

	for(v=0;v<10;v++)O2[v]=N2[v]=H2O[v]=0.0;

	convert_ONH2O(O2,N2,H2O,M);// M ¨ O2,N2,H2O‚É–ß‚·B

	

	kvt[0][0]=kvt[1][0]=kvt[2][0]=kvt[3][0]=kvt[4][0]=kvt[5][0]=0.0;   /*k[][1<=v]‚È‚Ì‚Åk[][0]‚ÍŽg‚í‚È‚¢‚©‚ç”O‚Ì‚½‚ß‰Šú‰»*/
	for(w=0;w<Q;w++)kvv0[w][0]=kvv1[w][0]=kvv2[w][0]=kvv3[w][0]=0.0;/*ã‹L‚Æ“¯‚¶——R‚ÅVV‚Ì•û‚à‰Šú‰»*/


	for(n=0;n<=Q;n++){
		O2VT[n] +=kvt[0][n+1]*O2[n+1]*O2[0]  - kvt[0][n]*O2[n]*O2[0];     /*O2(v)-O2VT”½‰ž*/
		N2VT[n] +=kvt[1][n+1]*N2[n+1]*N2[0]  - kvt[1][n]*N2[n]*N2[0];     /*N2(v)-N2-VT”½‰ž*/
		H2OVT[n]+=kvt[3][n+1]*H2O[n+1]*H2O[0]- kvt[3][n]*H2O[n]*H2O[0];   /*H2O(v)-VT”½‰ž*/

		O2OVT[n]+=kvt[5][n+1]*O2[n+1]*Oz - kvt[5][n]*O2[n]*Oz;
	}

	for(n=0;n<=Q;n++){
		O2N2VT[n] += kvt[2][n+1]*O2[n+1]*N2[0]- kvt[2][n]*O2[n]*N2[0]; /*O2(v)-N2‚ÌVT”½‰ž*/
		N2O2VT[n] += kvt[4][n+1]*N2[n+1]*O2[0]- kvt[4][n]*N2[n]*O2[0]; /*N2(v)-O2‚ÌVT”½‰ž*/
	}


	N2OVT[0]+=   kN2oz*N2[1]*Oz;
	N2OVT[1]+= - kN2oz*N2[1]*Oz;


//V-T Time constant check
/*
n=1;
printf("%e\t%e\n",1/(kvt[5][n]*Oz)*1e6,1/(kN2oz*Oz)*1e6);
printf("%e\t%e\n",1/(kvt[0][1]*O2[0])*1e6,1/(kvt[1][1]*N2[0])*1e6);
printf("%e\t%e\t%e\n",1/(kvt[2][1]*N2[0])*1e6,1/(kvt[4][1]*O2[0])*1e6,1/(kvt[3][1]*H2O[0])*1e6);
printf("%e\n\n",1/(kvt[0][1]*O2[0])*1e6);



//O2OVT[0]+=   koz*O2[1]*Oz;
//O2OVT[1]+= - koz*O2[1]*Oz;


//V-V Time constant check

v=0,w=0;
printf("%e\t%e\n",1/(kvv0[w][v+1]*O2[w])*1e6,1/(kvv1[w][v+1]*N2[w])*1e6);
printf("%e\t%e\t%e\n",1/(kvv2[w][v+1]*O2[w])*1e6,1/(kvv3[w][v+1]*H2O[w])*1e6,1/(kvv4[w][v+1]*H2O[w])*1e6);
exit(0);
*/

	for(w=0;w<Q;w++){
		for(v=0;v<Q;v++){

		O2VV[v+1]-= kvv0[w][v+1]*O2[v+1]*O2[w];       /*O2-O2-VV”½‰žB*/
		O2VV[w]  -= kvv0[w][v+1]*O2[v+1]*O2[w];
		O2VV[v]  += kvv0[w][v+1]*O2[v+1]*O2[w];
		O2VV[w+1]+= kvv0[w][v+1]*O2[v+1]*O2[w];

		if(1){//”ñ’²˜a«‚ðl—¶‚·‚éê‡
			reO2VV[v]  -= re_kvv0[w][v+1]*O2[v]*O2[w+1];     //O2-O2_VV‚Ì‹t”½‰ž
			reO2VV[w+1]-= re_kvv0[w][v+1]*O2[v]*O2[w+1];
			reO2VV[v+1]+= re_kvv0[w][v+1]*O2[v]*O2[w+1];
			reO2VV[w]  += re_kvv0[w][v+1]*O2[v]*O2[w+1];
		}
		N2VV[v+1]-= kvv1[w][v+1]*N2[v+1]*N2[w];       /*N2-N2_VV”½‰žB*/
		N2VV[w]  -= kvv1[w][v+1]*N2[v+1]*N2[w];
		N2VV[v]  += kvv1[w][v+1]*N2[v+1]*N2[w];
		N2VV[w+1]+= kvv1[w][v+1]*N2[v+1]*N2[w];
//printf("%d\t%d\t%e\n",w,v,N2VV[v+1]);
//printf("%d\t%d\t%e\n",w,v,N2VV[w]);
//printf("%d\t%d\t%e\n",w,v,N2VV[v]);
//printf("%d\t%d\t%e\n",w,v,N2VV[w+1]);
		if(1){//”ñ’²˜a«‚ðl—¶‚·‚éê‡
			reN2VV[v]  -= re_kvv1[w][v+1]*N2[v]*N2[w+1];     //N2-N2_VV‚Ì‹t”½‰ž
			reN2VV[w+1]-= re_kvv1[w][v+1]*N2[v]*N2[w+1];
			reN2VV[v+1]+= re_kvv1[w][v+1]*N2[v]*N2[w+1];
			reN2VV[w]  += re_kvv1[w][v+1]*N2[v]*N2[w+1];
		}

		N2O2VV[v+1]-= kvv2[w][v+1]*N2[v+1]*O2[w];       /*N2_O2VV”½‰ž*/
		O2N2VV[w]  -= kvv2[w][v+1]*N2[v+1]*O2[w];
		N2O2VV[v]  += kvv2[w][v+1]*N2[v+1]*O2[w];
		O2N2VV[w+1]+= kvv2[w][v+1]*N2[v+1]*O2[w];

		reN2O2VV[v]  -= re_kvv2[w][v+1]*N2[v]*O2[w+1];     /*N2-O2_VV‚Ì‹t”½‰ž*/
		reO2N2VV[w+1]-= re_kvv2[w][v+1]*N2[v]*O2[w+1];
		reN2O2VV[v+1]+= re_kvv2[w][v+1]*N2[v]*O2[w+1];
		reO2N2VV[w]  += re_kvv2[w][v+1]*N2[v]*O2[w+1];

		O2H2OVV[v+1]-= kvv3[w][v+1]*O2[v+1]*H2O[w];       /*H2O-O2_VV”½‰ž*/
		H2OO2VV[w]  -= kvv3[w][v+1]*O2[v+1]*H2O[w];
		O2H2OVV[v]  += kvv3[w][v+1]*O2[v+1]*H2O[w];
		H2OO2VV[w+1]+= kvv3[w][v+1]*O2[v+1]*H2O[w];

		reO2H2OVV[v]  -= re_kvv3[w][v+1]*O2[v]*H2O[w+1];     /*H2O-O2‚Ì‹t”½‰ž*/
		reH2OO2VV[w+1]-= re_kvv3[w][v+1]*O2[v]*H2O[w+1];
		reO2H2OVV[v+1]+= re_kvv3[w][v+1]*O2[v]*H2O[w+1];
		reH2OO2VV[w]  += re_kvv3[w][v+1]*O2[v]*H2O[w+1];

/*
if(i==0 && j==0 ){
printf("%3d, %3d\t",w,v);
printf("%e\t",O2H2OVV[v+1]);
printf("%e\t",O2H2OVV[v]);
printf("%e\t",reO2H2OVV[v]);
printf("%e\t",reO2H2OVV[v+1]);
printf("%e\t",re_kvv3[w][v+1]);
printf("\n");
}
*/


		N2H2OVV[v+1]-= kvv4[w][v+1]*N2[v+1]*H2O[w];       //H2O-N2_VV”½‰ž
		H2ON2VV[w]  -= kvv4[w][v+1]*N2[v+1]*H2O[w];
		N2H2OVV[v]  += kvv4[w][v+1]*N2[v+1]*H2O[w];
		H2ON2VV[w+1]+= kvv4[w][v+1]*N2[v+1]*H2O[w];

		reN2H2OVV[v]  -= re_kvv4[w][v+1]*N2[v]*H2O[w+1];     //H2O-N2‚Ì‹t”½‰ž
		reH2ON2VV[w+1]-= re_kvv4[w][v+1]*N2[v]*H2O[w+1];
		reN2H2OVV[v+1]+= re_kvv4[w][v+1]*N2[v]*H2O[w+1];
		reH2ON2VV[w]  += re_kvv4[w][v+1]*N2[v]*H2O[w+1];
		}
	}


	for(n=0;n<=Q;n++){
		if(n == 0){
			reO2VT[n] += -re_kvt[0][1]*O2[0]*O2[0];  //reVT:O2-O2
			reN2VT[n] += -re_kvt[1][1]*N2[0]*N2[0];  //reVT:N2-N2

			reO2N2VT[n] += -re_kvt[2][1]*O2[0]*N2[0];  //reVT:O2-N2
			reN2O2VT[n] += -re_kvt[4][1]*N2[0]*O2[0];  //reVT:N2-O2

			reH2OVT[n] += -re_kvt[3][1]*H2O[0]*H2O[0];//reVT:H2O-H2O

			reO2OVT[n] += -re_kvt[5][1]*O2[0]*Oz;
		}else {
			reO2VT[n] +=  re_kvt[0][n]*O2[n-1]*O2[0]   - re_kvt[0][n+1]*O2[n]*O2[0];
			reN2VT[n] +=  re_kvt[1][n]*N2[n-1]*N2[0]   - re_kvt[1][n+1]*N2[n]*N2[0];

			reO2N2VT[n] +=  re_kvt[2][n]*O2[n-1]*N2[0]   - re_kvt[2][n+1]*O2[n]*N2[0];
			reN2O2VT[n] +=  re_kvt[4][n]*N2[n-1]*O2[0]   - re_kvt[1][n+1]*N2[n]*O2[0];

			reH2OVT[n] +=  re_kvt[3][n]*H2O[n-1]*H2O[0] - re_kvt[3][n+1]*H2O[n]*H2O[0];

			reO2OVT[n] +=  re_kvt[5][n]*O2[n-1]*Oz  - re_kvt[5][n+1]*O2[n]*Oz;
		}
	}
	reN2OVT[0] += -kN2oz*exp(-(E[1][1]-E[1][0])/T)*N2[0]*Oz;
	reN2OVT[1] +=  kN2oz*exp(-(E[1][1]-E[1][0])/T)*N2[0]*Oz;

	H2Ov1VT = 2.2e-17*M[20]*H2O[0]; // H2O(v1) + H2O(0) => 2 H2O(0)
	H2Ov3VT = 2.2e-17*M[21]*H2O[0]; // H2O(v3) + H2O(0) => 2 H2O(0)

	for(n=0;n<=Q;n++){
			//O2VT		   //O2-O VT		//O2-N2 VT
		dM[n]         = (O2VT[n]+reO2VT[n])+(O2OVT[n]+reO2OVT[n])+(O2N2VT[n]+reO2N2VT[n])
			+(O2VV[n]+reO2VV[n])+(O2N2VV[n]+reO2N2VV[n])+(O2H2OVV[n]+reO2H2OVV[n]);//O2

			//N2VT		  //N2-O2 VT	
		dM[n+(Q+1)]   =(N2VT[n]+reN2VT[n])+(N2OVT[n]+reN2OVT[n])+(N2O2VT[n]+reN2O2VT[n])
			+(N2VV[n]+reN2VV[n])+(N2O2VV[n]+reN2O2VV[n])+(N2H2OVV[n]+reN2H2OVV[n]);//N2

		dM[n+2*(Q+1)] =(H2OVT[n]+reH2OVT[n])
			+(H2OO2VV[n]+reH2OO2VV[n])+(H2ON2VV[n]+reH2ON2VV[n]);//H2O

	}

	dM[18] +=  H2Ov1VT+H2Ov3VT;
	dM[20] = -H2Ov1VT;
	dM[21] = -H2Ov3VT;


//if(i==0 && j==0){
//for(v=0;v<5;v++)printf("%e\t",dM[v]);
//printf("\n");
//}
	free_vec(O2,20),free_vec(N2,20),free_vec(H2O,20);

	free_vec(O2VT,Q+5),free_vec(N2VT,Q+5),free_vec(H2OVT,Q+5),free_vec(O2OVT,Q+5),free_vec(reO2VT,Q+5);
	free_vec(reN2VT,Q+5),free_vec(reH2OVT,Q+5),free_vec(reO2OVT,Q+5),free_vec(O2N2VT,Q+5),free_vec(N2O2VT,Q+5);
	free_vec(reO2N2VT,Q+5),free_vec(reN2O2VT,Q+5),free_vec(N2OVT,Q+5),free_vec(reN2OVT,Q+5);

	free_vec(O2VV,Q+5),free_vec(N2VV,Q+5),free_vec(O2N2VV,Q+5),free_vec(N2O2VV,Q+5),free_vec(O2H2OVV,Q+5);
	free_vec(H2OO2VV,Q+5),free_vec(N2H2OVV,Q+5),free_vec(H2ON2VV,Q+5);
	free_vec(reO2VV,Q+5),free_vec(reN2VV,Q+5),free_vec(reO2N2VV,Q+5),free_vec(reN2O2VV,Q+5),free_vec(reO2H2OVV,Q+5);
	free_vec(reH2OO2VV,Q+5),free_vec(reN2H2OVV,Q+5),free_vec(reH2ON2VV,Q+5);

}



/////////
void Read_constant(double **dE,double **E,double *c1,double *c2,double *c3){

	int i,j;
	char filename[256];
	FILE *fp;

	for(i=0;i<5;i++)for(j=0;j<30;j++)dE[i][j]=0.0;

	fp=fopen("inputdata/dEv_O2.dat","r");
	for(i=1;i<=10;i++)fscanf(fp,"%lf\n",&dE[0][i]);
	fclose(fp);

	fp=fopen("inputdata/dEv_N2.dat","r");
	for(i=1;i<=10;i++)fscanf(fp,"%lf\n",&dE[1][i]);
	fclose(fp);

	fp=fopen("inputdata/dEv_H2O.dat","r");
	for(i=1;i<=10;i++)fscanf(fp,"%lf\n",&dE[3][i]);
	fclose(fp);

	for(i=0;i<=3;i++){ if(i==2)i++;
		sprintf(filename, "inputdata/sum_dE(m-1)_%d.dat",i);
		fp=fopen(filename,"r");
		for(j=0;j<=10;j++)fscanf(fp,"%lf\n",&E[i][j]); /*ƒ{ƒ‹ƒcƒ}ƒ“•ª•z‚Ì‚½‚ß‚ÌU“®€ˆÊƒGƒlƒ‹ƒM[(Kelvin)“Ç‚Ýž‚Ý*/
		fclose(fp);
  	}

	fp=fopen("inputdata/c1.dat","r");
	for(i=1;i<=5;i++)fscanf(fp,"%lf\n",&c1[i]);
	fclose(fp);

	fp=fopen("inputdata/c2.dat","r");
	for(i=1;i<=5;i++)fscanf(fp,"%lf\n",&c2[i]);
	fclose(fp);

	fp=fopen("inputdata/c3.dat","r");
	for(i=1;i<=5;i++)fscanf(fp,"%lf\n",&c3[i]);
	fclose(fp);

}

///////////
