// This program (kmeans-openacc.c) is an OpenACC implementation of K means algorithm //
// The input and output data are same to previous program kmeans_baseline_input.c //

#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <math.h>
#include <assert.h>


#define FLT_MAX 3.40282347e+38

int		setup(int argc, char** argv);

float **kmeans_clustering(float* , int, int, int, int, int*);

void usage(char *argv0);

int cluster(int      npoints,				/* number of data points */
			int      nfeatures,				/* number of attributes for each point */
			float   *features,				/* in: [npoints][nfeatures] */
			int      nclusters,				/* the number of clusters */
			float ***cluster_centres,		/* out: [best_nclusters][nfeatures] */
			int		 nloops,				/* number of iteration for each number of clusters */
			int*	 membership
		);

void
kmeansPoint(float  *features,			/* in: [npoints*nfeatures] */
			int     nfeatures,
			int     npoints,
			int     nclusters,
			int    *membership,
			float  *clusters
		);

int	// delta -- had problems when return value was of float type
kmeans(float   *feature,				/* in: [npoints][nfeatures] */
		int      nfeatures,				/* number of attributes for each point */
		int      npoints,				/* number of data points */
		int      nclusters,				/* number of clusters */
		int     *membership,			/* which cluster the point belongs to */
		int     *membership_new,        /* newly assignment membership */
		float  **clusters,				/* coordinates of cluster centers */
		int     *new_centers_len,		/* number of elements in each cluster */
		float  **new_centers			/* sum of elements in each cluster */
	);

float** kmeans_clustering(float *restrict feature,    /* in: [npoints][nfeatures] */
						int     nfeatures,
						int     npoints,
						int     nclusters,
						int		nLoopCnt,
						int *restrict membership
)
{
	int      i, j, n = 0;		/* counters */
	int		 loop = 0, temp;
	int     *new_centers_len;	/* [nclusters]: no. of points in each cluster */
	int    delta;				/* if the point moved */
	float  **clusters;			/* out: [nclusters][nfeatures] */
	float  **new_centers;		/* [nclusters][nfeatures] */

	int     *membership_new;	/* newly assignment membership */

	int     *initial;			/* used to hold the index of points not yet selected
								prevents the "birthday problem" of dual selection 
								(which essentially states that it only takes 23 people in a room 
								to have a 50% chance that 2 of those people have the same birthday)
								so considered holding initial cluster indices, but changed due to
								possible, though unlikely, infinite loops */
	int      initial_points;
	int		 c = 0;

	/* nclusters should never be > npoints
	that would guarantee a cluster without points */
	if (nclusters > npoints)
		nclusters = npoints;

	/* allocate memory for membership and membership_new */
	membership_new = (int*)malloc(npoints * sizeof(int));

	/* allocate space for and initialize returning variable clusters[] */
	clusters = (float**)malloc(nclusters * sizeof(float*));
	clusters[0] = (float*)malloc(nclusters * nfeatures * sizeof(float));
	for (i = 1; i<nclusters; i++)
		clusters[i] = clusters[i - 1] + nfeatures;

	

	for (i = 0; i < nclusters; i++)
	{
		n = /*rand()*/(2 * i) % npoints;

		for (j = 0; j < nfeatures; j++)
			clusters[i][j] = feature[n * nfeatures + j];
	}

	#pragma acc data create(membership[0:npoints], membership_new[0:npoints])
	{
		/* initialize the membership and membership_new to -1 for all */
		#pragma acc kernels present(membership)
		for (i = 0; i < npoints; i++)
			membership[i] = membership_new[i] = -1;

		/* allocate space for and initialize new_centers_len and new_centers */
		new_centers_len = (int*)calloc(nclusters, sizeof(int));

		new_centers = (float**)malloc(nclusters * sizeof(float*));
		new_centers[0] = (float*)calloc(nclusters * nfeatures, sizeof(float));
		for (i = 1; i<nclusters; i++)
			new_centers[i] = new_centers[i - 1] + nfeatures;

		/* iterate until convergence */
		do 
		{
			printf("\nStep[%d]", loop + 1);
			delta = 0;
			delta = kmeans(feature,			/* in: [npoints][nfeatures] */
							nfeatures,		/* number of attributes for each point */
							npoints,		/* number of data points */
							nclusters,		/* number of clusters */
							membership,		/* which cluster the point belongs to */
							membership_new,  /* newly assignment membership */
							clusters,		/* out: [nclusters][nfeatures] */
							new_centers_len,/* out: number of points in each cluster */
							new_centers		/* sum of points in each cluster */
						);

			/* replace old cluster centers with new_centers */
			/* CPU side of reduction */
			for (i = 0; i<nclusters; i++) 
			{
				for (j = 0; j<nfeatures; j++) 
				{
					if (new_centers_len[i] > 0)
						clusters[i][j] = new_centers[i][j] / new_centers_len[i];	/* take average i.e. sum/n */
					new_centers[i][j] = 0.0;	/* set back to 0 */
				}
				new_centers_len[i] = 0;			/* set back to 0 */
			}
			c++;
		} while (delta > 0 && ++loop < nLoopCnt);	/* makes sure loop terminates */
	} /* end acc data */

	free(new_centers[0]);
	free(new_centers);
	free(new_centers_len);
	free(membership_new);

	return clusters;
}

