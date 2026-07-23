void Poisson_GPU_function(
	dim3 dimGrid, 
	dim3 dimBlock, 
	double *d_phi, 
	double *d_rho, 
	double *d_rh, 
	double *d_temp, 
	int NX, 
	int NY, 
	double *d_P1, 
	double *d_P2, 
	double *d_P3, 
	double *d_P4, 
	double *d_P5,
	int *d_flag, 
	double OMEGA, 
	int itnum)
{

	int it;

	// Gauss-Siedel-SOR
	for ( it=0;it<itnum ; it++ ){

		// odd sites (Red Point)
		Red_Black_SOR_Kernel<<< dimGrid, dimBlock >>>
			( d_phi, d_rho, d_rh, d_temp, 0, NX, NY, 
				d_P1,d_P2,d_P3,d_P4,d_P5,d_flag,OMEGA);
		cudaThreadSynchronize();// sync the threads

		// even sites (Black Point)
		Red_Black_SOR_Kernel<<< dimGrid, dimBlock >>>
			( d_phi, d_rho, d_rh, d_temp, 1, NX, NY, 
				d_P1,d_P2,d_P3,d_P4,d_P5,d_flag,OMEGA);
		cudaThreadSynchronize();// sync the threads

	}

}

void Error_poisson_GPU(dim3 dimGrid, dim3 dimBlock,
			double *d_phi, double *d_rho, double *d_rh, int NX, int NY,
				double *d_P1, double *d_P2, double *d_P3, double *d_P4, double *d_P5,
					int *d_flag, double *d_err){

	Error_Kernel<<< dimGrid, dimBlock >>>
		( d_phi, d_rho, d_rh, NX, NY, d_P1,d_P2,d_P3,d_P4,d_P5,d_flag,d_err);
	cudaThreadSynchronize();

}

void Ristriction_GPU(dim3 dimGrid2, dim3 dimBlock2,
			int NX2, int NY2, int NY, double *d_err, double *dd_rho, double *dd_Cphi){

	Restriction_Kernel<<< dimGrid2, dimBlock2 >>>
				(NX2,NY2,NY, d_err, dd_rho,dd_Cphi);
	cudaThreadSynchronize();

}

void Interporation_GPU(dim3 dimGrid, dim3 dimBlock, int NX, int NY, int NY2,
			int *d_flag, double *d_phi, double *dd_Cphi){

		Interporation_kernel<<< dimGrid, dimBlock >>>
					(NX,NY,NY2,d_flag,d_phi,dd_Cphi);
		cudaThreadSynchronize();

}

void Convergence_check_GPU(int N,double *d_temp, double *d_phi, double mf,
				double *temp, double *pphi,double *error, double *Maxphi){

	int i;

	cudaMemcpy( temp, d_temp, mf, cudaMemcpyDeviceToHost );
	cudaMemcpy( pphi, d_phi, mf, cudaMemcpyDeviceToHost );

	(*error) = (*Maxphi) = 0.0;

	for ( i=0; i<N; i++ ){
		if ( temp[i] > (*error) )(*error) = temp[i]; 
		//tempÇ…ÇÕ1ÉãÅ[ÉvëOÇÃílÇ∆ÇÃç∑Çäiî[
		//errorÇ…ÇÕtempÇÃç≈ëÂílÇ™ì¸ÇÈ
		if((*Maxphi)<fabs(pphi[i]))(*Maxphi)=fabs(pphi[i]);
		//MaxphiÇÕpphiÇÃç≈ëÂílÇ™ì¸ÇÈ(errorÇÃãKäiâªÇÃÇΩÇﬂ)

	}
}

void Error_Helm_GPU(
	dim3 dimGrid, 
	dim3 dimBlock,
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
	int *d_iflag,
	int *d_jflag, 
	int *d_oflag,  
	double *d_err,
	int pnum,
	double pO2)
{

	Helm_Error_Kernel<<< dimGrid, dimBlock >>>
			( d_phi, d_rho, NX, NY, d_rh, d_P1, d_P2, d_P3, d_P4, d_P5, 
					d_flag, d_iflag, d_jflag, d_oflag , d_err, pnum,pO2);
	cudaThreadSynchronize();

}

