// Student ID: 0416014
// Name      : 王立洋
// Date      : 2017.11.24

#include "bmpReader.h"
#include "bmpReader.cpp"
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#define THREAD_NUM 16
#define PAUSE printf("PAUSE\n");while(getchar() != '\n')
using namespace std;

#define MYRED	2
#define MYGREEN 1
#define MYBLUE	0

int imgWidth, imgHeight;
int FILTER_SIZE;
int FILTER_SIZE_HALF;
int FILTER_SCALE;
int *filter_G;

sem_t sema;

pthread_mutex_t mutex_j;
pthread_mutex_t mutex_filter;

const char *inputfile_name[5] = {
	"input1.bmp",
	"input2.bmp",
	"input3.bmp",
	"input4.bmp",
	"input5.bmp"
};
const char *outputBlur_name[5] = {
	"Blur1.bmp",
	"Blur2.bmp",
	"Blur3.bmp",
	"Blur4.bmp",
	"Blur5.bmp"
};

unsigned char *pic_in, *pic_grey, *pic_final;

unsigned char RGB2grey(int w, int h)
{
	int tmp = (
		pic_in[3 * (h*imgWidth + w) + MYRED] +
		pic_in[3 * (h*imgWidth + w) + MYGREEN] +
		pic_in[3 * (h*imgWidth + w) + MYBLUE] )/3;

	if (tmp < 0) tmp = 0;
	if (tmp > 255) tmp = 255;
	return (unsigned char)tmp;
}


unsigned char GaussianFilter(int w, int h)
{
	int tmp = 0;
	int a, b;
	int ws = (int)sqrt((float)FILTER_SIZE);

	int wsHalf  = ws/2;
	int w_minus_wsHalf = w - ws/2;
	int h_minus_wsHalf = h - ws/2;

	for (int j = 0; j<ws; j++){
		int jws = j*ws;
		for (int i = 0; i<ws; i++){

			a = i + w_minus_wsHalf;
			b = j + h_minus_wsHalf;

			// detect for borders of the image
			if (a<0 || b<0 || a>=imgWidth || b>=imgHeight) continue;

			tmp += filter_G[jws + i] * pic_grey[b*imgWidth + a];
		}
	}
	tmp /= FILTER_SCALE;
	if (tmp < 0) tmp = 0;
	if (tmp > 255) tmp = 255;
	return (unsigned char)tmp;
}

void *RGB2greyMult(void *argu){
	int pval = *(int*)argu;
	//apply the Gaussian filter to the image
	for (int j = 0; j<imgHeight; j++) {
		for (int i = 0; i<imgWidth; i++){
			pic_grey[j*imgWidth + i] = RGB2grey(i, j);
		}
		if( j >= FILTER_SIZE_HALF ){
			sem_post( &sema );
/*			if(j <= frameBegin+5){
			 	printf("[Th %d][RGB2grey][j = %d] sema+\n", pval, j);
				PAUSE;
			}*/
		}
	}

	for(int i=0; i < FILTER_SIZE_HALF + THREAD_NUM ; i++){
		sem_post( &sema );
	}
}

int j_share;

void *GaussianFilterMult(void *argu){
	int pval = *(int*)argu;
	//apply the Gaussian filter to the image
	for ( ; j_share < imgHeight ; ) {
		sem_wait( &sema );
/*		if(j_share <= frameBegin+5){
			printf("[Th %d][GaussianFilterMult][imgHeight = %d][j_share = %d] sema-\n", pval, imgHeight, j_share);
			PAUSE;
		}*/

		pthread_mutex_lock(&mutex_j);
		int j = j_share;
		j_share++;
		pthread_mutex_unlock(&mutex_j);

		if(j >= imgHeight) break;

		for (int i = 0; i<imgWidth; i++){
			//extend the size form WxHx1 to WxHx3
			int tmp = GaussianFilter(i, j);
			pic_final[3 * (j*imgWidth + i) + MYRED] = tmp;
			pic_final[3 * (j*imgWidth + i) + MYGREEN] = tmp;
			pic_final[3 * (j*imgWidth + i) + MYBLUE] = tmp;
		}


	}
}

int main()
{
	// read mask file
	FILE* mask;
	mask = fopen("mask_Gaussian.txt", "r");
	fscanf(mask, "%d", &FILTER_SIZE);
	fscanf(mask, "%d", &FILTER_SCALE);

	filter_G = new int[FILTER_SIZE];
	for (int i = 0; i<FILTER_SIZE; i++)
		fscanf(mask, "%d", &filter_G[i]);
	fclose(mask);


	BmpReader* bmpReader = new BmpReader();
	for (int k = 0; k<5; k++){
		// read input BMP file
		pic_in = bmpReader->ReadBMP(inputfile_name[k], &imgWidth, &imgHeight);
		// allocate space for output image
		pic_grey = (unsigned char*)malloc(imgWidth*imgHeight*sizeof(unsigned char));
		pic_final = (unsigned char*)malloc(3 * imgWidth*imgHeight*sizeof(unsigned char));
		
		FILTER_SIZE_HALF = FILTER_SIZE / 2;

		pthread_t threadRGB;
		pthread_t threadFilter[ THREAD_NUM ];
		sem_init(&sema, 0, 1);
		sem_wait(&sema);
		int apple;
		pthread_mutex_init(&mutex_j, NULL);
		pthread_mutex_init(&mutex_filter, NULL);
		pthread_create(&threadRGB, NULL, RGB2greyMult, &apple);		
		j_share = 0;	
		for(int i=0; i<THREAD_NUM; i++){
			pthread_create(&threadFilter[i], NULL, GaussianFilterMult, &apple);
		}


		pthread_join(threadRGB, NULL);
		for(int i=0; i<THREAD_NUM; i++){
			pthread_join(threadFilter[i], NULL);
		}

		


		// write output BMP file
		bmpReader->WriteBMP(outputBlur_name[k], imgWidth, imgHeight, pic_final);

		//free memory space
		free(pic_in);
		free(pic_grey);
		free(pic_final);
	}

	return 0;
}