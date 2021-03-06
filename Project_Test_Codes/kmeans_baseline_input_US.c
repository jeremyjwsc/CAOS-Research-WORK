#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>

// update the center for all the centroids.
// return:
// 0 if the centroids do not change, 
// 1 otherwise
int recalcularCentre(int* Punts, int N, int D, int C, int* Centroides, int* PC, int *Sep)
{
	int i, j, k, x;
	int* v = (int*)malloc(sizeof(int) * D);		//Auxiliary vector to calculate the new position of the centroids
	int r = 0;
	
	for(i=0; i<C; i++){
		// initialize V to 0;
		for(x=0; x<D; x++) 
			v[x] = 0;

		// update the centre of i-th centroid
		for(j=0; j<Sep[i]; j++){
			for(k=0; k<D; k++) 
				v[k] += (int)Punts[PC[i*N+j]*D+k];
		}
		for(x=0; x<D; x++){
			v[x] = v[x] / (int)Sep[i];

			// if centre is updated, set flag <r> to 1.
			if(v[x] != Centroides[(i*D)+x])
				r = 1;
			// update with new centroid
			Centroides[i*D+x] = v[x];
		}
	}
	free(v);
	return r;
}

//It generates N points with D - dimensions randomly and makes the choice of centroid
int read_CSV_data(char* filename, char**row_name, int* Punts, int N, int D, int C, int* Centroides)
{
	// open input csv data file
	FILE* fp = fopen(filename, "r");
	// if failed to open file, exit print error message and program
	if (!fp) {
		printf("Can't find input file - %s\n", filename);
		return 1;
	}

	printf("reading input data from %s\n", filename);

	int i=0, j=0, n_read = 0;			// n_read: number of acually read lines
	int n_total_idx = 0;			// unique index of reading int data
	char line[1024];					// one line to be read from csv file

	// read first line (caseid,....)
	fgets(line, sizeof(line), fp);

	// loop reading line by line from input file
	while (fgets(line, sizeof(line), fp)) {
		// get first string in line with separating comma
		char* token = strtok(line, ",");
		j = 0;
		int n_each_in_line = 0;		// number of int values in each line

		// get all int value from line
		while (token != NULL) {
			// if arrived to end of line, break 
			if (strcmp(token, "\n") == 0)		// compare two strings - token and "\n" and if equals
				break;
			if (j == 0) {		// first string at line (means row name)
				strcpy(row_name[i], token);
				j += 1;
				token = strtok(NULL, ",");
				continue;
			}

			Punts[n_total_idx++] = atoi(token);	// convert string to int value and puts to Punts array
			n_each_in_line += 1;				// increase number of read int value
			j += 1;
			// if each line includes more values than <D>, ignore remain values
			if(n_each_in_line >= D)
				break;
			// get next string from line
			token = strtok(NULL, ",");
		}

		// if number of values at each line doesnot equal to <D>, print error message and exit
		if (n_each_in_line != D) {
			printf("Now each line components is %d\n", n_each_in_line);
			printf("\nEach line should include %d values.\n", D);
			return 2;
		}

		// increase number of read line
		i += 1;
		n_read += 1;
		// if line number is greater than <N>, ignore remain lines
		if (n_read > N)
			break;
	}

	// close opened file
	fclose(fp);

	// confirm read-lines is equal to <N>
	assert(n_read == N);

	// if n_read is less than <N>, set <N> as n_read.
	N = n_read;
	printf("\nRead %d x %d points data from %s.\n", N, D, filename);

	// assign C points as center of each centroids
	for (i = 0; i<C; i++) {
		int k = rand() % N;
		for (j = 0; j<D; j++) {
			Centroides[(i*D) + j] = Punts[(k*D) + j];
		}
	}

	printf("\nCentroides:\n\t");
	for (i = 0; i<C; i++) {
		for (j = 0; j<D; j++)
			printf("%3.0d\t", Centroides[i*D + j]);
		printf("\n\t");
	}

	return 0;
}

//Returns the position(centroid number) where the minimum value of a vector is found
int mmin(int* v, int C)
{
	int m = v[0];
	int i, r = 0;
	for (i = 1; i<C; i++) {
		if (m > v[i]) {
			r = i;
			m = v[i];
		}
	}
	return r;
}

