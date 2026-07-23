// find solution via Gauss-Siedel iterations
// SOR (mix of new/old field) to improve convergence
__global__ void Helmholtz_Kernel0(
	double *d_phi, 
	double *d_rho, 
	double *d_rh,
	double *d_temp, 
	int RB_control, 
	int NX, 
	int NY,
	double *d_P1, 
	double *d_P2, 
	double *d_P3, 
	double *d_P4, 
	double *d_P5,
	int *d_flag, 
	int *d_iflag, 
	int *d_jflag, 
	int *d_oflag,
	double omega,
	double ppO2)
{

	int  i, j, ij,ijb;
	double new_field, old_field;
	double pO2=760.0*ppO2;
	double a,b,c,d,e,f;
	double g,A;

//	__shared__ double d_A[3];
//	__shared__ double d_g[3];

	// physical grid coordinates
	i = blockDim.x*blockIdx.x + threadIdx.x;
	j = blockDim.y*blockIdx.y + threadIdx.y;

	// i & j grid point
	ij = point( NY, i, j );

	// odd-even grid points
	if ( (i+j) % 2 != RB_control ) return;
	if(d_flag[ij])return;

/*
	d_A[0]=1.986e-4*1e4;//m^-2 Torr^-2
	d_A[1]=0.0051  *1e4;//m^-2 Torr^-2
	d_A[2]=0.4886  *1e4;//m^-2 Torr^-2

	d_g[0]=0.0553  *1e2; //in m^-1 Torr^-1
	d_g[1]=0.1460  *1e2; //in m^-1 Torr^-1
	d_g[2]=0.89    *1e2; //in m^-1 Torr^-1	
*/
	A=1.986e-4*1e4;
	g=0.0553  *1e2;

	// finite difference
	old_field = d_phi[ij];


		if(i==0){

			b = 0.5;
			d = d_P3[ij]*0.0;
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];

			if(j==0){
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				f = d_P5[ij]*0.0;
			} else if(d_jflag[ij]){
				c = d_P2[ij]*d_phi[ij];
				f = d_P5[ij]*d_phi[point(NY,i,j-1)];
			} else {
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				f = d_P5[ij]*d_phi[point(NY,i,j-1)];
			}

		}else if(j==0){

			if(i==NX-1){
				i=i-1;
				b = d_rh[ij];
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				d = d_P3[ij]*d_phi[point(NY,i-1,j)];
				e = d_P4[ij]*d_phi[point(NY,i+1,j)];
				f = d_P5[ij]*d_phi[point(NY,i,j)];
			}else{
				b = d_rh[ij];
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				d = d_P3[ij]*d_phi[point(NY,i-1,j)];
				e = d_P4[ij]*d_phi[point(NY,i+1,j)];
				f = d_P5[ij]*d_phi[point(NY,i,j)];
			}

		}else if(i==NX-1){
			
			if(j==NY-1){
				i=i-1;
				j=j-1;
				ijb=point( NY, i, j );

				b = d_rh[ijb];
				c = d_P2[ijb]*d_phi[point(NY,i,j+1)];
				d = d_P3[ijb]*d_phi[point(NY,i-1,j)];
				e = d_P4[ijb]*d_phi[point(NY,i+1,j)];
				f = d_P5[ijb]*d_phi[point(NY,i,j-1)];
			}else{
				i=i-1;
				ijb=point( NY, i, j );

				b = d_rh[ijb];
				c = d_P2[ijb]*d_phi[point(NY,i,j+1)];
				d = d_P3[ijb]*d_phi[point(NY,i-1,j)];
				e = d_P4[ijb]*d_phi[point(NY,i+1,j)];
				f = d_P5[ijb]*d_phi[point(NY,i,j-1)];
			}

		}else if(j==NY-1){
			j=j-1;
			ijb=point( NY, i, j );

			b = d_rh[ij];
			c = d_P2[ijb]*d_phi[point(NY,i,j+1)];
			d = d_P3[ijb]*d_phi[point(NY,i-1,j)];
			e = d_P4[ijb]*d_phi[point(NY,i+1,j)];
			f = d_P5[ijb]*d_phi[point(NY,i,j-1)];

		}else if(d_jflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[ij];
			d = d_P3[ij]*d_phi[point(NY,i-1,j)];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else if(d_iflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[point(NY,i,j+1)];
			d = d_P3[ij]*d_phi[ij];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else if(d_oflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[ij];
			d = d_P3[ij]*d_phi[ij];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else{

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[point(NY,i,j+1)];
			d = d_P3[ij]*d_phi[point(NY,i-1,j)];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}

//	a = -(pow(d_g[pnum]*pO2,2)*d_phi[ij] - d_rho[ij]*pow(pO2,2)*d_A[pnum]);
	a = -(pow(g*pO2,2)*d_phi[ij] - d_rho[ij]*pow(pO2,2)*A);

	new_field = d_P1[ij] * ( a*b - c - d - e - f );


	// SOR mix of old & new fields
  	d_phi[ij] = (1.0f-omega)*old_field + omega*new_field;
	
	// calculate residual
	d_temp[ij] = fabs( d_phi[ij] - old_field );

}


// find solution via Gauss-Siedel iterations
// SOR (mix of new/old field) to improve convergence
__global__ void Helmholtz_Kernel1(
	double *d_phi, 
	double *d_rho, 
	double *d_rh,
	double *d_temp, 
	int RB_control, 
	int NX, 
	int NY,
	double *d_P1, 
	double *d_P2, 
	double *d_P3, 
	double *d_P4, 
	double *d_P5,
	int *d_flag, 
	int *d_iflag, 
	int *d_jflag, 
	int *d_oflag,
	double omega,
	double ppO2)
{
	int  i, j, ij,ijb;
	double new_field, old_field;
	double pO2=760.0*ppO2;
	double a,b,c,d,e,f;
	double g,A;

//	__shared__ double d_A[3];
//	__shared__ double d_g[3];

	// physical grid coordinates
	i = blockDim.x*blockIdx.x + threadIdx.x;
	j = blockDim.y*blockIdx.y + threadIdx.y;

	// i & j grid point
	ij = point( NY, i, j );

	// odd-even grid points
	if ( (i+j) % 2 != RB_control ) return;
	if(d_flag[ij])return;

/*
	d_A[0]=1.986e-4*1e4;//m^-2 Torr^-2
	d_A[1]=0.0051  *1e4;//m^-2 Torr^-2
	d_A[2]=0.4886  *1e4;//m^-2 Torr^-2

	d_g[0]=0.0553  *1e2; //in m^-1 Torr^-1
	d_g[1]=0.1460  *1e2; //in m^-1 Torr^-1
	d_g[2]=0.89    *1e2; //in m^-1 Torr^-1	
*/

	A=0.0051  *1e4;
	g=0.1460  *1e2;

	// finite difference
	old_field = d_phi[ij];


		if(i==0){

			b = 0.5;
			d = d_P3[ij]*0.0;
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];

			if(j==0){
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				f = d_P5[ij]*0.0;
			} else if(d_jflag[ij]){
				c = d_P2[ij]*d_phi[ij];
				f = d_P5[ij]*d_phi[point(NY,i,j-1)];
			} else {
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				f = d_P5[ij]*d_phi[point(NY,i,j-1)];
			}

		}else if(j==0){

			if(i==NX-1){
				i=i-1;
				b = d_rh[ij];
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				d = d_P3[ij]*d_phi[point(NY,i-1,j)];
				e = d_P4[ij]*d_phi[point(NY,i+1,j)];
				f = d_P5[ij]*d_phi[point(NY,i,j)];
			}else{
				b = d_rh[ij];
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				d = d_P3[ij]*d_phi[point(NY,i-1,j)];
				e = d_P4[ij]*d_phi[point(NY,i+1,j)];
				f = d_P5[ij]*d_phi[point(NY,i,j)];
			}

		}else if(i==NX-1){
			
			if(j==NY-1){
				i=i-1;
				j=j-1;
				ijb=point( NY, i, j );

				b = d_rh[ijb];
				c = d_P2[ijb]*d_phi[point(NY,i,j+1)];
				d = d_P3[ijb]*d_phi[point(NY,i-1,j)];
				e = d_P4[ijb]*d_phi[point(NY,i+1,j)];
				f = d_P5[ijb]*d_phi[point(NY,i,j-1)];
			}else{
				i=i-1;
				ijb=point( NY, i, j );

				b = d_rh[ijb];
				c = d_P2[ijb]*d_phi[point(NY,i,j+1)];
				d = d_P3[ijb]*d_phi[point(NY,i-1,j)];
				e = d_P4[ijb]*d_phi[point(NY,i+1,j)];
				f = d_P5[ijb]*d_phi[point(NY,i,j-1)];
			}

		}else if(j==NY-1){
			j=j-1;
			ijb=point( NY, i, j );

			b = d_rh[ij];
			c = d_P2[ijb]*d_phi[point(NY,i,j+1)];
			d = d_P3[ijb]*d_phi[point(NY,i-1,j)];
			e = d_P4[ijb]*d_phi[point(NY,i+1,j)];
			f = d_P5[ijb]*d_phi[point(NY,i,j-1)];

		}else if(d_jflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[ij];
			d = d_P3[ij]*d_phi[point(NY,i-1,j)];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else if(d_iflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[point(NY,i,j+1)];
			d = d_P3[ij]*d_phi[ij];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else if(d_oflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[ij];
			d = d_P3[ij]*d_phi[ij];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else{

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[point(NY,i,j+1)];
			d = d_P3[ij]*d_phi[point(NY,i-1,j)];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}

//	a = -(pow(d_g[pnum]*pO2,2)*d_phi[ij] - d_rho[ij]*pow(pO2,2)*d_A[pnum]);
	a = -(pow(g*pO2,2)*d_phi[ij] - d_rho[ij]*pow(pO2,2)*A);

	new_field = d_P1[ij] * ( a*b - c - d - e - f );


	// SOR mix of old & new fields
  	d_phi[ij] = (1.0f-omega)*old_field + omega*new_field;
	
	// calculate residual
	d_temp[ij] = fabs( d_phi[ij] - old_field );

}


// find solution via Gauss-Siedel iterations
// SOR (mix of new/old field) to improve convergence
__global__ void Helmholtz_Kernel2(
	double *d_phi, 
	double *d_rho, 
	double *d_rh,
	double *d_temp, 
	int RB_control, 
	int NX, 
	int NY,
	double *d_P1, 
	double *d_P2, 
	double *d_P3, 
	double *d_P4, 
	double *d_P5,
	int *d_flag, 
	int *d_iflag, 
	int *d_jflag, 
	int *d_oflag,
	double omega,
	double ppO2)
{
	int  i, j, ij,ijb;
	double new_field, old_field;
	double pO2=760.0*ppO2;
	double a,b,c,d,e,f;
	double g,A;

//	__shared__ double d_A[3];
//	__shared__ double d_g[3];

	// physical grid coordinates
	i = blockDim.x*blockIdx.x + threadIdx.x;
	j = blockDim.y*blockIdx.y + threadIdx.y;

	// i & j grid point
	ij = point( NY, i, j );

	// odd-even grid points
	if ( (i+j) % 2 != RB_control ) return;
	if(d_flag[ij])return;

/*
	d_A[0]=1.986e-4*1e4;//m^-2 Torr^-2
	d_A[1]=0.0051  *1e4;//m^-2 Torr^-2
	d_A[2]=0.4886  *1e4;//m^-2 Torr^-2

	d_g[0]=0.0553  *1e2; //in m^-1 Torr^-1
	d_g[1]=0.1460  *1e2; //in m^-1 Torr^-1
	d_g[2]=0.89    *1e2; //in m^-1 Torr^-1	
*/

	A=0.4886  *1e4;
	g=0.89    *1e2;

	// finite difference
	old_field = d_phi[ij];


		if(i==0){

			b = 0.5;
			d = d_P3[ij]*0.0;
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];

			if(j==0){
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				f = d_P5[ij]*0.0;
			} else if(d_jflag[ij]){
				c = d_P2[ij]*d_phi[ij];
				f = d_P5[ij]*d_phi[point(NY,i,j-1)];
			} else {
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				f = d_P5[ij]*d_phi[point(NY,i,j-1)];
			}

		}else if(j==0){

			if(i==NX-1){
				i=i-1;
				b = d_rh[ij];
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				d = d_P3[ij]*d_phi[point(NY,i-1,j)];
				e = d_P4[ij]*d_phi[point(NY,i+1,j)];
				f = d_P5[ij]*d_phi[point(NY,i,j)];
			}else{
				b = d_rh[ij];
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				d = d_P3[ij]*d_phi[point(NY,i-1,j)];
				e = d_P4[ij]*d_phi[point(NY,i+1,j)];
				f = d_P5[ij]*d_phi[point(NY,i,j)];
			}

		}else if(i==NX-1){
			
			if(j==NY-1){
				i=i-1;
				j=j-1;
				ijb=point( NY, i, j );

				b = d_rh[ijb];
				c = d_P2[ijb]*d_phi[point(NY,i,j+1)];
				d = d_P3[ijb]*d_phi[point(NY,i-1,j)];
				e = d_P4[ijb]*d_phi[point(NY,i+1,j)];
				f = d_P5[ijb]*d_phi[point(NY,i,j-1)];
			}else{
				i=i-1;
				ijb=point( NY, i, j );

				b = d_rh[ijb];
				c = d_P2[ijb]*d_phi[point(NY,i,j+1)];
				d = d_P3[ijb]*d_phi[point(NY,i-1,j)];
				e = d_P4[ijb]*d_phi[point(NY,i+1,j)];
				f = d_P5[ijb]*d_phi[point(NY,i,j-1)];
			}

		}else if(j==NY-1){
			j=j-1;
			ijb=point( NY, i, j );

			b = d_rh[ij];
			c = d_P2[ijb]*d_phi[point(NY,i,j+1)];
			d = d_P3[ijb]*d_phi[point(NY,i-1,j)];
			e = d_P4[ijb]*d_phi[point(NY,i+1,j)];
			f = d_P5[ijb]*d_phi[point(NY,i,j-1)];

		}else if(d_jflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[ij];
			d = d_P3[ij]*d_phi[point(NY,i-1,j)];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else if(d_iflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[point(NY,i,j+1)];
			d = d_P3[ij]*d_phi[ij];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else if(d_oflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[ij];
			d = d_P3[ij]*d_phi[ij];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else{

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[point(NY,i,j+1)];
			d = d_P3[ij]*d_phi[point(NY,i-1,j)];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}

//	a = -(pow(d_g[pnum]*pO2,2)*d_phi[ij] - d_rho[ij]*pow(pO2,2)*d_A[pnum]);
	a = -(pow(g*pO2,2)*d_phi[ij] - d_rho[ij]*pow(pO2,2)*A);

	new_field = d_P1[ij] * ( a*b - c - d - e - f );


	// SOR mix of old & new fields
  	d_phi[ij] = (1.0f-omega)*old_field + omega*new_field;
	
	// calculate residual
	d_temp[ij] = fabs( d_phi[ij] - old_field );

}

__global__ void Helm_Error_Kernel( 
	double *d_phi, 
	double *d_rho, 
	int NX, 
	int NY, 
	double *d_rh,
	double *d_P1,
	double *d_P2, 
	double *d_P3, 
	double *d_P4, 
	double *d_P5, 
	int *d_flag, 
	int *d_iflag, 
	int *d_jflag, 
	int *d_oflag , 
	double *d_err,
	int pnum,
	double ppO2)
{

	int  i, j, ij,ijb;
	double left;
	double pO2=760.0*ppO2;
	double a,b,c,d,e,f;
	double g,A;

//	__shared__ double d_A[3];
//	__shared__ double d_g[3];

	// physical grid coordinates
	i = blockDim.x*blockIdx.x + threadIdx.x;
	j = blockDim.y*blockIdx.y + threadIdx.y;

	// i & j grid point
	ij = point( NY, i, j );

	if(d_flag[ij]){
		d_err[ij] = 0.0;
		return;
	}
/*
	d_A[0]=1.986e-4*1e4;//m^-2 Torr^-2
	d_A[1]=0.0051  *1e4;//m^-2 Torr^-2
	d_A[2]=0.4886  *1e4;//m^-2 Torr^-2

	d_g[0]=0.0553  *1e2; //in m^-1 Torr^-1
	d_g[1]=0.1460  *1e2; //in m^-1 Torr^-1
	d_g[2]=0.89    *1e2; //in m^-1 Torr^-1	
*/
	if(pnum==0){
		A=1.986e-4*1e4;
		g=0.0553  *1e2;
	}else if (pnum==1){
		A=0.0051  *1e4;
		g=0.1460  *1e2;
	}else if (pnum==2){
		A=0.4886  *1e4;
		g=0.89    *1e2;
	}

		if(i==0){

			b = 0.5;
			d = d_P3[ij]*0.0;
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];

			if(j==0){
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				f = d_P5[ij]*0.0;
			} else if(d_jflag[ij]){
				c = d_P2[ij]*d_phi[ij];
				f = d_P5[ij]*d_phi[point(NY,i,j-1)];
			} else {
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				f = d_P5[ij]*d_phi[point(NY,i,j-1)];
			}

		}else if(j==0){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[point(NY,i,j+1)];
			d = d_P3[ij]*d_phi[point(NY,i-1,j)];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j)];

		}else if(i==NX-1){
			i=i-1;
			ijb=point( NY, i, j );

			b = d_rh[ijb];
			c = d_P2[ijb]*d_phi[point(NY,i,j+1)];
			d = d_P3[ijb]*d_phi[point(NY,i-1,j)];
			e = d_P4[ijb]*d_phi[point(NY,i+1,j)];
			f = d_P5[ijb]*d_phi[point(NY,i,j-1)];

		}else if(j==NY-1){
			j=j-1;
			ijb=point( NY, i, j );

			b = d_rh[ij];
			c = d_P2[ijb]*d_phi[point(NY,i,j+1)];
			d = d_P3[ijb]*d_phi[point(NY,i-1,j)];
			e = d_P4[ijb]*d_phi[point(NY,i+1,j)];
			f = d_P5[ijb]*d_phi[point(NY,i,j-1)];

		}else if(d_jflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[ij];
			d = d_P3[ij]*d_phi[point(NY,i-1,j)];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else if(d_iflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[point(NY,i,j+1)];
			d = d_P3[ij]*d_phi[ij];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else if(d_oflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[ij];
			d = d_P3[ij]*d_phi[ij];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else{

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[point(NY,i,j+1)];
			d = d_P3[ij]*d_phi[point(NY,i-1,j)];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}

//	a = -(pow(d_g[pnum]*pO2,2)*d_phi[ij] - d_rho[ij]*pow(pO2,2)*d_A[pnum]);
	a = -(pow(g*pO2,2)*d_phi[ij] - d_rho[ij]*pow(pO2,2)*A);

//	left = d_P1[ij] * ( a*b - c - d - e - f );
	left = d_phi[ij]/d_P1[ij] + c + d + e + f;

	d_err[ij] = a*b -left;
}




// find solution via Gauss-Siedel iterations
// SOR (mix of new/old field) to improve convergence
__global__ void multi_Helmholtz_Kernel(
	double *d_phi, 
	double *d_rho, 
	double *d_rh,
	int RB_control, 
	int NX, 
	int NY,
	double *d_P1, 
	double *d_P2, 
	double *d_P3, 
	double *d_P4, 
	double *d_P5,
	int *d_flag, 
	int *d_iflag, 
	int *d_jflag, 
	int *d_oflag,
	double omega)
{

	int  i, j, ij,ijb;
	double new_field, old_field;
	double a,b,c,d,e,f;

	// physical grid coordinates
	i = blockDim.x*blockIdx.x + threadIdx.x;
	j = blockDim.y*blockIdx.y + threadIdx.y;

	// i & j grid point
	ij = point( NY, i, j );

	// odd-even grid points
	if ( (i+j) % 2 != RB_control ) return;
	if(d_flag[ij])return;

	// finite difference
	old_field = d_phi[ij];


		if(i==0){

			b = 0.5;
			d = d_P3[ij]*0.0;
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];

			if(j==0){
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				f = d_P5[ij]*0.0;
			} else if(d_jflag[ij]){
				c = d_P2[ij]*d_phi[ij];
				f = d_P5[ij]*d_phi[point(NY,i,j-1)];
			} else {
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				f = d_P5[ij]*d_phi[point(NY,i,j-1)];
			}

		}else if(j==0){
			if(i==NX-1){
				i=i-1;
				b = d_rh[ij];
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				d = d_P3[ij]*d_phi[point(NY,i-1,j)];
				e = d_P4[ij]*d_phi[point(NY,i+1,j)];
				f = d_P5[ij]*d_phi[point(NY,i,j)];
			}else{
				b = d_rh[ij];
				c = d_P2[ij]*d_phi[point(NY,i,j+1)];
				d = d_P3[ij]*d_phi[point(NY,i-1,j)];
				e = d_P4[ij]*d_phi[point(NY,i+1,j)];
				f = d_P5[ij]*d_phi[point(NY,i,j)];
			}

		}else if(i==NX-1){

			if(j==NY-1){
				j=j-1;
				i=i-1;
				ijb=point( NY, i, j );

				b = d_rh[ijb];
				c = d_P2[ijb]*d_phi[point(NY,i,j+1)];
				d = d_P3[ijb]*d_phi[point(NY,i-1,j)];
				e = d_P4[ijb]*d_phi[point(NY,i+1,j)];
				f = d_P5[ijb]*d_phi[point(NY,i,j-1)];
			}else{
				i=i-1;
				ijb=point( NY, i, j );

				b = d_rh[ijb];
				c = d_P2[ijb]*d_phi[point(NY,i,j+1)];
				d = d_P3[ijb]*d_phi[point(NY,i-1,j)];
				e = d_P4[ijb]*d_phi[point(NY,i+1,j)];
				f = d_P5[ijb]*d_phi[point(NY,i,j-1)];
			}

		}else if(j==NY-1){
			j=j-1;
			ijb=point( NY, i, j );

			b = d_rh[ij];
			c = d_P2[ijb]*d_phi[point(NY,i,j+1)];
			d = d_P3[ijb]*d_phi[point(NY,i-1,j)];
			e = d_P4[ijb]*d_phi[point(NY,i+1,j)];
			f = d_P5[ijb]*d_phi[point(NY,i,j-1)];

		}else if(d_jflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[ij];
			d = d_P3[ij]*d_phi[point(NY,i-1,j)];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else if(d_iflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[point(NY,i,j+1)];
			d = d_P3[ij]*d_phi[ij];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else if(d_oflag[ij]){

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[ij];
			d = d_P3[ij]*d_phi[ij];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}else{

			b = d_rh[ij];
			c = d_P2[ij]*d_phi[point(NY,i,j+1)];
			d = d_P3[ij]*d_phi[point(NY,i-1,j)];
			e = d_P4[ij]*d_phi[point(NY,i+1,j)];
			f = d_P5[ij]*d_phi[point(NY,i,j-1)];

		}

//	a = -(pow(d_g[pnum]*pO2,2)*d_phi[ij] - d_rho[ij]*pow(pO2,2)*d_A[pnum]);
	a = d_rho[ij];

	new_field = d_P1[ij] * ( a - c - d - e - f );


	// SOR mix of old & new fields
  	d_phi[ij] = (1.0f-omega)*old_field + omega*new_field;

}