void usage(char *argv0) 
{
	char *help =
		"\nUsage: %s  filename clusters loops\n\n"
		"    filename: file containing data to be clustered\n"
		"    clusters: maximum number of clusters allowed    [default=5]\n"
		"    nloops: iteration for each number of clusters [default=1000]\n";
	fprintf(stderr, help, argv0);
	exit(-1);
}

int setup(int argc, char **argv) 
{
	int		opt;
	char   *filename = "InputData_Large.csv";
	char	line[1024];

	int		nclusters = 500;		/* default value */
	int		best_nclusters = 0;
	int		nfeatures = 0;
	int		npoints = 0;
	float	len;

	float  *features;
	float **cluster_centres = NULL;
	int		i, j, index;
	int		nloops = 3;				/* default value */

	float	rmse;

	if (argc == 4)
	{
		filename = argv[1];
		nclusters = atoi(argv[2]);
		nloops = atoi(argv[3]);
	}
	
	FILE *infile;
	if ((infile = fopen(filename, "r")) == NULL) 
	{
		fprintf(stderr, "Error: no such file (%s)\n", filename);
		exit(1);
	}
	while (fgets(line, 1024, infile) != NULL)
		if (strtok(line, " \t\n") != 0)
			npoints++;

	rewind(infile);
	while (fgets(line, 1024, infile) != NULL) 
	{
		char *token = strtok(line, " ,\t");
		while (token != NULL)
		{
			nfeatures++;
			token = strtok(NULL, " ,\t");
		}
		break;
	}

	npoints--;
	nfeatures--;

	char **rownames = (char**)malloc(sizeof(char*)*npoints);
	int *membership = (int*)malloc(sizeof(int)*npoints);
	
	/* allocate space for features[] and read attributes of all objects */
	features = (float*)malloc(npoints*nfeatures * sizeof(float));
	rewind(infile);
	i = 0;
	int pointId = -1;
	while (fgets(line, 1024, infile) != NULL) 
	{
		if (pointId == -1)
		{
			pointId = 0;
			continue;
		}
		char *token = strtok(line, " ,\t");
		if (token == NULL) continue;

		rownames[pointId] = strdup(token);
		pointId++;

		for (j = 0; j<nfeatures; j++) 
		{
			features[i] = atof(strtok(NULL, " ,\t"));
			i++;
		}
	}
	fclose(infile);

	printf("\nI/O completed\n");
	printf("\nNumber of objects: %d\n", npoints);
	printf("Number of features: %d\n", nfeatures);
	
	  

	// error check for clusters
	if (npoints < nclusters)
	{
		printf("Error: min_nclusters(%d) > npoints(%d) -- cannot proceed\n", nclusters, npoints);
		exit(0);
	}

	/* ======================= core of the clustering ===================*/

	cluster_centres = NULL;

	#pragma acc data copyin(features[0:npoints*nfeatures]) \
			copyout(cluster_centres[0:best_nclusters])
	index = cluster(npoints,				/* number of data points */
					nfeatures,				/* number of features for each point */
					features,				/* array: [npoints][nfeatures] */
					nclusters,				/* range of min to max number of clusters */
					&cluster_centres,		/* return: [best_nclusters][nfeatures] */
					nloops,					/* number of iteration for each number of clusters */
					membership);				


	/* =============== Command Line Output =============== */

	/* cluster center coordinates
	:displayed only for when k=1*/
	printf("\n================= Centroid Coordinates =================\n");
	for (i = 0; i < nclusters; i++)
	{
		printf("%d:", i);
		for (j = 0; j < nfeatures; j++) 
		{
			printf(" %.2f", cluster_centres[i][j]);
		}
		printf("\n\n");
	}

	    exit(0); 
	
	
	// get yellow line
	FILE* fp = fopen(filename, "r");	// read first line (caseid,.....)
	fgets(line, sizeof(line), fp);
	fclose(fp);

	FILE* result_file = fopen("result.csv", "w");
	if (!result_file) {
		printf("Can't create result file.\n");
		return 1;
	}
	fprintf(result_file, "Number of clusters: %d\n", nclusters);
	printf("Number of clusters: %d\n", nclusters);

	int *cluster_elem_cnt = (int*)malloc(sizeof(int)*nclusters);
	memset(cluster_elem_cnt, 0, sizeof(int)*nclusters);

	for (i = 0; i < npoints; i++)
	{
		if (0 <= membership[i] && membership[i] < nclusters)
			cluster_elem_cnt[membership[i]]++;
	}

	for (int k = 0; k < nclusters; k++) 
	{
		fprintf(result_file, "\tCluster-%d : %d elements\n", k + 1, cluster_elem_cnt[k]);
		printf("\tCluster-%d : %d elements\n", k + 1, cluster_elem_cnt[k]);
	}

	for (i = 0; i < nclusters; i++) 
	{
		fprintf(result_file, "\nCluster - %d\n", i + 1);
		fprintf(result_file, "\t%s\n", line);

		for (j = 0; j < npoints; j++)
		{
			if (i == membership[j])
			{
				fprintf(result_file, "\t%s,", rownames[j]);
				for (int k = 0; k < nfeatures; k++) 
				{
					fprintf(result_file, "%3.1f,", features[j*nfeatures + k]);
				}
				fprintf(result_file, "\n");
			}
		}
	}
	// close file
	fclose(result_file);
	

	/* free up memory */
	free(features);

	for (i = 0; i < npoints; i++)
	{
		free(rownames[i]);
	}
	free(rownames);
	free(membership);
	return(0);
}
										