// Assign each point to the nearest centroid
void PointsToCentroides(int *Punts, int N, int D, int C, int *Centroides, int *PC, int *Sep){
	int i, j, k, m;
	int* dist = (int*)malloc(sizeof(int)*C);
	for(i=0; i<C; i++)
		Sep[i]=0;		// Number of assigned points in each class
	for(i=0; i<N; i++){
		
		for(j=0; j<C; j++){
			dist[j] = 0;
			for(k=0; k<D; k++){
				//Let's calculate the distance from point i to centered j
				dist[j] += ((int)Punts[i*D+k] - Centroides[j*D+k]) * ((int)Punts[i*D+k] - Centroides[j*D+k]);
			}
		}
		//We take the minimum distance between a point and all centroids and assign it to the nearest centroide
		m = mmin(dist, C);

		// centroid (m)  <--  point (i)
		PC[m*N+Sep[m]] = i;
		Sep[m]+=1;
	}
	free(dist);
}



int main(int argc, char **argv){

	int N;			//Number of points
	int D;			//Dimensions of the point
	int C;			//Number of centroides

	// string array to be read file name
	char* file = (char*)malloc(256);
	// if all parameters don't given, set as default parameter
	if (argc < 5) {
		// Default parameters
		strcpy(file, "InputData_Large.csv");
		C = 3;
		N = 1000000;
		D = 68;
		
		printf("Usage: %s<InputData_Large.csv>, <K=Number of Clusters>, <N=Number of Rows>, <D=Number of Columns>\n", argv[0]);
		printf("We now use default parameters with InputData_Large.csv, K=%d, N=%d, D=%d\n",C,N,D);
	}
	else {
		// copy file name from argument to variable
		strcpy(file, argv[1]);
		C = atoi(argv[2]);
		N = atoi(argv[3]);
		D = atoi(argv[4]);
	}
	
	// allocate memory for points matrix, centres and so on
	int *Punts = (int*)malloc(sizeof(int) * N * D);			//Points matrix	
	int *Centroides = (int*) malloc( sizeof(int) * C * D );	//Centroides
	int *PC = (int*) malloc( sizeof(int)*N*C );						//Index array of points which assigned to each centroid
	int *Sep = (int*) malloc( sizeof(int)*C );						//Number of points for each centroid
	int cont = 0;			// iteration count
	int final;			// flag of convergence
	int k;

	// allocate string array for caseid (row name) 
	char** row_name = (char**)malloc(sizeof(char*) * N);
	for (k = 0; k < N; k++)
		row_name[k] = (char*)malloc(sizeof(char) * 10);

	printf("K-means:\n\t Points: %d\n\t Dimension: %d\n\t Centroides: %d\n",N,D,C);

	// read input data from csv file
	if(read_CSV_data(file, row_name, Punts, N, D, C, Centroides) > 0)
		exit(1);

	do{
		// reassignment each points to nearest centroids
		PointsToCentroides(Punts, N, D, C, Centroides, PC, Sep);

		// counting iteration round
		cont += 1;

		// check if no changed
		final = recalcularCentre(Punts, N, D, C, Centroides, PC, Sep);
	}while(final || cont<10000);

	printf("\nNumber of iterations needed: %d\n", cont);
	
	printf("\nNew centroides:\n\t");
	int i = 0, j = 0;
	for (i = 0; i<C; i++) {
		for (j = 0; j<D; j++)
			printf("%3.0d\t", Centroides[i*D + j]);
		printf("\n\t");
	}
	printf("\n");

	// get yellow line
	char line[256];					// one line to be read from csv file
	FILE* fp = fopen(file, "r");	// read first line (caseid,...)
	fgets(line, sizeof(line), fp);
	fclose(fp);

	// print new clusters to result csv file
	FILE* result_file = fopen("result.csv", "w");
	if (!result_file) {
		printf("Can't create result file.\n");
		return 1;
	}
	fprintf(result_file, "Number of clusters: %d\n", C);
	for (k = 0; k < C; k++) {
		fprintf(result_file, "\tCluster-%d : %d elements\n", k + 1, Sep[k]);
	}

	for (i = 0; i < C; i++) {
		fprintf(result_file, "\nCluster - %d\n", i + 1);
		fprintf(result_file, "\t%s\n", line);

		for (j = 0; j < Sep[i]; j++) {
			fprintf(result_file, "\t%s,", row_name[PC[i*N + j]]);
			for (k = 0; k < D; k++) {
				fprintf(result_file, "%3.0d,", Punts[PC[i*N+j]*D+k]);
			}
			fprintf(result_file, "\n");
		}
	}
	// close file
	fclose(result_file);

	// free allocated memory
	free(file);
	free(Punts);
	free(Centroides);
	free(PC);
	free(Sep);
	free(row_name);

	exit(0);
}

