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
#define THREAD_NUM 8
using namespace std;

#define MYRED	2
#define MYGREEN 1
#define MYBLUE	0

int imgWidth, imgHeight;
int FILTER_SIZE;
int FILTER_SCALE;
int *filter_G;


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

unsigned char *pic_in, *pic_grey, *pic_blur, *pic_final;

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

void *RGB2greyMult(void *argu){
	int pval = *(int*)argu;
	//apply the Gaussian filter to the image
	for (int j = 0; j<imgHeight; j++) {
		for (int i = pval; i<imgWidth; i+=THREAD_NUM){
			pic_grey[j*imgWidth + i] = RGB2grey(i, j);
		}
	}
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

void *GaussianFilterMult(void *argu){
	int pval = *(int*)argu;
	//apply the Gaussian filter to the image
	for (int j = 0; j<imgHeight; j++) {
		for (int i = pval; i<imgWidth; i+=THREAD_NUM){
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
		pic_blur = (unsigned char*)malloc(imgWidth*imgHeight*sizeof(unsigned char));
		pic_final = (unsigned char*)malloc(3 * imgWidth*imgHeight*sizeof(unsigned char));
		


		//convert RGB image to grey image
		pthread_t thread[ THREAD_NUM ];
		for(int i=0; i<THREAD_NUM; i++){
			int *pval = new int;
			*pval = i;
			pthread_create(&thread[i], NULL, RGB2greyMult, pval);
		}
		for(int i=0; i<THREAD_NUM; i++)	pthread_join(thread[i], NULL);


		//apply the Gaussian filter to the image
		for(int i=0; i<THREAD_NUM; i++){
			int *pval = new int;
			*pval = i;
			pthread_create(&thread[i], NULL, GaussianFilterMult, pval);
		}
		for(int i=0; i<THREAD_NUM; i++)	pthread_join(thread[i], NULL);

		


		// write output BMP file
		bmpReader->WriteBMP(outputBlur_name[k], imgWidth, imgHeight, pic_final);

		//free memory space
		free(pic_in);
		free(pic_grey);
		free(pic_blur);
		free(pic_final);
	}

	return 0;
}