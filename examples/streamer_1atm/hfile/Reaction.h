
/*
 *  particle_number --
 *    粒子名の文字列 p から対応する粒子番号を返す
 */
int particle_number(char *p,char **particle,int n_particles){

	int i;

	/*  strcmp(const char *s1, const char *s2) 	*/
	/*  文字列s1とs2を比較する			*/
	/*  s1>s2で正の値、s1<s2で負の値、s1=s2で0を返す*/
	/*  大小関係は文字コード順			*/

	for (i = 0; i < n_particles; i++) {
		if (strcmp(p, particle[i]) == 0)
		return i;
	}

	fprintf(stderr, "Error: Particle \"%s\" does not exist \n",p);
	exit(0);
}

int Read_reaction(FILE *fp,int NUM_L, int NUM_R, int n_particles,int *Rnum, 
			int **reactl,int **reactr,double *A,double *B,double *E,double *ER,char **particle){

	int i,j,n;
	char line[1000];
	char p[100];


	/* fgets(buf,n,fp)						W*/
	/* ファイルfpからbufに1行読み込みを行う。読み込みは\nに出会うか	*/
	/* n-1個の文字を読み込むまで実行される。ファイルエンドまたは	*/
	/* エラーならばNULLを返す					*/

	/* isspace(int c)		*/
	/* cが空白でない時 = 0		*/
	/* cが空白の時 	   = 0以外を返す*/

	// 反応式の数を数える
	for (n = 0; fgets(line, 1000, fp) != NULL; n++) {
		// 空行や "#" で始まるコメント行を読みとばす
		if (line[0] == '#' || isspace(line[0])) {
			n--;
			continue;
		}
	}
	rewind(fp);//ファイルポインタを先頭に戻す

	// ループを回して、反応 i に関する反応式データを格納する 
	for (i = 0; i < n; i++) {
		if (fgets(line, 1000, fp) == NULL)break;
		// ファイルから１行読み込む。ファイルの最後に達したらループを抜ける
		if (line[0] == '#' || isspace(line[0])){ // 空行や "#" で始まるコメント行を読みとばす
			i--;
			continue;
		}
		// ファイルポインタを行の先頭に戻す
		fseek(fp, -strlen(line), SEEK_CUR);

		fscanf(fp,"%d",&Rnum[i]);
		// 反応式左辺の NUM_L 個の粒子種を読み込む 
		for (j = 0; j < NUM_L; j++){
			fscanf(fp, "%s", p);
			reactl[Rnum[i]][j] = particle_number(p,particle,n_particles);
		}
		// 反応式右辺の NUM_R 個の粒子種を読み込む 
		for (j = 0; j < NUM_R; j++){
			fscanf(fp, "%s", p);
			reactr[Rnum[i]][j] = particle_number(p,particle,n_particles);
		}
		// 反応係数を読み込む 
		fscanf(fp, "%lf\t%lf\t%lf\t%lf\n", &A[Rnum[i]],&B[Rnum[i]],&E[Rnum[i]],&ER[Rnum[i]]);
	}
	return n;
}


int Initial_condition(FILE *fp, char **particle,double *y){

	int i,j,n,dumn;
	char line[1000];


	// 粒子の数を数える
	for (n = 2; fgets(line, 1000, fp) != NULL; n++) {
		if (line[0] == '#' || isspace(line[0])) {  // 空行や "#" で始まるコメント行を読みとばす
			n--;
			continue;
		}
	}
	rewind(fp);//ファイルポインタを先頭に戻す

	// ループ開始。particle[] に粒子名、y[] に初期濃度を読み込む 
	strcpy(particle[0], "-"),y[0]=1.0; // 空白 
	strcpy(particle[1], "M"),y[1]=0.0; // 三体反応 M 

	for (i = 2; i < n; i++) {
		// line[] に１行ずつ読み込み、ファイルの最後に達したらループを抜ける 
		if (fgets(line, 1000, fp) == NULL)break;    

		// "#" や空白で始まる行はコメント行として読みとばす
		if (line[0] == '#' || isspace(line[0])) {
			i--;
			continue;
		}
		// particle[] に粒子名、y[] に初期濃度を読み込む 
		sscanf(line, "%d  %s  %lf", &dumn, particle[i], &y[i]); 
	}

	return n;
}
