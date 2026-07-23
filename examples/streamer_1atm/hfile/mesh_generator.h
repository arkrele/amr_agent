
void mesh_generator(char *FILE_MESH_R, char *FILE_MESH_Z, int NR,int NZ,double *r,double *z,double *rh,double *zh,
                                double *dr,double *dz,double **Vol,double **Sr,double **Sz){

	int i,j,dum;
	FILE *fp;


	fp=fopen(FILE_MESH_Z,"r");
	for(j=0;j<NZ;j++)fscanf(fp,"%d\t%le\n",&dum,&z[j]);
	fclose(fp);

	fp=fopen(FILE_MESH_R,"r");
	for(i=0;i<NR;i++)fscanf(fp,"%d\t%le\n",&dum,&r[i]);
	fclose(fp);

	for(i=0;i<NR-1;i++){
		for(j=0;j<NZ-1;j++){
			rh[i]=(r[i]+r[i+1])*0.5; //検査体積中心の座標
			zh[j]=(z[j]+z[j+1])*0.5;
		}
	}

	j=NZ-1;
	for(i=0;i<NR-1;i++){
		rh[i]=(r[i]+r[i+1])*0.5; //検査体積中心の座標
		zh[j]=(z[j]+z[j])*0.5;
	}
  	i=NR-1;
  	for(j=0;j<NZ-1;j++){
		rh[i]=(r[i]+r[i])*0.5; //検査体積中心の座標
		zh[j]=(z[j]+z[j+1])*0.5;
	}

	i=NR-1,j=NZ-1;
	rh[i]=(r[i]+r[i])*0.5; //検査体積中心の座標
	zh[j]=(z[j]+z[j])*0.5;

////////////////adaptive用////////////////////////
	for(i=0;i<NR-1;i++)dr[i]=r[i+1]-r[i];//dr,dzを計算
	for(j=0;j<NZ-1;j++)dz[j]=z[j+1]-z[j];
	dr[NR-1]=dr[NR-2];//端っこは一つとなりの値を代入
	dz[NZ-1]=dz[NZ-2];

//Vol:検査体域の体積、r断面積、z断面積を計算
	for(i=0;i<NR;i++){
		for(j=0;j<NZ;j++){
			if(i==NR-1){
				Vol[i][j] = dz[j]*(r[i]+r[i])*dr[i]/2.0;
				Sr[i][j]  = r[i]*dz[j];  //rθ*dz
				Sz[i][j]  = (r[i]+r[i])*dr[i]/2.0;  //台形公式、(rθ+rθ)*dr/2
			}else{
				Vol[i][j] = dz[j]*(r[i]+r[i+1])*dr[i]/2.0; //台形公式、θ=1度
				Sr[i][j]  = r[i]*dz[j];  //rθ*dz
				Sz[i][j]  = (r[i]+r[i+1])*dr[i]/2.0;  //台形公式、(rθ+rθ)*dr/2
			}
		}
	}

///////////////////////////////////////////////////

}
