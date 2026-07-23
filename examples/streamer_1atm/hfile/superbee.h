double minmod(double r1,double r2,double b){

	double sgn;

//superbee

	if(r2<0)sgn=-1.0;
	else sgn=1.0;

	return sgn*MAX3(0.0,MIN2(sgn*b*r1,fabs(r2)),MIN2(sgn*r1,b*fabs(r2)));

//

//minmod
/*
  if(r1<0)sgn=-1.0;
  else sgn=1.0;

return sgn*MAX2(0.0,MIN2(fabs(r1),sgn*b*r2));
*/
}

void MUSCL_superbee_methoed_for_pion(double **ro,double **u,double **v,
                     double dt,double kappa,double b,int numx,int numy,double **Sr,double **Sz,double **Vol,
                      int **iflag,int **jflag,int **otuflag){


int i,j;
double **UL,**UR,**Flhalf,**Glhalf,minmod_b;

UL=mat(numx,numy);
UR=mat(numx,numy);
Flhalf=mat(numx,numy);
Glhalf=mat(numx,numy);

////////////////Glhalf////////////////
	for(i=0;i<numx;i++){
		for(j=1;j<numy;j++){
			UL[i][j]=ro[i][j] + 0.25*( (1.0-kappa)*minmod(ro[i][j+1]-ro[i][j],ro[i][j]-ro[i][j-1],b)
					+(1.0+kappa)*minmod(ro[i][j]-ro[i][j-1],ro[i][j+1]-ro[i][j],b));

			UR[i][j]=ro[i][j] - 0.25*( (1.0-kappa)*minmod(ro[i][j]-ro[i][j-1],ro[i][j+1]-ro[i][j],b)
					+(1.0+kappa)*minmod(ro[i][j+1]-ro[i][j],ro[i][j]-ro[i][j-1],b));
		}
	}

//ionは平板では自由端
	j=0;
	for(i=0;i<numx;i++){
		UL[i][j]=ro[i][j] + 0.25*( (1-kappa)*minmod(ro[i][j+1]-ro[i][j],ro[i][j]-ro[i][j],b)
			+(1+kappa)*minmod(ro[i][j]-ro[i][j],ro[i][j+1]-ro[i][j],b));
		UR[i][j]=ro[i][j] - 0.25*( (1-kappa)*minmod(ro[i][j]-ro[i][j],ro[i][j+1]-ro[i][j],b)
			+(1+kappa)*minmod(ro[i][j+1]-ro[i][j],ro[i][j]-ro[i][j],b));
	}

	for(i=0;i<numx;i++){
		for(j=1;j<numy;j++){
			if(v[i][j]>=0.0)Glhalf[i][j]=dt*Sz[i][j]*(v[i][j]*UL[i][j-1]);
			else Glhalf[i][j]=dt*Sz[i][j]*(v[i][j]*UR[i][j]);

		}
	}

	j=0;  //平板では自由端なのでUR[j]=UL[j-1]となっている。
	for(i=0;i<numx;i++){
		Glhalf[i][j]=dt*Sz[i][j]*(v[i][j]*UR[i][j]);
	}

//////////////////////////////////////////

///////////////////Flhalf/////////////////

	for(i=1;i<numx;i++){
		for(j=0;j<numy;j++){
			UL[i][j]=ro[i][j] + 0.25*( (1-kappa)*minmod(ro[i+1][j]-ro[i][j],ro[i][j]-ro[i-1][j],b)
				+(1+kappa)*minmod(ro[i][j]-ro[i-1][j],ro[i+1][j]-ro[i][j],b));
			UR[i][j]=ro[i][j] - 0.25*( (1-kappa)*minmod(ro[i][j]-ro[i-1][j],ro[i+1][j]-ro[i][j],b)
				+(1+kappa)*minmod(ro[i+1][j]-ro[i][j],ro[i][j]-ro[i-1][j],b));
		}
	}

	i=0;
	for(j=0;j<numy;j++){
		UL[i][j]=ro[i][j] + 0.25*( (1-kappa)*minmod(ro[i+1][j]-ro[i][j],ro[i][j]-0.0,b)
				+(1+kappa)*minmod(ro[i][j]-0.0,ro[i+1][j]-ro[i][j],b));
		UR[i][j]=ro[i][j] - 0.25*( (1-kappa)*minmod(ro[i][j]-0.0,ro[i+1][j]-ro[i][j],b)
				+(1+kappa)*minmod(ro[i+1][j]-ro[i][j],ro[i][j]-0.0,b));
	}

	for(i=1;i<numx;i++){
		for(j=0;j<numy;j++){
			if(u[i][j]>0.0)Flhalf[i][j]=dt*Sr[i][j]*(u[i][j]*UL[i-1][j]);
			else Flhalf[i][j]=dt*Sr[i][j]*(u[i][j]*UR[i][j]);
		}
	}
	i=0;
	for(j=0;j<numy;j++)Flhalf[i][j]=0.0; //扇形のとんがり部分ではSr=0.0


	for(i=0;i<numx-1;i++){
		for(j=0;j<numy-1;j++){

			if(iflag[i][j]){
				if(u[i][j]>0.0)Flhalf[i][j] = 0.0;//針電極中はイオンゼロ
				else           Flhalf[i][j] = ro[i][j]*u[i][j]*Sr[i][j]*dt;
			}
        		if(jflag[i][j]){
				if(v[i][j+1]>0.0)Glhalf[i][j+1] = ro[i][j]*v[i][j+1]*Sz[i][j+1]*dt;
				else             Glhalf[i][j+1] = 0.0;  //針電極中はイオンゼロ
			}

        		if(otuflag[i][j]){
				if(u[i][j]>0.0)Flhalf[i][j] = 0.0;
				else           Flhalf[i][j] = ro[i][j]*u[i][j]*Sr[i][j]*dt;

				if(v[i][j]>0.0)Glhalf[i][j+1] = ro[i][j]*v[i][j+1]*Sz[i][j+1]*dt;
				else           Glhalf[i][j+1] = 0.0;
        		}  

			ro[i][j] = ro[i][j] - (Flhalf[i+1][j] - Flhalf[i][j] + Glhalf[i][j+1] - Glhalf[i][j])/Vol[i][j];
		}
	}

//  j=0;
//  for(i=0;i<numx;i++)bomb[i] = Glhalf[i][j];
//////////////////////////////

free_mat(UL,numx,numy);
free_mat(UR,numx,numy);
free_mat(Flhalf,numx,numy);
free_mat(Glhalf,numx,numy);
}

