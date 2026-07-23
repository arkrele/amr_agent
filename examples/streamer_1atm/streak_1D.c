#include<stdio.h>
#include <string.h>
#include<math.h>
#include"hfile/memory.h"
#include"hfile/spline.h"
#include "hfile/Reaction.h"

#define NZ  1450
#define NR 256

int NUM_L=3;
int NUM_R=3;

#define SKIPNUM 100
#define INITIAL_FILE  "inputdata/Initial/initial_dry_300K.dat"

double PARTICLE[3002][NZ][60];

char **particle;

static double bgap	= 13.0e-3;	//ギャップ長
static double cr	= 40e-6;	//先端曲率半径
static double dt	= 1.25e-13;	//時間刻み

int n_particles = 100;		//考慮する粒子数、適当に代入

main(){

	FILE *fp,*fq;
	char filename[256], buff[256];
	int i,j,k,ii,filenum,sfilenum,startii,num,pn,line;
	double *Ey,Eypeak,*z, **Ey_mem,**Ne_mem,time,dump,*ne,*cu,**cu_mem;

	Ey=vec(NZ),z=vec(NZ), ne=vec(NZ), cu=vec(NZ);

	sfilenum = 0;


	sprintf(filename,"outputdata/current/current.dat");
	if((fp = fopen(filename, "r")) == NULL){    /* ファイルオープン */
		printf("File is unable to open\n");
		exit(0);        /* 強制終了 */
	}
	line = 0;
	while(fgets(buff,256, fp) != NULL){    /* 1行読み込み */
		++line;    /* 行数カウント */
		//printf("%d:%s", line, buff);    /* 1行表示 */
	}
	filenum = line;

	
	if(filenum>300000)filenum = 300000;

	startii = sfilenum/SKIPNUM;
	filenum = filenum/SKIPNUM;
	printf("startii = %d, finishii = %d\n",startii,filenum);

	Ey_mem=mat(filenum+1,NZ),Ne_mem=mat(filenum+1,NZ),cu_mem=mat(filenum+1,NZ);
	for(i=0;i<filenum;i++)for(j=0;j<NZ;j++)Ey_mem[i][j]=Ne_mem[i][j]=cu_mem[i][j]=0.0;

	////////////////////////////////////////////////////////////
	double *y,**p;
	int chname, loop;

	particle = cmat(n_particles,50);
	y=vec(n_particles); 
	//考慮する粒子種と初期濃度を読み込む
	fp = fopen(INITIAL_FILE, "r");
	n_particles = Initial_condition(fp,particle,y);
	fclose(fp);

	p=mat(NZ,n_particles+1);

	for(ii=0;ii<=filenum;ii++){
		num=ii*SKIPNUM;

		sprintf(filename,"outputdata/Streak_Ey_e_cu/Ey_e_cu_%d.dat",num),fp=fopen(filename,"r");
		if(ii%100==0)printf(filename),printf("\n");
		for(j=0;j<NZ;j++){
			fscanf(fp,"%le\t%le\t%le\t%le\n",&z[j],&Ey[j],&ne[j],&cu[j]);
			Ey_mem[ii][j]=Ey[j];
			cu_mem[ii][j]=cu[j];
		}
		fclose(fp);


		sprintf(filename,"outputdata/Streak_particle_1D/particle_%d.dat",num),fp=fopen(filename,"r");

		for(j=0;j<NZ;j++){
			fscanf(fp,"%le\t",&z[j]);
			for(loop=2;loop<n_particles;loop++){
				chname = particle_number(particle[loop],particle,n_particles);

				fscanf(fp,"%le\t",&p[j][chname]);

				PARTICLE[ii][j][chname]=p[j][chname];
			}
			fscanf(fp,"\n");
		}
		fclose(fp);

	}

	for(loop=2;loop<n_particles;loop++){
//if(loop==particle_number("N2",particle,n_particles)){
		chname = particle_number(particle[loop],particle,n_particles);
		sprintf(filename,"outputdata/csv/streak1D/%s.csv",particle[loop]);
		fp=fopen(filename,"w");

		fprintf(fp,"ﾃﾞｰﾀ形式,1\n");
		fprintf(fp,"Ey_memo1\n");
		fprintf(fp,"X,Y,Z\n"); 

		fprintf(fp,",");

		time = 0.0e-9;
		for(i = 0; i < filenum; i++){
			fprintf(fp,"%.3f,",time*1e9); //nsec単位
			time += SKIPNUM*dt;
		}
		fprintf(fp,"\n");

		for(j = 0; j < 1430; j+=5){
			fprintf(fp,"%f,",z[j]); // mm単位
			for(i = 0; i < filenum; i++){
				fprintf(fp,"%e,",(PARTICLE[i][j][chname]*1e-6)); //＊＊逆配列注意
			}
			fprintf(fp,"\n");
		}
		fclose(fp);
	}

		sprintf(filename,"outputdata/csv/streak1D/absE.csv");
		fp=fopen(filename,"w");

		fprintf(fp,"ﾃﾞｰﾀ形式,1\n");
		fprintf(fp,"Ey_memo1\n");
		fprintf(fp,"X,Y,Z\n"); 

		fprintf(fp,",");

		time = 0.0e-9;
		for(i = 0; i < filenum; i++){
			fprintf(fp,"%.3f,",time*1e9); //nsec単位
			time += SKIPNUM*dt;
		}
		fprintf(fp,"\n");

		for(j = 0; j < 1430; j+=5){
			fprintf(fp,"%f,",z[j]); // mm単位
			for(i = 0; i < filenum; i++){
				fprintf(fp,"%e,",(Ey_mem[i][j])); //＊＊逆配列注意
			}
			fprintf(fp,"\n");
		}

		fclose(fp);

		sprintf(filename,"outputdata/csv/streak1D/Current.csv");
		fp=fopen(filename,"w");

		fprintf(fp,"ﾃﾞｰﾀ形式,1\n");
		fprintf(fp,"Ey_memo1\n");
		fprintf(fp,"X,Y,Z\n"); 

		fprintf(fp,",");

		time = 0.0e-9;
		for(i = 0; i < filenum; i++){
			fprintf(fp,"%.3f,",time*1e9); //nsec単位
			time += SKIPNUM*dt;
		}
		fprintf(fp,"\n");

		for(j = 0; j < 1430; j+=5){
			fprintf(fp,"%f,",z[j]); // mm単位
			for(i = 0; i < filenum; i++){
				fprintf(fp,"%e,",cu_mem[i][j]); //＊＊逆配列注意
			}
			fprintf(fp,"\n");
		}
		fclose(fp);
/////////////////////////////////////////////////////////////////

}