void Helmholtz0_GPU_function(
	dim3 dimGrid, 
	dim3 dimBlock, 
	double *d_phi, 
	double *d_rho, 
	double *d_rh, 
	double *d_temp, 
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
	double OMEGA, 
	int itnum,
	double pO2)
{

	int it;

		// Gauss-Siedel-SOR
		for ( it=0;it<itnum ; it++ ){

			Helmholtz_Kernel0<<< dimGrid, dimBlock >>>
				(d_phi,d_rho, d_rh,
					d_temp, 1, NX,  NY,d_P1,d_P2, d_P3, d_P4, d_P5,
					d_flag, d_iflag, d_jflag, d_oflag, OMEGA,pO2);

			cudaThreadSynchronize();// sync the threads

			Helmholtz_Kernel0<<< dimGrid, dimBlock >>>
				(d_phi,d_rho, d_rh,
					d_temp,0, NX,  NY,d_P1,d_P2, d_P3, d_P4, d_P5,
					d_flag, d_iflag, d_jflag, d_oflag, OMEGA,pO2);
                                              
			cudaThreadSynchronize();// sync the threads
		}

}

void Helmholtz1_GPU_function(
	dim3 dimGrid, 
	dim3 dimBlock, 
	double *d_phi, 
	double *d_rho, 
	double *d_rh, 
	double *d_temp, 
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
	double OMEGA, 
	int itnum,
	double pO2)
{

	int it;

		// Gauss-Siedel-SOR
		for ( it=0;it<itnum ; it++ ){

			Helmholtz_Kernel1<<< dimGrid, dimBlock >>>
				(d_phi,d_rho, d_rh,
					d_temp, 0, NX,  NY,d_P1,d_P2, d_P3, d_P4, d_P5,
					d_flag, d_iflag, d_jflag, d_oflag, OMEGA,pO2);

			cudaThreadSynchronize();// sync the threads

			Helmholtz_Kernel1<<< dimGrid, dimBlock >>>
				(d_phi,d_rho, d_rh,
					d_temp,1, NX,  NY,d_P1,d_P2, d_P3, d_P4, d_P5,
					d_flag, d_iflag, d_jflag, d_oflag, OMEGA,pO2);
                                              
			cudaThreadSynchronize();// sync the threads
		}

}

void Helmholtz2_GPU_function(
	dim3 dimGrid, 
	dim3 dimBlock, 
	double *d_phi, 
	double *d_rho, 
	double *d_rh, 
	double *d_temp, 
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
	double OMEGA, 
	int itnum,
	double pO2)
{

	int it;

		// Gauss-Siedel-SOR
		for ( it=0;it<itnum ; it++ ){

			Helmholtz_Kernel2<<< dimGrid, dimBlock >>>
				(d_phi,d_rho, d_rh,
					d_temp, 0, NX,  NY,d_P1,d_P2, d_P3, d_P4, d_P5,
					d_flag, d_iflag, d_jflag, d_oflag, OMEGA,pO2);

			cudaThreadSynchronize();// sync the threads

			Helmholtz_Kernel2<<< dimGrid, dimBlock >>>
				(d_phi,d_rho, d_rh,
					d_temp,1, NX,  NY,d_P1,d_P2, d_P3, d_P4, d_P5,
					d_flag, d_iflag, d_jflag, d_oflag, OMEGA,pO2);
                                              
			cudaThreadSynchronize();// sync the threads
		}

}

void Multi_Helmholtz_GPU_function(
	dim3 dimGrid, 
	dim3 dimBlock, 
	double *d_phi, 
	double *d_rho, 
	double *d_rh, 
	double *d_temp, 
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
	double OMEGA, 
	int itnum)
{

	int it;

		// Gauss-Siedel-SOR
		// Gauss-Siedel-SOR
		for ( it=0;it<itnum ; it++ ){

			multi_Helmholtz_Kernel<<< dimGrid, dimBlock >>>
				(d_phi,d_rho, d_rh,
					0, NX,  NY,d_P1,d_P2, d_P3, d_P4, d_P5
						,d_flag, d_iflag, d_jflag, d_oflag, 1.8);

			cudaThreadSynchronize();// sync the threads

			multi_Helmholtz_Kernel<<< dimGrid, dimBlock >>>
				(d_phi,d_rho, d_rh,
					1, NX,  NY,d_P1,d_P2, d_P3, d_P4, d_P5
						,d_flag, d_iflag, d_jflag, d_oflag, 1.8);

			cudaThreadSynchronize();// sync the threads
		}

}