void MUSCL_superbee_methoed_for_e(double **ro,double **u,double **v,
                     double dt,double kappa,double b,int numx,int numy,double **Sr,double **Sz,double **Vol,
                      int **iflag,int **jflag,int **otuflag){


	int i,j;
	double **UL,**UR,**Flh,**Glh,minmod_b;

	UL=mat(numx,numy),UR=mat(numx,numy),Flh=mat(numx,numy),Glh=mat(numx,numy);

////////////////Glhalf////////////////
	for(i=0;i<numx;i++){
		for(j=1;j<numy;j++){
			UL[i][j]=ro[i][j] + 0.25*( (1.0-kappa)*minmod(ro[i][j+1]-ro[i][j],ro[i][j]-ro[i][j-1],b)
                                		+(1.0+kappa)*minmod(ro[i][j]-ro[i][j-1],ro[i][j+1]-ro[i][j],b));

			UR[i][j]=ro[i][j] - 0.25*( (1.0-kappa)*minmod(ro[i][j]-ro[i][j-1],ro[i][j+1]-ro[i][j],b)
						+(1.0+kappa)*minmod(ro[i][j+1]-ro[i][j],ro[i][j]-ro[i][j-1],b));
		}
	}

	j=0;//j=0に対して。streamerだったらここにγを入れればよい。がしばらくは自由端に。
	for(i=0;i<numx;i++){
			UL[i][j]=ro[i][j] + 0.25*( (1-kappa)*minmod(ro[i][j+1]-ro[i][j],ro[i][j]-ro[i][j],b)
					+(1+kappa)*minmod(ro[i][j]-ro[i][j],ro[i][j+1]-ro[i][j],b));
			UR[i][j]=ro[i][j] - 0.25*( (1-kappa)*minmod(ro[i][j]-ro[i][j],ro[i][j+1]-ro[i][j],b)
					+(1+kappa)*minmod(ro[i][j+1]-ro[i][j],ro[i][j]-ro[i][j],b));
	}

	for(i=0;i<numx;i++){
		for(j=1;j<numy;j++){
			if(v[i][j]>=0.0)Glh[i][j]=dt*Sz[i][j]*(v[i][j]*UL[i][j-1]);
			else Glh[i][j]=dt*Sz[i][j]*(v[i][j]*UR[i][j]);
	}
	}

	j=0;  //j=0に対して。streamerだったらここにγを入れればよい。がしばらくは自由端に。
	for(i=0;i<numx;i++){
		Glh[i][j]=dt*Sz[i][j]*(v[i][j]*UR[i][j]);
	}
/*   //γ作用を考慮する場合
  j=0;
  for(i=0;i<numx;i++)Glh[i][j] = -0.1*bomb[i];
*/
//////////////////////////////////////////

///////////////////Flh/////////////////

	for(i=1;i<numx;i++){
		for(j=0;j<numy;j++){
			UL[i][j]=ro[i][j] + 0.25*( (1-kappa)*minmod(ro[i+1][j]-ro[i][j],ro[i][j]-ro[i-1][j],b)
					+(1+kappa)*minmod(ro[i][j]-ro[i-1][j],ro[i+1][j]-ro[i][j],b));
			UR[i][j]=ro[i][j] - 0.25*( (1-kappa)*minmod(ro[i][j]-ro[i-1][j],ro[i+1][j]-ro[i][j],b)
					+(1+kappa)*minmod(ro[i+1][j]-ro[i][j],ro[i][j]-ro[i-1][j],b));
		}
	}

	i=0;
	for(j=0;j<numy;j++){
		UL[i][j]=ro[i][j] + 0.25*( (1-kappa)*minmod(ro[i+1][j]-ro[i][j],ro[i][j]-0.0,b)
				  +(1+kappa)*minmod(ro[i][j]-0.0,ro[i+1][j]-ro[i][j],b));
		UR[i][j]=ro[i][j] - 0.25*( (1-kappa)*minmod(ro[i][j]-0.0,ro[i+1][j]-ro[i][j],b)
				  +(1+kappa)*minmod(ro[i+1][j]-ro[i][j],ro[i][j]-0.0,b));
	}

	for(i=1;i<numx;i++){
		for(j=0;j<numy;j++){
			if(u[i][j]>0.0)Flh[i][j]=dt*Sr[i][j]*(u[i][j]*UL[i-1][j]);
			else Flh[i][j]=dt*Sr[i][j]*(u[i][j]*UR[i][j]);
		}
	}

	i=0;
	for(j=0;j<numy;j++)Flh[i][j]=dt*Sr[i][j]*(u[i][j]*0.0);

	for(i=0;i<numx-1;i++){
		for(j=0;j<numy-1;j++){

			if(iflag[i][j]){
				if(u[i][j]>0.0)Flh[i][j]=ro[i][j]*u[i][j]*Sr[i][j]*dt;  //自由端。
				else           Flh[i][j]=ro[i][j]*u[i][j]*Sr[i][j]*dt;  //結局同じ。
			}
        		if(jflag[i][j]){
				if(v[i][j+1]>0.0)Glh[i][j+1]=ro[i][j]*v[i][j+1]*Sz[i][j+1]*dt; //自由端
				else             Glh[i][j+1]=ro[i][j]*v[i][j+1]*Sz[i][j+1]*dt; //同じ
			}
        		if(otuflag[i][j]){
        			Flh[i][j]=ro[i][j]*u[i][j]*Sr[i][j]*dt;      //自由端。よって同じ。
        			Glh[i][j+1]=ro[i][j]*v[i][j+1]*Sz[i][j+1]*dt;
        		}  

			ro[i][j] = ro[i][j] - (Flh[i+1][j] - Flh[i][j] + Glh[i][j+1] - Glh[i][j])/Vol[i][j];
		}
  	}
//////////////////////////////

	free_mat(UL,numx,numy),free_mat(UR,numx,numy),free_mat(Flh,numx,numy),free_mat(Glh,numx,numy);
}

