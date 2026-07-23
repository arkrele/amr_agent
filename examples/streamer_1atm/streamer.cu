#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
//#include <cutil.h>
#include <pthread.h>
#include <time.h>
#include "hfile/memory.h"
#include "hfile/minmax.h"
#include "hfile/spline.h"
#include "hfile/mesh_generator.h"
#include "hfile/mesh_context.h"
#include "hfile/hyperbolic_curve.h"
#include "hfile/boundary.h"
#include "hfile/first_q.h"
#include "hfile/poisson.h"
#include "hfile/calcE.h"
#include "hfile/m_calc_velo.h"
#include "hfile/superbee.h"
#include "hfile/Helmholtz.h"
#include "hfile/error.h"
#include "hfile/Helm_error.h"
#include "hfile/Reaction.h"
#include "hfile/symmetricTVD.h"

#define kb		1.38e-23	//�{���c�}���萔
#define h		6.62e-34 	// �v�����N�萔
#define Diff		0.18		//�g�U�W��

#include "hfile/gaussj.h"
#include "hfile/Energy_type.h"   /*�֐��������������Ă���*/
#include "hfile/rate_calc_H2O_N2.h"
#include "hfile/rate_calc_H2O_O2.h"
#include "hfile/rate_calc_N2.h"
#include "hfile/rate_calc_N2_O2.h"
#include "hfile/rate_calc_O2.h"
#include "hfile/rate_calc_Ozone.h"

#define  point( N, i, j ) ( (N)*(i) + (j) )
#include "hfile/Red_Black_SOR_Kernel.h"
#include "hfile/Helmholtz_Kernel.h"
#include "hfile/Multigrid_kernel.h"

#define INITIAL_FILE	"inputdata/Initial/initial_N2_80p_300K.dat"
#define BOLTZMANNFILE	"inputdata/Bolsig_Data/O2_20p.dat"
#define I_REACTION_FILE	"inputdata/i_reaction_300K_modmod0516.dat"
#define E_REACTION_FILE	"inputdata/e_reaction_mod1803.dat"
#define N_REACTION_FILE	"inputdata/n_reaction_modmod.dat"
#define MESH_R_FILE	"inputdata/mesh_r2_0624.dat"
#define MESH_Z_FILE	"inputdata/mesh_z2.dat"
#define VOLTAGE_FILE	"inputdata/V_Ono_single_str.dat"

double	T0	= 300.0;	//�����K�X���x(K)

#define PI	  3.1415926535897932			//�~����
#define QELEC	  1.602176462e-19			//�d�q�̓d�ׁi�f�d��) [C]
#define MELEC	  9.10938188e-31			//�d�q�̎���
#define MOL	  ((6.02e+23*1000.0)/(0.0820578*T0))	//���q��(const)
#define E0	  8.85e-12				//�^��̗U�d��[F/m]=[C/V/m]
#define Avogadro  6.022141e+23				//�A�{�K�h���萔 �R/mol
#define MassNum	  (6.022141e+23/(28.966/1000.0))	//���ʕ��q�� [�R/kg] (Avogadro/air_kg)
#define airpre	  1.0


#define NR 256		//�a�����̃��b�V����
#define NZ 1792		//�������̃��b�V����

#define VIB_SKIP 20000000

///////////////////GPU�̃u���b�N��////////////////
#define blockDim_x 1
#define blockDim_y 256

#define blockDim_x2 1
#define blockDim_y2 128

#define blockDim_x4 1
#define blockDim_y4 64
//////////////////////////////////////////////////

#define THREAD_NUM 64	//�}���`�X���b�h�̃X���b�h��
#define BN	1200	//�{���c�}���������̃f�[�^�[��
int AN	= 1200;	//�d���W���̃f�[�^��
#define DAnum	800	//DA reaction�̃f�[�^��
#define DEMnum	1000	//DEM reaction�̃f�[�^��

double	bgap	= 13.0e-3;	//�M���b�v��
double	cr	= 500e-6;	//��[�ȗ����a
double	EPS	= 1.0e-5;	//����

int n_particles = 100;		//�l�����闱�q���A�K���ɑ��

int NUM_L=3;
int NUM_R=3;
#define NUM_REACT 41 //C53+1
#define DATA 200 //�e�L�g�[�ɓ���Ă����B������(C53�Ƃ�)��葽�����ok
int n_react,ne_react,ni_react,nn_react,neR,nDAR,nDER;
int **ereactl, **ereactr,**ireactl, **ireactr,**nreactl, **nreactr;
int *ereactl_1st,*ereactl_2nd,*ereactl_3rd,*ereactr_1st,*ereactr_2nd,*ereactr_3rd;
int *nreactl_1st,*nreactl_2nd,*nreactl_3rd,*nreactr_1st,*nreactr_2nd,*nreactr_3rd;
int *ireactl_1st,*ireactl_2nd,*ireactl_3rd,*ireactr_1st,*ireactr_2nd,*ireactr_3rd;
double *eK0,*eK1,*eK2,*eER,*eER_mod, *eK2_mod;
double *iK0,*iK1,*iK2,*iER,*iK0m,*iER_mod;
double *nK0,*nK1,*nK2,*nER,*nK0m,*nK1m,*nK2m,*nER_mod;
double **T,power,**POWER;
int numO2,numO2p,nume,numN2,numH2O;
int *nRnum,*iRnum,*eRnum;
int **neflag,Prinstp2,nstp;
double z_limiter;

int n1,n2,n3,n4,n5,n6,n7,n8,n9,n10,n11,n12,n13,n14,n15,n16;
int o2v0,o2v1,o2v2,o2v3,o2v4,n2v0,n2v1,n2v2,n2v3,n2v4,n2v5,n2v6,n2v7,n2v8, h2ov0,h2ov1,h2ov2,h2ov3,oatm;


//double PARTICLE[100][NR][NZ];
double krate[100][NR][NZ];
double PARTICLE[NR][NZ][100];
double molP[NR][NZ][40];
double Yk[NR][NZ][100];

double **ne,**N2p,**N4p,**O2p,**O4p,**N2O2p,**O2pH2O,**H3Op,**O2m,**Om,**O2,**N2,**H2Op,**OHm,**Hm;
double **H3OpH2O,**H3OpH2O2, **H3OpH2O3;
double pN2,pO2,pH2O;

double *TdE,*DAeV,*DEMeV,*deV,*ddeV,*dv,*ddv;
double **dc,**ddc;
double **vr,**vz,**piv_r,**piv_z,**miv_r,**miv_z,**Vol,**Sr,**Sz,kappa,superbee;
double **v,**piv,**miv,*rrho;
double current,current2,CurrDens,InducedCurr,V, current_c;

double **absE,dt,**phot_num,**TeV,*HelmG,**dtELr,**dtELz,**preEr,**preEz,**Ex,**Ey;
int **flag,**iflag,**jflag,**tflag,**oflag,*HelmA;
double **P1,**P2,**P3,**P4,**P5,**rho,**phi,**Lphi,**Cphi,**Cphi1,**Cphi2;
double *dalp,*ddalp,*alpTd,**I,*rh,*zh,**S_ph0,**S_ph1,**S_ph2,**sink,**e_sink,**i_sink;
double *dpower,*ddpower, *powerTd;
double **Br, **Bz, **mvr, **mvz, **cE, *r,*z,*dr,*dz, **rou, **p;
char **particle;
static MeshContext g_mesh;

void rk4(int ii,int jj,double *y,double *yout,int n,double dt,int num_react,int **,int **);
void e_reaction_derivs(int ii,int jj,double *y,int *,int,double *yout,int **,int **,double *);
void i_reaction_derivs(int ii,int jj,double *y,int *,int,double *yout,int **,int **,double *);
void n_reaction_derivs(int ii,int jj,double *y,int *,int,double *yout,int **,int **,double *);
void get_constant();
void store(int Z,double time,double **Ey, char **particle, double atmpa, double air_kg, double molV);
void data_input(char *Mol,int numdata,double **n);
void vib_store(int Z);

/* �X���b�h�֐��̈����f�[�^�\���� */
typedef struct _thread_arg {
    int thread_no;
    pthread_mutex_t *mutex;
} thread_arg_t;

void symmetric_TVD(double u0,double v0,double p0,double t0,double rou0,double dt,double g0,double rgas,double ecp,double *res,
                      double **rou,double **vr,double **vz,double **p,double **q1,double **q2,double **q3,double **q4,
                        int **flag,int **iflag,int **jflag,int **oflag,int **tflag);
void bndcnd(double **rou,double **vr,double **vz,double **p,double **q1,double **q2,
                double **q3,double **q4,double g0,double rgas,double u0,double v0,double p0,double t0,
                  double rou0,int **flag,int **iflag,int **jflag,int **oflag,int **tflag);
void calrhs(double **rou,double **vr,double **vz,double **p,
              double **q1,double **q2,double **q3,double **q4,double rgas,double g0,
                double dt,double ecp,double **dq1,double **dq2,double **dq3,double **dq4,int **flag,int te);

void* thread_func_fluid(void *arg);

void* thread_func_2D_to_3D(void *arg) {
    thread_arg_t *targ;
    targ = (thread_arg_t *)arg;

	int ist,ifi,i,j;
	double inv_rou;

	ist=NR*targ->thread_no/THREAD_NUM;
	ifi=NR*(targ->thread_no+1)/THREAD_NUM;

	for(i=ist;i<ifi;i++){
		for(j=0;j<NZ;j++){
			inv_rou = 1.0/(rou[i][j]*MassNum);

			PARTICLE[i][j][n1]	=	  ne[i][j]*inv_rou;
			PARTICLE[i][j][n2]	=	 N2p[i][j]*inv_rou;
			PARTICLE[i][j][n3]	=	 O2p[i][j]*inv_rou;
			PARTICLE[i][j][n4]	=	 O2m[i][j]*inv_rou;
			PARTICLE[i][j][n5]	=	  Om[i][j]*inv_rou;
			PARTICLE[i][j][n6]	=	H2Op[i][j]*inv_rou;
			PARTICLE[i][j][n7]	= 	 OHm[i][j]*inv_rou;
			PARTICLE[i][j][n8]	= 	  Hm[i][j]*inv_rou;
			PARTICLE[i][j][n9]	= 	 N4p[i][j]*inv_rou;
			PARTICLE[i][j][n10]	= 	 O4p[i][j]*inv_rou;
			PARTICLE[i][j][n11]	=      N2O2p[i][j]*inv_rou;
			PARTICLE[i][j][n12]	=      O2pH2O[i][j]*inv_rou;
			PARTICLE[i][j][n13]	=      H3Op[i][j]*inv_rou;
			PARTICLE[i][j][n14]	=      H3OpH2O[i][j]*inv_rou;
			PARTICLE[i][j][n15]	=      H3OpH2O2[i][j]*inv_rou;
			PARTICLE[i][j][n16]	=      H3OpH2O3[i][j]*inv_rou;
		}
	}
}

void* thread_func_3D_to_2D(void *arg) {
    thread_arg_t *targ;
    targ = (thread_arg_t *)arg;

	int ist,ifi,i,j;
	double rrou;

	ist=NR*targ->thread_no/THREAD_NUM;
	ifi=NR*(targ->thread_no+1)/THREAD_NUM;

	for(i=ist;i<ifi;i++){
		for(j=0;j<NZ;j++){
			rrou = rou[i][j]*MassNum;

			ne[i][j]	=	PARTICLE[i][j][n1]*rrou;
			N2p[i][j]	=	PARTICLE[i][j][n2]*rrou;
			O2p[i][j]	=	PARTICLE[i][j][n3]*rrou;
			O2m[i][j]	=	PARTICLE[i][j][n4]*rrou;
			Om[i][j]	=	PARTICLE[i][j][n5]*rrou;
			H2Op[i][j]	=	PARTICLE[i][j][n6]*rrou;
			OHm[i][j]	=	PARTICLE[i][j][n7]*rrou;
			Hm[i][j]	=	PARTICLE[i][j][n8]*rrou;
			N4p[i][j]	=	PARTICLE[i][j][n9]*rrou;
			O4p[i][j]	=	PARTICLE[i][j][n10]*rrou;
			N2O2p[i][j]	=	PARTICLE[i][j][n11]*rrou;
			O2pH2O[i][j]	=	PARTICLE[i][j][n12]*rrou;
			H3Op[i][j]	=	PARTICLE[i][j][n13]*rrou;
			H3OpH2O[i][j]	=	PARTICLE[i][j][n14]*rrou;
			H3OpH2O2[i][j]	=	PARTICLE[i][j][n15]*rrou;
			H3OpH2O3[i][j]	=	PARTICLE[i][j][n16]*rrou;
		}
	}
}

void* thread_func_forCurrent(void *arg) {
    thread_arg_t *targ;
    targ = (thread_arg_t *)arg;

	int ist,ifi,i,j;

	ist=NR*targ->thread_no/THREAD_NUM;
	ifi=NR*(targ->thread_no+1)/THREAD_NUM;

	double cnst1=MOL*1e-21;
	double cnst2=1.0/dt;

		//�d���v�Z�̂���
		for(i=ist;i<ifi;i++){
			for(j=0;j<NZ;j++){
				preEr[i][j]=Ex[i][j]*cnst1;
				preEz[i][j]=Ey[i][j]*cnst1;

				dtELr[i][j] += preEr[i][j];
				dtELz[i][j] += preEz[i][j];

				dtELr[i][j] = dtELr[i][j]*cnst2;
				dtELz[i][j] = dtELz[i][j]*cnst2;
			}
		}
}

void* thread_func_forPHT(void *arg) {
    thread_arg_t *targ;
    targ = (thread_arg_t *)arg;

	int ist,ifi,i,j;
	double eV;
	double TeVkeisu=2.0/3.0*QELEC/kb;  //�d�q���x�v�Z�ɗp����W��
	double ph_const = 0.1*30/(760*airpre+30)*100;
	double ph_consto = 0.02*36/(760*airpre+36)*100;
	double kalp;
	double vo2i;
	//printf("ph_const=%f, ph_const2=%f\n",ph_const,ph_consto);
	
	ist=NR*targ->thread_no/THREAD_NUM;
	ifi=NR*(targ->thread_no+1)/THREAD_NUM;


		for(i=ist;i<ifi;i++){
			for(j=0;j<NZ;j++){
				if(i==NR-1 || j==NZ-1)v[i][j]=sqrt(pow(0.5*(vz[i][j]+vz[i][j]),2)+pow(0.5*(vr[i][j]+vr[i][j]),2));
				else v[i][j]=sqrt(pow(0.5*(vz[i][j]+vz[i][j+1]),2)+pow(0.5*(vr[i][j]+vr[i+1][j]),2));
				piv[i][j]=sqrt(piv_z[i][j]*piv_z[i][j]+piv_r[i][j]*piv_r[i][j]);
				miv[i][j]=sqrt(miv_z[i][j]*miv_z[i][j]+miv_r[i][j]*miv_r[i][j]);

				splint(alpTd,dalp,ddalp,AN,absE[i][j],&kalp);
				splint(TdE,dc[41],ddc[41],BN,absE[i][j],&vo2i);
				I[i][j] = ph_const*(kalp)*v[i][j]*ne[i][j]; //in m^-3 s^-1
				//if (kalp>1) printf("N2 ion= %e\n,O2i ion= %e\n",ph_const*(kalp)*v[i][j]*ne[i][j],ph_consto*vo2i*pO2*MOL*ne[i][j]);
				
				rrho[point(NZ,i,j)] = ph_const*(kalp)*v[i][j]*ne[i][j]+ph_consto*vo2i*ne[i][j]*pO2*MOL; //in m^-3 s^-1

				//�d�q���x�v�Z
				if(flag[i][j])TeV[i][j]=0.0;
				else{
					splint(TdE,deV,ddeV,BN,fabs(absE[i][j]),&eV);
					TeV[i][j]=TeVkeisu*eV;
				}
			}
		}
}


