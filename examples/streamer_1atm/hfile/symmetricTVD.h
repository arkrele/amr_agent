
double sign(double a1,double a2){

if(a2>=0)return fabs(a1);
else return -fabs(a1);

}


void bndcnd(double **rou,double **u,double **v,double **p,double **t,double **q1,double **q2,
                double **q3,double **q4,double g0,double rgas,double u0,double v0,double p0,double t0,
                  double rou0,int mx,int my,int **flag,int **iflag,int **jflag,int **otuflag,int **totuflag){

int i,j,k,wall;
double cpgas,htotal;

	for(i=0;i<mx;i++){
		for(j=0;j<my;j++){
//			if(!flag[i][j]){
				rou[i][j] = q1[i][j];
				u[i][j] = q2[i][j]/q1[i][j];
				v[i][j] = q3[i][j]/q1[i][j];
				p[i][j] = (g0-1.0)*(q4[i][j]-0.5*rou[i][j]*(u[i][j]*u[i][j]+v[i][j]*v[i][j]));
				t[i][j] = p[i][j]/(rgas*rou[i][j]);
/*
			}else if(jflag[i][j-1]==1 && otuflag[i][j-1]==1){
				rou[i][j] = rou[i][j-1];
				u[i][j] = u[i][j-1];
				v[i][j] = v[i][j-1];
				p[i][j] = p[i][j-1];
				t[i][j] = t[i][j-1];
			}else{
				rou[i][j] = rou0;
				u[i][j] = u0;
				v[i][j] = v0;
				p[i][j] = p0;
				t[i][j] = t0;
			}
*/
		}
	}

/*
  i=0;
	for(j=0;j<my;j++){
		if(!flag[i][j]){
			u[i][j] = u[i+1][j];
			v[i][j] = v[i+1][j];
			p[i][j] = p[i+1][j];
			t[i][j] = t[i+1][j];
			rou[i][j] = rou[i+1][j];
		}
	}
*/

  cpgas = rgas*g0/(g0-1.0);  //Cp - Cv = R(気体定数)とγ=Cp/Cv　より。
  htotal= 0.5*(u0*u0+v0*v0) + t0*cpgas;


  j=0;
	for(i=1;i<mx;i++){
//		if(!flag[i][j]){
			  u[i][j] = 0.0;//2.0*u[i][j-1]-u[i][j-2];//0参照、いいのか？
			  v[i][j] = 0.0;
			  p[i][j] = p[i][1];
			  t[i][j] = (htotal - 0.5*(u[i][j]*u[i][j]+v[i][j]*v[i][j]))/cpgas;
			rou[i][j] = p[i][j]/(rgas*t[i][j]);
//		}
	}

  j=my-1;
	for(i=0;i<mx;i++){
//		if(!flag[i][j]){
			  u[i][j] = 2.0*u[i][j-1]-u[i][j-2];
			  v[i][j] = 2.0*v[i][j-1]-v[i][j-2];
			  p[i][j] = 2.0*p[i][j-1]-p[i][j-2];
		  	  t[i][j] = (htotal - 0.5*(u[i][j]*u[i][j]+v[i][j]*v[i][j]))/cpgas;
			rou[i][j] = p[i][j]/(rgas*t[i][j]);
//		}
	}

/*
  i=wall=40;
  for(j=my/2-5;j<=my/2+5;j++){
    u[i][j] = 0.0;
    v[i][j] = 2.0*v[wall-1][j]-v[wall-2][j];
    p[i][j] = p[wall-1][j];
    t[i][j] = (htotal - 0.5*(u[i][j]*u[i][j]+v[i][j]*v[i][j]))/cpgas;
  rou[i][j] = p[i][j]/(rgas*t[i][j]);
  }
*/
  i=mx-1;
	for(j=0;j<my;j++){
//		if(!flag[i][j]){
			  u[i][j] = 2.0*u[i-1][j]-u[i-2][j];
			  v[i][j] = 2.0*v[i-1][j]-v[i-2][j];
			  p[i][j] = 2.0*p[i-1][j]-p[i-2][j];
		  	  t[i][j] = (htotal - 0.5*(u[i][j]*u[i][j]+v[i][j]*v[i][j]))/cpgas;
			rou[i][j] = p[i][j]/(rgas*t[i][j]);
//		}
	}

/*
	for(i=0;i<mx;i++){
		for(j=0;j<my;j++){
			if(iflag[i][j]){
			  u[i][j] = u[i+1][j];
			  v[i][j] = v[i+1][j];
			  p[i][j] = p[i+1][j];
		  	  t[i][j] = t[i+1][j];
			rou[i][j] = rou[i+1][j];
			}else if(jflag[i][j]){
			  u[i][j] = u[i][j-1];
			  v[i][j] = v[i][j-1];
			  p[i][j] = p[i][j-1];
		  	  t[i][j] = t[i][j-1];
			rou[i][j] = rou[i][j-1];
			}else if(otuflag[i][j]){
			  u[i][j] = u[i][j-1];
			  v[i][j] = v[i][j-1];
			  p[i][j] = p[i][j-1];
		  	  t[i][j] = t[i][j-1];
			rou[i][j] = rou[i][j-1];
			}
		}
	}
*/

	for(i=0;i<mx;i++){
		for(j=0;j<my;j++){
//			if(!flag[i][j]){
				q1[i][j] = rou[i][j];
				q2[i][j] = rou[i][j]*u[i][j];
				q3[i][j] = rou[i][j]*v[i][j];
				q4[i][j]=rou[i][j]*(rgas*t[i][j]/(g0-1.0) + (u[i][j]*u[i][j]+v[i][j]*v[i][j])*0.5);
/*
			}else if(jflag[i][j-1]==1 && otuflag[i][j-1]==1){
				q1[i][j] = q1[i][j-1];
				q2[i][j] = q2[i][j-1];
				q3[i][j] = q3[i][j-1];
				q4[i][j] = q4[i][j-1];
			}else{
				q1[i][j] = rou0;
				q2[i][j] = rou0*u0;
				q3[i][j] = rou0*v0;
				q4[i][j] = rou0*(rgas*t0/(g0-1.0) + (u0*u0+v0*v0)/2.0);
			}
*/
		}
	}
}

