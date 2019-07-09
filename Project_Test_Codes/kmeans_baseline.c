#include <stdio.h>
#include <stdlib.h>
#include <limits.h>


// update the center for all the centroids.
// return:
// 0 if the centroids do not change, 
// 1 otherwise
int recalcularCentre(int *Punts, int N, int D, int C, double *Centroides, int *PC, int *Sep)
{
	int i, j, k, x;
	double* v = (double*)malloc(sizeof(double)*D);		//Auxiliary vector to calculate the new position of the centroids
	int r = 0;
	
	for(i=0; i<C; i++){
		// initialize V to 0;
		for(x=0; x<D; x++) 
			v[x] = 0;

		// update the centre of i-th centroid
		for(j=0; j<Sep[i]; j++){
			for(k=0; k<D; k++) 
				v[k] += (double)Punts[PC[i*N+j]*D+k];
		}
		for(x=0; x<D; x++){
			v[x] = v[x] / (double)Sep[i];

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
void generatePunts(int *Punts, int N, int D, int C, double *Centroides, int DIMENSION)
{
	int i, j;
	// generate N points(D-th vector) randomly
	for(i=0; i<N; i++){
		for(j=0; j<D; j++){
			Punts[(D*i)+j] = rand() % DIMENSION;
		}
	}

	printf("\nGenerated %d x %d random data.\n", N, D);

	// assign C points as center of each centroids
	for(i=0; i<C; i++){
		int k = rand() % N;
		for(j=0; j<D; j++){
			Centroides[(i*D)+j] = (double)Punts[(k*D)+j];	
		}
	}
	
	printf("\nCentroides:\n\t");
	for(i=0; i<C; i++){
		for(j=0; j<D; j++)
			printf("%3.1f\t",Centroides[i*D+j]);
		printf("\n\t");
	}
}

//Returns the position(centroid number) where the minimum value of a vector is found
int mmin(double* v, int C)
{
	double m = v[0];
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
void PointsToCentroides(int *Punts, int N, int D, int C, double *Centroides, int *PC, int *Sep){
	int i, j, k, m;
	double* dist = (double*)malloc(sizeof(double)*C);
	for(i=0; i<C; i++)
		Sep[i]=0;		// Number of assigned points in each class
	for(i=0; i<N; i++){
		
		for(j=0; j<C; j++){
			dist[j] = 0;
			for(k=0; k<D; k++){
				//Let's calculate the distance from point i to centered j
				dist[j] += ((double)Punts[i*D+k] - Centroides[j*D+k]) * ((double)Punts[i*D+k] - Centroides[j*D+k]);
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



void main(int argc, char **argv){

	int N;			//Number of points
	int D;			//Dimensions of the point
	int C;			//Number of centroides
	int DIMENSION;	//Dimension of the point space
	srand(7);

	if(argc < 5){
		// Default parameters
		N = 1000;
		D = 3;
		C = 4;
		DIMENSION = 1000;
	}
	else{
		// Parameters passed by argument
		N = atoi(argv[1]); 
		D = atoi(argv[2]); 
		C = atoi(argv[3]);
		DIMENSION = atoi(argv[4]);
	}

	int *Punts = (int*) malloc( sizeof(int)*N*D );					//Points matrix
	double *Centroides = (double*) malloc( sizeof(double)*C*D );	//Centroides
	int *PC = (int*) malloc( sizeof(int)*N*C );						//Index array of points which assigned to each centroid
	int *Sep = (int*) malloc( sizeof(int)*C );						//Number of points for each centroid
	int cont = 0;													// iteration count
	int final;														// flag of convergence

	printf("K-means:\n\t Points: %d\n\t Dimension: %d\n\t Centroides: %d\n",N,D,C);
	printf("Points space is from %d x %d\n", DIMENSION, D);
	
	// initial generation of N x D points
	generatePunts(Punts, N, D, C, Centroides, DIMENSION);

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
			printf("%3.1f\t", Centroides[i*D + j]);
		printf("\n\t");
	}
	printf("\n");
}