void* thread_func_react(void *arg) {
    thread_arg_t *targ;
    targ = (thread_arg_t *)arg;

	int ist,ifi,i,j,k,R1,R2,R3,L1,L2,L3,nnum;
	int kk, klo, khi;
	double inv_h, hh, bb, aa,r,dT,kr;
	//react
	double *Y,*nYout,*eYout,*iYout,RE,dph;
	double temp1=exp(0.39*log(T[i][j]));
	double temp2=exp(0.7*log(T[i][j]));
	double temp3, temp4, temp5,temp6;
	double inv_rou,rrou;
	int numOH;

	numOH =  particle_number("OH",particle,n_particles);

	Y=vec(n_particles),nYout=vec(n_particles),eYout=vec(n_particles),iYout=vec(n_particles);

	ist=NR*targ->thread_no/THREAD_NUM;
	ifi=NR*(targ->thread_no+1)/THREAD_NUM;

	if(targ->thread_no==0)printf("---Reaction-Calculation... ");

	for(i=ist;i<ifi;i++){  //�d�q���x�A�C�I�����x�̌v�Z
		for(j=0;j<NZ;j++){
			if(!flag[i][j]){

				rrou = rou[i][j]*MassNum;

				for(k=0 ;k<n_particles;k++){
					Y[k]= PARTICLE[i][j][k]*rrou;
					eYout[k] = iYout[k] = nYout[k] = 0.0;
				}
				Y[0]= 1.0;
				dT=0.0;

				for(k=0;k<ne_react;k++){

					RE  = absE[i][j];

					nnum=eRnum[k];
					  // x[klo] < x < x[khi] �Ȃ� klo �� khi �����߂� 
					klo = 0;
					khi = BN - 1;
					while (khi - klo > 1) {
						kk = (khi + klo) >> 1;
						if (TdE[kk] > RE)khi = kk;
						else klo = kk;
					}

					  // y = Ay[j] + By[j+1] + {(A^3 - A)y"[j] + (B^3 - B)y"[j+1]}*(x[j+1]-x[j])^2/6 �����߂� 
					hh = TdE[khi] - TdE[klo];
					if (hh == 0.0)fprintf(stderr, "Bad xa input to routine splint\n");

					inv_h = 1.0/hh;

					aa = (TdE[khi] - RE)*inv_h;
					bb = (RE - TdE[klo])*inv_h;
					kr = aa*dc[nnum][klo] + bb*dc[nnum][khi] + ((aa*aa*aa-aa)*ddc[nnum][klo] + (bb*bb*bb-bb)*ddc[nnum][khi])*(hh*hh)*sp6;//splint6 = 1/6 = 1.666667

							  // �f�[�^�O�̓_�ɂ��� 
					if( RE > TdE[BN-1])kr = dc[nnum][BN-1];
					if( RE < TdE[0]  )kr = dc[nnum][0];  


					L1=ereactl_1st[k], R1=ereactr_1st[k];
					L2=ereactl_2nd[k], R2=ereactr_2nd[k];
					L3=ereactl_3rd[k], R3=ereactr_3rd[k];

// k = 15��N2�̓d��
// k = 28��O2�̓d��

if(nstp > Prinstp2){
		if(zh[j] > z_limiter && ne[i][j]>1e16){
		if(k==15 || k == 27){
			if(neflag[i][j]){
				if(RE>120.0){
					kr = kr*exp(-0.01*(RE-120.0));
				}else{
				}
			}else{
			}
		}else{
		}
	}
}

					r=dt*kr*Y[L1]*Y[L2]*Y[L3];
					dT += r*eER_mod[k];

					if(L1!=0)eYout[L1] += -r;
					if(L2!=0)eYout[L2] += -r;
					if(L3!=0)eYout[L3] += -r;

					if(R1!=0)eYout[R1] += r;
					if(R2!=0)eYout[R2] += r;
					if(R3!=0)eYout[R3] += r;

						{
							int sink_bin = mesh_reaction_sink_bin(&g_mesh, i, j);
							if(sink_bin >= 0)e_sink[sink_bin][k] += r*Vol[i][j];
						}

					molP[i][j][k] = r * eK2_mod[k]*Vol[i][j];

				}

				////////////�e��C�I�����܂ޔ���/////////////

				temp3=log(TeV[i][j]);
				temp6=0.5*log(300/TeV[i][j]);
				iK0m[0]=iK0[1]*temp1*exp(-0.39*temp3);
				iK0m[1]=iK0[2]*temp2*exp(-0.70*temp3);
				iK0m[2]=iK0[3]*exp(temp6);
				iK0m[3]=iK0[4]*exp(temp6);
				iK0m[4]=iK0[5]*exp(temp6);
				iK0m[5]=iK0[6]*exp(temp6);

//				i_reaction_derivs(i,j,Y,iRnum,ni_react,iYout,ireactl,ireactr,iER);

					k=0;

					L1=ireactl_1st[k], R1=ireactr_1st[k];
					L2=ireactl_2nd[k], R2=ireactr_2nd[k];
					L3=ireactl_3rd[k], R3=ireactr_3rd[k];


					r=dt*iK0m[k]*Y[L1]*Y[L2]*Y[L3];
					dT += r*iER_mod[k];

					if(L1!=0)iYout[L1] += -r;
					if(L2!=0)iYout[L2] += -r;
					if(L3!=0)iYout[L3] += -r;

					if(R1!=0)iYout[R1] += r;
					if(R2!=0)iYout[R2] += r;
					if(R3!=0)iYout[R3] += r;

						{
							int sink_bin = mesh_reaction_sink_bin(&g_mesh, i, j);
							if(sink_bin >= 0)i_sink[sink_bin][k] += r*Vol[i][j];
						}

					/////////////////////////////////
					k=1;

					L1=ireactl_1st[k], R1=ireactr_1st[k];
					L2=ireactl_2nd[k], R2=ireactr_2nd[k];
					L3=ireactl_3rd[k], R3=ireactr_3rd[k];


					r=dt*iK0m[k]*Y[L1]*Y[L2]*Y[L3];
					dT += r*iER_mod[k];

					if(L1!=0)iYout[L1] += -r;
					if(L2!=0)iYout[L2] += -r;
					if(L3!=0)iYout[L3] += -r;

					if(R1!=0)iYout[R1] += r;
					if(R2!=0)iYout[R2] += r;
					if(R3!=0)iYout[R3] += r;

						{
							int sink_bin = mesh_reaction_sink_bin(&g_mesh, i, j);
							if(sink_bin >= 0)i_sink[sink_bin][k] += r*Vol[i][j];
						}

					/////////////////////////////////
					k=2;

					L1=ireactl_1st[k], R1=ireactr_1st[k];
					L2=ireactl_2nd[k], R2=ireactr_2nd[k];
					L3=ireactl_3rd[k], R3=ireactr_3rd[k];


					r=dt*iK0m[k]*Y[L1]*Y[L2]*Y[L3];
					dT += r*iER_mod[k];

					if(L1!=0)iYout[L1] += -r;
					if(L2!=0)iYout[L2] += -r;
					if(L3!=0)iYout[L3] += -r;

					if(R1!=0)iYout[R1] += r;
					if(R2!=0)iYout[R2] += r;
					if(R3!=0)iYout[R3] += r;

						{
							int sink_bin = mesh_reaction_sink_bin(&g_mesh, i, j);
							if(sink_bin >= 0)i_sink[sink_bin][k] += r*Vol[i][j];
						}

					/////////////////////////////////
					k=3;

					L1=ireactl_1st[k], R1=ireactr_1st[k];
					L2=ireactl_2nd[k], R2=ireactr_2nd[k];
					L3=ireactl_3rd[k], R3=ireactr_3rd[k];


					r=dt*iK0m[k]*Y[L1]*Y[L2]*Y[L3];
					dT += r*iER_mod[k];

					if(L1!=0)iYout[L1] += -r;
					if(L2!=0)iYout[L2] += -r;
					if(L3!=0)iYout[L3] += -r;

					if(R1!=0)iYout[R1] += r;
					if(R2!=0)iYout[R2] += r;
					if(R3!=0)iYout[R3] += r;

						{
							int sink_bin = mesh_reaction_sink_bin(&g_mesh, i, j);
							if(sink_bin >= 0)i_sink[sink_bin][k] += r*Vol[i][j];
						}

					/////////////////////////////////
					k=4;

					L1=ireactl_1st[k], R1=ireactr_1st[k];
					L2=ireactl_2nd[k], R2=ireactr_2nd[k];
					L3=ireactl_3rd[k], R3=ireactr_3rd[k];


					r=dt*iK0m[k]*Y[L1]*Y[L2]*Y[L3];
					dT += r*iER_mod[k];

					if(L1!=0)iYout[L1] += -r;
					if(L2!=0)iYout[L2] += -r;
					if(L3!=0)iYout[L3] += -r;

					if(R1!=0)iYout[R1] += r;
					if(R2!=0)iYout[R2] += r;
					if(R3!=0)iYout[R3] += r;

						{
							int sink_bin = mesh_reaction_sink_bin(&g_mesh, i, j);
							if(sink_bin >= 0)i_sink[sink_bin][k] += r*Vol[i][j];
						}
					/////////////////////////////////
					k=5;

					L1=ireactl_1st[k], R1=ireactr_1st[k];
					L2=ireactl_2nd[k], R2=ireactr_2nd[k];
					L3=ireactl_3rd[k], R3=ireactr_3rd[k];


					r=dt*iK0m[k]*Y[L1]*Y[L2]*Y[L3];
					dT += r*iER_mod[k];

					if(L1!=0)iYout[L1] += -r;
					if(L2!=0)iYout[L2] += -r;
					if(L3!=0)iYout[L3] += -r;

					if(R1!=0)iYout[R1] += r;
					if(R2!=0)iYout[R2] += r;
					if(R3!=0)iYout[R3] += r;

						{
							int sink_bin = mesh_reaction_sink_bin(&g_mesh, i, j);
							if(sink_bin >= 0)i_sink[sink_bin][k] += r*Vol[i][j];
						}

				for(k=6;k<ni_react;k++){

					L1=ireactl_1st[k], R1=ireactr_1st[k];
					L2=ireactl_2nd[k], R2=ireactr_2nd[k];
					L3=ireactl_3rd[k], R3=ireactr_3rd[k];


					r=dt*iK0m[k]*Y[L1]*Y[L2]*Y[L3];
					dT += r*iER_mod[k];

					if(L1!=0)iYout[L1] += -r;
					if(L2!=0)iYout[L2] += -r;
					if(L3!=0)iYout[L3] += -r;

					if(R1!=0)iYout[R1] += r;
					if(R2!=0)iYout[R2] += r;
					if(R3!=0)iYout[R3] += r;

						{
							int sink_bin = mesh_reaction_sink_bin(&g_mesh, i, j);
							if(sink_bin >= 0)i_sink[sink_bin][k] += r*Vol[i][j];
						}

				}


				////////////�������q�̉��w����/////////////

				temp4 = 1.0/T[i][j];
				temp5 = log(T[i][j]/298.0);

				for(k=0;k<nn_react;k++){

					L1=nreactl_1st[k], R1=nreactr_1st[k];
					L2=nreactl_2nd[k], R2=nreactr_2nd[k];
					L3=nreactl_3rd[k], R3=nreactr_3rd[k];

					r=dt*nK0m[k]*exp( -nK2m[k]*temp4 + nK1m[k]*temp5 )*Y[L1]*Y[L2]*Y[L3];
					dT += r*nER_mod[k];

					if(L1!=0)nYout[L1] += -r;
					if(L2!=0)nYout[L2] += -r;
					if(L3!=0)nYout[L3] += -r;

					if(R1!=0)nYout[R1] += r;
					if(R2!=0)nYout[R2] += r;
					if(R3!=0)nYout[R3] += r;

				//////////////
					{
						int sink_bin = mesh_reaction_sink_bin(&g_mesh, i, j);
						if(sink_bin >= 0)sink[sink_bin][k] += r*Vol[i][j];
					}
				}

				cE[i][j] = dT;


				/////////////�d�E�ɂ�����p���[(elastic + inelestic)���v�Z�BPower[eV m^3/s]//////////////
				splint(powerTd,dpower,ddpower,BN,RE,&power);
				POWER[i][j] = dt*(PARTICLE[i][j][nume]*rrou)*power*((PARTICLE[i][j][numO2] + PARTICLE[i][j][numN2] + PARTICLE[i][j][numH2O])*rrou)*Vol[i][j];
				//////////////////////////////////////////////////////////////////////////

				////////////�������킹+���d������//////////////
				inv_rou = 1.0/rrou;
				for(k=0;k<n_particles;k++)PARTICLE[i][j][k]+= (eYout[k]+iYout[k]+nYout[k])*inv_rou;
				dph = dt*phot_num[i][j];


				PARTICLE[i][j][numO2]	-= dph*inv_rou;
				PARTICLE[i][j][numO2p]	+= dph*inv_rou;
				PARTICLE[i][j][nume]	+= dph*inv_rou;

			}
		}
	}

	free_vec(Y,n_particles),free_vec(nYout,n_particles),free_vec(eYout,n_particles),free_vec(iYout,n_particles);

	printf("%d  ",targ->thread_no);
}

void Debug_Boltzmann(int debug){

	if(debug){

		//Boltzmann Check
		double d0,d1,d2;
		int i,k;
		for(i=0;i<1000;i++){
			k=1,splint(TdE,dc[k],ddc[k],BN,(double)i,&d0);
			k=2,splint(TdE,dc[k],ddc[k],BN,(double)i,&d1);
			k=43,splint(TdE,dc[k],ddc[k],BN,(double)i,&d2);
			printf("%f\t%.4e\t%.4e\t%.4e\n",(double)i,d0,d1,d2);
		}
		printf("Debug_for_Boltzmann_spline\n"),exit(0);
	}else;

}

void vib_convert(int i, int j, double *vib, double **Ev){

	double inv_O2Tv,rrou;

	rrou = rou[i][j]*MassNum;
	
	vib[0] = PARTICLE[i][j][o2v0]*rrou;
	vib[1] = PARTICLE[i][j][o2v1]*rrou;
	vib[2] = PARTICLE[i][j][o2v2]*rrou;
	vib[3] = PARTICLE[i][j][o2v3]*rrou;
	vib[4] = PARTICLE[i][j][o2v4]*rrou;

	inv_O2Tv = 1.0/((Ev[0][4]-Ev[0][3])/(log(vib[3]/vib[4])));

	vib[5] = vib[4]*exp((Ev[0][4]-Ev[0][5])*inv_O2Tv); //O2v5
	vib[6] = vib[5]*exp((Ev[0][5]-Ev[0][6])*inv_O2Tv); //O2v6
	vib[7] = vib[6]*exp((Ev[0][6]-Ev[0][7])*inv_O2Tv); //O2v7
	vib[8] = vib[7]*exp((Ev[0][7]-Ev[0][8])*inv_O2Tv); //O2v8

	vib[9]  = PARTICLE[i][j][n2v0]*rrou;
	vib[10] = PARTICLE[i][j][n2v1]*rrou;
	vib[11] = PARTICLE[i][j][n2v2]*rrou;
	vib[12] = PARTICLE[i][j][n2v3]*rrou;
	vib[13] = PARTICLE[i][j][n2v4]*rrou;
	vib[14] = PARTICLE[i][j][n2v5]*rrou;
	vib[15] = PARTICLE[i][j][n2v6]*rrou;
	vib[16] = PARTICLE[i][j][n2v7]*rrou;
	vib[17] = PARTICLE[i][j][n2v8]*rrou;

	vib[18] = PARTICLE[i][j][h2ov0]*rrou;
	vib[19] = PARTICLE[i][j][h2ov2]*rrou;
	vib[20] = PARTICLE[i][j][h2ov1]*rrou;
	vib[21] = PARTICLE[i][j][h2ov3]*rrou;

}

void vib_assemble(int i, int j, double *dvib,double dt){

	double inv_rou;

	inv_rou = dt/(rou[i][j]*MassNum);

	PARTICLE[i][j][o2v0] += dvib[0]*inv_rou;
	PARTICLE[i][j][o2v1] += dvib[1]*inv_rou;
	PARTICLE[i][j][o2v2] += dvib[2]*inv_rou;
	PARTICLE[i][j][o2v3] += dvib[3]*inv_rou;
	PARTICLE[i][j][o2v4] += dvib[4]*inv_rou;
	
	PARTICLE[i][j][n2v0] += dvib[9]*inv_rou;
	PARTICLE[i][j][n2v1] += dvib[10]*inv_rou;
	PARTICLE[i][j][n2v2] += dvib[11]*inv_rou;
	PARTICLE[i][j][n2v3] += dvib[12]*inv_rou;
	PARTICLE[i][j][n2v4] += dvib[13]*inv_rou;
	PARTICLE[i][j][n2v5] += dvib[14]*inv_rou;
	PARTICLE[i][j][n2v6] += dvib[15]*inv_rou;
	PARTICLE[i][j][n2v7] += dvib[16]*inv_rou;
	PARTICLE[i][j][n2v8] += dvib[17]*inv_rou;
	
	PARTICLE[i][j][h2ov0] += dvib[18]*inv_rou;
	PARTICLE[i][j][h2ov2] += dvib[19]*inv_rou;
	PARTICLE[i][j][h2ov1] += dvib[20]*inv_rou;
	PARTICLE[i][j][h2ov3] += dvib[21]*inv_rou;

}


