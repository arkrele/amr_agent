#include<stdio.h>
#include<math.h>
#include <string.h>
#include <stdlib.h>
#include"hfile/memory.h"
#include"hfile/spline.h"
#include "hfile/Reaction.h"

#define NR  256
#define NZ  1792

#define SKIPNUM  2000
#define ijSKIP 2


#define READFILE "outputdata/Particle/%s_%d.dat"
#define OUTPUTFILE "1Ddata/SUM.dat"

#define INITIAL_FILE  "inputdata/Initial/initial_dry_300K.dat"

int n_particles = 100;		//考慮する粒子数、適当に代入
static double bgap	= 13.0e-3;	//ギャップ長
static double cr	= 500e-6;	//先端曲率半径
static double dt1	= 1.25e-13;	//時間刻み


main(){

	FILE *fp;
	char filename[256],buff[256];
	int i,j,ii,num,filenum,data, loop, chname,line;
	double dumm,**ne,*r,*z,**flag,**SUM,*time,t1,*y, **Vol;
	double a = sqrt(cr*bgap);
	char **particle;

	ne=mat(NR,NZ),r=vec(NR),z=vec(NZ),flag=mat(NR,NZ),Vol=mat(NR,NZ);
	time = vec(1000);

	particle = cmat(n_particles,100);
	y=vec(n_particles); 

	fp=fopen("inputdata/mesh_r2_0624.dat","r");
	for(i=0;i<NR;i++)fscanf(fp,"%d\t%le\n",&data,&r[i]);
	fclose(fp);
	fp=fopen("inputdata/mesh_z2.dat","r");
	for(j=0;j<NZ;j++)fscanf(fp,"%d\t%le\n",&data,&z[j]);
	fclose(fp);

	for(i=0;i<NR;i++){
		for(j=0;j<NZ;j++){
			//針の内部にフラグをたてる。
			if (pow(z[j]/bgap, 2) - pow(r[i]/a, 2) >= 1)flag[i][j]=1;
			else flag[i][j]=0;

			Vol[i][j] = 0.5*(r[i+1]+r[i])*(r[i+1]-r[i])*(z[j+1]-z[j]);  //θ = 1 rad 分
		}
	}


	//考慮する粒子種と初期濃度を読み込む
	fp = fopen(INITIAL_FILE, "r");
	n_particles = Initial_condition(fp,particle,y);
	fclose(fp);

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

	SUM=mat(n_particles+1,2000);

	filenum = (filenum/SKIPNUM); //　小数点以下切り捨て
///////////////////////////////////////////////////////////////////////////////////

	data=0;
	for(ii=0;ii<=filenum;ii++){
		num=ii*SKIPNUM;


		/////2次元データの取り込み//////////

//	    	sprintf(filename,"outputdata/vibN2/v2/v2_%d.dat",num);
		for(loop=2;loop<n_particles;loop++){

			SUM[loop][data] = 0.0;

			chname = particle_number(particle[loop],particle,n_particles);
			sprintf(filename,READFILE,particle[loop],num);
			
			if((fp = fopen(filename, "r")) == NULL){    /* ファイルオープン */
				//for(i=0;i<NR;i++)for(j=0;j<NZ;j++)ne[i][j]=0.0;
			}else{
				for(i=0;i<NR;i++)for(j=0;j<NZ;j++)fscanf(fp,"%le\n",&ne[i][j]);

				//////////////////////NR-1->130(0.3mm)  164(0.5mm) 179(0.7mm) 193(1mm)
				/////////////////////j/0~1420-1     6~12mm(661~1261)
				for(i=0;i<193;i++){
					for(j=0;j<1420-1;j++){
						if(flag[i][j]);
						else SUM[loop][data] += Vol[i][j]*ne[i][j];
					}
				}
				//////////////////////
				fclose(fp);
			}



		}
		printf("%d\n",num);
		//////////////////////////////
		time[data] = dt1*num;
		data++;
	}

	sprintf(filename,OUTPUTFILE);
	fp=fopen(filename,"w");
	fprintf(fp,"Time\t");
	for(loop=2;loop<n_particles;loop++)fprintf(fp,"%s\t",particle[loop]);
	fprintf(fp,"\n");

	for(ii=0;ii<data;ii++){
		fprintf(fp,"%f\t",time[ii]*1e9);
		for(loop=2;loop<n_particles;loop++)fprintf(fp,"%e\t",2*M_PI*SUM[loop][ii]); // 2piをかけることでシータ方向へ積分している
		fprintf(fp,"\n");
	}
	
	fclose(fp);

/////////////////////////////////////////////////////////////////

}