int cluster(int      npoints,				/* number of data points */
			int      nfeatures,				/* number of attributes for each point */
			float   *features,				/* in: [npoints][nfeatures] */
			int      nclusters,				/* the number of clusters */
			float ***cluster_centres,		/* out: [best_nclusters][nfeatures] */
			int		 nLoopCnt,				/* number of iteration for each number of clusters */
			int*	membership
)
{
	int		index = 0;						/* number of iteration to reach the best RMSE */
	float **tmp_cluster_centres;			/* hold coordinates of cluster centers */
	int		i;

	/* sweep k from min to max_nclusters to find the best number of clusters */
	
	if (nclusters > npoints) return -1;	/* cannot have more clusters than points */

	/* initialize initial cluster centers */ 
	tmp_cluster_centres = kmeans_clustering(features,
											nfeatures,
											npoints,
											nclusters,
											nLoopCnt,
											membership);

	if (*cluster_centres) 
	{
		free((*cluster_centres)[0]);
		free(*cluster_centres);
	}
	*cluster_centres = tmp_cluster_centres;
	
	return index;
}

/* find the index of nearest cluster centers and change membership*/
void
kmeansPoint(float  *restrict features,			/* in: [npoints*nfeatures] */
			int     nfeatures,
			int     npoints,
			int     nclusters,
			int    *restrict membership_new,
			float  *restrict clusters)
{
	int point_id;

	#pragma acc kernels \
			present(features) \
			copyin(clusters[0:nclusters*nfeatures])
	for (point_id = 0; point_id < npoints; point_id++)
	{
		int i, j;
		int index = -1;												/* index of closest cluster center id */
		float min_dist = FLT_MAX;
		float dist;													/* distance square between a point to cluster center */

																	/* find the cluster center id with min distance to pt */
		for (i = 0; i < nclusters; i++)
		{
			int cluster_base_index = i*nfeatures;					/* base index of cluster centers for inverted array */
			float ans = 0.0;											/* Euclidean distance sqaure */

			#pragma acc loop reduction(+:ans)
			for (j = 0; j < nfeatures; j++)
			{
				//int addr = point_id + j*npoints;					/* appropriate index of data point */
				float diff = features[point_id*nfeatures+j] - //(tex1Dfetch(t_features, addr) -
					clusters[cluster_base_index + j];	/* distance between a data point to cluster centers */
				ans += diff*diff;									/* sum of squares */
			}
			dist = sqrt(ans);

			/* see if distance is smaller than previous ones:
			if so, change minimum distance and save index of cluster center */
			if (dist < min_dist)
			{
				min_dist = dist;
				index = i;
			}
		}

		/* assign the new membership to object point_id */
		membership_new[point_id] = index;
	}

	
}


int	// delta -- had problems when return value was of float type
kmeans(float   *features,				/* in: [npoints][nfeatures] */
		int      nfeatures,				/* number of attributes for each point */
		int      npoints,				/* number of data points */
		int      nclusters,				/* number of clusters */
		int     *membership,			/* which cluster the point belongs to */
		int     *membership_new,        /* newly assignment membership */
		float  **clusters,				/* coordinates of cluster centers */
		int     *new_centers_len,		/* number of elements in each cluster */
		float  **new_centers			/* sum of elements in each cluster */
)
{
	int delta = 0;			/* if point has moved */
	int i, j;				/* counters */

	/* execute the kernel */
	kmeansPoint(features,
				nfeatures,
				npoints,
				nclusters,
				membership_new,
				clusters[0]);    /////////// clusters);

	/* copy back membership (device to host) */
	#pragma acc update host(membership_new[0:npoints])

	/* for each point, sum data points in each cluster
	and see if membership has changed:
	if so, increase delta and change old membership, and update new_centers;
	otherwise, update new_centers */
	delta = 0;
	for (i = 0; i < npoints; i++)
	{
		int cluster_id = membership_new[i];
		new_centers_len[cluster_id]++;
		if (membership_new[i] != membership[i])
		{
			delta++;
			membership[i] = membership_new[i];
		}
		for (j = 0; j < nfeatures; j++)
		{
			new_centers[cluster_id][j] += features[i*nfeatures+j];   
		}
	}

	return delta;
}


int main(int argc, char** argv)
{
	setup(argc, argv);
}






