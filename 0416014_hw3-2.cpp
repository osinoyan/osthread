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
#define PAUSE printf("PAUSE\n");while(getchar() != '\n')
using namespace std;

#define MYRED	2
#define MYGREEN 1
#define MYBLUE	0

int imgWidth, imgHeight;
int FILTER_SIZE;
int WIN_SIZE;
int EXTEND;
int EXTEND_WIDTH;
int FILTER_SCALE;
int *filter_Sx;
int *filter_Sy;



pthread_mutex_t mutex_j_x;
pthread_mutex_t mutex_j_y;
pthread_mutex_t mutex_filter;

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

unsigned char *pic_in, *pic_grey, *pic_x, *pic_y, *pic_final;

/*unsigned char RGB2grey(int w, int h)
{
	int tPxl = 3 * (h*imgWidth + w);

	int tmp = (
		pic_in[tPxl + MYRED] +
		pic_in[tPxl + MYGREEN] +
		pic_in[tPxl + MYBLUE] )/3;

	if (tmp < 0) tmp = 0;
	if (tmp > 255) tmp = 255;
	return (unsigned char)tmp;
}

unsigned char edgeDetection(int w, int h, bool pval)
{
	w += EXTEND;
	h += EXTEND;
	int tmp = 0;
	int a, b;
	int ws = (int)sqrt((float)FILTER_SIZE);

	int w_minus_wsHalf = w - ws/2;
	int h_minus_wsHalf = h - ws/2;

	for (int j = 0; j<ws; j++){
		int jws = j*ws;
		for (int i = 0; i<ws; i++){

			a = i + w_minus_wsHalf;
			b = j + h_minus_wsHalf;

			// detect for borders of the image
			// if (a<0 || b<0 || a>=imgWidth || b>=imgHeight) continue;

			if (pval)
				tmp += filter_Sx[jws + i] * pic_grey[b*EXTEND_WIDTH + a];
			else
				tmp += filter_Sy[jws + i] * pic_grey[b*EXTEND_WIDTH + a];
			
			}
	}
	return (unsigned char)(tmp<0 ? 0 : tmp>255 ? 255 : tmp);
}

void *RGB2greyMult(void *argu){
	int pval = *(int*)argu;
	//apply the Gaussian filter to the image
	for (int j = 0; j<imgHeight; j++) {
		int tPxlJX = (j + EXTEND) * EXTEND_WIDTH + EXTEND;
		for (int i = 0; i<imgWidth; i++){

			int tPxl = 3 * (j*imgWidth + i);
			int tmp = (
				pic_in[tPxl + MYRED] +
				pic_in[tPxl + MYGREEN] +
				pic_in[tPxl + MYBLUE] )/3;

			pic_grey[ tPxlJX + i ] = (unsigned char)(tmp<0 ? 0 : tmp>255 ? 255 : tmp);
		}
	}
}*/

int j_share_x;
int j_share_y;

void *edgeDetectionMult(void *argu){
	bool pval = *(bool*)argu;
	int j_share = pval ? j_share_x : j_share_y;
	//apply the Gaussian filter to the image
	for ( ; j_share < imgHeight ; ) {
		int j;
		if(pval){
			pthread_mutex_lock(&mutex_j_x);
			j = j_share_x;
			j_share_x++;
			pthread_mutex_unlock(&mutex_j_x);
		}else{
			pthread_mutex_lock(&mutex_j_y);
			j = j_share_y;
			j_share_y++;
			pthread_mutex_unlock(&mutex_j_y);			
		}


		if(j >= imgHeight) break;

		for (int i = 0; i<imgWidth; i++){
			//extend the size form WxHx1 to WxHx3
			int tmp = 0;
			{
				int w = i+EXTEND;
				int h = j+EXTEND;
				int a, b;
				int ws = (int)sqrt((float)FILTER_SIZE);

				int w_minus_wsHalf = w - ws/2;
				int h_minus_wsHalf = h - ws/2;

				for (int j = 0; j<ws; j++){
					int jws = j*ws;
					for (int i = 0; i<ws; i++){

						a = i + w_minus_wsHalf;
						b = j + h_minus_wsHalf;

						// detect for borders of the image
						//if (a<0 || b<0 || a>=imgWidth || b>=imgHeight) continue;

						if (pval)
							tmp += filter_Sx[jws + i] * pic_grey[b*EXTEND_WIDTH + a];
						else
							tmp += filter_Sy[jws + i] * pic_grey[b*EXTEND_WIDTH + a];

					}
				}
				tmp = (unsigned char)(tmp<0 ? 0 : tmp>255 ? 255 : tmp);
				if(pval)
					pic_x[ j*imgWidth + i ] = tmp;
				else
					pic_y[ j*imgWidth + i ] = tmp;
			}
		}


	}

}

