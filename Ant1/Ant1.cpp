#include "stdafx.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <omp.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

#define MAX_CITIES	30
#define MAX_DISTANCE	100
#define MAX_TOUR	(MAX_CITIES * MAX_DISTANCE)
#define MAX_ANTS	30
#define getSRand()	((float)rand() / (float)RAND_MAX)
#define getRand(x)	(int)((x) * getSRand())
#define ALPHA		0.5
#define BETA		5.0
#define RHO			0.5	
#define QVAL		100
#define MAX_TOURS	500
#define MAX_TIME	(MAX_TOURS * MAX_CITIES)
#define INIT_PHEROMONE	(1.0 / MAX_CITIES)

typedef struct {
	int x;
	int y;
} cityType;

typedef struct {
	int curCity;
	int nextCity;
	unsigned char tabu[MAX_CITIES];
	int pathIndex;
	unsigned char path[MAX_CITIES];
	double tourLength;
} antType;

cityType cities[MAX_CITIES];
antType ants[MAX_ANTS];

double distance[MAX_CITIES][MAX_CITIES];

double pheromone[MAX_CITIES][MAX_CITIES];

double  best = (double)MAX_TOUR;
int     bestIndex;

void init()
{
	int from, to, ant;
	FILE *f;
	for (from = 0; from < MAX_CITIES; from++) {
		cities[from].x = getRand(MAX_DISTANCE);
		cities[from].y = getRand(MAX_DISTANCE);
	}

	for (from = 0; from < MAX_CITIES; from++) {
		for (to = 0; to < MAX_CITIES; to++) {
			distance[from][to] = 0.0;
			pheromone[from][to] = INIT_PHEROMONE;
		}
	}

	for (from = 0; from < MAX_CITIES; from++) {

		for (to = 0; to < MAX_CITIES; to++) {

			if ((to != from) && (distance[from][to] == 0.0)) {
				int xd = abs(cities[from].x - cities[to].x);
				int yd = abs(cities[from].y - cities[to].y);

				distance[from][to] = sqrt((xd * xd) + (yd * yd));
				distance[to][from] = distance[from][to];
			}

		}

	}

	//муравьи
	to = 0;
	for (ant = 0; ant < MAX_ANTS; ant++) {
		if (to == MAX_CITIES) to = 0;
		ants[ant].curCity = to++;

		for (from = 0; from < MAX_CITIES; from++) {
			ants[ant].tabu[from] = 0;
			ants[ant].path[from] = -1;
		}

		ants[ant].pathIndex = 1;
		ants[ant].path[0] = ants[ant].curCity;
		ants[ant].nextCity = -1;
		ants[ant].tourLength = 0.0;

		ants[ant].tabu[ants[ant].curCity] = 1;
	}
}

void restartAnts()
{
	int ant, i, to = 0;

	for (ant = 0; ant < MAX_ANTS; ant++) {

		if (ants[ant].tourLength < best) {
			best = ants[ant].tourLength;
			bestIndex = ant;
		}

		ants[ant].nextCity = -1;
		ants[ant].tourLength = 0.0;

		for (i = 0; i < MAX_CITIES; i++) {
			ants[ant].tabu[i] = 0;
			ants[ant].path[i] = -1;
		}

		if (to == MAX_CITIES) to = 0;
		ants[ant].curCity = to++;

		ants[ant].pathIndex = 1;
		ants[ant].path[0] = ants[ant].curCity;

		ants[ant].tabu[ants[ant].curCity] = 1;

	}

}

double antProduct(int from, int to)
{
	return ((pow(pheromone[from][to], ALPHA) *
		pow((1.0 / distance[from][to]), BETA)));
}

int selectNextCity(int ant)
{
	int from, to;
	double denom = 0.0;

	from = ants[ant].curCity;

	for (to = 0; to < MAX_CITIES; to++) {
		if (ants[ant].tabu[to] == 0) {
			denom += antProduct(from, to);
		}
	}

	assert(denom != 0.0);

	do {

		double p;

		to++;
		if (to >= MAX_CITIES) to = 0;

		if (ants[ant].tabu[to] == 0) {

			p = antProduct(from, to) / denom;

			if (getSRand() < p) break;

		}

	} while (1);

	return to;
}