void calrhs(int mx,int my,double **rou,double **u,double **v,double **t,double **p,
              double **q1,double **q2,double **q3,double **q4,double rgas,double g0,
                double dt,double *dx,double *dy,double ecp,double **dq1,double **dq2,double **dq3,double **dq4,int **flag){

	int i,j,k;
	double dash,*um,*vm,*hm,*uum,*cmm,*cm,*cpm,*dlt,d1,d2,d3,d4,*a1,*a2,*a3,*a4,cm2,aa,bb,b1,b2,cc,H,q;
	double *p1,*p2,*p3,*p4,*tv1,*tv2,*tv3,*tv4,ff1,ff2,delta,s,qq,e1,e2,e3,e4;
	double f1,f2,f3,f4;
	double dt_per_dx,dt_per_dy,inv_delta;

	um = vec(my),vm = vec(my),hm = vec(my),uum= vec(my);
	cmm= vec(my),cm = vec(my),cpm= vec(my),dlt= vec(my);
	a1 = vec(my),a2 = vec(my),a3 = vec(my),a4 = vec(my);
	p1 = vec(my),p2 = vec(my),p3 = vec(my),p4 = vec(my);
	tv1= vec(my),tv2= vec(my),tv3= vec(my),tv4= vec(my);



/////////////////x方向について計算////////////////////////
	for(j=0;j<my;j++){
		for(i=0;i<mx-1;i++){
//			if(!flag[i][j]){
				dash  = sqrt(rou[i+1][j]/rou[i][j]);                  //Roe平均を算出 dashは計算を簡略化するため
				um[i] = (dash*u[i+1][j]+u[i][j])/(dash+1.0);          //速度のRoe平均
				vm[i] = (dash*v[i+1][j]+v[i][j])/(dash+1.0);          //
				hm[i] = (dash*(q4[i+1][j]+p[i+1][j])/rou[i+1][j]      //単位質量あたりの全エンタルピー
				             +(q4[i  ][j]+p[i  ][j])/rou[i  ][j])/(dash+1.0);
				cm2   = (g0-1.0)*(hm[i]-0.5*(um[i]*um[i]+vm[i]*vm[i]));  //音速cのRoe平均の２乗。おそらく
				cm[i] = sqrt(cm2);

				uum[i]=um[i];               //Eigenvalues
				cmm[i]=uum[i] - cm[i];
				cpm[i]=uum[i] + cm[i];
	
				d1 = q1[i+1][j] -q1[i][j];
				d2 = q2[i+1][j] -q2[i][j];
				d3 = q3[i+1][j] -q3[i][j];
				d4 = q4[i+1][j] -q4[i][j];

				b1 = ((um[i]*um[i]+vm[i]*vm[i])*0.5)*(g0-1.0)/(cm[i]*cm[i]);
				b2 = (g0-1.0)/(cm[i]*cm[i]);

				a1[i] = 0.5*(b1 + um[i]/cm[i])*d1 - 0.5*(1.0/cm[i] + b2*um[i])*d2 - 0.5*b2*vm[i]*d3 + 0.5*b2*d4;
				a2[i] =             (1.0 - b1)*d1 +                   b2*um[i]*d2 +     b2*vm[i]*d3 -     b2*d4;
				a3[i] = 0.5*(b1 - um[i]/cm[i])*d1 + 0.5*(1.0/cm[i] - b2*um[i])*d2 - 0.5*b2*vm[i]*d3 + 0.5*b2*d4;
				a4[i] =                 -vm[i]*d1 +                        0.0*d2 +              d3 +    0.0*d4;
	
				dlt[i]= fabs(uum[i]) + fabs(vm[i]) + cm[i]*sqrt(2.0);
//			}
		}


//		a1[0] = a1[1];
//		a2[0] = a2[1];
//		a3[0] = a3[1];
//		a4[0] = a4[1];
		a1[mx-1] = a1[mx-2];
		a2[mx-1] = a2[mx-2];
		a3[mx-1] = a3[mx-2];
		a4[mx-1] = a4[mx-2];

		for(i=0;i<mx-1;i++){
			dt_per_dx=dt/dx[i];

//			if(!flag[i][j]){
				ff1 = (dt_per_dx)*cmm[i]*cmm[i];
				ff2=fabs(cmm[i]);
				delta=dlt[i]*ecp;
				inv_delta=1.0/delta;
				if(ff2<delta)ff2 = 0.5*(cm[i]*cm[i] + delta*delta)*inv_delta;
				s=sign(1.0,a1[i]);
				if(i)qq=s*MAX2(0.0,MIN3(s*a1[i-1], s*a1[i], s*a1[i+1]));
				else qq=s*MAX2(0.0,MIN3(s*a1[i] , s*a1[i], s*a1[i+1]));

				p1[i]=-ff1*qq-ff2*(a1[i]-qq);
				  ff1 = (dt_per_dx)*cpm[i]*cpm[i];
				  ff2=fabs(cpm[i]);
				  if(ff2<delta)ff2 = 0.5*(cpm[i]*cpm[i] + delta*delta)*inv_delta;
				  s=sign(1.0,a3[i]);
				  if(i)qq=s*MAX2(0.0,MIN3(s*a3[i-1], s*a3[i], s*a3[i+1]));
				  else qq=s*MAX2(0.0,MIN3(s*a3[i]  , s*a3[i], s*a3[i+1]));

				p3[i]=-ff1*qq-ff2*(a3[i]-qq);
				  ff1 = (dt_per_dx)*uum[i]*uum[i];
				  ff2=fabs(uum[i]);
				  if(ff2<delta)ff2 = 0.5*(uum[i]*uum[i] + delta*delta)*inv_delta;
				    s=sign(1.0,a2[i]);
				  if(i)qq=s*MAX2(0.0,MIN3(s*a2[i-1], s*a2[i], s*a2[i+1]));
				  else qq=s*MAX2(0.0,MIN3(s*a2[i]  , s*a2[i], s*a2[i+1]));

				p2[i]=-ff1*qq-ff2*(a2[i]-qq);
				  s=sign(1.0,a4[i]);
				  if(i)qq=s*MAX2(0.0,MIN3(s*a4[i-1], s*a4[i], s*a4[i+1]));
				  else qq=s*MAX2(0.0,MIN3(s*a4[i]  , s*a4[i], s*a4[i+1]));

				p4[i]=-ff1*qq-ff2*(a4[i]-qq);
//			}
		}
		i=mx-1;
		p1[i] = p1[i-1];
		p2[i] = p2[i-1];
		p3[i] = p3[i-1];
		p4[i] = p4[i-1];

		for(i=0;i<mx-1;i++){
//			if(!flag[i][j]){

				q      = um[i]*um[i] + vm[i]*vm[i];
				H      = cm[i]*cm[i]/(g0-1.0) + 0.5*q;

				tv1[i] =                     p1[i] +       p2[i] +                     p3[i] +   0.0*p4[i];
				tv2[i] =       (um[i]-cm[i])*p1[i] + um[i]*p2[i] +       (um[i]+cm[i])*p3[i] +   0.0*p4[i];
				tv3[i] =             (vm[i])*p1[i] + vm[i]*p2[i] +             (vm[i])*p3[i] +       p4[i];
				tv4[i] = (hm[i]-cm[i]*um[i])*p1[i] + 0.5*q*p2[i] + (hm[i]+cm[i]*um[i])*p3[i] + vm[i]*p4[i];

				e1 = q2[i][j] + q2[i+1][j];
				e2 = q2[i][j]*u[i][j] + p[i][j] + q2[i+1][j]*u[i+1][j] + p[i+1][j];
				e3 = q2[i][j]*v[i][j] + q2[i+1][j]*v[i+1][j];
				e4 = (q4[i][j]+p[i][j])*u[i][j] + (q4[i+1][j] + p[i+1][j])*u[i+1][j];

				tv1[i] = (e1 + tv1[i])*0.5;
				tv2[i] = (e2 + tv2[i])*0.5;
				tv3[i] = (e3 + tv3[i])*0.5;
				tv4[i] = (e4 + tv4[i])*0.5;
//			}
		}
		i=mx-1;
		tv1[i] = tv1[i-1];
		tv2[i] = tv2[i-1];
		tv3[i] = tv3[i-1];
		tv4[i] = tv4[i-1];

		for(i=0;i<mx;i++){
			dt_per_dx=dt/dx[i];

//			if(!flag[i][j]){
				if(i==0){
					dq1[i][j] = -(tv1[i] - tv1[i])*dt_per_dx;
					dq2[i][j] = -(tv2[i] - tv2[i])*dt_per_dx;
					dq3[i][j] = -(tv3[i] - tv3[i])*dt_per_dx;
					dq4[i][j] = -(tv4[i] - tv4[i])*dt_per_dx;
				}else{
					dq1[i][j] = -(tv1[i] - tv1[i-1])*dt_per_dx;
					dq2[i][j] = -(tv2[i] - tv2[i-1])*dt_per_dx;
					dq3[i][j] = -(tv3[i] - tv3[i-1])*dt_per_dx;
					dq4[i][j] = -(tv4[i] - tv4[i-1])*dt_per_dx;
				}
//			}else{
//				dq1[i][j]=dq2[i][j]=dq3[i][j]=dq4[i][j]=0.0;
//			}
		}
	}


/////////////////////////////////////////////////////////////

/////////////////////////y方向について////////////////////////
	for(i=0;i<mx;i++){
		for(j=0;j<my-1;j++){
//			if(!flag[i][j]){
				dash  = sqrt(rou[i][j+1]/rou[i][j]);
				um[j] = (dash*u[i][j+1]+u[i][j])/(dash+1.0);
				vm[j] = (dash*v[i][j+1]+v[i][j])/(dash+1.0);
				if(flag[i][j+1]){
					hm[j] = (dash*(q4[i][j+1]+p[i][j+1])/rou[i][j] 
					     +(q4[i  ][j]+p[i  ][j])/rou[i  ][j])/(dash+1.0);
				}else{
					hm[j] = (dash*(q4[i][j+1]+p[i][j+1])/rou[i][j+1] 
					     +(q4[i  ][j]+p[i  ][j])/rou[i  ][j])/(dash+1.0);
				}
				cm2   = (g0-1.0)*(hm[j]-0.5*(um[j]*um[j]+vm[j]*vm[j]));
				cm[j] = sqrt(cm2);

				uum[j]= vm[j];
				cmm[j]=uum[j]-cm[j];
				cpm[j]=uum[j]+cm[j];
	
				d1 = q1[i][j+1] -q1[i][j];
				d2 = q2[i][j+1] -q2[i][j];
				d3 = q3[i][j+1] -q3[i][j];
				d4 = q4[i][j+1] -q4[i][j];

				b1 = ((um[i]*um[i]+vm[i]*vm[i])*0.5)*(g0-1.0)/(cm[i]*cm[i]);
				b2 = (g0-1.0)/(cm[i]*cm[i]);

				a1[j] = 0.5*(b1 + vm[j]/cm[j])*d1 - 0.5*b2*um[j]*d2 - 0.5*(1.0/cm[j] + b2*vm[j])*d3 + 0.5*b2*d4;
				a2[j] =             (1.0 - b1)*d1 +     b2*um[j]*d2 +                   b2*vm[j]*d3 -     b2*d4;
				a3[j] = 0.5*(b1 - vm[j]/cm[j])*d1 - 0.5*b2*um[j]*d2 + 0.5*(1.0/cm[j] - b2*vm[j])*d3 + 0.5*b2*d4;
				a4[j] =                 -um[j]*d1 +              d2 +              0.0              + 0.0;

				dlt[j]= fabs(uum[j]) + fabs(um[j]) + cm[j]*sqrt(2.0);
//			}
		}

//		a1[0] = a1[1];
//		a2[0] = a2[1];
//		a3[0] = a3[1];
//		a4[0] = a4[1];
		a1[my-1] = a1[my-2];
		a2[my-1] = a2[my-2];
		a3[my-1] = a3[my-2];
		a4[my-1] = a4[my-2];

		for(j=1;j<my-1;j++){
			dt_per_dy=dt/dy[j];

//			if(!flag[i][j]){
				ff1 = (dt_per_dy)*cmm[j]*cmm[j];
				ff2=fabs(cmm[j]);
				delta=dlt[j]*ecp;
				inv_delta=1.0/delta;
				if(ff2<delta)ff2 = 0.5*(cm[j]*cm[j] + delta*delta)*inv_delta;
				s=sign(1.0,a1[j]);
				qq=s*MAX2(0.0,MIN3(s*a1[j-1], s*a1[j], s*a1[j+1]));

				p1[j]=-ff1*qq-ff2*(a1[j]-qq);
				  ff1 = (dt_per_dy)*cpm[j]*cpm[j];
			  	  ff2=fabs(cpm[j]);
			  	  if(ff2<delta)ff2 = 0.5*(cpm[j]*cpm[j] + delta*delta)*inv_delta;
			  	  s=sign(1.0,a3[j]);
				  qq=s*MAX2(0.0,MIN3(s*a3[j-1], s*a3[j], s*a3[j+1]));

				p3[j]=-ff1*qq-ff2*(a3[j]-qq);
				  ff1 = (dt_per_dy)*uum[j]*uum[j];
				  ff2=fabs(uum[j]);
				  delta=ecp*dlt[j];
				  if(ff2<delta)ff2 = 0.5*(uum[j]*uum[j] + delta*delta)*inv_delta;
				  s=sign(1.0,a2[j]);
				  qq=s*MAX2(0.0,MIN3(s*a2[j-1], s*a2[j], s*a2[j+1]));

				p2[j]=-ff1*qq-ff2*(a2[j]-qq);
				  s=sign(1.0,a4[j]);
				  qq=s*MAX2(0.0,MIN3(s*a4[j-1], s*a4[j], s*a4[j+1]));

				p4[j]=-ff1*qq-ff2*(a4[j]-qq);
//		}
		}


		for(j=1;j<my-1;j++){
//			if(!flag[i][j]){

				q      = um[j]*um[j] + vm[j]*vm[j];
				H      = cm[j]*cm[j]/(g0-1.0) + 0.5*q;

				tv1[j] =                 p1[j] +       p2[j] +                 p3[j] +   0.0*p4[j];
				tv2[j] =         (um[j])*p1[j] + um[j]*p2[j] +         (um[j])*p3[j] +       p4[j];
				tv3[j] =   (vm[j]-cm[j])*p1[j] + vm[j]*p2[j] +   (vm[j]+cm[j])*p3[j] +   0.0*p4[j];
				tv4[j] = (hm[j]-cm[j]*vm[j])*p1[j] + 0.5*q*p2[j] + (hm[j]+cm[j]*vm[j])*p3[j] + um[i]*p4[j];

				f1 = q3[i][j] + q3[i][j+1];
				f2 = q3[i][j]*u[i][j] + q3[i][j+1]*u[i][j+1];
				f3 = q3[i][j]*v[i][j] + p[i][j] + q3[i][j+1]*v[i][j+1] + p[i][j+1];
				f4 = (q4[i][j]+p[i][j])*v[i][j] + (q4[i][j+1] + p[i][j+1])*v[i][j+1];

				tv1[j] = (f1 + tv1[j])*0.5;
				tv2[j] = (f2 + tv2[j])*0.5;
				tv3[j] = (f3 + tv3[j])*0.5;
				tv4[j] = (f4 + tv4[j])*0.5;
//			}
		}

		for(j=2;j<=my-1;j++){
			dt_per_dy=dt/dy[j];

			dq1[i][j] = dq1[i][j] - (tv1[j] - tv1[j-1])*dt_per_dy;
			dq2[i][j] = dq2[i][j] - (tv2[j] - tv2[j-1])*dt_per_dy;
			dq3[i][j] = dq3[i][j] - (tv3[j] - tv3[j-1])*dt_per_dy;
			dq4[i][j] = dq4[i][j] - (tv4[j] - tv4[j-1])*dt_per_dy;
		}
	}
/////////////////////////////////////////////////////////////

	free_vec(um ,my),free_vec(vm ,my),free_vec(hm ,my),free_vec(uum,my);
	free_vec(cmm,my),free_vec(cm ,my),free_vec(cpm,my),free_vec(dlt,my);
	free_vec(a1 ,my),free_vec(a2 ,my),free_vec(a3 ,my),free_vec(a4 ,my);
	free_vec(p1 ,my),free_vec(p2 ,my),free_vec(p3 ,my),free_vec(p4 ,my);
	free_vec(tv1,my),free_vec(tv2,my),free_vec(tv3,my),free_vec(tv4,my);
}



