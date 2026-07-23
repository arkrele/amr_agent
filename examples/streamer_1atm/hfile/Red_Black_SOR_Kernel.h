// find solution via Gauss-Siedel iterations
// SOR (mix of new/old field) to improve convergence
__global__ void Red_Black_SOR_Kernel( 
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
			double omega )
{

	int  i, j, ij;
	double  new_field, old_field;
	double b,c,d,e,f;

	// physical grid coordinates
	i = blockDim.x*blockIdx.x + threadIdx.x;
	j = blockDim.y*blockIdx.y + threadIdx.y;

	// i & j grid point
	ij = point( NY, i, j );

	// odd-even grid points
	if ( (i+j) % 2 != RB_control )return;
	if(d_flag[ij])return;

	// finite difference
	old_field = d_phi[ij];
	
		if(i==0){
			b = 0.5;
			c = d_phi[point(NY,i,j+1)];
			d = 0.0;
			e = d_phi[point(NY,i+1,j)];

			if(j==0)f = 0.0;
			else f = d_phi[point(NY,i,j-1)];
			// j==NY-1 はフラグで除外されるからいらない
		}else if(j==0){
			b = d_rh[ij];
			c = d_phi[point(NY,i,j+1)];
			d = d_phi[point(NY,i-1,j)];

			if(i==NX-1)e = d_phi[point(NY,i,j)];
			else e = d_phi[point(NY,i+1,j)];

			f = 0.0;


		}else if(i==NX-1){
			i=i-1;

			b = d_rh[point(NY,i,j)];
			if(j==NY-1)c = d_phi[point(NY,i,j)];
			else c = d_phi[point(NY,i,j+1)];

			d = d_phi[point(NY,i-1,j)];
			e = d_phi[point(NY,i+1,j)];
			f = d_phi[point(NY,i,j-1)];

		}else if(j==NY-1){
			j=j-1;

			b = d_rh[point(NY,i,j)];
			c = d_phi[point(NY,i,j+1)];
			d = d_phi[point(NY,i-1,j)];
			e = d_phi[point(NY,i+1,j)];
			f = d_phi[point(NY,i,j-1)];
		}else{

			b = d_rh[ij];
			c = d_phi[point(NY,i,j+1)];
			d = d_phi[point(NY,i-1,j)];
			e = d_phi[point(NY,i+1,j)];
			f = d_phi[point(NY,i,j-1)];

		}

		new_field = d_P1[ij] * ( d_rho[ij]
						- c*d_P2[ij] 
							- d*d_P3[ij] 
								- e*d_P4[ij]
									- f*d_P5[ij] );

	// SOR mix of old & new fields
  	d_phi[ij] = (1.0 - omega)*old_field + omega*new_field;

	// calculate residual
	d_temp[ij] = fabs( d_phi[ij] - old_field );

}



__global__ void Error_Kernel( 	
				double *d_phi, 
				double *d_rho,
				double *d_rh, 
				int NX, 
				int NY,
				double *d_P1, 
				double *d_P2, 
				double *d_P3, 
				double *d_P4, 
				double *d_P5,
				int *d_flag, 
				double *d_err	
){

	int  i, j, ij;
	double left;
	double b,c,d,e,f;

	// physical grid coordinates
	i = blockDim.x*blockIdx.x + threadIdx.x;
	j = blockDim.y*blockIdx.y + threadIdx.y;

	// i & j grid point
	ij = point( NY, i, j );

	if(d_flag[ij]){
		d_err[ij] = 0.0;
		return;
	}

		if(i==0){
			b = 0.5;
			c = d_phi[point(NY,i,j+1)];
			d = 0.0;
			e = d_phi[point(NY,i+1,j)];

			if(j==0)f = 0.0;
			else f = d_phi[point(NY,i,j-1)];
			// j==NY-1 はフラグで除外されるからいらない
		}else if(j==0){

			b = d_rh[ij];
			c = d_phi[point(NY,i,j+1)];
			d = d_phi[point(NY,i-1,j)];

			if(i==NX-1)e = d_phi[point(NY,i,j)];
			else e = d_phi[point(NY,i+1,j)];

			f = 0.0;


		}else if(i==NX-1){
			i=i-1;

			b = d_rh[point(NY,i,j)];
			if(j==NY-1)c = d_phi[point(NY,i,j)];
			else c = d_phi[point(NY,i,j+1)];

			d = d_phi[point(NY,i-1,j)];
			e = d_phi[point(NY,i+1,j)];
			f = d_phi[point(NY,i,j-1)];

		}else if(j==NY-1){
			j=j-1;

			b = d_rh[point(NY,i,j)];
			c = d_phi[point(NY,i,j+1)];
			d = d_phi[point(NY,i-1,j)];
			e = d_phi[point(NY,i+1,j)];
			f = d_phi[point(NY,i,j-1)];
		}else{

			b = d_rh[ij];
			c = d_phi[point(NY,i,j+1)];
			d = d_phi[point(NY,i-1,j)];
			e = d_phi[point(NY,i+1,j)];
			f = d_phi[point(NY,i,j-1)];

		}

		left = d_phi[ij]/d_P1[ij] + c*d_P2[ij] + d*d_P3[ij] + e*d_P4[ij]+ f*d_P5[ij];

	// SOR mix of old & new fields
  		d_err[ij] =  d_rho[ij] - left;


}