main(){

	int i,j,START,Helm_iter,k;
	int chname, loop;
	double e_max,pureAir_kg,OMEGA;
	double time,CFLmemo;
	double A,B1,B2,C,D,CPp,CPm;
	double *spTime,*dspV,spDATA,*ddspV,spV;
	double **q1,**q2,**q3,**q4,res;
	double inv_rou, rrou, sum, inv_sum, MolMass;
	double eV,**dTvib, **sumN2C,**Dvx,**Dvy;
	FILE *fp;

	double a = sqrt(cr*bgap);
	char filename[256];

	pthread_t handle[THREAD_NUM];
	thread_arg_t targ[THREAD_NUM];
	pthread_mutex_t mutex;

	r=vec(NR),z=vec(NZ),rh=vec(NR),zh=vec(NZ),dr=vec(NR),dz=vec(NZ);Vol=mat(NR,NZ),Sr=mat(NR,NZ),Sz=mat(NR,NZ);
	flag=imat(NR,NZ),iflag=imat(NR,NZ),jflag=imat(NR,NZ),tflag=imat(NR,NZ),oflag=imat(NR,NZ);
	ne=mat(NR,NZ),N2p=mat(NR,NZ),O2p=mat(NR,NZ),O2m=mat(NR,NZ),Om=mat(NR,NZ),H2Op=mat(NR,NZ),OHm=mat(NR,NZ),Hm=mat(NR,NZ);
	N4p=mat(NR,NZ),O4p=mat(NR,NZ),N2O2p=mat(NR,NZ),O2pH2O=mat(NR,NZ),H3Op=mat(NR,NZ),H3OpH2O=mat(NR,NZ),H3OpH2O2=mat(NR,NZ),H3OpH2O3=mat(NR,NZ);
	O2=mat(NR,NZ),N2=mat(NR,NZ);P1=mat(NR,NZ),P2=mat(NR,NZ),P3=mat(NR,NZ),P4=mat(NR,NZ),P5=mat(NR,NZ);
	rho=mat(NR,NZ),phi=mat(NR,NZ),Lphi=mat(NR,NZ),Cphi=mat(NR,NZ);	
	absE=mat(NR,NZ),Ex=mat(NR,NZ),Ey=mat(NR,NZ),vr=mat(NR,NZ),vz=mat(NR,NZ);
	piv_r=mat(NR,NZ),piv_z=mat(NR,NZ),miv_r=mat(NR,NZ),miv_z=mat(NR,NZ);v=mat(NR,NZ),piv=mat(NR,NZ),miv=mat(NR,NZ);
	dtELr=mat(NR,NZ),dtELz=mat(NR,NZ);
	POWER = mat(NR,NZ),sumN2C=mat(NR,NZ),Dvx=mat(NR,NZ),Dvy=mat(NR,NZ);

	phot_num=mat(NR,NZ),HelmA=ivec(3),HelmG=vec(3);;
	TeV=mat(NR,NZ),Cphi1=mat(NR,NZ),Cphi2=mat(NR,NZ),I=mat(NR,NZ),preEr=mat(NR,NZ),preEz=mat(NR,NZ);
	S_ph0=mat(NR,NZ),S_ph1=mat(NR,NZ),S_ph2=mat(NR,NZ);
	sink = mat(4,200),e_sink=mat(4,200),i_sink=mat(4,200);
	for(i=0;i<4;i++)for(j=0;j<200;j++)sink[i][j]=i_sink[i][j]=e_sink[i][j]=0.0;

	Br=mat(NR,NZ), Bz=mat(NR,NZ), mvr=mat(NR,NZ), mvz=mat(NR,NZ), rou=mat(NR,NZ), p=mat(NR,NZ);
	q1=mat(NR,NZ), q2=mat(NR,NZ), q3=mat(NR,NZ), q4=mat(NR,NZ), cE=mat(NR,NZ);

	dTvib=mat(NR,NZ);

	get_constant();//�萔��ǂݍ���
	Debug_Boltzmann(0);


/////////////////////////////////////////////////////


///////////////////////////////////////CUDA////////////////////////////////////////////

	int cuda,cuda_flag,iter,itnum;
	double error, Maxphi;

	double *d_err, *dd_err;
	double *d_phi, *d_rho,*d_temp,*d_rh,*d_data;
	double *d_Sph0,*d_Sph1,*d_Sph2;
	double *d_P1,*d_P2,*d_P3,*d_P4,*d_P5;
	double toP,*each_mP;
	int *d_flag,*d_iflag,*d_jflag,*d_oflag;

	double *pphi,*temp,*rrh;
	double *SSph0,*SSph1,*SSph2;
	double *PP1,*PP2,*PP3,*PP4,*PP5;
	int *fflag,*iiflag,*jjflag,*ooflag;

	int  N ,mi, mf;

	// 2-D GPU mapping
	N = NR*NZ; // grid size

	dim3 dimGrid( NR/blockDim_x, NZ/blockDim_y );
	dim3 dimBlock( blockDim_x, blockDim_y );

	// space for arrays on CPU and GPU
	mi = N * sizeof( int ); //memory size int
	mf = N * sizeof( double );//memory size double

////////////////////////////////////////////
///////////////////2nd Grid/////////////////
////////////////////////////////////////////
	int  NN ,mi2, mf2;

	int NR2 = NR/2;
	int NZ2 = NZ/2;

	// 2-D GPU mapping
	NN = NR2*NZ2; // grid size

	dim3 dimGrid2( NR2/blockDim_x2, NZ2/blockDim_y2 );
	dim3 dimBlock2( blockDim_x2, blockDim_y2 );

	// space for arrays on CPU and GPU
	mi2 = NN * sizeof( int ); //memory size int
	mf2 = NN * sizeof( double );//memory size double

	double *dd_rho, *dd_Cphi, *dd_rh;
	double *dd_P1,*dd_P2,*dd_P3,*dd_P4,*dd_P5,*dd_temp;
	int *dd_flag,*dd_iflag,*dd_jflag,*dd_oflag,*dd_tflag;

	double *rrho2;
	int *fflag2;
	rrho2=vec(NN);
	fflag2 = ivec(NN);

	cudaMalloc( (void **)&dd_rho,  mf2);
	cudaMalloc( (void **)&dd_Cphi,  mf2);
	cudaMalloc( (void **)&dd_rh,  mf2);
	cudaMalloc( (void **)&dd_P1,  mf2);
	cudaMalloc( (void **)&dd_P2,  mf2);
	cudaMalloc( (void **)&dd_P3,  mf2);
	cudaMalloc( (void **)&dd_P4,  mf2);
	cudaMalloc( (void **)&dd_P5,  mf2);
	cudaMalloc( (void **)&dd_temp,  mf2);

	cudaMalloc( (void **)&dd_flag,  mi2);
	cudaMalloc( (void **)&dd_iflag,  mi2);
	cudaMalloc( (void **)&dd_jflag,  mi2);
	cudaMalloc( (void **)&dd_oflag,  mi2);
	cudaMalloc( (void **)&dd_tflag,  mi2);

////////////////////////////////////////////
///////////////////4th Grid/////////////////
////////////////////////////////////////////
	int  NNN ,mi4, mf4;

	int NR4 = NR2/2;
	int NZ4 = NZ2/2;

	// 2-D GPU mapping
	NNN = NR4*NZ4; // grid size

	dim3 dimGrid4( NR4/blockDim_x4, NZ4/blockDim_y4 );
	dim3 dimBlock4( blockDim_x4, blockDim_y4 );

	// space for arrays on CPU and GPU
	mi4 = NNN * sizeof( int ); //memory size int
	mf4 = NNN * sizeof( double );//memory size double

	double *ddd_rho, *ddd_Cphi, *ddd_rh;
	double *ddd_P1,*ddd_P2,*ddd_P3,*ddd_P4,*ddd_P5,*ddd_temp;
	int *ddd_flag,*ddd_iflag,*ddd_jflag,*ddd_oflag,*ddd_tflag;

	double *rrho4;
	int *fflag4;
	rrho4=vec(NN);
	fflag4 = ivec(NN);

	cudaMalloc( (void **)&ddd_rho,  mf4);
	cudaMalloc( (void **)&ddd_Cphi,  mf4);
	cudaMalloc( (void **)&ddd_rh,  mf4);
	cudaMalloc( (void **)&ddd_P1,  mf4);
	cudaMalloc( (void **)&ddd_P2,  mf4);
	cudaMalloc( (void **)&ddd_P3,  mf4);
	cudaMalloc( (void **)&ddd_P4,  mf4);
	cudaMalloc( (void **)&ddd_P5,  mf4);
	cudaMalloc( (void **)&ddd_temp,  mf4);

	cudaMalloc( (void **)&ddd_flag,  mi4);
	cudaMalloc( (void **)&ddd_iflag,  mi4);
	cudaMalloc( (void **)&ddd_jflag,  mi4);
	cudaMalloc( (void **)&ddd_oflag,  mi4);
	cudaMalloc( (void **)&ddd_tflag,  mi4);

/////////////////////

	rrho=vec(N),rrh=vec(N);
	PP1=vec(N),PP2=vec(N),PP3=vec(N),PP4=vec(N),PP5=vec(N);
	fflag=ivec(N),iiflag=ivec(N),jjflag=ivec(N),ooflag=ivec(N);

	//�y�[�W���b�N�������̊m�ہBCPU��GPU�Ԃ̃f�[�^�]���������ɍs�������ꍇ�B
	cudaMallocHost((void **)&temp,mf);
	cudaMallocHost((void **)&pphi,mf);
	cudaMallocHost((void **)&SSph0,mf);
	cudaMallocHost((void **)&SSph1,mf);
	cudaMallocHost((void **)&SSph2,mf);


	//GPU�́u�f�o�C�X�������v��ɁAmemsize_**�̔z����m�ۂ���
	cudaMalloc( (void **)&d_err,  mf );
	cudaMalloc( (void **)&dd_err,  mf2 );

	cudaMalloc( (void **)&d_phi,  mf );
	cudaMalloc( (void **)&d_rho,  mf );
	cudaMalloc( (void **)&d_temp, mf );
	cudaMalloc( (void **)&d_data, mf );
	cudaMalloc( (void **)&d_rh, mf );
	cudaMalloc( (void **)&d_Sph0, mf );
	cudaMalloc( (void **)&d_Sph1, mf );
	cudaMalloc( (void **)&d_Sph2, mf );

	cudaMalloc( (void **)&d_P1,  mf );
	cudaMalloc( (void **)&d_P2,  mf );
	cudaMalloc( (void **)&d_P3,  mf );
	cudaMalloc( (void **)&d_P4,  mf );
	cudaMalloc( (void **)&d_P5,  mf );
	cudaMalloc( (void **)&d_flag,  mi );
	cudaMalloc( (void **)&d_iflag,  mi );
	cudaMalloc( (void **)&d_jflag,  mi );
	cudaMalloc( (void **)&d_oflag,  mi );

///////////////////////////////////////CUDA////////////////////////////////////////////


	superbee = 2.0;  //���l�������邲�Ƃɋ}�s�B�ł�superbee��������2�܂ŁB
	kappa    = 1.0/3.0;  //superbee


	////////�w�����z���c�������̌W��
	HelmA[0]=1.986e-4*1e4;//m^-2 Torr^-2
	HelmA[1]=0.0051  *1e4;//m^-2 Torr^-2
	HelmA[2]=0.4886  *1e4;//m^-2 Torr^-2

	HelmG[0]=0.0553  *1e2; //in m^-1 Torr^-1
	HelmG[1]=0.1460  *1e2; //in m^-1 Torr^-1
	HelmG[2]=0.89    *1e2; //in m^-1 Torr^-1
	/////////////////////////


	mesh_generator(MESH_R_FILE,MESH_Z_FILE,NR,NZ,r,z,rh,zh,dr,dz,Vol,Sr,Sz);

	mesh_context_set_defaults(&g_mesh);
	if (mesh_context_load_cfg(&g_mesh, "inputdata/mesh_physics.cfg") != 0) {
		fprintf(stderr, "Failed to load inputdata/mesh_physics.cfg\n");
		exit(1);
	}
	int mesh_context_status = mesh_context_finalize(&g_mesh, NR, NZ, rh, zh);
	if (mesh_context_status != 0) {
		fprintf(stderr, "Invalid MeshContext: status=%d\n", mesh_context_status);
		exit(1);
	}
	mesh_context_print(&g_mesh, stdout);

	for(i=0;i<NR;i++){
		for(j=0;j<NZ;j++){
			if (pow(zh[j]/bgap, 2) - pow(rh[i]/a, 2) >= 1)flag[i][j]=1;
			else flag[i][j]=0;
			phi[i][j] = dtELr[i][j] = dtELz[i][j] = 0.0;
			Dvx[i][j]=Dvy[i][j]=0.0;
		}
	}


	//�ڗ��v�Z�̋��E������ݒ肷�邽�߂̃t���O����
	call_flag(NR,NZ,flag,tflag,oflag,iflag,jflag);

	double *y;

	particle = cmat(n_particles,50);
	y=vec(n_particles); 
	ireactl=imat(200,3),ireactr=imat(200,3);
	ereactl=imat(200,3),ereactr=imat(200,3);

	ereactl_1st=ivec(200),ereactl_2nd=ivec(200),ereactl_3rd=ivec(200);
	ereactr_1st=ivec(200),ereactr_2nd=ivec(200),ereactr_3rd=ivec(200);

	ireactl_1st=ivec(200),ireactl_2nd=ivec(200),ireactl_3rd=ivec(200);
	ireactr_1st=ivec(200),ireactr_2nd=ivec(200),ireactr_3rd=ivec(200);

	nreactl_1st=ivec(200),nreactl_2nd=ivec(200),nreactl_3rd=ivec(200);
	nreactr_1st=ivec(200),nreactr_2nd=ivec(200),nreactr_3rd=ivec(200);

	nreactl=imat(200,3),nreactr=imat(200,3);
	eK0 = vec(200),eK1 = vec(200),eK2 = vec(200),eER=vec(200),eRnum=ivec(200);
	iK0 = vec(200),iK1 = vec(200),iK2 = vec(200),iER=vec(200),iRnum=ivec(200);
	iK0m = vec(200);
	nK0 = vec(200),nK1 = vec(200),nK2 = vec(200),nER=vec(200),nRnum=ivec(200);
	nK0m = vec(200), nK1m = vec(200), nK2m = vec(200);
	eER_mod=vec(200),iER_mod=vec(200),nER_mod=vec(200);
	eK2_mod=vec(200);
	T=mat(NR,NZ);

	/////////////////////////////vibrational relaxation////////////////////////

	double **dEv, **Ev, *c1, *c2, *c3;
	double *TN2TH2O, *kN2H2O, *nkN2H2O;

	dEv=mat(5,30),Ev=mat(5,30),c1=vec(6),c2=vec(6),c3=vec(6);
	TN2TH2O=vec(5),kN2H2O=vec(5),nkN2H2O=vec(5);

	TN2TH2O[0]=293.0 ,kN2H2O[0]=1.2e-14;
	TN2TH2O[1]=523.0 ,kN2H2O[1]=2.5e-14;
	TN2TH2O[2]=693.0 ,kN2H2O[2]=5.1e-14;
	TN2TH2O[3]=963.0 ,kN2H2O[3]=1.1e-13;
	spline(TN2TH2O,kN2H2O,4,nkN2H2O);  //N2-H2O��2������

	Read_constant(dEv,Ev,c1,c2,c3);

	//////////////////////////////////////////////////////////////////////

	//�l�����闱�q��Ə����Z�x��ǂݍ���
	fp = fopen(INITIAL_FILE, "r");
	n_particles = Initial_condition(fp,particle,y);
	fclose(fp);
	
	//�C���̕ω��ɂ���Ĉ����N������闱�q���x�̕ω����l������
	for (i = 0; i < n_particles; ++i) {
		y[i] *= airpre;
	}		

	//���d���Ŏg���B�v�Z�R�X�g���팸���邽�߁B
	numO2	= particle_number("O2",particle,n_particles);
	numO2p	= particle_number("O2p",particle,n_particles);
	nume	= particle_number("e",particle,n_particles);

	numN2	= particle_number("N2",particle,n_particles);
	numH2O	= particle_number("H2O",particle,n_particles);


	//�����Z�x��PARTICLE�Ɋi�[
	sum = 0.0;
	for(k=0;k<n_particles;k++)sum += y[k];
	inv_sum = 1.0/sum;
	for(i=0;i<NR;i++)for(j=0;j<NZ;j++)for(k=0;k<n_particles;k++)PARTICLE[i][j][k] = y[k]*inv_sum;

	//�_�f�ƒ��f�̃p�[�Z���g�Z�x���Z�o���Ă���
	pO2 = y[ particle_number("O2",particle,n_particles) ]*inv_sum;
	pN2 = y[ particle_number("N2",particle,n_particles) ]*inv_sum;
	//pH2O = (1-pO2-pN2);

	MolMass = (pN2*(28.0) + pO2*(32.0));

	//Townsent_ionization coefficient (��/cm)

	int icn = 0;
	for(i=260;i<BN;i++){
		alpTd[icn] = TdE[i];
		
		/*
		if(pN2==0.0){ // pN2 = 0.0���ƃw�����z���c�������Ȃ��Ȃ邩��B
			dalp[icn]= (dc[16][i]*(MOL*1e-30))/(dv[i]*TdE[i]*1e-21)*0.01;
		}else{
			dalp[icn]= (dc[16][i]*(MOL*pN2))/(dv[i]*TdE[i]*1e-21)*0.01;
		}
		
		
		*/
		dalp[icn]= (dc[16][i]*(MOL*pN2) + 0.2*1.19*dc[28][i]*(MOL*pO2))/(dv[i]*TdE[i]*1e-21)*0.01;
		icn++;
	}
	AN = icn;
	spline(alpTd,dalp,AN,ddalp);

/*
double kalp;
for(i=0;i<10000;i++){
splint(alpTd,dalp,ddalp,AN,(double)i*0.1,&kalp);
printf("%d\t%e\n",i,kalp);
}
exit(0);
*/

	printf("\n*** Atmospheric Pressure Streamer Simulation ***\n");
	printf("*** Gas Component: O2 = %.2f\%, N2 = %.2f\%, H2O = %.2f\%\n\n", pO2*100,pN2*100,100*pH2O);

	double molV, air_kg, g0, atmpa, rgas, u0, v0, rou0, t0, p0, ecp, hrm, hzm, absr;

	air_kg   = MolMass/1000.0;// �K�X�̕��ώ��� kg/mol

	g0   = 1.4;              //��M��
	atmpa=1.013e+5*airpre;          // (Pa) = 1atm
	rgas =8.314/air_kg;      // gas constant (Pa*m^3)/(K*kg)  R=8.314427[(Pa*m^3) /( K mol)]
	//  rgas =rgas/(Avogadro); // gas constant (Pa/((�R/m^3)*K))  1 (mol/m^3) = Avogadro (�R/m^3)

	u0   = 0.0;
	v0   = 0.0;
	p0   = atmpa;
	t0   = T0;
	molV = rgas*300.0/atmpa;//22.413996e-3; //�����̐�(m^3/mol) (300K, 1atm�̂Ƃ�)

	rou0 = 1.0/(rgas*t0/p0);  // mol �� ���x(kg/m^3)  �Ⴆ��1mol,1�C���ł� T = p0/(rou0*rgas) =273 K

//	t0   = T0;            // K
//	p0   = rou0*rgas*t0;     //


	//�d�ׁA���q�̏������z
	first_q(NR,NZ,flag,rh,zh,ne,N2p,O2p,O2m,Om);

	//�d�q�Փ˔����̔������X�g��ǂݍ���
	fp = fopen(E_REACTION_FILE, "r");
	ne_react = Read_reaction(fp,NUM_L,NUM_R,n_particles,eRnum,ereactl,ereactr,eK0,eK1,eK2,eER,particle);
	fclose(fp);

	for(i=0;i<ne_react;i++){
		ereactl_1st[i]=ereactl[eRnum[i]][0];
		ereactl_2nd[i]=ereactl[eRnum[i]][1];
		ereactl_3rd[i]=ereactl[eRnum[i]][2];

		ereactr_1st[i]=ereactr[eRnum[i]][0];
		ereactr_2nd[i]=ereactr[eRnum[i]][1];
		ereactr_3rd[i]=ereactr[eRnum[i]][2];

		eER_mod[i]=eER[eRnum[i]];
		eK2_mod[i]=eK2[eRnum[i]];
	}

	//�C�I�����܂ޔ����̔������X�g��ǂݍ���
	fp = fopen(I_REACTION_FILE, "r");  //���w�������X�g��ǂݍ���
	ni_react = Read_reaction(fp,NUM_L,NUM_R,n_particles,iRnum,ireactl,ireactr,iK0,iK1,iK2,iER,particle);
	fclose(fp);
	for(i=0;i<ni_react;i++)iK0m[i]=iK0[iRnum[i]];


	for(i=0;i<ni_react;i++){
		ireactl_1st[i]=ireactl[iRnum[i]][0];
		ireactl_2nd[i]=ireactl[iRnum[i]][1];
		ireactl_3rd[i]=ireactl[iRnum[i]][2];

		ireactr_1st[i]=ireactr[iRnum[i]][0];
		ireactr_2nd[i]=ireactr[iRnum[i]][1];
		ireactr_3rd[i]=ireactr[iRnum[i]][2];

		iER_mod[i]=iER[iRnum[i]];
	}

	//�������q�̔������X�g��ǂݍ���
	fp = fopen(N_REACTION_FILE, "r");  //���w�������X�g��ǂݍ���
	nn_react = Read_reaction(fp,NUM_L,NUM_R,n_particles,nRnum,nreactl,nreactr,nK0,nK1,nK2,nER,particle);
	fclose(fp);

	for(i=0;i<nn_react;i++){
		nK0m[i]=nK0[nRnum[i]];
		nK1m[i]=nK1[nRnum[i]];
		nK2m[i]=nK2[nRnum[i]];
	}
	for(i=0;i<nn_react;i++){
		nreactl_1st[i]=nreactl[nRnum[i]][0];
		nreactl_2nd[i]=nreactl[nRnum[i]][1];
		nreactl_3rd[i]=nreactl[nRnum[i]][2];

		nreactr_1st[i]=nreactr[nRnum[i]][0];
		nreactr_2nd[i]=nreactr[nRnum[i]][1];
		nreactr_3rd[i]=nreactr[nRnum[i]][2];

		nER_mod[i]=nER[nRnum[i]];
	}

/*
for (i=0; i<ne_react; i++) {
    printf("%2d %4s  %4s  %4s  -->  %4s  %4s  %4s  %2.2e\t%2.2f\t%2.2f\n",eRnum[i],
	particle[ereactl[eRnum[i]][0]], particle[ereactl[eRnum[i]][1]],
	particle[ereactl[eRnum[i]][2]],
	particle[ereactr[eRnum[i]][0]], particle[ereactr[eRnum[i]][1]],
	particle[ereactr[eRnum[i]][2]], eK0[eRnum[i]], eK1[eRnum[i]], eK2[eRnum[i]]);
}
exit(0);
*/

	pureAir_kg= (pN2*(28) + pO2*(32))/1000.0;       // ��C�̕��ώ��� kg/mol
	for(i=0;i<NR;i++)for(j=0;j<NZ;j++)T[i][j]=T0;

	//���U��
	discretization(NR,NZ,rh,zh,P1,P2,P3,P4,P5,a,bgap,iflag,jflag,oflag,flag);  //poisson�v�Z�ɕK�v��mesh�Ԋu��hh�ɕۑ�

	//CUDA�p��1�����z��ɃR�s�[
	for(i=0;i<NR;i++){
		for(j=0;j<NZ;j++){
			PP1[point(NZ,i,j)]= P1[i][j];
			PP2[point(NZ,i,j)]= P2[i][j];
			PP3[point(NZ,i,j)]= P3[i][j];
			PP4[point(NZ,i,j)]= P4[i][j];
			PP5[point(NZ,i,j)]= P5[i][j];
			rrh[point(NZ,i,j)] = rh[i];

			fflag[point(NZ,i,j)]= flag[i][j];
			iiflag[point(NZ,i,j)]= iflag[i][j];
			jjflag[point(NZ,i,j)]= jflag[i][j];
			ooflag[point(NZ,i,j)]= oflag[i][j];			
		}
	}


	//CUDA�������ɃR�s�[
	cudaMemcpy( d_P1, PP1, mf, cudaMemcpyHostToDevice );
	cudaMemcpy( d_P2, PP2, mf, cudaMemcpyHostToDevice );
	cudaMemcpy( d_P3, PP3, mf, cudaMemcpyHostToDevice );
	cudaMemcpy( d_P4, PP4, mf, cudaMemcpyHostToDevice );
	cudaMemcpy( d_P5, PP5, mf, cudaMemcpyHostToDevice );
	cudaMemcpy( d_rh, rrh, mf, cudaMemcpyHostToDevice );
	cudaMemcpy( d_flag, fflag, mi, cudaMemcpyHostToDevice );
	cudaMemcpy( d_iflag, iiflag, mi, cudaMemcpyHostToDevice );
	cudaMemcpy( d_jflag, jjflag, mi, cudaMemcpyHostToDevice );
	cudaMemcpy( d_oflag, ooflag, mi, cudaMemcpyHostToDevice );

	for(i=0;i<NR;i++)for(j=0;j<NZ;j++)pphi[point(NZ,i,j)] = 0.0;

	cudaMemcpy( d_err, pphi, mf, cudaMemcpyHostToDevice );
	cudaMemcpy( d_temp, pphi, mf, cudaMemcpyHostToDevice );
	cudaMemcpy( d_phi, pphi, mf, cudaMemcpyHostToDevice );
	cudaMemcpy( d_Sph0, pphi, mf, cudaMemcpyHostToDevice );
	cudaMemcpy( d_Sph1, pphi, mf, cudaMemcpyHostToDevice );
	cudaMemcpy( d_Sph2, pphi, mf, cudaMemcpyHostToDevice );

	START=-1;
	dt = 0.125e-12;
	time = -5.0e-9;

	fp=fopen("outputdata/current/current.dat","w");
	fclose(fp);

	fp=fopen("outputdata/current/Power.dat","w");
	fclose(fp);


	V        = 28.0e+3;
	A        = V;
	B1       = 30.276901e-9;
	B2       = B1 + 0.0e-9;
	C        = 16.5e-9;
	D	 = 0.003*1e9;


	//���E����Br, Bz �̐ݒ�
	for(i=0;i<NR;i++){
		for(j=0;j<NZ;j++){
			if(iflag[i][j] || jflag[i][j] || oflag[i][j]){
				double electrode_arg = pow(zh[j]/bgap,2)-1.0;
				if(electrode_arg <= 0.0){
					Br[i][j] = 0.0;
					Bz[i][j] = 0.0;
				}else{
					hrm=rh[i] - a*sqrt(electrode_arg);
					hzm=bgap*sqrt(1.0+pow(rh[i]/a,2)) - zh[j];
					absr = sqrt(hrm*hrm + hzm*hzm);

					if(absr <= 0.0 || !isfinite(absr)){
						Br[i][j] = 0.0;
						Bz[i][j] = 0.0;
					}else{
						Br[i][j] = hrm/absr;
						Bz[i][j] = hzm/absr;
					}
				}
			}
		}
	}

	spDATA=331;
	spTime=vec(spDATA),dspV=vec(spDATA),ddspV=vec(spDATA);
	fp=fopen(VOLTAGE_FILE,"r");
	for(j=0;j<spDATA;j++)fscanf(fp,"%le\t%le\n",&spTime[j],&dspV[j]);
	spline(spTime,dspV,spDATA,ddspV);

////////////////////////////////////////////////////////////////////////////

	fp=fopen("Production_rate_cathode.dat","w");
	fprintf(fp,"\t");
	for(j=0;j<ne_react;j++)fprintf(fp,"e%d\t",eRnum[j]);
	for(j=0;j<ni_react;j++)fprintf(fp,"i%d\t",iRnum[j]);
	for(j=0;j<nn_react;j++)fprintf(fp,"n%d\t",nRnum[j]);
	fprintf(fp,"\n");
	fclose(fp);

	fp=fopen("Production_rate_1st.dat","w");
	fprintf(fp,"\t");
	for(j=0;j<ne_react;j++)fprintf(fp,"e%d\t",eRnum[j]);
	for(j=0;j<ni_react;j++)fprintf(fp,"i%d\t",iRnum[j]);
	for(j=0;j<nn_react;j++)fprintf(fp,"n%d\t",nRnum[j]);
	fprintf(fp,"\n");
	fclose(fp);

	fp=fopen("Production_rate_2nd.dat","w");
	fprintf(fp,"\t");
	for(j=0;j<ne_react;j++)fprintf(fp,"e%d\t",eRnum[j]);
	for(j=0;j<ni_react;j++)fprintf(fp,"i%d\t",iRnum[j]);
	for(j=0;j<nn_react;j++)fprintf(fp,"n%d\t",nRnum[j]);
	fprintf(fp,"\n");
	fclose(fp);

	fp=fopen("Production_rate_anode.dat","w");
	fprintf(fp,"\t");
	for(j=0;j<ne_react;j++)fprintf(fp,"e%d\t",eRnum[j]);
	for(j=0;j<ni_react;j++)fprintf(fp,"i%d\t",iRnum[j]);
	for(j=0;j<nn_react;j++)fprintf(fp,"n%d\t",nRnum[j]);
	fprintf(fp,"\n");
	fclose(fp);

/////////////////////////////////////////////////////////////////////////�}���`�O���b�h�p///////////////////////////

	int **flag2,**tflag2,**oflag2,**iflag2,**jflag2;
	double *zh2,*rh2;
	double **cCphi;
	double **cP1,**cP2,**cP3,**cP4,**cP5;

	int **flag4,**tflag4,**oflag4,**iflag4,**jflag4;
	double *zh4,*rh4;
	double **Cphi4;
	double **ccP1,**ccP2,**ccP3,**ccP4,**ccP5;

	zh2=vec(NZ2),rh2=vec(NR2),cCphi=mat(NR2,NZ2);
	flag2=imat(NR2,NZ2),tflag2=imat(NR2,NZ2),oflag2=imat(NR2,NZ2),iflag2=imat(NR2,NZ2),jflag2=imat(NR2,NZ2);
	cP1=mat(NR2,NZ2),cP2=mat(NR2,NZ2),cP3=mat(NR2,NZ2),cP4=mat(NR2,NZ2),cP5=mat(NR2,NZ2);

	zh4=vec(NZ4),rh4=vec(NR4),Cphi4=mat(NR4,NZ4);
	flag4=imat(NR4,NZ4),tflag4=imat(NR4,NZ4),oflag4=imat(NR4,NZ4),iflag4=imat(NR4,NZ4),jflag4=imat(NR4,NZ4);
	ccP1=mat(NR4,NZ4),ccP2=mat(NR4,NZ4),ccP3=mat(NR4,NZ4),ccP4=mat(NR4,NZ4),ccP5=mat(NR4,NZ4);


/////////////


	for(i=0;i<NR2;i++){
		for(j=0;j<NZ2;j++){
			zh2[j]=zh[j*2];
			rh2[i]=rh[i*2];
			if (pow(zh2[j]/bgap, 2) - pow(rh2[i]/a, 2) >= 1)flag2[i][j]=1;
			else flag2[i][j]=0;
			cCphi[i][j] = 0.0;
		}
	}

	//�ڗ��v�Z�̋��E������ݒ肷�邽�߂̃t���O����
	call_flag(NR2,NZ2,flag2,tflag2,oflag2,iflag2,jflag2);

	//���U��
	discretization(NR2,NZ2,rh2,zh2,cP1,cP2,cP3,cP4,cP5,a,bgap,iflag2,jflag2,oflag2,flag2);  //poisson�v�Z�ɕK�v��mesh�Ԋu��hh�ɕۑ�

	for(i=0;i<NR2;i++)for(j=0;j<NZ2;j++)rrho2[point(NZ2,i,j)] = rh2[i];
	cudaMemcpy( dd_rh, rrho2, mf2, cudaMemcpyHostToDevice );
	for(i=0;i<NR2;i++)for(j=0;j<NZ2;j++)rrho2[point(NZ2,i,j)] = cP1[i][j];
	cudaMemcpy( dd_P1, rrho2, mf2, cudaMemcpyHostToDevice );
	for(i=0;i<NR2;i++)for(j=0;j<NZ2;j++)rrho2[point(NZ2,i,j)] = cP2[i][j];
	cudaMemcpy( dd_P2, rrho2, mf2, cudaMemcpyHostToDevice );
	for(i=0;i<NR2;i++)for(j=0;j<NZ2;j++)rrho2[point(NZ2,i,j)] = cP3[i][j];
	cudaMemcpy( dd_P3, rrho2, mf2, cudaMemcpyHostToDevice );
	for(i=0;i<NR2;i++)for(j=0;j<NZ2;j++)rrho2[point(NZ2,i,j)] = cP4[i][j];
	cudaMemcpy( dd_P4, rrho2, mf2, cudaMemcpyHostToDevice );
	for(i=0;i<NR2;i++)for(j=0;j<NZ2;j++)rrho2[point(NZ2,i,j)] = cP5[i][j];
	cudaMemcpy( dd_P5, rrho2, mf2, cudaMemcpyHostToDevice );

	for(i=0;i<NR2;i++)for(j=0;j<NZ2;j++)fflag2[point(NZ2,i,j)] = flag2[i][j];
	cudaMemcpy( dd_flag, fflag2, mi2, cudaMemcpyHostToDevice );
	for(i=0;i<NR2;i++)for(j=0;j<NZ2;j++)fflag2[point(NZ2,i,j)] = iflag2[i][j];
	cudaMemcpy( dd_iflag, fflag2, mi2, cudaMemcpyHostToDevice );
	for(i=0;i<NR2;i++)for(j=0;j<NZ2;j++)fflag2[point(NZ2,i,j)] = jflag2[i][j];
	cudaMemcpy( dd_jflag, fflag2, mi2, cudaMemcpyHostToDevice );
	for(i=0;i<NR2;i++)for(j=0;j<NZ2;j++)fflag2[point(NZ2,i,j)] = tflag2[i][j];
	cudaMemcpy( dd_tflag, fflag2, mi2, cudaMemcpyHostToDevice );
	for(i=0;i<NR2;i++)for(j=0;j<NZ2;j++)fflag2[point(NZ2,i,j)] = oflag2[i][j];
	cudaMemcpy( dd_oflag, fflag2, mi2, cudaMemcpyHostToDevice );
////
	for(i=0;i<NR4;i++){
		for(j=0;j<NZ4;j++){
			zh4[j]=zh2[j*2];
			rh4[i]=rh2[i*2];
			if (pow(zh4[j]/bgap, 2) - pow(rh4[i]/a, 2) >= 1)flag4[i][j]=1;
			else flag4[i][j]=0;
			Cphi4[i][j] = 0.0;
		}
	}

	//�ڗ��v�Z�̋��E������ݒ肷�邽�߂̃t���O����
	call_flag(NR4,NZ4,flag4,tflag4,oflag4,iflag4,jflag4);

	//���U��
	discretization(NR4,NZ4,rh4,zh4,ccP1,ccP2,ccP3,ccP4,ccP5,a,bgap,iflag4,jflag4,oflag4,flag4);  //poisson�v�Z�ɕK�v��mesh�Ԋu��hh�ɕۑ�

	for(i=0;i<NR4;i++)for(j=0;j<NZ4;j++)rrho4[point(NZ4,i,j)] = rh4[i];
	cudaMemcpy( ddd_rh, rrho4, mf4, cudaMemcpyHostToDevice );
	for(i=0;i<NR4;i++)for(j=0;j<NZ4;j++)rrho4[point(NZ4,i,j)] = ccP1[i][j];
	cudaMemcpy( ddd_P1, rrho4, mf4, cudaMemcpyHostToDevice );
	for(i=0;i<NR4;i++)for(j=0;j<NZ4;j++)rrho4[point(NZ4,i,j)] = ccP2[i][j];
	cudaMemcpy( ddd_P2, rrho4, mf4, cudaMemcpyHostToDevice );
	for(i=0;i<NR4;i++)for(j=0;j<NZ4;j++)rrho4[point(NZ4,i,j)] = ccP3[i][j];
	cudaMemcpy( ddd_P3, rrho4, mf4, cudaMemcpyHostToDevice );
	for(i=0;i<NR4;i++)for(j=0;j<NZ4;j++)rrho4[point(NZ4,i,j)] = ccP4[i][j];
	cudaMemcpy( ddd_P4, rrho4, mf4, cudaMemcpyHostToDevice );
	for(i=0;i<NR4;i++)for(j=0;j<NZ4;j++)rrho4[point(NZ4,i,j)] = ccP5[i][j];
	cudaMemcpy( ddd_P5, rrho4, mf4, cudaMemcpyHostToDevice );

	for(i=0;i<NR4;i++)for(j=0;j<NZ4;j++)fflag4[point(NZ4,i,j)] = flag4[i][j];
	cudaMemcpy( ddd_flag, fflag4, mi4, cudaMemcpyHostToDevice );
	for(i=0;i<NR4;i++)for(j=0;j<NZ4;j++)fflag4[point(NZ4,i,j)] = iflag4[i][j];
	cudaMemcpy( ddd_iflag, fflag4, mi4, cudaMemcpyHostToDevice );
	for(i=0;i<NR4;i++)for(j=0;j<NZ4;j++)fflag4[point(NZ4,i,j)] = jflag4[i][j];
	cudaMemcpy( ddd_jflag, fflag4, mi4, cudaMemcpyHostToDevice );
	for(i=0;i<NR4;i++)for(j=0;j<NZ4;j++)fflag4[point(NZ4,i,j)] = tflag4[i][j];
	cudaMemcpy( ddd_tflag, fflag4, mi4, cudaMemcpyHostToDevice );
	for(i=0;i<NR4;i++)for(j=0;j<NZ4;j++)fflag4[point(NZ4,i,j)] = oflag4[i][j];
	cudaMemcpy( ddd_oflag, fflag4, mi4, cudaMemcpyHostToDevice );

//////////////////

	double a2, b2,**hyperboroid;
	double thetaI,theta, Q,sumne,sumph,Q_div_E0,coef;

	hyperboroid=mat(NR,NZ);

		for(i=0;i<NR;i++){
			for(j=0;j<NZ;j++){
				// �d�ɓ��Ȃ�d�ɓd����Ԃ�
				if (flag[i][j]) {
				} else if (zh[j] == 0) { // �ڒn�Ȃ�0��Ԃ�
				} else {
					thetaI = atan2(a, bgap);
					Q = sqrt(bgap*bgap + a*a); // �œ_
					a2 = ((-(rh[i]*rh[i] + zh[j]*zh[j] - Q*Q) + sqrt(pow((rh[i]*rh[i] + zh[j]*zh[j] - Q*Q), 2) - 4.0*(-rh[i]*rh[i] *Q*Q)))*0.5);
					a2 = sqrt(a2);
					b2 = sqrt(Q*Q - a2*a2);
					theta = atan2(a2, b2);

					hyperboroid[i][j]= (log(1.0/tan(theta*0.5)))/(log(1.0/(tan(thetaI*0.5))));
				}
			}
		}

//////////////////////////////intcnd(nbegin)//////////////////////////
	for(i=0;i<NR;i++){
		for(j=0;j<NZ;j++){
				mvr[i][j] = 0.0;
				mvz[i][j] = 0.0;
				rou[i][j] = rou0;
				p[i][j] = rou[i][j]*rgas*T[i][j];

				q1[i][j]=rou[i][j];            // kg/m^3
				q2[i][j]=(rou[i][j])*mvr[i][j];  // kg / (s*m^2) �P�ʑ̐ρA�P�ʎ��ԓ�����̗���(kg)  
				q3[i][j]=(rou[i][j])*mvz[i][j];  // kg/m^3*m/s
				q4[i][j]=p[i][j]/(g0-1.0) + (rou[i][j]*Vol[i][j])*(mvr[i][j]*mvr[i][j]+mvz[i][j]*mvz[i][j])*0.5; // kg/m^3 * m^3 *m^2/s^2 = kg m^2/s^2
		}
	}

	bndcnd(rou,mvr,mvz,p,q1,q2,q3,q4,g0,rgas,u0,v0,p0,t0,rou0,flag,iflag,jflag,oflag,tflag);

/////////////////////////////////////////////////////////////

	n1 = particle_number("e",particle,n_particles);
	n2 = particle_number("N2p",particle,n_particles);
	n3 = particle_number("O2p",particle,n_particles);
	n4 = particle_number("O2m",particle,n_particles);
	n5 = particle_number("Om",particle,n_particles);
	n6 = particle_number("H2Op",particle,n_particles);
	n7 = particle_number("OHm",particle,n_particles);
	n8 = particle_number("Hm",particle,n_particles);
	n9 = particle_number("N4p",particle,n_particles);
	n10 = particle_number("O4p",particle,n_particles);
	n11 = particle_number("N2O2p",particle,n_particles);
	n12 = particle_number("O2pH2O",particle,n_particles);
	n13 = particle_number("H3Op",particle,n_particles);
	n14 = particle_number("H3OpH2O",particle,n_particles);
	n15 = particle_number("H3OpH2O2",particle,n_particles);
	n16 = particle_number("H3OpH2O3",particle,n_particles);

	o2v0=particle_number("O2",  particle,n_particles);
	o2v1=particle_number("O2v1",  particle,n_particles);
	o2v2=particle_number("O2v2",  particle,n_particles);
	o2v3=particle_number("O2v3",  particle,n_particles);
	o2v4=particle_number("O2v4",  particle,n_particles);

	n2v0=particle_number("N2",   particle,n_particles);
	n2v1=particle_number("N2v1", particle,n_particles);
	n2v2=particle_number("N2v2", particle,n_particles);
	n2v3=particle_number("N2v3", particle,n_particles);
	n2v4=particle_number("N2v4", particle,n_particles);
	n2v5=particle_number("N2v5", particle,n_particles);
	n2v6=particle_number("N2v6", particle,n_particles);
	n2v7=particle_number("N2v7", particle,n_particles);
	n2v8=particle_number("N2v8", particle,n_particles);

	h2ov0 = particle_number("H2O",   particle,n_particles);
	h2ov1 = particle_number("H2Ov1",   particle,n_particles);
	h2ov2 = particle_number("H2Ov2", particle,n_particles);
	h2ov3 = particle_number("H2Ov3",   particle,n_particles);

	oatm = particle_number("O(P)",particle,n_particles);

	each_mP=vec(ne_react);

	Q_div_E0 = QELEC/E0; //�v�Z�̏ȗ�

	double adt,cdt,ddt,dt_vib;
	int bdt;
	adt = 1.25e-13; //a+2c = 5e-13
	bdt = 300000;
	cdt = 0.875e-13;
	ddt = 1.6895269E-5;

	dt_vib=dt;

	FILE *fdbg;

	double pEPS, **LEx;
	int Priflag = 0;
	int Priflag2 = 0;
	int Prinstp = 1000000000;
	Prinstp2 = 1000000000;
	z_limiter = g_mesh.z_limiter_initial;
	neflag = imat(NR,NZ);
	LEx = mat(NR,NZ);

////////////Routine Start!!!////////////////////////
	for(nstp=(START+1);;nstp++){

		//if(nstp==START+1)pEPS = 1e-10;
		//else pEPS = EPS;
		pEPS = EPS;

//		V = VOLTAGE_RATE*time*1e9*1e3;

		splint(spTime,dspV,ddspV,spDATA,time*1e9,&spV);
		V = spV*1e3;

		//�j�d�ɂ̉�͉�
		for(i=0;i<NR;i++){
			for(j=0;j<NZ;j++){
				// �d�ɓ��Ȃ�d�ɓd����Ԃ�
				if (flag[i][j])Lphi[i][j]=V;
				else if (z[j] == 0.0)Lphi[i][j]=0.0;
				else Lphi[i][j]=V*hyperboroid[i][j];
			}
		}

		e_max=0.0;
		for(i=0;i<NR;i++){
			for(j=0;j<NZ;j++){

				CPp = N2p[i][j]+O2p[i][j]+H2Op[i][j]+N4p[i][j]+O4p[i][j]+N2O2p[i][j]+O2pH2O[i][j]+H3Op[i][j]+H3OpH2O[i][j]+H3OpH2O2[i][j]+H3OpH2O3[i][j];
				CPm = Om[i][j]+O2m[i][j]+ne[i][j]+OHm[i][j]+Hm[i][j];

				if(i==0)rho[i][j] = 0.5*Q_div_E0*(CPp - CPm);
				else rho[i][j] =  rh[i]*Q_div_E0*(CPp - CPm);

				if(e_max<ne[i][j])e_max=ne[i][j];
				if(ne[i][j]<-1.0){
					printf("e_dens_over\n%d\t%d\t%e\n",i,j,ne[i][j]);
					system("/usr/sbin/sendmail -t < mail_ne_over.txt");
					exit(0);
				}
			}
		}
		printf("%d\t%e\t%e\t%f\t%e\n",nstp,time,dt,V,e_max);

		//GPU�������ɃR�s�[
		for(i=0;i<NR;i++)for(j=0;j<NZ;j++)rrho[point(NZ,i,j)] = rho[i][j];
		cudaMemcpy( d_rho, rrho, mf, cudaMemcpyHostToDevice );

/////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////Solve poisson equation by multigrid methoed//////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

		cuda_flag=0;
		iter=0;

		for(cuda=1;cuda<3000;cuda++){

			OMEGA=1.0, itnum=2;
			Poisson_GPU_function(dimGrid,dimBlock,d_phi,d_rho,d_rh, d_temp, NR,NZ,
						d_P1,d_P2,d_P3,d_P4,d_P5,d_flag,OMEGA,itnum);
			iter += itnum;

			Error_poisson_GPU(dimGrid,dimBlock,d_phi,d_rho,d_rh,NR,NZ,d_P1,d_P2,d_P3,d_P4,d_P5,d_flag,d_err);

			Ristriction_GPU(dimGrid2,dimBlock2,NR2,NZ2,NZ,d_err,dd_rho,dd_Cphi);

				OMEGA=1.0, itnum=2;
				Poisson_GPU_function(dimGrid2,dimBlock2,dd_Cphi,dd_rho,dd_rh,dd_temp,NR2,NZ2,
							dd_P1,dd_P2,dd_P3,dd_P4,dd_P5,dd_flag,OMEGA,itnum);

				Error_poisson_GPU(dimGrid2,dimBlock2,dd_Cphi,dd_rho,dd_rh,NR2,NZ2,dd_P1,dd_P2,dd_P3,dd_P4,dd_P5,dd_flag,dd_err);

				Ristriction_GPU(dimGrid4,dimBlock4,NR4,NZ4,NZ2,dd_err,ddd_rho,ddd_Cphi);

					OMEGA=1.0, itnum=3;
					Poisson_GPU_function(dimGrid4,dimBlock4,ddd_Cphi,ddd_rho,ddd_temp,ddd_temp,NR4,NZ4,
								ddd_P1,ddd_P2,ddd_P3,ddd_P4,ddd_P5,ddd_flag,OMEGA,itnum);

					Interporation_GPU(dimGrid2,dimBlock2,NR2,NZ2,NZ4,dd_flag,dd_Cphi,ddd_Cphi);


				OMEGA=1.5, itnum=10;
				Poisson_GPU_function(dimGrid2,dimBlock2,dd_Cphi,dd_rho,dd_rh,dd_temp,NR2,NZ2,
							dd_P1,dd_P2,dd_P3,dd_P4,dd_P5,dd_flag,OMEGA,itnum);


				Interporation_GPU(dimGrid,dimBlock,NR,NZ,NZ2,d_flag,d_phi,dd_Cphi);

			OMEGA=1.5, itnum=12;
			Poisson_GPU_function(dimGrid,dimBlock,d_phi,d_rho, d_rh,d_temp,NR,NZ,
						d_P1,d_P2,d_P3,d_P4,d_P5,d_flag,OMEGA,itnum);
			iter += itnum;

			if(cuda%1==0){
				Convergence_check_GPU(N,d_temp,d_phi,mf,temp,pphi,&error,&Maxphi);
				if ( error/Maxphi < EPS )break;
			}

		}

		printf("---Finish_Poisson_equation---");
		printf("%05d  %e\t%e\n", iter,Maxphi,error/Maxphi);

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

		//CPU�p��2�����z��ɃR�s�[
		for(i=0;i<NR;i++)for(j=0;j<NZ;j++)Cphi[i][j] = pphi[point(NZ,i,j)];


		for(i=0;i<NR;i++){
			for(j=0;j<NZ;j++){
				if (flag[i][j])phi[i][j] = V;
				else phi[i][j] = Cphi[i][j] + Lphi[i][j];
			}
		}

		calc_E(NR,NZ,Lphi,absE,Ey,Ex,air_kg,tflag,oflag,iflag,jflag,flag,a,bgap,rh,zh,rou,nstp,Prinstp); //�d�E�v�Z Lphi��


		//�d���v�Z�̂���
		for (i = 0; i < THREAD_NUM; i++) {
        		targ[i].thread_no = i;  // �X���b�h�֐��̈����f�[�^�̏�����
			pthread_create(&handle[i], NULL, thread_func_forCurrent, (void *)&targ[i]);// �X���b�h�̐��� 
		}
		for (i = 0; i < THREAD_NUM; i++)pthread_join(handle[i], NULL);//�S�ẴX���b�h�̏������I������܂őҋ@



		calc_E(NR,NZ,phi,absE,Ey,Ex,air_kg,tflag,oflag,iflag,jflag,flag,a,bgap,rh,zh,rou,nstp,Prinstp); //�d�E�v�Z phi��

		calc_e_velo(NR,NZ,absE,Ex,Ey,vr,vz,ne,rh,zh,TdE,dv,ddv,BN);  //�d�q���x�v�Z
		calc_ie_velo(NR,NZ,piv_r,piv_z,Ex,Ey,0);  //+�C�I�����x�v�Z
		calc_ie_velo(NR,NZ,miv_r,miv_z,Ex,Ey,1);  //-�C�I�����x�v�Z


if(fabs(Ey[0][0]<-500.0) && Priflag==0){
	Priflag = 1;
	Prinstp = nstp;
	z_limiter = g_mesh.z_limiter_after;
	for(i=0;i<NR-1;i++){
		for(j=0;j<NZ;j++){
			if(ne[i][j]>1e18)neflag[i][j] = 0;
			else neflag[i][j] = 1;
		}
	}
	fp = fopen("Priflag.dat","w");
	fprintf(fp,"%d\n",Prinstp);
	fclose(fp);
}
// Former fixed-index trigger is now sampled at the configured physical z coordinate.
double ne_probe_priflag2 = mesh_sample_axis_linear(ne, zh, NZ, g_mesh.z_trigger_priflag2);
if(ne_probe_priflag2>1e19 && Priflag2==0){
	Priflag2 = 1;			// z = 10 mm�Ńt���O�����Ă�
	Prinstp2 = nstp;
	fp = fopen("Priflag2.dat","w");
	fprintf(fp,"%d\n",Prinstp2);
	fclose(fp);

	for(i=0;i<NR-1;i++){
		for(j=0;j<NZ;j++){
			if(ne[i][j]>1e18)neflag[i][j] = 0;
			else neflag[i][j] = 1;
		}
	}
}



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////���̌v�Z////////////////////////////////
		///////////////////////////////////////////////////////////////////////////
			i = 0;
			for(j=0;j<g_mesh.j_gap;j++)Dvx[i][j] =dt*Diff*(Sr[i+1][j]/(rh[i+1]-rh[i])*(ne[i+1][j]-ne[i][j]))/Vol[i][j];

		for(i=1;i<NR-1;i++){
			for(j=0;j<NZ;j++){
				if(flag[i][j]){
					Dvx[i][j]=0.0;
				}else{
					if(flag[i-1][j]==1){
						Dvx[i][j] = dt*Diff*(Sr[i+1][j]/(rh[i+1]-rh[i])*(ne[i+1][j]-ne[i][j]))/Vol[i][j];

					}else{
						Dvx[i][j] =dt*Diff*(-Sr[i][j]/(rh[i]-rh[i-1])*(ne[i][j]-ne[i-1][j])
								+Sr[i+1][j]/(rh[i+1]-rh[i])*(ne[i+1][j]-ne[i][j]))/Vol[i][j];
					}
				}
			}
		}
			i=NR-1;
			for(j=0;j<NZ;j++)Dvx[i][j] =dt*Diff*(-Sr[i][j]/(rh[i]-rh[i-1])*(ne[i][j]-ne[i-1][j]))/Vol[i][j];


		for(i=0;i<NR;i++){
			for(j=1;j<NZ-1;j++){
				if(flag[i][j]){
					Dvy[i][j]=0.0;
				}else{
					if(flag[i][j+1]==1){
						Dvy[i][j] = dt*Diff*(-Sz[i][j]/(zh[j]-zh[j-1])*(ne[i][j]-ne[i][j-1]))/Vol[i][j];

					}else{
						Dvy[i][j] =dt*Diff*(-Sz[i][j]/(zh[j]-zh[j-1])*(ne[i][j]-ne[i][j-1])
								+Sz[i][j+1]/(zh[j+1]-zh[j])*(ne[i][j+1]-ne[i][j]))/Vol[i][j];
					}
				}
			}
		}
		for(i=0;i<NR;i++){
			j=NZ-1;
			Dvy[i][j] =dt*Diff*(-Sz[i][j]/(zh[j]-zh[j-1])*(ne[i][j]-ne[i][j-1]))/Vol[i][j];
		}
		for(i=0;i<NR;i++){
			j=0;
			Dvy[i][j] =dt*Diff*(Sz[i][j+1]/(zh[j+1]-zh[j])*(ne[i][j+1]-ne[i][j]))/Vol[i][j];
		}

		for(i=0;i<NR;i++){
			for(j=0;j<NZ;j++){
				ne[i][j] += (Dvx[i][j]+Dvy[i][j]);
			}
		}


		mol_boundary(NR,NZ,flag,ne,N2p,O2p,H2Op,O2m,Om,OHm,Hm,N4p,O4p,N2O2p,O2pH2O,H3Op,H3OpH2O,H3OpH2O2,H3OpH2O3,Ex,Ey,absE);

		printf("---Finish_Fluid_equation... ");
		for (i = 0; i < 16; i++) {
        		targ[i].thread_no = i;  // �X���b�h�֐��̈����f�[�^�̏�����
        		pthread_create(&handle[i], NULL, thread_func_fluid, (void *)&targ[i]);// �X���b�h�̐��� 
		}
		for (i = 0; i < 16; i++)pthread_join(handle[i], NULL);//�S�ẴX���b�h�̏������I������܂őҋ@
		printf("\n");


		mol_boundary(NR,NZ,flag,ne,N2p,O2p,H2Op,O2m,Om,OHm,Hm,N4p,O4p,N2O2p,O2pH2O,H3Op,H3OpH2O,H3OpH2O2,H3OpH2O3,Ex,Ey,absE);


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


		////////////////////////Photo Calculation//////////////////////////////////////

		//���d���v�Z�̂���
		for (i = 0; i < THREAD_NUM; i++) {
        		targ[i].thread_no = i;  // �X���b�h�֐��̈����f�[�^�̏�����
			pthread_create(&handle[i], NULL, thread_func_forPHT, (void *)&targ[i]);// �X���b�h�̐��� 
		}
		for (i = 0; i < THREAD_NUM; i++)pthread_join(handle[i], NULL);//�S�ẴX���b�h�̏������I������܂őҋ@



		// copy arrays from CPU to GPU (d_**)
		cudaMemcpy( d_rho, rrho, mf, cudaMemcpyHostToDevice );

///////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Helmholtz Calculation /////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////


		Helm_iter =0;

		///////////////////////////////S_ph0/////////////////////////////////

		iter =0;
		for(cuda=1;cuda<2000;cuda++){


			OMEGA=1.0, itnum=100;
			Helmholtz0_GPU_function(dimGrid,dimBlock,d_Sph0,d_rho,d_rh, d_temp, NR,NZ,
					d_P1,d_P2,d_P3,d_P4,d_P5,d_flag,d_iflag, d_jflag, d_oflag,OMEGA,itnum,pO2);
			iter += itnum;

/*
			Error_Helm_GPU(dimGrid,dimBlock,d_Sph0,d_rho,d_rh, NR, NZ,
						d_P1, d_P2, d_P3, d_P4, d_P5, d_flag,d_iflag,d_jflag,d_oflag,d_err,0,pO2);

			Ristriction_GPU(dimGrid2,dimBlock2,NR2,NZ2,NZ,d_err,dd_rho,dd_Cphi);


				OMEGA=1.0, itnum=2;
				Multi_Helmholtz_GPU_function(dimGrid2,dimBlock2,dd_Cphi,dd_rho,dd_rh, dd_temp, NR2,NZ2,
						dd_P1,dd_P2,dd_P3,dd_P4,dd_P5,dd_flag,dd_iflag, dd_jflag, dd_oflag,OMEGA,itnum);


				Error_Helm_GPU(dimGrid2,dimBlock2,dd_Cphi,dd_rho,dd_rh, NR2, NZ2,
							dd_P1, dd_P2, dd_P3, dd_P4, dd_P5, dd_flag,dd_iflag,dd_jflag,dd_oflag,dd_err,0,pO2);

				Ristriction_GPU(dimGrid4,dimBlock4,NR4,NZ4,NZ2,dd_err,ddd_rho,ddd_Cphi);

					OMEGA=1.0, itnum=4;
					Multi_Helmholtz_GPU_function(dimGrid4,dimBlock4,ddd_Cphi,ddd_rho,ddd_rh, ddd_temp, NR4,NZ4,
							ddd_P1,ddd_P2,ddd_P3,ddd_P4,ddd_P5,ddd_flag,ddd_iflag, ddd_jflag, ddd_oflag,OMEGA,itnum);

					Interporation_GPU(dimGrid2,dimBlock2,NR2,NZ2,NZ4,dd_flag,dd_Cphi,ddd_Cphi);

// 20220826 ������itnum�𑝂₵����AO2�����Ă����肵�Čv�Z�ł���悤�ɂȂ����B
//
				OMEGA=1.9, itnum=10000;
				Multi_Helmholtz_GPU_function(dimGrid2,dimBlock2,dd_Cphi,dd_rho,dd_rh, dd_temp, NR2,NZ2,
						dd_P1,dd_P2,dd_P3,dd_P4,dd_P5,dd_flag,dd_iflag, dd_jflag, dd_oflag,OMEGA,itnum);


				Interporation_GPU(dimGrid,dimBlock,NR,NZ,NZ2,d_flag,d_Sph0,dd_Cphi);

			OMEGA=1.8, itnum=50;
			Helmholtz0_GPU_function(dimGrid,dimBlock,d_Sph0,d_rho,d_rh, d_temp, NR,NZ,
					d_P1,d_P2,d_P3,d_P4,d_P5,d_flag,d_iflag, d_jflag, d_oflag,OMEGA,itnum,pO2);
			iter += itnum;
*/

			if(cuda%1==0){
				Convergence_check_GPU(N,d_temp,d_Sph0,mf,temp,SSph0,&error,&Maxphi);
//				printf("%d\t%e\t%e\n",iter,error/Maxphi,Maxphi);
				if ( error/Maxphi < EPS )break;
			}

		}

		printf("Finish Helmholtz0 function--%05d\t%e\t%e\n",iter,Maxphi,error/Maxphi);
		Helm_iter += iter;

		//////////////////////////////////S_ph1//////////////////

		iter =0;
		for(cuda=1;cuda<2000;cuda++){


			OMEGA=1.0, itnum=2;
			Helmholtz1_GPU_function(dimGrid,dimBlock,d_Sph1,d_rho,d_rh, d_temp, NR,NZ,
					d_P1,d_P2,d_P3,d_P4,d_P5,d_flag,d_iflag, d_jflag, d_oflag,OMEGA,itnum,pO2);
			iter += itnum;


			Error_Helm_GPU(dimGrid,dimBlock,d_Sph1,d_rho,d_rh, NR, NZ,
						d_P1, d_P2, d_P3, d_P4, d_P5, d_flag,d_iflag,d_jflag,d_oflag,d_err,1,pO2);

			Ristriction_GPU(dimGrid2,dimBlock2,NR2,NZ2,NZ,d_err,dd_rho,dd_Cphi);

				OMEGA=1.0, itnum=5;
				Multi_Helmholtz_GPU_function(dimGrid2,dimBlock2,dd_Cphi,dd_rho,dd_rh, dd_temp, NR2,NZ2,
						dd_P1,dd_P2,dd_P3,dd_P4,dd_P5,dd_flag,dd_iflag, dd_jflag, dd_oflag,OMEGA,itnum);
	
				Interporation_GPU(dimGrid,dimBlock,NR,NZ,NZ2,d_flag,d_Sph1,dd_Cphi);

// 20220826 ������OMEGA�����炵����AO2�����Ă����肵�Čv�Z�ł���悤�ɂȂ����B
			OMEGA=1.4, itnum=30;
			Helmholtz1_GPU_function(dimGrid,dimBlock,d_Sph1,d_rho,d_rh, d_temp, NR,NZ,
					d_P1,d_P2,d_P3,d_P4,d_P5,d_flag,d_iflag, d_jflag, d_oflag,OMEGA,itnum,pO2);
			iter += itnum;

			if(cuda%1==0){
				Convergence_check_GPU(N,d_temp,d_Sph1,mf,temp,SSph1,&error,&Maxphi);
//				printf("%d\t%e\t%e\n",iter,error/Maxphi,Maxphi);
				if ( error/Maxphi < EPS )break;
			}
	
		}
		printf("Finish Helmholtz1 function--%05d\t%e\t%e\n",iter,Maxphi,error/Maxphi);
			Helm_iter += iter;


		//////////////////////////////////S_ph2//////////////////

		iter =0;
		for(cuda=1;cuda<2000;cuda++){


// 20220826 Multigrid����߂�
// ������OMEGA��0.2�܂Ō��炵����AO2�����Ă����肵�Čv�Z�ł���悤�ɂȂ����B
			OMEGA=0.1, itnum=200;
			Helmholtz2_GPU_function(dimGrid,dimBlock,d_Sph2,d_rho,d_rh, d_temp, NR,NZ,
					d_P1,d_P2,d_P3,d_P4,d_P5,d_flag,d_iflag, d_jflag, d_oflag,OMEGA,itnum,pO2);
			iter += itnum;


//			Error_Helm_GPU(dimGrid,dimBlock,d_Sph2,d_rho,d_rh, NR, NZ,
//						d_P1, d_P2, d_P3, d_P4, d_P5, d_flag,d_iflag,d_jflag,d_oflag,d_err,2,pO2);
//			Ristriction_GPU(dimGrid2,dimBlock2,NR2,NZ2,NZ,d_err,dd_rho,dd_Cphi);
//				OMEGA=1.0, itnum=20;
//				Multi_Helmholtz_GPU_function(dimGrid2,dimBlock2,dd_Cphi,dd_rho,dd_rh, dd_temp, NR2,NZ2,
//						dd_P1,dd_P2,dd_P3,dd_P4,dd_P5,dd_flag,dd_iflag, dd_jflag, dd_oflag,OMEGA,itnum);
//				Interporation_GPU(dimGrid,dimBlock,NR,NZ,NZ2,d_flag,d_Sph2,dd_Cphi);


//			OMEGA=1.7, itnum=10;
//			OMEGA=1.1, itnum=10;
//			Helmholtz2_GPU_function(dimGrid,dimBlock,d_Sph2,d_rho,d_rh, d_temp, NR,NZ,
//					d_P1,d_P2,d_P3,d_P4,d_P5,d_flag,d_iflag, d_jflag, d_oflag,OMEGA,itnum,pO2);
//			iter += itnum;


			if(cuda%1==0){
				Convergence_check_GPU(N,d_temp,d_Sph2,mf,temp,SSph2,&error,&Maxphi);
//				printf("%d\t%e\t%e\n",iter,error/Maxphi,Maxphi);
				if ( error/Maxphi < EPS )break;
			}

		}
		printf("Finish Helmholtz2 function--%05d\t%e\t%e\n\n",iter,Maxphi,error/Maxphi);
		Helm_iter += iter;

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

		printf("---Finish_Helmholtz_equation--%05d\n",Helm_iter);

		for(i=0;i<NR;i++)for(j=0;j<NZ;j++)phot_num[i][j] = SSph0[point(NZ,i,j)] + SSph1[point(NZ,i,j)] + SSph2[point(NZ,i,j)];

/////////////////////////////////////////////////////////////////////
////////////////////////Reaction Calculation/////////////////////////
/////////////////////////////////////////////////////////////////////


		//2�����z��3������
		for (i = 0; i < THREAD_NUM; i++) {
        		targ[i].thread_no = i;  // �X���b�h�֐��̈����f�[�^�̏�����
			pthread_create(&handle[i], NULL, thread_func_2D_to_3D, (void *)&targ[i]);// �X���b�h�̐��� 
		}
		for (i = 0; i < THREAD_NUM; i++)pthread_join(handle[i], NULL);//�S�ẴX���b�h�̏������I������܂őҋ@


		//�������Ŕ���
		for (i = 0; i < THREAD_NUM; i++) {
        		targ[i].thread_no = i;  // �X���b�h�֐��̈����f�[�^�̏�����
			pthread_create(&handle[i], NULL, thread_func_react, (void *)&targ[i]);// �X���b�h�̐��� 
		}
		for (i = 0; i < THREAD_NUM; i++)pthread_join(handle[i], NULL);//�S�ẴX���b�h�̏������I������܂őҋ@
		printf("\n");

		//3�����z��2������
		for (i = 0; i < THREAD_NUM; i++) {
        		targ[i].thread_no = i;  // �X���b�h�֐��̈����f�[�^�̏�����
			pthread_create(&handle[i], NULL, thread_func_3D_to_2D, (void *)&targ[i]);// �X���b�h�̐��� 
		}
		for (i = 0; i < THREAD_NUM; i++)pthread_join(handle[i], NULL);//�S�ẴX���b�h�̏������I������܂őҋ@

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

		if(0){

			printf("V-V, V-T calculation\tdt_vib=%e\n",dt_vib);
			#pragma omp parallel num_threads(THREAD_NUM)
			{
	
				int i,j,k,x;
				double **kvt, **re_kvt;
				double **kvv0, **re_kvv0, **kvv1, **re_kvv1, **kvv2, **re_kvv2;
				double **kvv3, **re_kvv3, **kvv4, **re_kvv4;
				double *vib, *dvib;
	
				kvt =mat(15,15) ,re_kvt =mat(15,15);
				kvv0=mat(15,15),re_kvv0=mat(15,15);
				kvv1=mat(15,15),re_kvv1=mat(15,15);
				kvv2=mat(15,15),re_kvv2=mat(15,15);
				kvv3=mat(15,15),re_kvv3=mat(15,15);
				kvv4=mat(15,15),re_kvv4=mat(15,15);
				vib=vec(30),dvib=vec(30);
	
				#pragma omp for
				for(i=0;i<NR;i++){
					for(j=0;j<NZ;j++){
		
		
						O2_rate(T[i][j],kvt,re_kvt,kvv0,re_kvv0,dEv[0],Ev);
						N2_rate(T[i][j],kvt,re_kvt,kvv1,re_kvv1,dEv[1],Ev);
						N2_O2_rate(T[i][j],kvt,re_kvt,kvv2,re_kvv2,dEv[0],dEv[1],Ev);
						H2O_O2_rate(T[i][j],kvt,re_kvt,kvv3,re_kvv3,dEv[3],Ev);
					        H2O_N2_rate(T[i][j],kvv4,re_kvv4,TN2TH2O,kN2H2O,nkN2H2O,Ev);
					        Ozone_rate(T[i][j],c1,c2,c3,kvt,re_kvt,Ev);
	
						vib_convert(i,j,vib,Ev);

						vib_relaxation(i,j,vib,dvib,PARTICLE[i][j][oatm]*rou[i][j]*MassNum,
							kvt,kvv0,kvv1,kvv2,kvv3,kvv4,re_kvt,re_kvv0,re_kvv1,re_kvv2,re_kvv3,re_kvv4,T[i][j],Ev);
	
						vib_assemble(i,j,dvib,dt_vib);

						dTvib[i][j] = 0.0;
						for(k=0  ;k<5  ;k++)dTvib[i][j] -= dt_vib*dvib[k]*Ev[0][k];  //Ev��Kelvin�P��, dvib/dt = (n+1�X�e�b�v�̖��x) - (n�X�e�b�v�̖��x)
						for(k=9  ;k<18 ;k++)dTvib[i][j] -= dt_vib*dvib[k]*Ev[1][k-9];  // dTvib(���o�����G�l���M�[) = (n�X�e�b�v�ɂ�����U���G�l���M�[)
						for(k=18 ;k<20 ;k++)dTvib[i][j] -= dt_vib*dvib[k]*Ev[3][k-18];  // �@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@ - (n+1�X�e�b�v�ɂ�����U���G�l���M�[) �@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@ - (n+1�X�e�b�v�ɂ�����U���G�l���M�[)
						dTvib[i][j] -= dt_vib*dvib[20]*5266.4;  // =3649cm-1
						dTvib[i][j] -= dt_vib*dvib[21]*5399.2;  // =3741cm-1


// Axis debug output uses configured physical z coordinates.
//for(k=0  ;k<5  ;k++)printf("%d\t%f\n",k,Ev[0][k]);
//for(k=9  ;k<18 ;k++)printf("%d\t%f\n",k,Ev[1][k-9]);
//for(k=18 ;k<20 ;k++)printf("%d\t%f\n",k,Ev[3][k-18]);
//exit(0);
//}
					}
				}
				free_mat(kvt ,15,15),free_mat(re_kvt ,15,15);
				free_mat(kvv0,15,15),free_mat(re_kvv0,15,15);
				free_mat(kvv1,15,15),free_mat(re_kvv1,15,15);
				free_mat(kvv2,15,15),free_mat(re_kvv2,15,15);
				free_mat(kvv3,15,15),free_mat(re_kvv3,15,15);
				free_mat(kvv4,15,15),free_mat(re_kvv4,15,15);
				free_vec(vib,30),free_vec(dvib,30);
			}

			#pragma omp parallel num_threads(THREAD_NUM)
			{
				int i,j;
				#pragma omp for
				for(i=0;i<NR;i++)for(j=0;j<NZ;j++){
					if(flag[i][j]);
					else q4[i][j] += (1.602e-19*cE[i][j] +  kb*dTvib[i][j]); //q4�͒P�ʑ̐ϓ�����̃G�l���M�[
				}
			}

			printf("End_VV_VT\n");
			i=0,j=mesh_nearest_index(zh, NZ, g_mesh.z_current_integral_max),printf("%d\t%d\t%.9e\t%e\t%e\n",i,j,zh[j],1.602e-19*cE[i][j],kb*dTvib[i][j]);
			i=0,j=mesh_nearest_index(zh, NZ, g_mesh.z_anode),printf("%d\t%d\t%.9e\t%e\t%e\n",i,j,zh[j],1.602e-19*cE[i][j],kb*dTvib[i][j]);
			i=0,j=mesh_nearest_index(zh, NZ, g_mesh.z_trigger_priflag2),printf("%d\t%d\t%.9e\t%e\t%e\n",i,j,zh[j],1.602e-19*cE[i][j],kb*dTvib[i][j]);
			i=0,j=mesh_nearest_index(zh, NZ, g_mesh.z_limiter_after),printf("%d\t%d\t%.9e\t%e\t%e\n",i,j,zh[j],1.602e-19*cE[i][j],kb*dTvib[i][j]);
			i=0,j=mesh_nearest_index(zh, NZ, g_mesh.z_reaction_fall),printf("%d\t%d\t%.9e\t%e\t%e\n",i,j,zh[j],1.602e-19*cE[i][j],kb*dTvib[i][j]);

			dt_vib = dt;

		}else{
		
			#pragma omp parallel num_threads(THREAD_NUM)
			{
				int i,j;
				#pragma omp for
				for(i=0;i<NR;i++)for(j=0;j<NZ;j++){
					if(flag[i][j]);
					else q4[i][j] += (1.602e-19*cE[i][j]);
				}
			}
			dt_vib += dt;

		}

		symmetric_TVD(u0,v0,p0,t0,rou0,dt,g0,rgas,ecp,&res,rou,mvr,mvz,p,q1,q2,q3,q4,flag,iflag,jflag,oflag,tflag);

		if(!isfinite(res)){
			printf("Residual is over!!\n");
			system("/usr/sbin/sendmail -t < mail_res_over.txt");
			exit(0);
		}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////





//		mol_boundary(NR,NZ,flag,ne,N2p,O2p,H2Op,O2m,Om,OHm,Hm,Ex,Ey,absE);

		current = current2 = 0.0;
		current_c = 0.0;
		sumne = 0.0;
		sumph=  0.0;

		for(i=0;i<NR;i++){     //�d���v�Z
			for(j=0;j<g_mesh.j_current_end;j++){
//				if(flag[i][j]){
//				}else{
					//���ʂ͉~�����O���l����current= ��Q rdrd��dz = 2�΁�Q*r drdz �E�E�E
					//[Q/(s*m^2)]  �d����I=Q/s(A),����ēd�����x��J = I/m^2 = Q/(s*m^2)
					// 1Td = 1e-21*E/N [Vm^2] �� E[V/m]  J=W*s , W=V*A
					//MOL[�R/m^3]*mass[g/�R]��g/m^3  //[W/m^3]=[J/(s*m^3)]

					CurrDens = ( (N2p[i][j]+O2p[i][j]+H2Op[i][j]+N4p[i][j]+O4p[i][j]+N2O2p[i][j]+O2pH2O[i][j]+H3Op[i][j]+H3OpH2O[i][j]+H3OpH2O2[i][j]+H3OpH2O3[i][j])*piv_z[i][j]
							- (Om[i][j]+O2m[i][j]+OHm[i][j]+Hm[i][j])*miv_z[i][j]
								- ne[i][j]*vz[i][j]	)*preEz[i][j]
						+  ( (N2p[i][j]+O2p[i][j]+H2Op[i][j]+N4p[i][j]+O4p[i][j]+N2O2p[i][j]+O2pH2O[i][j]+H3Op[i][j]+H3OpH2O[i][j]+H3OpH2O2[i][j]+H3OpH2O3[i][j])*piv_r[i][j]
							- (Om[i][j]+O2m[i][j]+OHm[i][j]+Hm[i][j])*miv_r[i][j]
								- ne[i][j]*vr[i][j]	)*preEr[i][j];

					InducedCurr = dtELr[i][j]*preEr[i][j] + dtELz[i][j]*preEz[i][j];

					current  += QELEC/V*CurrDens*Vol[i][j];
					current2 += E0/V*InducedCurr*Vol[i][j];
					sumne += ne[i][j]*Vol[i][j];
					sumph += phot_num[i][j];
//				}
			}
		}

		//���̌v�Z�̂��߂̏���
		for(i=0;i<NR;i++){
			for(j=0;j<NZ;j++){
				dtELr[i][j] = - preEr[i][j];
				dtELz[i][j] = - preEz[i][j];
			}
		}

/// 2019/1/16�ǉ� ���̓��d�d��, configured radial window //
		for(i=0;i<g_mesh.i_plate_current_end;i++){     //�d���v�Z
			j=1;
			CurrDens = (N2p[i][j]+O2p[i][j]+H2Op[i][j]+N4p[i][j]+O4p[i][j]+N2O2p[i][j]+O2pH2O[i][j]+H3Op[i][j]+H3OpH2O[i][j]+H3OpH2O2[i][j]+H3OpH2O3[i][j])*piv_z[i][j]
					- (Om[i][j]+O2m[i][j]+OHm[i][j]+Hm[i][j])*miv_z[i][j]
						- ne[i][j]*vz[i][j];

			current_c  += QELEC*CurrDens*Sz[i][j];
		}


		if(nstp%1==0){
			fp=fopen("outputdata/current/current.dat","a");
			fprintf(fp,"%d\t%f\t%e\t%e\n",nstp,V,2.0*PI*(current+current2),2.0*PI*(current_c));
			fclose(fp);
		}
/////////////////////////////////

		if(nstp%100==0){
			printf("Streak_write\t");
			sprintf(filename,"outputdata/Streak_Ey_e_cu/Ey_e_cu_%d.dat",nstp);
			fp=fopen(filename,"w");
			i=0;
			for(j=0;j<g_mesh.j_streak_end;j++){
				CurrDens = ( (N2p[i][j]+O2p[i][j]+H2Op[i][j]+N4p[i][j]+O4p[i][j]+N2O2p[i][j]+O2pH2O[i][j]+H3Op[i][j]+H3OpH2O[i][j]+H3OpH2O2[i][j]+H3OpH2O3[i][j])*piv_z[i][j]
						- (Om[i][j]+O2m[i][j]+OHm[i][j]+Hm[i][j])*miv_z[i][j]
							- ne[i][j]*vz[i][j]	)*preEz[i][j]
					+  ( (N2p[i][j]+O2p[i][j]+H2Op[i][j]+N4p[i][j]+O4p[i][j]+N2O2p[i][j]+O2pH2O[i][j]+H3Op[i][j]+H3OpH2O[i][j]+H3OpH2O2[i][j]+H3OpH2O3[i][j])*piv_r[i][j]
						- (Om[i][j]+O2m[i][j]+OHm[i][j]+Hm[i][j])*miv_r[i][j]
							- ne[i][j]*vr[i][j]	)*preEr[i][j];

				InducedCurr = dtELr[i][j]*preEr[i][j] + dtELz[i][j]*preEz[i][j];

				current  = QELEC/V*CurrDens*Vol[i][j];
				current2 = E0/V*InducedCurr*Vol[i][j];
	
				fprintf(fp,"%f\t%f\t%e\t%e\n",zh[j]*1000,-Ey[i][j],ne[i][j],2.0*PI*(current+current2) );
			}
			fclose(fp);

			sprintf(filename,"outputdata/Streak_particle/particle_%d.dat",nstp);
			fp=fopen(filename,"w");
			for(j=0;j<g_mesh.j_streak_end;j++){
				fprintf(fp,"%f\t",zh[j]*1000);
				for(loop=2;loop<n_particles;loop++){
					chname = particle_number(particle[loop],particle,n_particles);
					sum = 0.0;
					for(i=0;i<NR;i++)sum += PARTICLE[i][j][chname]*Vol[i][j]*rou[i][j];
					fprintf(fp,"%e\t",sum*MassNum);
				}
				fprintf(fp,"\n");
			}
			fclose(fp);

			sprintf(filename,"outputdata/Streak_particle_1D/particle_%d.dat",nstp);
			fp=fopen(filename,"w");
			for(j=0;j<g_mesh.j_streak_end;j++){
				fprintf(fp,"%f\t",zh[j]*1000);
				for(loop=2;loop<n_particles;loop++){
					chname = particle_number(particle[loop],particle,n_particles);
					fprintf(fp,"%e\t",PARTICLE[0][j][chname]*rou[0][j]*MassNum);
				}
				fprintf(fp,"\n");
			}
			fclose(fp);
			printf("finish\n");
		}

		if(nstp%100==0)store(nstp,time,Ey,particle,atmpa,air_kg,molV);
//		if(nstp%40000==0)vib_store(nstp);

		// N2C�̎��Ԑϕ�
		loop = 	particle_number("N2C",particle,n_particles);
		chname = particle_number(particle[loop],particle,n_particles);
		for(i=0;i<NR;i++)for(j=0;j<NZ;j++)sumN2C[i][j]+=PARTICLE[i][j][chname]*rou[i][j]*MassNum;

		if(nstp%100==0){
			loop = 	particle_number("N2C",particle,n_particles);
			sprintf(filename,"outputdata/2DN2C/2D%s_%d.dat",particle[loop],nstp);
			fp=fopen(filename,"w");
			chname = particle_number(particle[loop],particle,n_particles);
			for(i=0;i<NR;i++)for(j=0;j<NZ;j++)fprintf(fp,"%e\n",sumN2C[i][j]);
			fclose(fp);
			for(i=0;i<NR;i++)for(j=0;j<NZ;j++)sumN2C[i][j]=0.0;
		}
		if(nstp%2000==0){
			loop = 	particle_number("N2Bp",particle,n_particles);
			sprintf(filename,"outputdata/2DN2Bp/2D%s_%d.dat",particle[loop],nstp);
			fp=fopen(filename,"w");
			chname = particle_number(particle[loop],particle,n_particles);
			for(i=0;i<NR;i++)for(j=0;j<NZ;j++)fprintf(fp,"%e\n",PARTICLE[i][j][chname]*rou[i][j]*MassNum);
			fclose(fp);

			sprintf(filename,"outputdata/2Dpht/2Dpht_%d.dat",nstp);
			fp=fopen(filename,"w");
			for(i=0;i<NR;i++)for(j=0;j<NZ;j++)fprintf(fp,"%e\n",phot_num[i][j]);
			fclose(fp);
		}


		if(nstp%2000==0){
			for(loop=2;loop<n_particles;loop++){
				sprintf(filename,"outputdata/Particle/%s_%d.dat",particle[loop],nstp);
				fp=fopen(filename,"w");
				chname = particle_number(particle[loop],particle,n_particles);
				for(i=0;i<NR;i++)for(j=0;j<NZ;j++){
					
					fprintf(fp,"%e\n",PARTICLE[i][j][chname]*rou[i][j]*MassNum);
				}
				fclose(fp);
			}
		}

		/////////////////����G�l���M�[�v�Z./////////////////////////
		toP = 0;
		for(k=0;k<ne_react;k++)each_mP[k] = 0.0;
		for(i=0;i<NR;i++){    
			for(j=0;j<g_mesh.j_power_end;j++){

				toP += POWER[i][j];
				for(k=0;k<ne_react;k++)each_mP[k] += molP[i][j][k];
			}
		}

		if(nstp%100==0){
			fp=fopen("outputdata/current/Power.dat","a");
			fprintf(fp,"%e\t",toP);
			for(k=0;k<ne_react;k++)fprintf(fp,"%e\t",each_mP[k]);
			fprintf(fp,"\n");
			fclose(fp);
		}
		//////////////////////////////////////////////////////////

////////////////////////////////////////
		if(nstp%100==0){
			fp=fopen("Production_rate_cathode.dat","a");
			fprintf(fp,"%d\t",nstp);
			for(j=0;j<ne_react;j++)fprintf(fp,"%e\t",e_sink[0][j]);
			for(j=0;j<ni_react;j++)fprintf(fp,"%e\t",i_sink[0][j]);
			for(j=0;j<nn_react;j++)fprintf(fp,"%e\t",sink[0][j]);
			fprintf(fp,"\n");
			fclose(fp);

			fp=fopen("Production_rate_1st.dat","a");
			fprintf(fp,"%d\t",nstp);
			for(j=0;j<ne_react;j++)fprintf(fp,"%e\t",e_sink[1][j]);
			for(j=0;j<ni_react;j++)fprintf(fp,"%e\t",i_sink[1][j]);
			for(j=0;j<nn_react;j++)fprintf(fp,"%e\t",sink[1][j]);
			fprintf(fp,"\n");
			fclose(fp);

			fp=fopen("Production_rate_2nd.dat","a");
			fprintf(fp,"%d\t",nstp);
			for(j=0;j<ne_react;j++)fprintf(fp,"%e\t",e_sink[2][j]);
			for(j=0;j<ni_react;j++)fprintf(fp,"%e\t",i_sink[2][j]);
			for(j=0;j<nn_react;j++)fprintf(fp,"%e\t",sink[2][j]);
			fprintf(fp,"\n");
			fclose(fp);

			fp=fopen("Production_rate_anode.dat","a");
			fprintf(fp,"%d\t",nstp);
			for(j=0;j<ne_react;j++)fprintf(fp,"%e\t",e_sink[3][j]);
			for(j=0;j<ni_react;j++)fprintf(fp,"%e\t",i_sink[3][j]);
			for(j=0;j<nn_react;j++)fprintf(fp,"%e\t",sink[3][j]);
			fprintf(fp,"\n");
			fclose(fp);
			for(i=0;i<4;i++)for(j=0;j<200;j++)sink[i][j]=i_sink[i][j]=e_sink[i][j]=0.0;
		}
///////////////////////////////////

		time += dt;

		CFLmemo=0.0;
		for(j=0;j<NZ;j++){
			if(dt*fabs(vz[0][j])/dz[j]>0.9)printf("CFL_OVER!!==%e  (%d,%d)\n",dt*fabs(vz[0][j])/dz[j],0,j);
			if(CFLmemo<dt*fabs(vz[0][j])/dz[j])CFLmemo = dt*fabs(vz[0][j])/dz[j];
		}
		printf("\n");

	}//Routine�I���

}//main�I���

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////