void symmetric_TVD(int mx,int my,double u0,double v0,double p0,double t0,double rou0,double dt,double g0,double rgas,double ecp,double res,
                      double **rou,double **u,double **v,double **t,double **p,double **q1,double **q2,double **q3,double **q4,
                        int **flag,int **iflag,int **jflag,int **otuflag,int **totuflag,double *dxx,double *dyy){

	int i,j;
	double **qold1,**qold2,**qold3,**qold4,**dq1,**dq2,**dq3,**dq4,**dqn1,**dqn2,**dqn3,**dqn4;
	double memo,grid;

	qold1=mat(mx,my),qold2=mat(mx,my),qold3=mat(mx,my),qold4=mat(mx,my);
	dqn1=mat(mx,my) ,dqn2=mat(mx,my) ,dqn3=mat(mx,my) ,dqn4=mat(mx,my);
	dq1=mat(mx,my)  ,dq2=mat(mx,my)  ,dq3=mat(mx,my)  ,dq4=mat(mx,my);

	grid = (float)(mx*my);

//////////////////////Two-step Runge-Kutta time-marching//////////////////////////


	for(i=0;i<mx;i++){            
		for(j=0;j<my;j++){
			qold1[i][j]=q1[i][j];
			qold2[i][j]=q2[i][j];
			qold3[i][j]=q3[i][j];
			qold4[i][j]=q4[i][j];
		}
	}

	calrhs(mx,my,rou,u,v,t,p,q1,q2,q3,q4,rgas,g0,dt,dxx,dyy,ecp,dq1,dq2,dq3,dq4,flag); // step1...k1を計算

	for(i=0;i<mx;i++){
		for(j=0;j<my;j++){
			dqn1[i][j] = dq1[i][j];
			dqn2[i][j] = dq2[i][j];
			dqn3[i][j] = dq3[i][j];
			dqn4[i][j] = dq4[i][j];

			q1[i][j] += dq1[i][j];
			q2[i][j] += dq2[i][j];
			q3[i][j] += dq3[i][j];
			q4[i][j] += dq4[i][j];
		}
	}

	bndcnd(rou,u,v,p,t,q1,q2,q3,q4,g0,rgas,u0,v0,p0,t0,rou0,mx,my,flag,iflag,jflag,otuflag,totuflag);
	calrhs(mx,my,rou,u,v,t,p,q1,q2,q3,q4,rgas,g0,dt,dxx,dyy,ecp,dq1,dq2,dq3,dq4,flag); // step2...k2を計算

	for(i=0;i<mx;i++){		// y = y + 1/2*( k1 + k2 )
		for(j=0;j<my;j++){
			q1[i][j] = qold1[i][j] + 0.5*(dqn1[i][j] + dq1[i][j]);
			q2[i][j] = qold2[i][j] + 0.5*(dqn2[i][j] + dq2[i][j]);
			q3[i][j] = qold3[i][j] + 0.5*(dqn3[i][j] + dq3[i][j]);
			q4[i][j] = qold4[i][j] + 0.5*(dqn4[i][j] + dq4[i][j]);
		}
	}

	bndcnd(rou,u,v,p,t,q1,q2,q3,q4,g0,rgas,u0,v0,p0,t0,rou0,mx,my,flag,iflag,jflag,otuflag,totuflag);

	res = 0.0;
	for(i=0;i<mx;i++){
		for(j=0;j<my;j++){
			memo = (q1[i][j]-qold1[i][j])*(q1[i][j]-qold1[i][j])
			      +(q2[i][j]-qold2[i][j])*(q2[i][j]-qold2[i][j])
			      +(q3[i][j]-qold3[i][j])*(q3[i][j]-qold3[i][j])
			      +(q4[i][j]-qold4[i][j])*(q4[i][j]-qold4[i][j]);
			res += memo;
		}
	}
	res = sqrt(res/grid);

	free_mat(dq1,mx,my),free_mat(dq2,mx,my),free_mat(dq3,mx,my),free_mat(dq4,mx,my);
	free_mat(dqn1,mx,my),free_mat(dqn2,mx,my),free_mat(dqn3,mx,my),free_mat(dqn4,mx,my);
	free_mat(qold1,mx,my),free_mat(qold2,mx,my),free_mat(qold3,mx,my),free_mat(qold4,mx,my);

}
