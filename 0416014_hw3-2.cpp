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
using namespace std;

#define MYRED	2
#define MYGREEN 1
#define MYBLUE	0

int imgWidth, imgHeight;
int FILTER_SIZE;
int FILTER_SCALE;
int *filter_Sx;
int *filter_Sy;

const char *inputfile_name[5] = {
	"input1.bmp",
	"input2.bmp",
	"input3.bmp",
	"input4.bmp",
	"input5.bmp"
};
const char *outputSobel_name[5] = {
	"Sobel1.bmp",
	"Sobel2.bmp",
	"Sobel3.bmp",
	"Sobel4.bmp",
	"Sobel5.bmp"
};

unsigned char *pic_in, *pic_grey, *pic_edge, *pic_x, *pic_y, *pic_final;

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

unsigned char edgeDetection(int w, int h, bool pval)
{
	int tmp = 0;
	int a, b;
	int ws = (int)sqrt((float)FILTER_SIZE);
	for (int j = 0; j<ws; j++)
	for (int i = 0; i<ws; i++)
	{
		a = w + i - (ws / 2);
		b = h + j - (ws / 2);

		// detect for borders of the image
		if (a<0 || b<0 || a>=imgWidth || b>=imgHeight) continue;

		if (pval)
			tmp += filter_Sx[j*ws + i] * pic_grey[b*imgWidth + a];
		else
			tmp += filter_Sy[j*ws + i] * pic_grey[b*imgWidth + a];
		
	};
	if (tmp < 0) tmp = 0;
	if (tmp > 255) tmp = 255;
	return (unsigned char)tmp;
}

void *edgeDetectionMult(void *argu){
	bool pval = *(bool*)argu;
	//apply the Gaussian filter to the image
	for (int j = 0; j<imgHeight; j++) {
		for (int i = 0; i<imgWidth; i++){
			//extend the size form WxHx1 to WxHx3
			unsigned char tmp = edgeDetection(i, j, pval);
			if(pval)
				pic_x[ j*imgWidth + i ] = tmp;
			else
				pic_y[ j*imgWidth + i ] = tmp;
		}
	}
}

int main()
{
	// read mask file
	FILE* mask;
	mask = fopen("mask_Sobel.txt", "r");
	fscanf(mask, "%d", &FILTER_SIZE);

	filter_Sx = new int[FILTER_SIZE];
	for (int i = 0; i<FILTER_SIZE; i++)
		fscanf(mask, "%d", &filter_Sx[i]);
	filter_Sy = new int[FILTER_SIZE];
	for (int i = 0; i<FILTER_SIZE; i++)
		fscanf(mask, "%d", &filter_Sy[i]);
	fclose(mask);


	BmpReader* bmpReader = new BmpReader();
	for (int k = 0; k<5; k++){
		// read input BMP file
		pic_in = bmpReader->ReadBMP(inputfile_name[k], &imgWidth, &imgHeight);
		// allocate space for output image
		pic_grey = (unsigned char*)malloc(imgWidth*imgHeight*sizeof(unsigned char));
		pic_edge = (unsigned char*)malloc(imgWidth*imgHeight*sizeof(unsigned char));
		pic_x = (unsigned char*)malloc(imgWidth*imgHeight*sizeof(unsigned char));
		pic_y = (unsigned char*)malloc(imgWidth*imgHeight*sizeof(unsigned char));
		pic_final = (unsigned char*)malloc(3 * imgWidth*imgHeight*sizeof(unsigned char));
		


		//convert RGB image to grey image
		for (int j = 0; j<imgHeight; j++) {
			for (int i = 0; i<imgWidth; i++){
				pic_grey[j*imgWidth + i] = RGB2grey(i, j);
			}
		}

		//apply the Gaussian filter to the image
		pthread_t thread[3];
		bool pval0 = 0, pval1 = 1;
		pthread_create(&thread[0], NULL, edgeDetectionMult, &pval0 );
		pthread_create(&thread[1], NULL, edgeDetectionMult, &pval1 );
		for(int i=0; i<2; i++)	pthread_join(thread[i], NULL);

		for (int j = 0; j<imgHeight; j++) {
			for (int i = 0; i<imgWidth; i++){
				int tmp = sqrt( (float) (
					pic_x[ j*imgWidth + i ] * pic_x[ j*imgWidth + i ] + 
				 	pic_y[ j*imgWidth + i ] * pic_y[ j*imgWidth + i ] ) );
					if (tmp < 0) tmp = 0;
					if (tmp > 255) tmp = 255;
				pic_final[ 3 * ( j*imgWidth + i ) + MYRED ] = (unsigned char)tmp;
				pic_final[ 3 * ( j*imgWidth + i ) + MYGREEN ] = (unsigned char)tmp;
				pic_final[ 3 * ( j*imgWidth + i ) + MYBLUE ] = (unsigned char)tmp;
			}
		}


		// write output BMP file
		bmpReader->WriteBMP(outputSobel_name[k], imgWidth, imgHeight, pic_final);

		//free memory space
		free(pic_in);
		free(pic_grey);
		free(pic_edge);
		free(pic_final);
	}

	return 0;
}