void get_constant(){

	int i,j;
	FILE *fp;

	dc =mat(DATA,BN),ddc=mat(DATA,BN);
	TdE=vec(BN),deV=vec(BN),dv=vec(BN);
	ddeV=vec(BN),ddv=vec(BN),alpTd=vec(AN),dalp=vec(AN),ddalp=vec(AN);
	powerTd = vec(BN), dpower = vec(BN), ddpower = vec(BN);

	fp=fopen(BOLTZMANNFILE,"r");
	for(i=0;i<BN;i++){
		if(feof(fp)!=0)printf("Wrong Num of BN!!\n"),exit(0);
		fscanf(fp,"%lf",&TdE[i]);
		fscanf(fp,"%lf",&deV[i]);
		fscanf(fp,"%lf",&dv[i]);

		for(j=1;j<NUM_REACT;j++){
			fscanf(fp,"%lf",&dc[j][i]);
		}
	}
	fclose(fp);

/*
	fp=fopen("inputdata/ion_co.dat","r");
	for(i=0;i<AN;i++){
		fscanf(fp,"%lf",&alpTd[i]);	//Townsent_ionization coefficient
		fscanf(fp,"%lf",&dalp[i]);	//Townsent_ionization coefficient
	}
	fclose(fp);
*/

	spline(TdE,dv,BN,ddv);
	spline(TdE,deV,BN,ddeV);
	for(i=1;i<NUM_REACT;i++)spline(TdE,dc[i],BN,ddc[i]);
//	spline(alpTd,dalp,AN,ddalp);

	fp=fopen("inputdata/PowerEdep.dat","r");
	for(i=0;i<BN;i++){
		fscanf(fp,"%lf",&powerTd[i]);	//Townsent_ionization coefficient
		fscanf(fp,"%lf",&dpower[i]);	//Townsent_ionization coefficient
	}
	fclose(fp);
	spline(powerTd,dpower,BN,ddpower);

}