int j_share;

void *finalMult(void *argu){
	bool pval = *(bool*)argu;
	for ( ; j_share < imgHeight ; ) {

		pthread_mutex_lock(&mutex_j_x);
		int j = j_share;
		j_share++;
		pthread_mutex_unlock(&mutex_j_x);

		for (int i = 0; i<imgWidth; i++){
			int tmp = sqrt( (float) (
				pic_x[ j*imgWidth + i ] * pic_x[ j*imgWidth + i ] + 
			 	pic_y[ j*imgWidth + i ] * pic_y[ j*imgWidth + i ] ) );
				tmp = (unsigned char)(tmp<0 ? 0 : tmp>255 ? 255 : tmp);
			int tPxl = 3 * (j*imgWidth + i);
			pic_final[tPxl + MYRED] = tmp;
			pic_final[tPxl + MYGREEN] = tmp;
			pic_final[tPxl + MYBLUE] = tmp;
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
		//extended zeros	
		WIN_SIZE = (int)sqrt((float)FILTER_SIZE);	
		EXTEND = WIN_SIZE / 2;
		EXTEND_WIDTH = imgWidth + EXTEND*2;
		pic_grey = (unsigned char*)calloc( EXTEND_WIDTH * (imgHeight + EXTEND*2), sizeof(unsigned char) );
		pic_x = (unsigned char*)malloc(imgWidth*imgHeight*sizeof(unsigned char));
		pic_y = (unsigned char*)malloc(imgWidth*imgHeight*sizeof(unsigned char));
		pic_final = (unsigned char*)malloc(3 * imgWidth*imgHeight*sizeof(unsigned char));


		//convert RGB image to grey image
		for (int j = 0; j<imgHeight; j++) {
			int tPxlJX = (j + EXTEND) * EXTEND_WIDTH + EXTEND;
			for (int i = 0; i<imgWidth; i++){

				int tPxl = 3 * (j*imgWidth + i);
				int tmp = (
					pic_in[tPxl + MYRED] +
					pic_in[tPxl + MYGREEN] +
					pic_in[tPxl + MYBLUE] )/3;

				pic_grey[ tPxlJX + i ] = (unsigned char)(tmp<0 ? 0 : tmp>255 ? 255 : tmp);
			}
		}

		//apply the Gaussian filter to the image
		pthread_t thread[ THREAD_NUM ];		
		pthread_mutex_init(&mutex_j_x, NULL);
		pthread_mutex_init(&mutex_j_y, NULL);
		j_share_x = 0;
		j_share_y = 0;
		bool pval0 = 0, pval1 = 1;
		for(int i=0; i<THREAD_NUM; i++) 
			pthread_create(&thread[i], NULL, edgeDetectionMult, (i&0x1) ? &pval1 : &pval0 );
		for(int i=0; i<THREAD_NUM; i++)	pthread_join(thread[i], NULL);


		j_share = 0;
		for(int i=0; i<THREAD_NUM; i++) pthread_create(&thread[i], NULL, finalMult, &pval0);
		for(int i=0; i<THREAD_NUM; i++)	pthread_join(thread[i], NULL);

		// for (int j = 0; j<imgHeight; j++) {
		// 	for (int i = 0; i<imgWidth; i++){
		// 		int tmp = sqrt( (float) (
		// 			pic_x[ j*imgWidth + i ] * pic_x[ j*imgWidth + i ] + 
		// 		 	pic_y[ j*imgWidth + i ] * pic_y[ j*imgWidth + i ] ) );
		// 			tmp = (unsigned char)(tmp<0 ? 0 : tmp>255 ? 255 : tmp);
		// 		int tPxl = 3 * (j*imgWidth + i);
		// 		pic_final[tPxl + MYRED] = tmp;
		// 		pic_final[tPxl + MYGREEN] = tmp;
		// 		pic_final[tPxl + MYBLUE] = tmp;
		// 	}
		// }


		// write output BMP file
		bmpReader->WriteBMP(outputSobel_name[k], imgWidth, imgHeight, pic_final);

		//free memory space
		free(pic_in);
		free(pic_grey);
		free(pic_final);
	}

	return 0;
}