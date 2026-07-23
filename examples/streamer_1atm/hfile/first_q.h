//初期電子密度分布
void first_q(int NR,int NZ,int **flag,double *rh,double *zh,double **ne,
		double **N2p,double **O2p,double **O2m,double **Om){

	int i,j;
	double n_max, r0,z0,sr0,sz0;

	n_max=1E9; //m-3

	r0=0.0e-3;
	z0=13.0e-3;
	sr0=100*1.0e-6;
	sz0=100*1.0e-6;

	for(i=0;i<NR;i++){
		for(j=0;j<NZ;j++){
			if(flag[i][j])ne[i][j] = 0.0;
			else ne[i][j] = n_max * exp(-pow(rh[i]-r0,2)/(2*sr0*sr0) - pow(zh[j]-z0,2)/(2*sz0*sz0));
		}
	}



	for(i=0;i<NR;i++){
		for(j=0;j<NZ;j++){
			if(flag[i][j]){
				N2p[i][j]=O2p[i][j]=O2m[i][j]=Om[i][j]=0.0;
			}else{
				N2p[i][j]=ne[i][j]/3.0;  // [/m^3]
				O2p[i][j]=ne[i][j]/2.0;  // [/m^3]
				O2m[i][j]=0.0;  // [/m^3]
				Om [i][j]=0.0;  // [/m^3]
			}
		}
	}
}