void store(int Z,double time,double **Ey, char **particle, double atmpa, double air_kg, double molV){

FILE *fp;
int i,j,skipnum;
char filename[256];

	if(Z%1000==0){
		skipnum=1;

		sprintf(filename,"outputdata/2DE/2DE_%d.dat",Z),	fp=fopen(filename,"w");
		for(i=0;i<NR;i=i+skipnum)for(j=0;j<NZ;j=j+skipnum)fprintf(fp,"%lf\n",absE[i][j]);
		fclose(fp);

		sprintf(filename,"outputdata/2DEx/2DEx_%d.dat",Z),	fp=fopen(filename,"w");
		for(i=0;i<NR;i=i+skipnum)for(j=0;j<NZ;j=j+skipnum)fprintf(fp,"%lf\n",Ex[i][j]);
		fclose(fp);

		sprintf(filename,"outputdata/2DEy/2DEy_%d.dat",Z),	fp=fopen(filename,"w");
		for(i=0;i<NR;i=i+skipnum)for(j=0;j<NZ;j=j+skipnum)fprintf(fp,"%lf\n",Ey[i][j]);
		fclose(fp);

		sprintf(filename,"outputdata/2Dne/ne_%d.dat",Z),	fp=fopen(filename,"w");
		for(i=0;i<NR;i=i+skipnum)for(j=0;j<NZ;j=j+skipnum)fprintf(fp,"%e\n",ne[i][j]);
		fclose(fp);

		sprintf(filename,"outputdata/2DCphi/Cphi_%d.dat",Z),	fp=fopen(filename,"w");
		for(i=0;i<NR;i=i+skipnum)for(j=0;j<NZ;j=j+skipnum)fprintf(fp,"%e\n",Cphi[i][j]);
		fclose(fp);

		sprintf(filename,"outputdata/2DLphi/Lphi_%d.dat",Z),	fp=fopen(filename,"w");
		for(i=0;i<NR;i=i+skipnum)for(j=0;j<NZ;j=j+skipnum)fprintf(fp,"%e\n",Lphi[i][j]);
		fclose(fp);
	}

	if(0){

		sprintf(filename,"outputdata/2DT/2DT_%d.dat",Z),	fp=fopen(filename,"w");
		for(i=0;i<NR;i=i+skipnum)for(j=0;j<NZ;j=j+skipnum)fprintf(fp,"%e\n",T[i][j]);
		fclose(fp);

		sprintf(filename,"outputdata/2Dp/2Dp_%d.dat",Z),	fp=fopen(filename,"w");
		for(i=0;i<NR;i=i+skipnum)for(j=0;j<NZ;j=j+skipnum)fprintf(fp,"%e\n",p[i][j]/atmpa);
		fclose(fp);

		sprintf(filename,"outputdata/2Ddens/2Ddens_%d.dat",Z),	fp=fopen(filename,"w");
		for(i=0;i<NR;i=i+skipnum)for(j=0;j<NZ;j=j+skipnum)fprintf(fp,"%e\n",rou[i][j]);
		fclose(fp);

	}

	sprintf(filename,"outputdata/Ey_ne/ne_%d.dat",Z),		fp=fopen(filename,"w");
	i=0;
	for(j=0;j<g_mesh.j_ey_ne_end;j++){
		fprintf(fp,"%f\t",zh[j]*1000.0);
		fprintf(fp,"%f\t",Ey[i][j]);
		fprintf(fp,"%e\t",ne[i][j]);
		fprintf(fp,"%e\t",N2p[i][j]);
		fprintf(fp,"%e\t",N4p[i][j]);
		fprintf(fp,"%e\t",O2p[i][j]);
		fprintf(fp,"%e\t",O4p[i][j]);
		fprintf(fp,"%e\t",N2O2p[i][j]);
		fprintf(fp,"%e\t",O2m[i][j]);
		fprintf(fp,"%e\t",Om[i][j]);
		fprintf(fp,"%e\t",H2Op[i][j]);
		fprintf(fp,"%e\t",OHm[i][j]);
		fprintf(fp,"%e\t",Hm[i][j]);
		fprintf(fp,"%e\t",O2pH2O[i][j]);
		fprintf(fp,"%e\t",H3Op[i][j]);
		fprintf(fp,"%e\t",H3OpH2O[i][j]);
		fprintf(fp,"%e\t",H3OpH2O2[i][j]);
		fprintf(fp,"%e\n",H3OpH2O3[i][j]);
	}
	fclose(fp);

}

