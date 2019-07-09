#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>

//It generates N points with D - dimensions randomly and makes the choice of centroid
int read_CSV_data(char* filename, char**row_name, float* Punts, int N, int D, int C, float* Centroides)
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
	int n_total_idx = 0;			// unique index of reading float data
	char line[1024];					// one line to be read from csv file

	// read first line (caseid,....)
	fgets(line, sizeof(line), fp);

	// loop reading line by line from input file
	while (fgets(line, sizeof(line), fp)) {
		// get first string in line with separating comma
		char* token = strtok(line, ",");
		j = 0;
		int n_each_in_line = 0;		// number of float values in each line

		// get all float value from line
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

			Punts[n_total_idx++] = atof(token);	// convert string to float value and puts to Punts array
			n_each_in_line += 1;				// increase number of read float value
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
			printf("%3.1f\t", Centroides[i*D + j]);
		printf("\n\t");
	}

	return 0;
}


// Assign each point to the nearest centroid
int PointsToCentroides(float *Punts, int N, int D, int C, float *Centroides, float *New_Centr, int *Labels, int *Sep)
{
	int i, j, k, min_arg1, min_arg2, r=0;
	float dist1, dist2, dist3, dist4, min_dist1, min_dist2;

        min_dist1= min_dist2= FLT_MAX;

	for(i=0; i<C; i++)
        {
	   Sep[i]=0;		// Number of assigned points in each class
           for (k=0; k<D; k++) 
	     New_Centr[i*D+k] = 0;
        }

	for(i=0; i<N; i+=2)
        {
           // FIXME: only works if N is even
	        dist1= dist2= 0; j=0;	
		if (C&1 == 1)
                {   // even number
		  for(k=0; k<D; k++)
                  {
                      float punt1= Punts[i*D+k];
                      float punt2= Punts[(i+1)*D+k];
                      float centr= Centroides[j*D+k];
		      float temporal1 = punt1 - centr;
		      float temporal2 = punt2 - centr;
		      dist1 += temporal1*temporal1;
		      dist2 += temporal1*temporal1;
		  }
                  min_arg1 = (dist1<min_dist1)? j    : min_arg1;
                  min_dist1= (dist1<min_dist1)? dist1: min_dist1;

                  min_arg2 = (dist2<min_dist2)? j    : min_arg2;
                  min_dist2= (dist2<min_dist2)? dist2: min_dist2;

                  j= 1;
                  dist1=dist2=0;
		}

	        dist3= dist4= 0;	
                
		for(; j<C; j+=2)
                {   
		   for(k=0; k<D; k++)
                   {
                      float punt1 = Punts[i*D+k];
                      float punt2 = Punts[(i+1)*D+k];
                      float centr1= Centroides[j*D+k];
                      float centr2= Centroides[(j+1)*D+k];

		     //Let's calculate the distance from point i to centered j
		      float temporal1 = punt1 - centr1;
		      float temporal2 = punt1 - centr2;
		      float temporal3 = punt2 - centr1;
		      float temporal4 = punt2 - centr2;

		      dist1 += temporal1*temporal1;
		      dist2 += temporal2*temporal2;
		      dist3 += temporal3*temporal3;
		      dist4 += temporal4*temporal4;
		    }

                  min_arg1 = (dist1<min_dist1)? j    : min_arg1;
                  min_dist1= (dist1<min_dist1)? dist1: min_dist1;

                  min_arg2 = (dist3<min_dist2)? j    : min_arg2;
                  min_dist2= (dist3<min_dist2)? dist3: min_dist2;

                  min_arg1 = (dist2<min_dist1)? j+1   : min_arg1;
                  min_dist1= (dist2<min_dist1)? dist1: min_dist1;

                  min_arg2 = (dist4<min_dist2)? j+1   : min_arg2;
                  min_dist2= (dist4<min_dist2)? dist4: min_dist2;
		}
                
		// centroid (min_arg1)  <--  point (i)
		Labels[i] = min_arg1;
		Sep[min_arg1]+=1;
		for (k=0; k<D; k++) 
		   New_Centr[min_arg1*D+k] += Punts[i*D+k];

		// centroid (min_arg2)  <--  point (i+1)
		Labels[i+1] = min_arg2;
		Sep[min_arg2]+=1;
		for (k=0; k<D; k++) 
		   New_Centr[min_arg2*D+k] += Punts[(i+1)*D+k];
	}
        // all points have been processed 

	for (j=0; j<C; j++)
        {  // for every cluster
  
	   for (k=0; k<D; k++)
           {
	     float value = New_Centr[j*D+k] / (float)Sep[j];

	     // if centre is updated, set flag <r> to 1.
	     if (value != Centroides[(j*D)+k])
               r = 1;
             
	     // update with new centroid
             Centroides[j*D+k] = value;
           }
         }
   return r;
}



int main(int argc, char **argv){

	int N;			//Number of points
	int D;			//Dimensions of the point
	int C;			//Number of centroides

	// string array to be read file name
	char* file = (char*)malloc(1024);
	// if all parameters don't given, set as default parameter
	if (argc < 5) {
		// Default parameters
		strcpy(file, "InputData_Large.csv");
		C = 4;
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
	float *Punts      = (float*) malloc( sizeof(float) * N * D );	// Points matrix	
	int   *Labels     = (int*)   malloc( sizeof(int)   * N );	// Centroid assigned to Point	
	float *Centroides = (float*) malloc( sizeof(float) * C * D );	// Centroides
	float *New_Centr  = (float*) malloc( sizeof(float) * C * D );	// New centroides
	int   *Sep        = (int*)   malloc( sizeof(int)   * C );       // Number of points for each centroid
	int cont = 0;	// iteration count
	int final;	// flag of convergence
	int k;

	// allocate string array for caseid (row name) 
	char** row_name = (char**)malloc(sizeof(char*) * N);
	for (k = 0; k < N; k++)
		row_name[k] = (char*)malloc(sizeof(char) * 10);

	printf("K-means:\n\t Points: %d\n\t Dimension: %d\n\t Centroides: %d\n",N,D,C);

	// read input data from csv file
	if(read_CSV_data(file, row_name, Punts, N, D, C, Centroides) > 0)
		exit(1);

	do {
	  // reassignment each points to nearest centroids
	  final = PointsToCentroides(Punts, N, D, C, Centroides, New_Centr, Labels, Sep);
	  cont += 1;
	} 
        while (final && cont<200);

	printf("\nNumber of iterations needed (T): %d\n", cont);
	printf("\nNumber of changes (F): %d\n", final);
	
        exit(0);

	printf("\nNew centroides:\n\t");
	int i = 0, j = 0;
	for (i = 0; i<C; i++) {
		for (j = 0; j<D; j++)
			printf("%3.1f\t", Centroides[i*D + j]);
		printf("\n\t");
	}
	printf("\n");

	// get yellow line
	char line[1024];		// one line to be read from csv file
	FILE* fp = fopen(file, "r");	// read first line (caseid,....)
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

		for (j = 0; j < N; j++) {
                  if (Labels[i] == i)
		  {
		    fprintf(result_file, "\t%s,", row_name[i]);
	            for (k = 0; k < D; k++) 
                    {
			fprintf(result_file, "%3.1f,", Punts[i*D+k]);
		    }
		    fprintf(result_file, "\n");
		  }
                }
	}
	// close file
	fclose(result_file);

	// free allocated memory
	free(file);
	free(Punts);
	free(Centroides);
	free(New_Centr);
	free(Labels);
	free(Sep);
	free(row_name);

	exit(0);
}