void MUSCL_superbee_methoed_for_mion(double **ro,double **u,double **v,
                     double dt,double kappa,double b,int numx,int numy,double **Sr,double **Sz,double **Vol,
                      int **iflag,int **jflag,int **otuflag){


int i,j;
double **UL,**UR,**Flhalf,**Glhalf,minmod_b;

UL=mat(numx,numy);
UR=mat(numx,numy);
Flhalf=mat(numx,numy);
Glhalf=mat(numx,numy);

////////////////Glhalf////////////////
  for(i=0;i<numx;i++){
    for(j=1;j<numy;j++){
      UL[i][j]=ro[i][j] + 0.25*( (1.0-kappa)*minmod(ro[i][j+1]-ro[i][j],ro[i][j]-ro[i][j-1],b)
                                +(1.0+kappa)*minmod(ro[i][j]-ro[i][j-1],ro[i][j+1]-ro[i][j],b));

      UR[i][j]=ro[i][j] - 0.25*( (1.0-kappa)*minmod(ro[i][j]-ro[i][j-1],ro[i][j+1]-ro[i][j],b)
                                +(1.0+kappa)*minmod(ro[i][j+1]-ro[i][j],ro[i][j]-ro[i][j-1],b));
    }
  }

j=0;//j=0に対して。0.0
  for(i=0;i<numx;i++){
      UL[i][j]=ro[i][j] + 0.25*( (1-kappa)*minmod(ro[i][j+1]-ro[i][j],ro[i][j]-0.0,b)
                       +(1+kappa)*minmod(ro[i][j]-0.0,ro[i][j+1]-ro[i][j],b));
      UR[i][j]=ro[i][j] - 0.25*( (1-kappa)*minmod(ro[i][j]-0.0,ro[i][j+1]-ro[i][j],b)
                                +(1+kappa)*minmod(ro[i][j+1]-ro[i][j],ro[i][j]-0.0,b));
  }

  for(i=0;i<numx;i++){
    for(j=1;j<numy;j++){
      if(v[i][j]>=0.0)Glhalf[i][j]=dt*Sz[i][j]*(v[i][j]*UL[i][j-1]);
      else Glhalf[i][j]=dt*Sz[i][j]*(v[i][j]*UR[i][j]);

    }
  }
  j=0;  //j=0に対して
  for(i=0;i<numx;i++){
	Glhalf[i][j]=dt*Sz[i][j]*(v[i][j]*UR[i][j]);
  }
//////////////////////////////////////////

///////////////////Flhalf/////////////////

  for(i=1;i<numx;i++){
	for(j=0;j<numy;j++){
		UL[i][j]=ro[i][j] + 0.25*( (1-kappa)*minmod(ro[i+1][j]-ro[i][j],ro[i][j]-ro[i-1][j],b)
				  +(1+kappa)*minmod(ro[i][j]-ro[i-1][j],ro[i+1][j]-ro[i][j],b));
		UR[i][j]=ro[i][j] - 0.25*( (1-kappa)*minmod(ro[i][j]-ro[i-1][j],ro[i+1][j]-ro[i][j],b)
				  +(1+kappa)*minmod(ro[i+1][j]-ro[i][j],ro[i][j]-ro[i-1][j],b));
	}
  }

i=0;
  for(j=0;j<numy;j++){
	UL[i][j]=ro[i][j] + 0.25*( (1-kappa)*minmod(ro[i+1][j]-ro[i][j],ro[i][j]-0.0,b)
				  +(1+kappa)*minmod(ro[i][j]-0.0,ro[i+1][j]-ro[i][j],b));
	UR[i][j]=ro[i][j] - 0.25*( (1-kappa)*minmod(ro[i][j]-0.0,ro[i+1][j]-ro[i][j],b)
				  +(1+kappa)*minmod(ro[i+1][j]-ro[i][j],ro[i][j]-0.0,b));
  }

  for(i=1;i<numx;i++){
	for(j=0;j<numy;j++){
		if(u[i][j]>0.0)Flhalf[i][j]=dt*Sr[i][j]*(u[i][j]*UL[i-1][j]);
		else Flhalf[i][j]=dt*Sr[i][j]*(u[i][j]*UR[i][j]);
	}
  }
  i=0;
  for(j=0;j<numy;j++)Flhalf[i][j]=dt*Sr[i][j]*(u[i][j]*0.0);

  for(i=0;i<numx-1;i++){
	for(j=0;j<numy-1;j++){

		if(iflag[i][j]){
			if(u[i][j]>0.0)Flhalf[i][j]=ro[i][j]*u[i][j]*Sr[i][j]*dt;  //自由端。
			else           Flhalf[i][j]=ro[i][j]*u[i][j]*Sr[i][j]*dt;  //結局同じ。
		}
        	if(jflag[i][j]){
			if(v[i][j+1]>0.0)Glhalf[i][j+1]=ro[i][j]*v[i][j+1]*Sz[i][j+1]*dt; //自由端
			else             Glhalf[i][j+1]=ro[i][j]*v[i][j+1]*Sz[i][j+1]*dt; //同じ
		}
        	if(otuflag[i][j]){
        		Flhalf[i][j]=ro[i][j]*u[i][j]*Sr[i][j]*dt;      //自由端。よって同じ。
        		Glhalf[i][j+1]=ro[i][j]*v[i][j+1]*Sz[i][j+1]*dt;
        	}  

		ro[i][j] = ro[i][j] - (Flhalf[i+1][j] - Flhalf[i][j] + Glhalf[i][j+1] - Glhalf[i][j])/Vol[i][j];
	}
  }
//////////////////////////////

free_mat(UL,numx,numy);
free_mat(UR,numx,numy);
free_mat(Flhalf,numx,numy);
free_mat(Glhalf,numx,numy);
}