void data_input(char Mol[],int numdata,double **n){

	FILE *fp;
	int i,j;
	double dumm;
	char filename[256];

	sprintf(filename,"outputdata/2D%s/%s_%d.dat",Mol,Mol,numdata);

	fp=fopen(filename,"r");

	printf(filename);
	printf("\n");

	for(i=0;i<NR;i++)for(j=0;j<NZ;j++)fscanf(fp,"%le\t%le\t%le\n",&dumm,&dumm,&n[i][j]);

	fclose(fp);
}

void* thread_func_fluid(void *arg) {
    thread_arg_t *targ;
    targ = (thread_arg_t *)arg;

	switch(targ->thread_no){
		case 0://�d�q
			MUSCL_superbee_methoed_for_e(ne,vr,vz,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			negative_boundary_condition(NR,NZ,ne,flag);
			printf("ne ");
			break;
		case 1://Om
			MUSCL_superbee_methoed_for_mion(Om,miv_r,miv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			negative_boundary_condition(NR,NZ,Om,flag);
			printf("Om ");
			break;
		case 2://O2m
			MUSCL_superbee_methoed_for_mion(O2m,miv_r,miv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			negative_boundary_condition(NR,NZ,O2m,flag);
			printf("O2m ");
			break;
		case 3:	//N2p
			MUSCL_superbee_methoed_for_pion(N2p,piv_r,piv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			positive_boundary_condition(NR,NZ,N2p,flag);
			printf("N2p ");
			break;
		case 4://OHm
			MUSCL_superbee_methoed_for_mion(OHm,miv_r,miv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			negative_boundary_condition(NR,NZ,OHm,flag);
			printf("OHm ");
			break;
		case 5://OHm
			MUSCL_superbee_methoed_for_mion(Hm,miv_r,miv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			negative_boundary_condition(NR,NZ,Hm,flag);
			printf("Hm ");
			break;
		case 6://O2p
			MUSCL_superbee_methoed_for_pion(O2p,piv_r,piv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			positive_boundary_condition(NR,NZ,O2p,flag);
			printf("O2p ");
			break;
		case 7: //H2Op
			MUSCL_superbee_methoed_for_pion(H2Op,piv_r,piv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			positive_boundary_condition(NR,NZ,H2Op,flag);
			printf("H2Op ");
			break;
		case 8: //H2Op
			MUSCL_superbee_methoed_for_pion(N4p,piv_r,piv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			positive_boundary_condition(NR,NZ,N4p,flag);
			printf("N4p ");
			break;
		case 9: //H2Op
			MUSCL_superbee_methoed_for_pion(O4p,piv_r,piv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			positive_boundary_condition(NR,NZ,O4p,flag);
			printf("O4p ");
			break;
		case 10: //H2Op
			MUSCL_superbee_methoed_for_pion(N2O2p,piv_r,piv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			positive_boundary_condition(NR,NZ,N2O2p,flag);
			printf("N2O2p ");
			break;
		case 11: //H2Op
			MUSCL_superbee_methoed_for_pion(O2pH2O,piv_r,piv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			positive_boundary_condition(NR,NZ,O2pH2O,flag);
			printf("O2pH2O ");
			break;
		case 12: //H2Op
			MUSCL_superbee_methoed_for_pion(H3Op,piv_r,piv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			positive_boundary_condition(NR,NZ,H3Op,flag);
			printf("H3Op ");
			break;
		case 13: //H2Op
			MUSCL_superbee_methoed_for_pion(H3OpH2O,piv_r,piv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			positive_boundary_condition(NR,NZ,H3OpH2O,flag);
			printf("H3OpH2O ");
			break;
		case 14: //H2Op
			MUSCL_superbee_methoed_for_pion(H3OpH2O2,piv_r,piv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			positive_boundary_condition(NR,NZ,H3OpH2O2,flag);
			printf("H3OpH2O2 ");
			break;
		case 15: //H2Op
			MUSCL_superbee_methoed_for_pion(H3OpH2O3,piv_r,piv_z,dt,kappa,superbee,NR,NZ,Sr,Sz,Vol,iflag,jflag,oflag);
			positive_boundary_condition(NR,NZ,H3OpH2O3,flag);
			printf("H3OpH2O3 ");
			break;
	}	
}

void bndcnd(double **rou,double **mvr,double **mvz,double **p,double **q1,double **q2,
                double **q3,double **q4,double g0,double rgas,double u0,double v0,double p0,double t0,
                  double rou0,int **flag,int **iflag,int **jflag,int **oflag,int **tflag){

	int i,j,k,wall;
	double cpgas,htotal,absv;

	#pragma omp parallel num_threads(THREAD_NUM)
	{

		int i,j;
		#pragma omp for
		for(i=0;i<NR;i++){
			for(j=0;j<NZ;j++){
					rou[i][j] = q1[i][j];
					mvr[i][j] = q2[i][j]/q1[i][j];
					mvz[i][j] = q3[i][j]/q1[i][j];
					p[i][j] = (g0-1.0)*(q4[i][j]-0.5*rou[i][j]*(mvr[i][j]*mvr[i][j]+mvz[i][j]*mvz[i][j]));
					T[i][j] = p[i][j]/(rgas*rou[i][j]);
			}
		}
	}

	cpgas = rgas*g0/(g0-1.0);  //Cp - Cv = R(�C�̒萔)�ƃ�=Cp/Cv�@���B
	htotal= 0.5*(u0*u0+v0*v0) + t0*cpgas;

	//�j�d�ɓ�
	for(i=0;i<NR;i++){
		for(j=0;j<NZ;j++){
			if(flag[i][j]){;
				mvr[i][j] = 0.0;
				mvz[i][j] = 0.0;
				p[i][j] = 1.0*rgas*T0;
			  	T[i][j] = T0;
				rou[i][j] = 28.966/1000.0/22.413996e-3;
			}else if(iflag[i][j] && i+1<NR){
				absv = sqrt(mvr[i][j]*mvr[i][j] + mvz[i][j]*mvz[i][j] );

				mvr[i][j] = Br[i][j]*absv;
				mvz[i][j] = Bz[i][j]*absv;

				p[i][j] = p[i+1][j];
			  	T[i][j] = T0;
				rou[i][j] = p[i][j]/(rgas*T[i][j]);
			}else if(jflag[i][j] && j>0){
				absv = sqrt(mvr[i][j]*mvr[i][j] + mvz[i][j]*mvz[i][j] );

				mvr[i][j] = Br[i][j]*absv;
				mvz[i][j] = Bz[i][j]*absv;

				p[i][j] = p[i][j-1];
			  	T[i][j] = T0;
				rou[i][j] = p[i][j]/(rgas*T[i][j]);
			}else if(oflag[i][j] && i+1<NR && j>0){
				absv = sqrt(mvr[i][j]*mvr[i][j] + mvz[i][j]*mvz[i][j] );

				mvr[i][j] = Br[i][j]*absv;
				mvz[i][j] = Bz[i][j]*absv;

				p[i][j] = p[i+1][j-1];
			  	T[i][j] = T0;
				rou[i][j] = p[i][j]/(rgas*T[i][j]);
			}
		}
	}			

	//�������E���E����
	i=0;
	for(j=0;j<g_mesh.j_gap;j++){
		if(jflag[i][j] && j>0){
			mvr[i][j] = mvr[i+1][j];
			mvz[i][j] = 0.0;
			p[i][j] = p[i][j-1];
		  	T[i][j] = T0;
			rou[i][j] = p[i][j]/(rgas*T[i][j]);
		}else {
			mvr[i][j] = mvr[i+1][j];
			mvz[i][j] = mvz[i+1][j];
			p[i][j] = p[i+1][j];
		  	T[i][j] = T[i+1][j];//(htotal - 0.5*(mvr[i][j]*mvr[i][j]+mvz[i][j]*mvz[i][j]))/cpgas;
			rou[i][j] = p[i][j]/(rgas*T[i][j]);
		}
	}

	//�㑤���E���E����
	j=NZ-1;
	for(i=0;i<NR;i++){
		if(flag[i][j]);//�d�ɓ������Ŋ��ɑ���ς�
		else if(iflag[i][j]){
			absv = sqrt(mvr[i][j]*mvr[i][j] + mvz[i][j]*mvz[i][j] );

			mvr[i][j] = Br[i][j]*absv;
			mvz[i][j] = Bz[i][j]*absv;
			p[i][j] = p[i][j-1];
		  	T[i][j] = T0;
			rou[i][j] = p[i][j]/(rgas*T[i][j]);
		}else{
			  mvr[i][j] = mvr[i][j-1];//2.0*mvr[i][j-1]-mvr[i][j-2];
			  mvz[i][j] = mvz[i][j-1];//2.0*mvz[i][j-1]-mvz[i][j-2];
			  p[i][j] = p[i][j-1];
		  	  T[i][j] = (htotal - 0.5*(mvr[i][j]*mvr[i][j]+mvz[i][j]*mvz[i][j]))/cpgas;
			rou[i][j] = p[i][j]/(rgas*T[i][j]);
		}
	}

	//�������E���E����
	j=0;
	for(i=0;i<NR;i++){

			mvr[i][j] = mvr[i][j+1];
			mvz[i][j] = 0.0;

			p[i][j] = p[i][j+1];
			T[i][j] = T0;
			rou[i][j] = p[i][j]/(rgas*T[i][j]);
	}

	//�E�����E���E����
	i=NR-1;
	for(j=0;j<NZ;j++){
			  mvr[i][j] = mvr[i-1][j];
			  mvz[i][j] = mvz[i-1][j];
			  p[i][j] = p[i-1][j];
		  	  T[i][j] = (htotal - 0.5*(mvr[i][j]*mvr[i][j]+mvz[i][j]*mvz[i][j]))/cpgas;
			rou[i][j] = p[i][j]/(rgas*T[i][j]);
	}

	#pragma omp parallel num_threads(THREAD_NUM)
	{

		int i,j;
		#pragma omp for
		for(i=0;i<NR;i++){
			for(j=0;j<NZ;j++){
					q1[i][j] = rou[i][j];
					q2[i][j] = rou[i][j]*mvr[i][j];
					q3[i][j] = rou[i][j]*mvz[i][j];
					q4[i][j]=rou[i][j]*(rgas*T[i][j]/(g0-1.0) + (mvr[i][j]*mvr[i][j]+mvz[i][j]*mvz[i][j])*0.5);
			}
		}
	}
}

void symmetric_TVD(double u0,double v0,double p0,double t0,double rou0,double dt,double g0,double rgas,double ecp,double *res,
                      double **rou,double **mvr,double **mvz,double **p,double **q1,double **q2,double **q3,double **q4,
                        int **flag,int **iflag,int **jflag,int **oflag,int **tflag){

	int i,j;
	double **qold1,**qold2,**qold3,**qold4,**dq1,**dq2,**dq3,**dq4,**dqn1,**dqn2,**dqn3,**dqn4;
	double memo,grid;

	qold1=mat(NR,NZ),qold2=mat(NR,NZ),qold3=mat(NR,NZ),qold4=mat(NR,NZ);

	dqn1=mat(NR,NZ),dqn2=mat(NR,NZ),dqn3=mat(NR,NZ),dqn4=mat(NR,NZ);
	dq1=mat(NR,NZ),dq2=mat(NR,NZ),dq3=mat(NR,NZ),dq4=mat(NR,NZ);

	grid = (float)(NR*NZ);


//////////////////////Two-step Runge-Kutta time-marching//////////////////////////

	#pragma omp parallel num_threads(THREAD_NUM)
	{
		int i,j;
		#pragma omp for
		for(i=0;i<NR;i++){            
			for(j=0;j<NZ;j++){
				qold1[i][j]=q1[i][j];
				qold2[i][j]=q2[i][j];
				qold3[i][j]=q3[i][j];
				qold4[i][j]=q4[i][j];
			}
		}
	}

	calrhs(rou,mvr,mvz,p,q1,q2,q3,q4,rgas,g0,dt,ecp,dq1,dq2,dq3,dq4,flag,0); // step1...k1���v�Z
	//������q1-4��rou,v,p,e�̒l���đ�������

	#pragma omp parallel num_threads(THREAD_NUM)
	{
		int i,j;
		#pragma omp for
		for(i=0;i<NR;i++){
			for(j=0;j<NZ;j++){
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
	}

	bndcnd(rou,mvr,mvz,p,q1,q2,q3,q4,g0,rgas,u0,v0,p0,t0,rou0,flag,iflag,jflag,oflag,tflag);

	calrhs(rou,mvr,mvz,p,q1,q2,q3,q4,rgas,g0,dt,ecp,dq1,dq2,dq3,dq4,flag,1); // step2...k2���v�Z


	#pragma omp parallel num_threads(THREAD_NUM)
	{
		int i,j;
		#pragma omp for
		for(i=0;i<NR;i++){		// 2�i�K�����Q�N�b�^ y = y + 1/2*( k1 + k2 )
			for(j=0;j<NZ;j++){
				q1[i][j] = qold1[i][j] + 0.5*(dqn1[i][j] + dq1[i][j]);
				q2[i][j] = qold2[i][j] + 0.5*(dqn2[i][j] + dq2[i][j]);
				q3[i][j] = qold3[i][j] + 0.5*(dqn3[i][j] + dq3[i][j]);
				q4[i][j] = qold4[i][j] + 0.5*(dqn4[i][j] + dq4[i][j]);
			}
		}
	}

	bndcnd(rou,mvr,mvz,p,q1,q2,q3,q4,g0,rgas,u0,v0,p0,t0,rou0,flag,iflag,jflag,oflag,tflag);

	(*res) = 0.0;
	for(i=0;i<NR;i++){
		for(j=0;j<NZ;j++){
			memo = (q1[i][j]-qold1[i][j])*(q1[i][j]-qold1[i][j])
			      +(q2[i][j]-qold2[i][j])*(q2[i][j]-qold2[i][j])
			      +(q3[i][j]-qold3[i][j])*(q3[i][j]-qold3[i][j])
			      +(q4[i][j]-qold4[i][j])*(q4[i][j]-qold4[i][j]);
			(*res) += memo;
		}
	}

	(*res) = sqrt((*res)/grid);

	free_mat(dq1,NR,NZ),  free_mat(dq2,NR,NZ),  free_mat(dq3,NR,NZ),  free_mat(dq4,NR,NZ);
	free_mat(dqn1,NR,NZ), free_mat(dqn2,NR,NZ), free_mat(dqn3,NR,NZ), free_mat(dqn4,NR,NZ);
	free_mat(qold1,NR,NZ),free_mat(qold2,NR,NZ),free_mat(qold3,NR,NZ),free_mat(qold4,NR,NZ);

}


void calrhs(double **rou,double **mvr,double **mvz,double **p,
              double **q1,double **q2,double **q3,double **q4,double rgas,double g0,
                double dt,double ecp,double **dq1,double **dq2,double **dq3,double **dq4,int **flag,int te){

	int i,j,k;

	double sqrt2 = sqrt(2.0);

	double **um,**vm,**hm,**cm;
	double **p1,**p2,**p3,**p4;
	double **a1,**a2,**a3,**a4;
	double **uum, **cmm, **cpm,**dlt;
	double **tv1,**tv2,**tv3,**tv4;


	tv1=mat(NR,NZ),tv2=mat(NR,NZ),tv3=mat(NR,NZ),tv4=mat(NR,NZ);
	um=mat(NR,NZ),vm=mat(NR,NZ),hm=mat(NR,NZ),cm=mat(NR,NZ);
	p1=mat(NR,NZ),p2=mat(NR,NZ),p3=mat(NR,NZ),p4=mat(NR,NZ);
	a1=mat(NR,NZ),a2=mat(NR,NZ),a3=mat(NR,NZ),a4=mat(NR,NZ);
	uum=mat(NR,NZ),cpm=mat(NR,NZ),cmm=mat(NR,NZ),dlt=mat(NR,NZ);

/////////////////x�����ɂ��Čv�Z////////////////////////

	#pragma omp parallel num_threads(THREAD_NUM)
	{
		double dash, pow_um,pow_cm, pow_vm, cm2, inv_cm,d1,d2,d3,d4,b1,b2;
		int i,j;

		#pragma omp for
		for(i=0;i<NR-1;i++){
			for(j=0;j<NZ;j++){
				dash  = sqrt(rou[i+1][j]/rou[i][j]);                  //Roe���ς��Z�o dash�͌v�Z���ȗ������邽��
				um[i][j] = (dash*mvr[i+1][j]+mvr[i][j])/(dash+1.0);          //���x��Roe����
				vm[i][j] = (dash*mvz[i+1][j]+mvz[i][j])/(dash+1.0);          //
				hm[i][j] = (dash*(q4[i+1][j]+p[i+1][j])/rou[i+1][j]      //�P�ʎ��ʂ�����̑S�G���^���s�[
				             +(q4[i  ][j]+p[i  ][j])/rou[i  ][j])/(dash+1.0);
		
				pow_um = um[i][j]*um[i][j];
				pow_vm = vm[i][j]*vm[i][j];
	
				cm2   = (g0-1.0)*(hm[i][j]-0.5*( pow_um + pow_vm ));  //����c��Roe���ς̂Q��B
				cm[i][j] = sqrt(cm2);
	
				pow_cm = cm[i][j]*cm[i][j];
				inv_cm = 1.0/cm[i][j];
	
				uum[i][j]=um[i][j];               //Eigenvalues
				cmm[i][j]=uum[i][j] - cm[i][j];
				cpm[i][j]=uum[i][j] + cm[i][j];
		
				d1 = q1[i+1][j] -q1[i][j];
				d2 = q2[i+1][j] -q2[i][j];
				d3 = q3[i+1][j] -q3[i][j];
				d4 = q4[i+1][j] -q4[i][j];
	
				b1 = (( pow_um + pow_vm )*0.5)*(g0-1.0)/(pow_cm);
				b2 = (g0-1.0)/(pow_cm);
	
				a1[i][j] = 0.5*(b1 + um[i][j]*inv_cm)*d1 - 0.5*(inv_cm + b2*um[i][j])*d2 - 0.5*b2*vm[i][j]*d3 + 0.5*b2*d4;
				a2[i][j] =                 (1.0 - b1)*d1 +                b2*um[i][j]*d2 +     b2*vm[i][j]*d3 -     b2*d4;
				a3[i][j] = 0.5*(b1 - um[i][j]*inv_cm)*d1 + 0.5*(inv_cm - b2*um[i][j])*d2 - 0.5*b2*vm[i][j]*d3 + 0.5*b2*d4;
				a4[i][j] =                  -vm[i][j]*d1 +                        0.0*d2 +                 d3 +    0.0*d4;
	
				dlt[i][j]= fabs(uum[i][j]) + fabs(vm[i][j]) + cm[i][j]*sqrt2;//dlt = mvr + mvz + c*��2
			}
		}
	}

	i= NR-1;
	for(j=0;j<NZ;j++){	
		a1[i][j] = a1[i-1][j];
		a2[i][j] = a2[i-1][j];
		a3[i][j] = a3[i-1][j];
		a4[i][j] = a4[i-1][j];
	}


	/////////


	#pragma omp parallel num_threads(THREAD_NUM)
	{
		double temp, ff1,ff2,delta,s,qq;
		int i,j;

		i=0;
		temp = dt/dr[i];

		#pragma omp for
		for(j=0;j<NZ;j++){

			  ff1 = (temp)*cmm[i][j]*cmm[i][j];
			  ff2 = fabs(cmm[i][j]);
			delta = dlt[i][j]*ecp;
			  if(ff2<delta)ff2 = 0.5*(cm[i][j]*cm[i][j] + delta*delta)/delta;
			    s = sign(1.0,a1[i][j]);
//			  if(i)qq = s*MAX2(0.0, MIN3(s*a1[i-1][j], s*a1[i][j], s*a1[i+1][j]));
			       qq = s*MAX2(0.0, MIN3(s*a1[i][j]  , s*a1[i][j], s*a1[i+1][j]));
			p1[i][j] = -ff1*qq-ff2*(a1[i][j]-qq);

			  ff1 = (temp)*cpm[i][j]*cpm[i][j];
			  ff2 = fabs(cpm[i][j]);
			  if(ff2<delta)ff2 = 0.5*(cpm[i][j]*cpm[i][j] + delta*delta)/delta;
			    s = sign(1.0,a3[i][j]);
//			  if(i)qq = s*MAX2(0.0, MIN3(s*a3[i-1][j], s*a3[i][j], s*a3[i+1][j]));
			       qq = s*MAX2(0.0, MIN3(s*a3[i][j]  , s*a3[i][j], s*a3[i+1][j]));
			p3[i][j] = -ff1*qq-ff2*(a3[i][j]-qq);

			  ff1 = (temp)*uum[i][j]*uum[i][j];
			  ff2 = fabs(uum[i][j]);
			  if(ff2<delta)ff2 = 0.5*(uum[i][j]*uum[i][j] + delta*delta)/delta;
			    s = sign(1.0,a2[i][j]);
//			  if(i)qq = s*MAX2(0.0, MIN3(s*a2[i-1][j], s*a2[i][j], s*a2[i+1][j]));
			       qq = s*MAX2(0.0, MIN3(s*a2[i][j]  , s*a2[i][j], s*a2[i+1][j]));
			p2[i][j] = -ff1*qq-ff2*(a2[i][j]-qq);

			  s = sign(1.0,a4[i][j]);
//			  if(i)qq = s*MAX2(0.0, MIN3(s*a4[i-1][j], s*a4[i][j], s*a4[i+1][j]));
			       qq = s*MAX2(0.0, MIN3(s*a4[i][j]  , s*a4[i][j], s*a4[i+1][j]));
			p4[i][j] = -ff1*qq-ff2*(a4[i][j]-qq);

		}
	}

	#pragma omp parallel num_threads(THREAD_NUM)
	{
		double temp, ff1,ff2,delta,s,qq;
		int i,j;

		#pragma omp for
		for(i=1;i<NR-1;i++){
			temp = (dt/dr[i]);
			for(j=0;j<NZ;j++){

				  ff1 = (temp)*cmm[i][j]*cmm[i][j];
				  ff2 = fabs(cmm[i][j]);
				delta = dlt[i][j]*ecp;
				  if(ff2<delta)ff2 = 0.5*(cm[i][j]*cm[i][j] + delta*delta)/delta;
				    s = sign(1.0,a1[i][j]);
				   qq = s*MAX2(0.0, MIN3(s*a1[i-1][j], s*a1[i][j], s*a1[i+1][j]));
//				  else qq = s*MAX2(0.0, MIN3(s*a1[i][j]  , s*a1[i][j], s*a1[i+1][j]));
				p1[i][j] = -ff1*qq-ff2*(a1[i][j]-qq);
	
				  ff1 = (temp)*cpm[i][j]*cpm[i][j];
				  ff2 = fabs(cpm[i][j]);
				  if(ff2<delta)ff2 = 0.5*(cpm[i][j]*cpm[i][j] + delta*delta)/delta;
				    s = sign(1.0,a3[i][j]);
				   qq = s*MAX2(0.0, MIN3(s*a3[i-1][j], s*a3[i][j], s*a3[i+1][j]));
//				  else qq = s*MAX2(0.0, MIN3(s*a3[i][j]  , s*a3[i][j], s*a3[i+1][j]));
				p3[i][j] = -ff1*qq-ff2*(a3[i][j]-qq);

				  ff1 = (temp)*uum[i][j]*uum[i][j];
				  ff2 = fabs(uum[i][j]);
				  if(ff2<delta)ff2 = 0.5*(uum[i][j]*uum[i][j] + delta*delta)/delta;
				    s = sign(1.0,a2[i][j]);
				   qq = s*MAX2(0.0, MIN3(s*a2[i-1][j], s*a2[i][j], s*a2[i+1][j]));
//				  else qq = s*MAX2(0.0, MIN3(s*a2[i][j]  , s*a2[i][j], s*a2[i+1][j]));
				p2[i][j] = -ff1*qq-ff2*(a2[i][j]-qq);

				    s = sign(1.0,a4[i][j]);
				   qq = s*MAX2(0.0, MIN3(s*a4[i-1][j], s*a4[i][j], s*a4[i+1][j]));
//				  else qq = s*MAX2(0.0, MIN3(s*a4[i][j]  , s*a4[i][j], s*a4[i+1][j]));
				p4[i][j] = -ff1*qq-ff2*(a4[i][j]-qq);
	
			}
		}
	}

	i=NR-1;
	for(j=0;j<NZ;j++){
		p1[i][j] = p1[i-1][j];
		p2[i][j] = p2[i-1][j];
		p3[i][j] = p3[i-1][j];
		p4[i][j] = p4[i-1][j];
	}
	//////////////////////
	#pragma omp parallel num_threads(THREAD_NUM)
	{
		double q, H, e1, e2, e3,e4;
		int i,j;

		#pragma omp for
		for(i=0;i<NR-1;i++){
			for(j=0;j<NZ;j++){

				q      = um[i][j]*um[i][j] + vm[i][j]*vm[i][j];
				H      = cm[i][j]*cm[i][j]/(g0-1.0) + 0.5*q;
	
				tv1[i][j] =                          1.0*p1[i][j] +      1.0*p2[i][j] +                          1.0*p3[i][j] +      0.0*p4[i][j];
				tv2[i][j] =          (um[i][j]-cm[i][j])*p1[i][j] + um[i][j]*p2[i][j] +          (um[i][j]+cm[i][j])*p3[i][j] +      0.0*p4[i][j];
				tv3[i][j] =                   (vm[i][j])*p1[i][j] + vm[i][j]*p2[i][j] +                   (vm[i][j])*p3[i][j] +      1.0*p4[i][j];
				tv4[i][j] = (hm[i][j]-cm[i][j]*um[i][j])*p1[i][j] +    0.5*q*p2[i][j] + (hm[i][j]+cm[i][j]*um[i][j])*p3[i][j] + vm[i][j]*p4[i][j];

				    e1 = q2[i][j] + q2[i+1][j];
				    e2 = q2[i][j]*mvr[i][j] + p[i][j] + q2[i+1][j]*mvr[i+1][j] + p[i+1][j];
				    e3 = q2[i][j]*mvz[i][j] + q2[i+1][j]*mvz[i+1][j];
				    e4 = (q4[i][j]+p[i][j])*mvr[i][j] + (q4[i+1][j] + p[i+1][j])*mvr[i+1][j];
				tv1[i][j] = (e1 + tv1[i][j])*0.5;
				tv2[i][j] = (e2 + tv2[i][j])*0.5;
				tv3[i][j] = (e3 + tv3[i][j])*0.5;
				tv4[i][j] = (e4 + tv4[i][j])*0.5;
			}
		}
	}

	i = NR-1;
	for(j=0;j<NZ;j++){
		tv1[i][j] = tv1[i-1][j];
		tv2[i][j] = tv2[i-1][j];
		tv3[i][j] = tv3[i-1][j];
		tv4[i][j] = tv4[i-1][j];
	}

	///////////////////////////

	#pragma omp parallel num_threads(THREAD_NUM)
	{
		double temp;
		int i,j;

		i = 0;
/*
		temp = dt/(dr[i]*r[i]);
		for(j=0;j<NZ;j++){
			dq1[i][j] = -(rh[i]*tv1[i][j] - rh[i+1]*tv1[i+1][j])*temp;
			dq2[i][j] = -(rh[i]*tv2[i][j] - rh[i+1]*tv2[i+1][j])*temp;// + dt*p[i][j]*1.013e+5;
			dq3[i][j] = -(rh[i]*tv3[i][j] - rh[i+1]*tv3[i+1][j])*temp;
			dq4[i][j] = -(rh[i]*tv4[i][j] - rh[i+1]*tv4[i+1][j])*temp;		
		}
*/
		temp = dt/(dr[i]);
		#pragma omp for
		for(j=0;j<NZ;j++){
			dq1[i][j] = -(tv1[i][j] - tv1[i+1][j])*temp;
			dq2[i][j] = -(tv2[i][j] - tv2[i+1][j])*temp;
			dq3[i][j] = -(tv3[i][j] - tv3[i+1][j])*temp;
			dq4[i][j] = -(tv4[i][j] - tv4[i+1][j])*temp;
		}
	}

	#pragma omp parallel num_threads(THREAD_NUM)
	{
		double temp;
		int i,j;

		#pragma omp for
		for(i=1;i<NR;i++){
/*
			temp = dt/(r[i]*dr[i]);
			for(j=0;j<NZ;j++){
				dq1[i][j] = -(rh[i]*tv1[i][j] - rh[i-1]*tv1[i-1][j])*temp;
				dq2[i][j] = -(rh[i]*tv2[i][j] - rh[i-1]*tv2[i-1][j])*temp;// + dt*p[i][j]*1.013e+5;
				dq3[i][j] = -(rh[i]*tv3[i][j] - rh[i-1]*tv3[i-1][j])*temp;
				dq4[i][j] = -(rh[i]*tv4[i][j] - rh[i-1]*tv4[i-1][j])*temp;
			}
*/
			temp = dt/(dr[i]);
			for(j=0;j<NZ;j++){
				dq1[i][j] = -(tv1[i][j] - tv1[i-1][j])*temp;
				dq2[i][j] = -(tv2[i][j] - tv2[i-1][j])*temp;
				dq3[i][j] = -(tv3[i][j] - tv3[i-1][j])*temp;
				dq4[i][j] = -(tv4[i][j] - tv4[i-1][j])*temp;
			}
		}
	}
/////////////////////////////////////////////////////////////

/////////////////////////y�����ɂ���////////////////////////

	#pragma omp parallel num_threads(THREAD_NUM)
	{

		double d1, d2, d3, d4, b1, b2;
		double pow_cm, inv_cm, cm2, pow_um, pow_vm, dash;
		int i,j;

		#pragma omp for
		for(i=0;i<NR;i++){
			for(j=0;j<NZ-1;j++){
	
				dash  = sqrt(rou[i][j+1]/rou[i][j]);
				um[i][j] = (dash*mvr[i][j+1]+mvr[i][j])/(dash+1.0);
				vm[i][j] = (dash*mvz[i][j+1]+mvz[i][j])/(dash+1.0);
				if(flag[i][j+1]){
					hm[i][j] = (dash*(q4[i][j+1]+p[i][j+1])/rou[i][j] 
				             	     +(q4[i  ][j]+p[i  ][j])/rou[i  ][j])/(dash+1.0);
				}else{
					hm[i][j] = (dash*(q4[i][j+1]+p[i][j+1])/rou[i][j+1] 
				                     +(q4[i  ][j]+p[i  ][j])/rou[i  ][j])/(dash+1.0);
				}
	
				pow_um = um[i][j]*um[i][j];
				pow_vm = vm[i][j]*vm[i][j];
	
				cm2   = (g0-1.0)*(hm[i][j]-0.5*(pow_um + pow_vm));
				cm[i][j] = sqrt(cm2);
	
				pow_cm = cm[i][j]*cm[i][j];
				inv_cm =   1.0/cm[i][j];
	
				uum[i][j] = vm[i][j];
				cmm[i][j] = uum[i][j] - cm[i][j];
				cpm[i][j] = uum[i][j] + cm[i][j];
		
				d1 = q1[i][j+1] - q1[i][j];
				d2 = q2[i][j+1] - q2[i][j];
				d3 = q3[i][j+1] - q3[i][j];
				d4 = q4[i][j+1] - q4[i][j];
	
				b1 = ((pow_um + pow_vm)*0.5)*(g0-1.0)/(pow_cm);
				b2 = (g0-1.0)/(pow_cm);
	
				a1[i][j] = 0.5*(b1 + vm[i][j]*inv_cm)*d1 - 0.5*b2*um[i][j]*d2 - 0.5*(inv_cm + b2*vm[i][j])*d3 + 0.5*b2*d4;
				a2[i][j] =                 (1.0 - b1)*d1 +     b2*um[i][j]*d2 +                b2*vm[i][j]*d3 -     b2*d4;
				a3[i][j] = 0.5*(b1 - vm[i][j]*inv_cm)*d1 - 0.5*b2*um[i][j]*d2 + 0.5*(inv_cm - b2*vm[i][j])*d3 + 0.5*b2*d4;
				a4[i][j] =                  -um[i][j]*d1 +             1.0*d2 +                        0.0*d3    + 0.0*d4;
	
				dlt[i][j]= fabs(uum[i][j]) + fabs(um[i][j]) + cm[i][j]*sqrt2;
			}
		}
	}

	for(i=0;i<NR;i++){
		j = NZ-1;
		a1[i][j] = a1[i][j-1];
		a2[i][j] = a2[i][j-1];
		a3[i][j] = a3[i][j-1];
		a4[i][j] = a4[i][j-1];
	}


	#pragma omp parallel num_threads(THREAD_NUM)
	{
		double temp, ff1,ff2,delta,s,qq;
		int i,j;
		
		j = 0;
		temp = dt/dz[j];

		#pragma omp for
		for(i=0;i<NR;i++){
			  ff1 = (temp)*cmm[i][j]*cmm[i][j];
			  ff2=fabs(cmm[i][j]);
			delta=dlt[i][j]*ecp;
			  if(ff2<delta)ff2 = 0.5*(cm[i][j]*cm[i][j] + delta*delta)/delta;
			    s=sign(1.0,a1[i][j]);
			  qq=s*MAX2(0.0,MIN3(s*a1[i][j], s*a1[i][j], s*a1[i][j+1]));
			p1[i][j]=-ff1*qq-ff2*(a1[i][j]-qq);

			  ff1 = (temp)*cpm[i][j]*cpm[i][j];
			  ff2=fabs(cpm[i][j]);
			  if(ff2<delta)ff2 = 0.5*(cpm[i][j]*cpm[i][j] + delta*delta)/delta;
			    s=sign(1.0,a3[i][j]);
			  qq=s*MAX2(0.0,MIN3(s*a3[i][j], s*a3[i][j], s*a3[i][j+1]));
			p3[i][j]=-ff1*qq-ff2*(a3[i][j]-qq);

			  ff1 = (temp)*uum[i][j]*uum[i][j];
			  ff2=fabs(uum[i][j]);
			  delta=ecp*dlt[i][j];
			  if(ff2<delta)ff2 = 0.5*(uum[i][j]*uum[i][j] + delta*delta)/delta;
			    s=sign(1.0,a2[i][j]);
			  qq=s*MAX2(0.0,MIN3(s*a2[i][j], s*a2[i][j], s*a2[i][j+1]));
			p2[i][j]=-ff1*qq-ff2*(a2[i][j]-qq);

			  s=sign(1.0,a4[i][j]);
			  qq=s*MAX2(0.0,MIN3(s*a4[i][j], s*a4[i][j], s*a4[i][j+1]));
			p4[i][j]=-ff1*qq-ff2*(a4[i][j]-qq);
		}
	}

	#pragma omp parallel num_threads(THREAD_NUM)
	{
		double temp, ff1,ff2,delta,s,qq;
		int i,j;
		#pragma omp for
		for(i=0;i<NR;i++){
			for(j=1;j<NZ-1;j++){
				temp = dt/dz[j];
	
				  ff1 = (temp)*cmm[i][j]*cmm[i][j];
				  ff2=fabs(cmm[i][j]);
				delta=dlt[i][j]*ecp;
				  if(ff2<delta)ff2 = 0.5*(cm[i][j]*cm[i][j] + delta*delta)/delta;
				    s=sign(1.0,a1[i][j]);
				  qq=s*MAX2(0.0,MIN3(s*a1[i][j-1], s*a1[i][j], s*a1[i][j+1]));
				p1[i][j]=-ff1*qq-ff2*(a1[i][j]-qq);
	
				  ff1 = (temp)*cpm[i][j]*cpm[i][j];
				  ff2=fabs(cpm[i][j]);
				  if(ff2<delta)ff2 = 0.5*(cpm[i][j]*cpm[i][j] + delta*delta)/delta;
				    s=sign(1.0,a3[i][j]);
				  qq=s*MAX2(0.0,MIN3(s*a3[i][j-1], s*a3[i][j], s*a3[i][j+1]));
				p3[i][j]=-ff1*qq-ff2*(a3[i][j]-qq);
	
				  ff1 = (temp)*uum[i][j]*uum[i][j];
				  ff2=fabs(uum[i][j]);
				  delta=ecp*dlt[i][j];
				  if(ff2<delta)ff2 = 0.5*(uum[i][j]*uum[i][j] + delta*delta)/delta;
				    s=sign(1.0,a2[i][j]);
				  qq=s*MAX2(0.0,MIN3(s*a2[i][j-1], s*a2[i][j], s*a2[i][j+1]));
				p2[i][j]=-ff1*qq-ff2*(a2[i][j]-qq);
	
				  s=sign(1.0,a4[i][j]);
				  qq=s*MAX2(0.0,MIN3(s*a4[i][j-1], s*a4[i][j], s*a4[i][j+1]));
				p4[i][j]=-ff1*qq-ff2*(a4[i][j]-qq);
			}
		}
	}

	j=NZ-1;
	for(i=0;i<NR;i++){
		p1[i][j] = p1[i][j-1];
		p2[i][j] = p2[i][j-1];
		p3[i][j] = p3[i][j-1];
		p4[i][j] = p4[i][j-1];
	}

	#pragma omp parallel num_threads(THREAD_NUM)
	{
		double q, H, e1, e2, e3,e4;
		int i,j;

		#pragma omp for
		for(i=0;i<NR;i++){
			for(j=0;j<NZ-1;j++){
	
				q      = um[i][j]*um[i][j] + vm[i][j]*vm[i][j];
				H      = cm[i][j]*cm[i][j]/(g0-1.0) + 0.5*q;
	
				tv1[i][j] =                 p1[i][j] +       p2[i][j] +                 p3[i][j] +   0.0*p4[i][j];
				tv2[i][j] =         (um[i][j])*p1[i][j] + um[i][j]*p2[i][j] +         (um[i][j])*p3[i][j] +       p4[i][j];
				tv3[i][j] =   (vm[i][j]-cm[i][j])*p1[i][j] + vm[i][j]*p2[i][j] +   (vm[i][j]+cm[i][j])*p3[i][j] +   0.0*p4[i][j];
				tv4[i][j] = (hm[i][j]-cm[i][j]*vm[i][j])*p1[i][j] + 0.5*q*p2[i][j] + (hm[i][j]+cm[i][j]*vm[i][j])*p3[i][j] + um[i][j]*p4[i][j];
	
				    e1 = q3[i][j] + q3[i][j+1];
				    e2 = q3[i][j]*mvr[i][j] + q3[i][j+1]*mvr[i][j+1];
				    e3 = q3[i][j]*mvz[i][j] + p[i][j] + q3[i][j+1]*mvz[i][j+1] + p[i][j+1];
				    e4 = (q4[i][j]+p[i][j])*mvz[i][j] + (q4[i][j+1] + p[i][j+1])*mvz[i][j+1];
				tv1[i][j] = (e1 + tv1[i][j])*0.5;
				tv2[i][j] = (e2 + tv2[i][j])*0.5;
				tv3[i][j] = (e3 + tv3[i][j])*0.5;
				tv4[i][j] = (e4 + tv4[i][j])*0.5;
			}
		}
	}


	j = NZ-1;
	for(i=0;i<NR;i++){
		tv1[i][j] = tv1[i][j-1];
		tv2[i][j] = tv2[i][j-1];
		tv3[i][j] = tv3[i][j-1];
		tv4[i][j] = tv4[i][j-1];
	}

	#pragma omp parallel num_threads(THREAD_NUM)
	{
		double temp;
		int i,j;

		#pragma omp for
		for(i=1;i<NR;i++){
			for(j=2;j<NZ;j++){
				temp = dt/dz[j];
				dq1[i][j] = dq1[i][j] - (tv1[i][j] - tv1[i][j-1])*temp;
				dq2[i][j] = dq2[i][j] - (tv2[i][j] - tv2[i][j-1])*temp;
				dq3[i][j] = dq3[i][j] - (tv3[i][j] - tv3[i][j-1])*temp;
				dq4[i][j] = dq4[i][j] - (tv4[i][j] - tv4[i][j-1])*temp;
			}
		}
	}
/////////////////////////////////////////////////////////////


	free_mat(um,NR,NZ),free_mat(vm,NR,NZ),free_mat(hm,NR,NZ),free_mat(uum,NR,NZ),free_mat(cmm,NR,NZ);
	free_mat(cm,NR,NZ),free_mat(cpm,NR,NZ),free_mat(dlt,NR,NZ);
	free_mat(a1,NR,NZ),free_mat(a2,NR,NZ),free_mat(a3,NR,NZ),free_mat(a4,NR,NZ);
	free_mat(p1,NR,NZ),free_mat(p2,NR,NZ),free_mat(p3,NR,NZ),free_mat(p4,NR,NZ);
	free_mat(tv1,NR,NZ),free_mat(tv2,NR,NZ),free_mat(tv3,NR,NZ),free_mat(tv4,NR,NZ);


}