int simulateAnts()
{
	int k;
	int moving = 0;

//#pragma omp parallel for
	for (k = 0; k < MAX_ANTS; k++) {
		if (ants[k].pathIndex < MAX_CITIES) {

			ants[k].nextCity = selectNextCity(k);

			ants[k].tabu[ants[k].nextCity] = 1;

			ants[k].path[ants[k].pathIndex++] = ants[k].nextCity;

			ants[k].tourLength += distance[ants[k].curCity][ants[k].nextCity];

			if (ants[k].pathIndex == MAX_CITIES) {
				ants[k].tourLength +=
					distance[ants[k].path[MAX_CITIES - 1]][ants[k].path[0]];
			}

			ants[k].curCity = ants[k].nextCity;

			moving++;

		}
	}

	return moving;
}

void updateTrails(void)
{
	int from, to, i, ant;
	//испарение
	for (from = 0; from < MAX_CITIES; from++) {

		for (to = 0; to < MAX_CITIES; to++) {

			if (from != to) {

				pheromone[from][to] *= (1.0 - RHO);

				if (pheromone[from][to] < 0.0) pheromone[from][to] = INIT_PHEROMONE;

			}

		}

	}
	//нанесение
	for (ant = 0; ant < MAX_ANTS; ant++) {
		for (i = 0; i < MAX_CITIES; i++) {

			if (i < MAX_CITIES - 1) {
				from = ants[ant].path[i];
				to = ants[ant].path[i + 1];
			}
			else {
				from = ants[ant].path[i];
				to = ants[ant].path[0];
			}

			pheromone[from][to] += (QVAL / ants[ant].tourLength);
			pheromone[to][from] = pheromone[from][to];

		}

	}


	for (from = 0; from < MAX_CITIES; from++) {
		for (to = 0; to < MAX_CITIES; to++) {
			pheromone[from][to] *= RHO;
		}
	}
}

void emitDataFile(int ant)
{
	int city;
	FILE *fp;

	fp = fopen("cities.txt", "w");
	for (city = 0; city < MAX_CITIES; city++) {
		fprintf(fp, "%d %d\n", cities[city].x, cities[city].y);
	}
	fclose(fp);

	fp = fopen("solution.txt", "w");
	for (city = 0; city < MAX_CITIES; city++) {
		/*fprintf(fp, "%d %d\n",
			cities[ants[ant].path[city]].x,
			cities[ants[ant].path[city]].y);*/
		fprintf(fp, "%d\n", ants[ant].path[city]);
	}
	/*fprintf(fp, "%d %d\n",
		cities[ants[ant].path[0]].x,
		cities[ants[ant].path[0]].y);*/

	fclose(fp);
}

void emitTable(void)
{
	int from, to;

	for (from = 0; from < MAX_CITIES; from++) {
		for (to = 0; to < MAX_CITIES; to++) {
			printf("%5.2g ", pheromone[from][to]);
		}
		printf("\n");
	}
	printf("\n");
}

int main()
{
	int curTime = 0;
	double start, stop;
	srand(time(NULL));
	FILE *f;
	f = fopen("temp.txt", "w");
	start = omp_get_wtime();
	init();

	while (curTime++ < MAX_TIME) {
		if (simulateAnts() == 0) {
			updateTrails();
			if (curTime != MAX_TIME)
				restartAnts();
			fprintf(f,"Time is %d (%g)\n", curTime, best);
		}

	}
	stop = omp_get_wtime();
	fclose(f);
	printf("best tour %g\n", best);
	printf("time %f\n", stop - start);
	printf("\n\n");
	emitDataFile(bestIndex);
	getchar();
	getchar();

	return 0;
}
