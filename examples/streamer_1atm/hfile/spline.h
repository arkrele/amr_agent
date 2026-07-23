/*
 *  spline --
 *    関数 x[], y[] (配列 n) の 自然 3 次スプライン補間を行い、
 *    2 階導関数 y2[] を返す。最初に 1 度 spline() を呼び出して y2[] を求め、
 *    あとは splint() を呼び出して x[], y[], y2[] から任意の点 x の値を
 *    求める。
 *    x[0] < x[1] < ... < x[n-1] を満たす必要あり。
 *    自然 3 次スプライン: 両端の点の2階導関数を 0 とおく
 */

double sp6=0.1666667;

void spline(double x[], double y[], int n, double y2[])
{
  int i;
  double p, sig, *u;

  u = vec(n-1);
  y2[0] = u[0] = 0.0;

//for(i=0;i<n;i++)printf("%e\t%e\t%e\n",x[i],y[i],y2[i]);
//exit(0);

  for (i = 1; i < n-1; i++) {
    sig = (x[i]-x[i-1])/(x[i+1]-x[i-1]);
    p = sig*y2[i-1] + 2.0;
    y2[i] = (sig - 1.0)/p;

    u[i] = (y[i+1]-y[i])/(x[i+1]-x[i]) - (y[i]-y[i-1])/(x[i]-x[i-1]);
    u[i] = (6.0*u[i]/(x[i+1]-x[i-1])-sig*u[i-1])/p;
  }


  y2[n-1] = 0.0;
  for (i = n-2; i >= 0; i--)y2[i] = y2[i]*y2[i+1] + u[i];


  free_vec(u, n-1);
}

/*
 *  splint --
 *    関数 xa[], ya[] (配列 n) および spline() で求めた 2 階導関数 y2a[] から、
 *    任意の点 x における補間値 y を求める。
 *
 *    Numerical Recipes in C" p.104 の式 (3.3.3)(3.3.4)より y を求める。
 *    すなわち x[j] < x < x[j+1] とすると、
 *    A, B, x[j], x[j+1], y[j], y[j+1] から y が得られる。
 */
void splint(double xa[], double ya[], double y2a[], int n, double x, double *y)
{
	int k, klo, khi;
	double inv_h, h, b, a;

  /* x[klo] < x < x[khi] なる klo と khi を求める */
	klo = 0;
	khi = n - 1;
	while (khi - klo > 1) {
		k = (khi + klo) >> 1;
		if (xa[k] > x)khi = k;
		else klo = k;
	}

  /* y = Ay[j] + By[j+1] + {(A^3 - A)y"[j] + (B^3 - B)y"[j+1]}*(x[j+1]-x[j])^2/6 を求める */
	h = xa[khi] - xa[klo];
	if (h == 0.0)fprintf(stderr, "Bad xa input to routine splint\n");

	inv_h = 1.0/h;

	a = (xa[khi] - x)*inv_h;
	b = (x - xa[klo])*inv_h;
	*y = a*ya[klo] + b*ya[khi] + ((a*a*a-a)*y2a[klo] + (b*b*b-b)*y2a[khi])*(h*h)*sp6;//splint6 = 1/6 = 1.666667

  /* データ外の点について */
	if( x > xa[n-1])*y = ya[n-1];
	if( x < xa[0]  )*y = ya[0];  
}


void splint_mod(double xa[], double ya[], double y2a[], int n, double x, double *y,int *klo, int *khi)
{
	int k;
	double inv_h, h, b, a;

  /* x[klo] < x < x[khi] なる klo と khi を求める */
	while (*khi - *klo > 1) {
//printf("No.1-%3d\t%3d\t%3d\n",*khi,*klo,k);
		k = (*khi + *klo) >> 1;
		if (xa[k] > x)*khi = k;
		else *klo = k;
//printf("No.2-%3d\t%3d\t%3d\n",*khi,*klo,k);
	}

  /* y = Ay[j] + By[j+1] + {(A^3 - A)y"[j] + (B^3 - B)y"[j+1]}*(x[j+1]-x[j])^2/6 を求める */
	h = xa[*khi] - xa[*klo];
	if (h == 0.0)fprintf(stderr, "Bad xa input to routine splint\n");

	inv_h = 1.0/h;

	a = (xa[*khi] - x)*inv_h;
	b = (x - xa[*klo])*inv_h;
	*y = a*ya[*klo] + b*ya[*khi] + ((a*a*a-a)*y2a[*klo] + (b*b*b-b)*y2a[*khi])*(h*h)*sp6;//splint6 = 1/6 = 1.666667

  /* データ外の点について */
	if( x > xa[n-1])*y = ya[n-1];
	if( x < xa[0]  )*y = ya[0];  
}