__global__ void Restriction_Kernel(int NX2, int NY2, int NY, double *d_Cres, double *d_Cres2,double *d_cCphi){

	int  i, j, ij;

	// physical grid coordinates
	i = blockDim.x*blockIdx.x + threadIdx.x;
	j = blockDim.y*blockIdx.y + threadIdx.y;

	// i & j grid point
	ij = point( NY2, i, j );

	d_Cres2[ij] = d_Cres[point( NY, i*2, j*2 )];
	d_cCphi[ij] = 0.0;

}

__global__ void Interporation_kernel(int NX,int NY,int NY2, int *d_flag,double *d_phi,double *dd_phi){

	int i,j,ij;

	// physical grid coordinates
	i = blockDim.x*blockIdx.x + threadIdx.x;
	j = blockDim.y*blockIdx.y + threadIdx.y;

	// i & j grid point
	ij = point( NY, i, j );

	if(d_flag[ij]){
		d_phi[point(NY,i,j)]=0.0;
		return;
	}

	if(i==0){
		if(j%2==0)d_phi[ij] += dd_phi[point( NY2, 0, j/2 )];
		if(j%2!=0)d_phi[ij] += (dd_phi[point( NY2, 0, (j-1)/2 )] 
								+ dd_phi[point( NY2, 0, (j+1)/2 )])/2.0;		
	}else if(j==0){
		if(i%2==0)d_phi[ij] += dd_phi[point( NY2, i/2, 0 )];
		if(i%2!=0)d_phi[ij] += (dd_phi[point( NY2, (i-1)/2, 0 )] 
								+ dd_phi[point( NY2, (i-1)/2, 0 )] )/2.0;
	}else if(i==NX-1){
		i=i-1;
		if(j==NY-1){
			j=j-1;
			if(i%2==0 && j%2==0)d_phi[ij] += dd_phi[point( NY2,i/2,j/2)];

			if(i%2!=0 && j%2==0)d_phi[ij] += (dd_phi[point( NY2, (i-1)/2, j/2 )] 
								+ dd_phi[point( NY2, (i+1)/2, j/2 )] )/2.0;

			if(i%2==0 && j%2!=0)d_phi[ij] += (dd_phi[point( NY2, i/2, (j-1)/2 )] 
								+ dd_phi[point( NY2, i/2, (j+1)/2 )])/2.0;			

			if(i%2!=0 && j%2!=0)d_phi[ij] += (dd_phi[point( NY2, (i-1)/2, (j-1)/2 )] 
								+ dd_phi[point( NY2, (i+1)/2, (j-1)/2 )]
									+ dd_phi[point( NY2, (i-1)/2, (j+1)/2 )]
										+ dd_phi[point( NY2, (i+1)/2, (j+1)/2 )])/4.0;
		}else {
			if(i%2==0 && j%2==0)d_phi[ij] += dd_phi[point( NY2,i/2,j/2)];

			if(i%2!=0 && j%2==0)d_phi[ij] += (dd_phi[point( NY2, (i-1)/2, j/2 )] 
								+ dd_phi[point( NY2, (i+1)/2, j/2 )] )/2.0;

			if(i%2==0 && j%2!=0)d_phi[ij] += (dd_phi[point( NY2, i/2, (j-1)/2 )] 
								+ dd_phi[point( NY2, i/2, (j+1)/2 )])/2.0;			

			if(i%2!=0 && j%2!=0)d_phi[ij] += (dd_phi[point( NY2, (i-1)/2, (j-1)/2 )] 
								+ dd_phi[point( NY2, (i+1)/2, (j-1)/2 )]
									+ dd_phi[point( NY2, (i-1)/2, (j+1)/2 )]
										+ dd_phi[point( NY2, (i+1)/2, (j+1)/2 )])/4.0;
		}
	}else if(j==NY-1){
		j=j-1;
		if(i%2==0 && j%2==0)d_phi[ij] += dd_phi[point( NY2,i/2,j/2)];

		if(i%2!=0 && j%2==0)d_phi[ij] += (dd_phi[point( NY2, (i-1)/2, j/2 )] 
								+ dd_phi[point( NY2, (i+1)/2, j/2 )] )/2.0;

		if(i%2==0 && j%2!=0)d_phi[ij] += (dd_phi[point( NY2, i/2, (j-1)/2 )] 
								+ dd_phi[point( NY2, i/2, (j+1)/2 )])/2.0;			

		if(i%2!=0 && j%2!=0)d_phi[ij] += (dd_phi[point( NY2, (i-1)/2, (j-1)/2 )] 
								+ dd_phi[point( NY2, (i+1)/2, (j-1)/2 )]
									+ dd_phi[point( NY2, (i-1)/2, (j+1)/2 )]
										+ dd_phi[point( NY2, (i+1)/2, (j+1)/2 )])/4.0;
	} else{

		if(i%2==0 && j%2==0)d_phi[ij] += dd_phi[point( NY2,i/2,j/2)];

		if(i%2!=0 && j%2==0)d_phi[ij] += (dd_phi[point( NY2, (i-1)/2, j/2 )] 
								+ dd_phi[point( NY2, (i+1)/2, j/2 )] )/2.0;

		if(i%2==0 && j%2!=0)d_phi[ij] += (dd_phi[point( NY2, i/2, (j-1)/2 )] 
								+ dd_phi[point( NY2, i/2, (j+1)/2 )])/2.0;			

		if(i%2!=0 && j%2!=0)d_phi[ij] += (dd_phi[point( NY2, (i-1)/2, (j-1)/2 )] 
								+ dd_phi[point( NY2, (i+1)/2, (j-1)/2 )]
									+ dd_phi[point( NY2, (i-1)/2, (j+1)/2 )]
										+ dd_phi[point( NY2, (i+1)/2, (j+1)/2 )])/4.0;
	}


}
