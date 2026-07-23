/*ガウスジョルダン法   a(n*n),b(n*m)の行列を入力すると、
                          逆行列がa(n*n)に、解がb(n*m)に格納される
       a[ |(列) の数][ ―（行） の数]*/

#define SWAP(a,b) {temp=(a);(a)=(b);(b)=temp;}

void gaussj(double **a, int n, double **b, int m)
{
  int *indxc, *indxr, *ipiv;
  int i,icol, irow, j,k,l,ll;
  double big, dum, pivinv, temp;

  indxc=ivec(n);
  indxr=ivec(n);
  ipiv=ivec(n);
  for(j=0;j<n;j++) ipiv[j]=0;

  for(i=0;i<n;i++){
    big=0.0;
    for(j=0;j<n;j++)
      if(ipiv[j] != 1)
	for(k=0;k<n;k++){
	  if(ipiv[k] == 0){
	    if(fabs(a[j][k]) >= big){
	      big=fabs(a[j][k]);
	      irow=j;
	      icol=k;
	    }
	  } else if(ipiv[k] > 1) printf("gauusju;error\n");
	}
    ++(ipiv[icol]);     

    if (irow != icol){
      for(l=0;l<n;l++) SWAP(a[irow][l],a[icol][l]);
      for(l=0;l<m;l++) SWAP(b[irow][l],b[icol][l]);
    }
    indxr[i]=irow;
    indxc[i]=icol;
    if(a[icol][icol] == 0.0)printf("%d;gauusj;error2\n",icol);
    pivinv=1.0/a[icol][icol];
    a[icol][icol]=1.0;
    for(l=0;l<n;l++) a[icol][l] *= pivinv;
    for(l=0;l<m;l++) b[icol][l] *= pivinv;
    for(ll=0;ll<n;ll++)

      if(ll != icol){
	dum = a[ll][icol];
	a[ll][icol]=0.0;
	for(l=0;l<n;l++) a[ll][l] -= a[icol][l]*dum;
	for(l=0;l<m;l++) b[ll][l] -= b[icol][l]*dum;
      }
  }
  for(l=n-1;l>=0;l--){
    if(indxr[l] != indxc[l])
      for(k=0;k<n;k++)
	SWAP(a[k][indxr[l]],a[k][indxc[l]]);
  }
  free_ivec(ipiv,n);
  free_ivec(indxc,n);
  free_ivec(indxr,n);
}
