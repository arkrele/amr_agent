#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#define FREE_ARG char*


/* double v[0..n] を確保してポインタを返す */
double *vec(int n){

  double *v;

  v = (double *)malloc((n+1)*sizeof(double));
  if (!v) printf("allocation error in vector()\n");
  return v;
}

/* double v[0..n] を確保してポインタを返す */
float *fvec(int n){

  float *v;

  v = (float *)malloc((n+1)*sizeof(float));
  if (!v) printf("allocation error in vector()\n");
  return v;
}

int *ivec(int n){

  int *v;

  v = (int *)malloc((n+1)*sizeof(int));
  if (!v) printf("allocation error in vector()\n");
  return v;
}

/* double m[0..n1][0..n2] を確保してポインタを返す */
double **mat(int n1, int n2)
{
  long i;
  double **m;

  m = (double **)malloc((n1+1)*sizeof(double*));
  if (!m) printf("allocation error1 in matrix()\n");
  
  m[0] = (double *)malloc((n1*(n2+1))*sizeof(double));   /*m[0]は列の先頭をさすポインタ。*/
  if (!m[0]) printf("allocation error2 in matrix()\n");  /*先に1行分のアドレスを確保してあるので　*/
                                                         /*のこりn1*(n2+1)コ分のアドレスを確保*/
  for(i = 0; i < n1; i++)
    m[i+1] = m[i] + n2 + 1;                              /*各行の先頭にアドレスを与える*/
  return m;
}

/* double m[0..n1][0..n2] を確保してポインタを返す */
float **fmat(int n1, int n2)
{
  long i;
  float **m;

  m = (float **)malloc((n1+1)*sizeof(float*));
  if (!m) printf("allocation error1 in matrix()\n");
  
  m[0] = (float *)malloc((n1*(n2+1))*sizeof(float));   /*m[0]は列の先頭をさすポインタ。*/
  if (!m[0]) printf("allocation error2 in matrix()\n");  /*先に1行分のアドレスを確保してあるので　*/
                                                         /*のこりn1*(n2+1)コ分のアドレスを確保*/
  for(i = 0; i < n1; i++)
    m[i+1] = m[i] + n2 + 1;                              /*各行の先頭にアドレスを与える*/
  return m;
}

int **imat(int n1, int n2)
{
  long i;
  int **m;

  m = (int **)malloc((n1+1)*sizeof(int*));
  if (!m) printf("allocation error1 in matrix()\n");
  
  m[0] = (int *)malloc((n1*(n2+1))*sizeof(int));   /*m[0]は列の先頭をさすポインタ。*/
  if (!m[0]) printf("allocation error2 in matrix()\n");  /*先に1行分のアドレスを確保してあるので　*/
                                                         /*のこりn1*(n2+1)コ分のアドレスを確保*/
  for(i = 0; i < n1; i++)
    m[i+1] = m[i] + n2 + 1;                              /*各行の先頭にアドレスを与える*/
  return m;
}

char **cmat(int n1, int n2)
{
  long i;
  char **m;

  m = (char **)malloc((n1+1)*sizeof(char*));
  if (!m) printf("allocation error1 in matrix()\n");
  
  m[0] = (char *)malloc((n1*(n2+1))*sizeof(char));   /*m[0]は列の先頭をさすポインタ。*/
  if (!m[0]) printf("allocation error2 in matrix()\n");  /*先に1行分のアドレスを確保してあるので　*/
                                                         /*のこりn1*(n2+1)コ分のアドレスを確保*/
  for(i = 0; i < n1; i++)
    m[i+1] = m[i] + n2 + 1;                              /*各行の先頭にアドレスを与える*/
  return m;
}

char *cvec(int n)
{
  char *v;
  v=(char *)malloc((n+1)*sizeof(char));
  if (!v) printf("allocation error in cvector()\n");
  return(v);
}

void free_vec(double *v, int n)
{
  free(v);
}

void free_mat(double **m, int n1, int n2)
{
  free(m[0]);
  free(m);
}
void free_cvec(char *v, int n)
{
  free((FREE_ARG)v);
}

void free_ivec(int *v, int n)
{
  free(v);
}
