//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de funções não seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include "vc.h"
#include <math.h>

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Alocar memória para uma imagem
IVC *vc_image_new(int width, int height, int channels, int levels)
{
	IVC *image = (IVC *) malloc(sizeof(IVC));

	if(image == NULL) return NULL;
	if((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char *) malloc(image->width * image->height * image->channels * sizeof(char));

	if(image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar memória de uma imagem
IVC *vc_image_free(IVC *image)
{
	if(image != NULL)
	{
		if(image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char *netpbm_get_token(FILE *file, char *tok, int len)
{
	char *t;
	int c;
	
	for(;;)
	{
		while(isspace(c = getc(file)));
		if(c != '#') break;
		do c = getc(file);
		while((c != '\n') && (c != EOF));
		if(c == EOF) break;
	}
	
	t = tok;
	
	if(c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));
		
		if(c == '#') ungetc(c, file);
	}
	
	*t = 0;
	
	return tok;
}


long int unsigned_char_to_bit(unsigned char *datauchar, unsigned char *databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char *p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);
				
				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}


void bit_to_unsigned_char(unsigned char *databit, unsigned char *datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char *p = databit;

	countbits = 1;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;
				
				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC *vc_read_image(char *filename)
{
	FILE *file = NULL;
	IVC *image = NULL;
	unsigned char *tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;
	
	// Abre o ficheiro
	if((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if(strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if(strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if(strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
			#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
			#endif

			fclose(file);
			return NULL;
		}
		
		if(levels == 1) // PBM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			if((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			size = image->width * image->height * image->channels;

			if((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}
		
		fclose(file);
	}
	else
	{
		#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
		#endif
	}
	
	return image;
}


int vc_write_image(char *filename, IVC *image)
{
	FILE *file = NULL;
	unsigned char *tmp;
	long int totalbytes, sizeofbinarydata;
	
	if(image == NULL) return 0;

	if((file = fopen(filename, "wb")) != NULL)
	{
		if(image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;
			
			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);
			
			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if(fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);
		
			if(fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				return 0;
			}
		}
		
		fclose(file);

		return 1;
	}
	
	return 0;
}



#pragma region Meu Codigo

/// <summary>
/// Metodo que negativa uma imagem binaria
/// </summary>
/// <param name="srcdst"></param>
/// <returns></returns>
int vc_bit_negative(IVC* srcdst) {
	int x, y;
	long int pos;
	if (srcdst->height <= 0 || srcdst->width <= 0 || srcdst->data == NULL) return 0;
	if (srcdst->channels != 1) return 0;

	for (y = 0; y < srcdst->height; y++) {
		for (x = 0; x < srcdst->width; x++) {
			pos = y * srcdst->bytesperline + x * srcdst->channels;
			if (srcdst->data[pos] == 1) srcdst->data[pos] = 0;
			else srcdst->data[pos] = 1;
		}
	}

	return 0;
}

/// <summary>
/// Função que pega uma imagem com 1 canal e negativa a imagem.
/// </summary>
/// <param name="srcdst"></param>
/// <returns>1 or 0</returns>

int vc_gray_negative(IVC* srcdst){
	int x, y;
	long int pos;
	if (srcdst->height <= 0 || srcdst->width <= 0 || srcdst->data == NULL) return 0;
	if (srcdst->channels != 1) return 0;
	
	for (y = 0; y < srcdst->height; y++) {
		for (x = 0; x < srcdst->width; x++) {
			pos = y * srcdst->bytesperline + x * srcdst->channels;
			srcdst->data[pos] = 255-srcdst->data[pos];
		}
	}

	return 0;
}

/// <summary>
/// Função que pega uma imagem com 3 canal e negativa a imagem.
/// </summary>
/// <param name="srcdst"></param>
/// <returns></returns>
int vc_rgb_negative(IVC* srcdst) {
	int x, y;
	long int pos;
	if (srcdst->height <= 0 || srcdst->width <= 0 || srcdst->data == NULL) return 0;
	if (srcdst->channels != 3) return 0;

	for (y = 0; y < srcdst->height; y++) {
		for (x = 0; x < srcdst->width; x++) {
			pos = y * srcdst->bytesperline + x * srcdst->channels;
			srcdst->data[pos] = 255 - srcdst->data[pos];
			srcdst->data[pos+1] = 255 - srcdst->data[pos+1];
			srcdst->data[pos+2] = 255 - srcdst->data[pos+2];
		}
	}

	return 1;
}

/// <summary>
/// Função que pega numa imagem rgb, pega os valores vermelhos, e transforma em cinzento
/// </summary>
/// <param name="srcdst"></param>
/// <returns></returns>
int vc_rgb_get_red_gray(IVC* srcdst) {
	int x, y;
	long int pos;
	if (srcdst->height <= 0 || srcdst->width <= 0 || srcdst->data == NULL) return 0;
	if (srcdst->channels != 3) return 0;

	for (y = 0; y < srcdst->height; y++) {
		for (x = 0; x < srcdst->width; x++) {
			pos = y * srcdst->bytesperline + x * srcdst->channels;
			srcdst->data[pos + 1] = 255 - srcdst->data[pos];
			srcdst->data[pos + 2] = 255 - srcdst->data[pos];
		}
	}
	return 1;
}

/// <summary>
/// Função que pega numa imagem rgb, pega os valores verdes, e transforma em cinzento
/// </summary>
/// <param name="srcdst"></param>
/// <returns></returns>
int vc_rgb_get_green_gray(IVC* srcdst) {
	int x, y;
	long int pos;
	if (srcdst->height <= 0 || srcdst->width <= 0 || srcdst->data == NULL) return 0;
	if (srcdst->channels != 3) return 0;

	for (y = 0; y < srcdst->height; y++) {
		for (x = 0; x < srcdst->width; x++) {
			pos = y * srcdst->bytesperline + x * srcdst->channels;
			pos++;
			srcdst->data[pos - 1] = 255 - srcdst->data[pos];
			srcdst->data[pos + 1] = 255 - srcdst->data[pos];
		}
	}
	return 1;
}

/// <summary>
/// Função que pega numa imagem rgb, pega os valores azuis, e transforma em cinzento
/// </summary>
/// <param name="srcdst"></param>
/// <returns></returns>
int vc_rgb_get_blue_gray(IVC* srcdst) {
	int x, y;
	long int pos;
	if (srcdst->height <= 0 || srcdst->width <= 0 || srcdst->data == NULL) return 0;
	if (srcdst->channels != 3) return 0;

	for (y = 0; y < srcdst->height; y++) {
		for (x = 0; x < srcdst->width; x++) {
			pos = y * srcdst->bytesperline + x * srcdst->channels;
			pos += 2;
			srcdst->data[pos - 1] = 255 - srcdst->data[pos];
			srcdst->data[pos - 2] = 255 - srcdst->data[pos];
		}
	}
	return 1;
}


/// <summary>
/// Função que recebe uma imagem RGB e transforma em cinzento
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <returns>1 or 0</returns>
int vc_rgb_to_gray(IVC* src, IVC* dst) {
	int x, y;
	long int possrc,posdst;
	if (src->height <= 0 || src->width <= 0 || src->channels != 3 || src->data == NULL) return 0;
	if (dst->data == NULL) return 0;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;
												//			Red										Green										Blue	
			dst->data[posdst] = (unsigned char) (((float)(src->data[possrc])  *0.299 + (float)(src->data[possrc + 1]) *0.587 + (float)(src->data[possrc + 2]) *0.114));
		}
	}
	return 1;
}

/// <summary>
/// recebe uma imagem rgb e transforma em hsv
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <returns></returns>

int vc_rgb_to_hsv(IVC* src, IVC* dst) {
	
	int x, y;
	int valor, saturação, matiz,menor, maior;
	long int possrc, posdst;
	if (src->height <= 0 || src->width <= 0 || src->channels != 3 || src->data == NULL) return 0;
	if (dst->data == NULL) return 0;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			possrc = y * src->bytesperline + x * src->channels;

			maior = MAX3(src->data[possrc], src->data[possrc + 1], src->data[possrc + 2]);
			valor = maior;
			menor = MIN3(src->data[possrc], src->data[possrc + 1], src->data[possrc + 2]);
			if (valor == 0) {
				saturação = 0;
			}
			else {
				saturação = 255.0f * (float)(maior - menor) / (float)valor;
			}
			
			if(maior != menor){
				if ( src->data[possrc] == maior) {
					if (src->data[possrc + 1] >= src->data[possrc + 2]) {
						matiz = 60 * (src->data[possrc + 1] - src->data[possrc + 2]) / (maior - menor);
					}
					else {
						matiz = 360 + 60 * (src->data[possrc + 1] - src->data[possrc + 2]) / (maior - menor);
					}
				}

				else if (src->data[possrc + 1] == maior) {
					matiz = 120 + 60 * (src->data[possrc + 2] -  src->data[possrc]) /  (maior - menor);
				}

				else if (src->data[possrc + 2] == maior) {
					matiz = 240 + 60 *  ( src->data[possrc] - src->data[possrc + 1]) /  (maior - menor);
				}
			}
			else
			{
				matiz = 0;
			}

			dst->data[possrc] = (float)((float)matiz * 255.0f) / 360.0f;
			dst->data[possrc + 1] = saturação;
			dst->data[possrc + 2] = valor;
		}
	}
	return 1;

}

/// <summary>
/// Faz a conversao de HSV para RGB
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <returns></returns>
int vc_hsv_to_rgb(IVC* src, IVC* dst) {
	int x, y;
	float h,s,v,hi,f,p,q,t;
	long int possrc, posdst;
	if (src->height <= 0 || src->width <= 0 || src->channels != 3 || src->data == NULL) return 0;
	if (dst->data == NULL) return 0;
	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;
			h = (src->data[possrc] * 360 / 255);
			s = src->data[possrc + 1] / 255.0;
			v = src->data[possrc + 2] / 255.0;
			if (s == 0) {
				dst->data[posdst] = v * 255;
				dst->data[posdst + 1] = v * 255;
				dst->data[posdst + 2] = v * 255;
			}
			else {
				hi = (int)(h / 60) % 6;
				f = (h / 60.0) - hi;
				p = v*(1 - s);
				q = v * (1 - f * s);
				t = v * (1 - (1 - f) * s);
				if (hi == 0) {
					dst->data[posdst] = v * 255;
					dst->data[posdst + 1] = t * 255;
					dst->data[posdst + 2] = p * 255;
				}
				else if (hi == 1) {
					dst->data[posdst] = q * 255;
					dst->data[posdst + 1] = v * 255;
					dst->data[posdst + 2] = p * 255;
				}
				else if (hi == 2) {
					dst->data[posdst] = p * 255;
					dst->data[posdst + 1] = v * 255;
					dst->data[posdst + 2] = t * 255;
				}
				else if (hi == 3) {
					dst->data[posdst] = p * 255;
					dst->data[posdst + 1] = q * 255;
					dst->data[posdst + 2] = v * 255;
				}
				else if (hi == 4) {
					dst->data[posdst] = t * 255;
					dst->data[posdst + 1] = p * 255;
					dst->data[posdst + 2] = v * 255;
				}
				else if (hi == 5) {
					dst->data[posdst] = v * 255;
					dst->data[posdst + 1] = p * 255;
					dst->data[posdst + 2] = q * 255;
				}
			}
		}
	}
	return 1;
}


/// <summary>
/// Função que recebe imagens de destino e origem. Analisa os pixeis de origem, os valores de matiz,saturação e valor, se estiverem dentro dos limites minimos e maximos
/// o pixel na imagem destino (que so tem um canal) será 255 senao é 0
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <param name="hmin"></param>
/// <param name="hmax"></param>
/// <param name="smin"></param>
/// <param name="smax"></param>
/// <param name="vmin"></param>
/// <param name="vmax"></param>
/// <returns></returns>
/// 
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax) {
	int x, y;
	long int possrc, posdst;

	if (src->height <= 0 || src->width <= 0 || src->channels != 3 || src->data == NULL) return 0;
	if (dst->data == NULL) return 0;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;

			if ((src->data[possrc] >= hmin && src->data[possrc] <= hmax) && (src->data[possrc + 1] >= smin && src->data[possrc + 1] <= smax) && (src->data[possrc + 2] >= vmin && src->data[possrc + 2] <= vmax)){
				dst->data[posdst] = 255;
			}
			else{
				dst->data[posdst] = 0;
			}
		}
	}
}

int vc_scale_gray_to_color_palette(IVC* src, IVC* dst) {
	int x, y;
	long int possrc, posdst;


	if (src->height <= 0 || src->width <= 0|| src->data == NULL) return 0;
	if (dst->data == NULL) return 0;


	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++)
		{
			/// BLUE GREEN RED
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;

			if (src->data[possrc] < 64) 
			{
				dst->data[posdst] = 0;
				///		positivo		(x - 64 * quadrante) * 4 
				dst->data[posdst + 1] = (src->data[possrc]- 64 * 0) * 4;
				dst->data[posdst + 2] = 255;
			}
			else if (src->data[possrc] < 128)
			{
				///			negativo		255 - (x - 64 * quadrante) * 4					
				dst->data[posdst] = 0;
				dst->data[posdst + 1] = 255;
				dst->data[posdst + 2] = 255 - ((src->data[possrc] - 64 * 1) * 4);
			}
			else if (src->data[possrc] < 192) {
				dst->data[posdst] = (src->data[possrc] - 64 * 2) * 4;
				dst->data[posdst + 1] = 255;
				dst->data[posdst + 2] = 0;
			}
			else {
				dst->data[posdst] = 255;
				dst->data[posdst + 1] = 255 - ((src->data[possrc] - 64 * 3) * 4);
				dst->data[posdst + 2] = 0;
			}
		}
	}
}

/// <summary>
/// Recebe uma imagem a cinzento e conta os pixeis brancos
/// </summary>
/// <param name="src"></param>
/// <returns></returns>
int vc_count_white_pixel_gray(IVC* src){
	int x, y, white;
	long int pos;

	if (src->height <= 0 || src->width <= 0 || src->data == NULL) return 0;

	white = 0;
	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			if (src->data[pos] == 255) white++;
		}
	}

	return white;
}

/// <summary>
/// metodo que pega numa imagem em cinzento e transforma em binario
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <param name="threshold"></param>
/// <returns></returns>

int vc_gray_to_binary(IVC* src, IVC* dst, int threshold) {
	int x, y;
	long int possrc,posdst;

	if (src->height <= 0 || src->width <= 0 || src->channels != 1 || src->data == NULL) return 0;
	if (dst->data == NULL) return 0;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++)
		{
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;
			if (src->data[possrc] > threshold) dst->data[posdst] = 1;
			else dst->data[posdst] = 0;
		}
	}

	return 0;
}
/// <summary>
/// Calcula a media dos valores de todos os pixeis
/// </summary>
/// <param name="src"></param>
/// <returns></returns>
int calculate_avg_gray(IVC* src) {
	int x, y;
	long int pos;
	int sum = 0;
	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			sum += src->data[pos];
		}
	}


	return (float)(sum) / (float)(src->height * src->width);
}

/// <summary>
/// Transformação do cinzento para binario atravez do metodo global
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <returns></returns>
int vc_gray_to_binary_global_mean(IVC* src, IVC* dst) {
	int x, y;
	long int possrc, posdst;
	int threshold = calculate_avg_gray(src);

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++)
		{
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;
			if (src->data[possrc] > threshold) dst->data[posdst] = 1;
			else dst->data[posdst] = 0;
		}
	}
	return 0;
}

/// <summary>
/// Transformação do cinzento para binario atravez do metodo midpoint
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <param name="kernel"></param>
/// <returns></returns>
int vc_gray_to_binary_midpoint_mean(IVC* src, IVC* dst, int kernel) {
	int x, y, xv, yv, pixel;
	int min, max;
	long int possrc, posv, posdst;
	int threshold;

	if (kernel % 2 == 0) return 0;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			min = 255;
			max = 0;

			for (yv = -(kernel / 2); yv <= kernel / 2; yv++) {
				for (xv = -(kernel / 2); xv <= kernel / 2; xv++) {
					if (y + yv < 0 || y + yv >= src->height || x + xv < 0 || x + xv >= src->width) {
						continue;
					}
					else {
						posv = (y + yv) * src->bytesperline + (x + xv) * src->channels;
						pixel = src->data[posv];
						if (pixel > max) max = pixel;
						if (pixel < min) min = pixel;
					}
				}
			}

			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;

			threshold = (float)(min + max) / 2.0f;
			if (src->data[possrc] > threshold) dst->data[posdst] = 1;
			else dst->data[posdst] = 0;
		}
	}
	return 0;
}

/// <summary>
/// Transformação do cinzento para binario atravez do metodo bernsen
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <param name="kernel"></param>
/// <param name="cmin"></param>
/// <returns></returns>
int vc_gray_to_binary_bernsen_mean(IVC* src, IVC* dst, int kernel, int cmin) {
	int x, y, xv, yv, pixel;
	int min, max;
	long int possrc, posv, posdst;
	int threshold;

	if (kernel % 2 == 0) return 0;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			min = 255;
			max = 0;

			for (yv = -(kernel / 2); yv <= kernel / 2; yv++) {
				for (xv = -(kernel / 2); xv <= kernel / 2; xv++) {
					if (y + yv < 0 || y + yv >= src->height || x + xv < 0 || x + xv >= src->width) {
						continue;
					}
					else {
						posv = (y + yv) * src->bytesperline + (x + xv) * src->channels;
						pixel = src->data[posv];
						if (pixel > max) max = pixel;
						if (pixel < min) min = pixel;
					}
				}
			}

			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;

			if (max - min < cmin){
				threshold = 255.0f / 2.0f;
				if (src->data[possrc] > threshold) dst->data[posdst] = 1;
				else dst->data[posdst] = 0;
			}
			else{
				threshold = (float)(min + max) / 2.0f;
				if (src->data[possrc] > threshold) dst->data[posdst] = 1;
				else dst->data[posdst] = 0;
			}
		}
	}
	return 0;
}

/// <summary>
/// Transformação de cinzento para binario atravez do metodo niblack
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <param name="kernel"></param>
/// <param name="k"></param>
/// <returns></returns>
int vc_gray_to_binary_niblack_mean(IVC* src, IVC* dst, int kernel, float k) {
	int x, y, xv, yv, somaPixeis, media,desvioPadrao;
	int min, max;
	long int possrc, posv, posdst;
	int threshold;

	if (kernel % 2 == 0) return 0;

	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			desvioPadrao = 0;
			media = 0;
			somaPixeis = 0;
			for (xv = -(kernel / 2); xv <= kernel / 2; xv++)
			{
				for (yv = -(kernel / 2); yv <= kernel / 2; yv++)
				{
					if (y + yv < 0 || y + yv >= src->height || x + xv < 0 || x + xv >= src->width)
						continue;
					posv = (y + yv) * src->bytesperline + (x + xv) * src->channels;
					somaPixeis += src->data[posv];
				}
			}

			media = somaPixeis / (kernel * kernel - 1);

			for (xv = -(kernel / 2); xv <= kernel / 2; xv++)
			{
				for (yv = -(kernel / 2); yv <= kernel / 2; yv++)
				{
					if (y + yv < 0 || y + yv >= src->height || x + xv < 0 || x + xv >= src->width)
						continue;

					posv = (y + yv) * src->bytesperline + (x + xv) * src->channels;
					desvioPadrao += (src->data[posv] - media) * (src->data[posv] - media);
				}
			}
			desvioPadrao = sqrt((desvioPadrao / (kernel * kernel - 1)));
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;
			threshold = media + k * desvioPadrao;
			if (src->data[possrc] > threshold) dst->data[posdst] = 1;
			else dst->data[posdst] = 0;
			
		}
	}
	return 0;
}
/// <summary>
/// Função que realiza uma dilatação na imagem source
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <param name="kernel"></param>
/// <returns></returns>
int vc_binary_dilate(IVC* src, IVC* dst, int kernel) {
	int x, y, xv, yv, pixel;
	long int possrc, posv, posdst;
	int black;

	if (kernel % 2 == 0) return 0;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {

			black = 0;

			for (yv = -(kernel / 2); yv <= kernel / 2; yv++) {
				for (xv = -(kernel / 2); xv <= kernel / 2; xv++) {
					if (y + yv < 0 || y + yv >= src->height || x + xv < 0 || x + xv >= src->width) {
						continue;
					}
					else {
						posv = (y + yv) * src->bytesperline + (x + xv) * src->channels;
						pixel = src->data[posv];
						if (pixel != 0){
							black = 1;
							break;
						}
					}
				}
				if (black) break;
			}

			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;

			if (black) {
				dst->data[posdst] = 1;
			}
			else {
				dst->data[posdst] = 0;
			}
		}
	}
	return 0;
}

/// <summary>
/// Função que realiza uma erosao na imagem source
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <param name="kernel"></param>
/// <returns></returns>
int vc_binary_erode(IVC* src, IVC* dst, int kernel) {
	int x, y, xv, yv, pixel;
	long int possrc, posv, posdst;
	int white;

	if (kernel % 2 == 0) return 0;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {

			white = 0;

			for (yv = -(kernel / 2); yv <= kernel / 2; yv++) {
				for (xv = -(kernel / 2); xv <= kernel / 2; xv++) {
					if (y + yv < 0 || y + yv >= src->height || x + xv < 0 || x + xv >= src->width) {
						continue;
					}
					else {
						posv = (y + yv) * src->bytesperline + (x + xv) * src->channels;
						pixel = src->data[posv];
						if (pixel == 0) {
							white = 1;
							break;
						}
					}
				}
				if (white) break;
			}

			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;

			if (white) {
				dst->data[posdst] = 0;
			}
			else {
				dst->data[posdst] = 1;
			}
		}
	}
	return 0;
}

/// <summary>
/// Função que realiza uma abertura na imagem source
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <param name="kernel"></param>
/// <returns></returns>
int vc_binary_open(IVC* src, IVC* dst, int kernel) {
	IVC* tempdst;

	if (kernel % 2 == 0) return 0;

	tempdst = vc_image_new(src->width, src->height, 1, 1);
	vc_binary_erode(src, tempdst, kernel);
	vc_binary_dilate(tempdst, dst, kernel);

	return 0;
}
/// <summary>
/// Função que realiza um fecho na imagem source
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <param name="kernel"></param>
/// <returns></returns>
int vc_binary_close(IVC* src, IVC* dst, int kernel){
	IVC* tempdst;

	if (kernel % 2 == 0) return 0;

	tempdst = vc_image_new(src->width, src->height, 1, 1);
	vc_binary_dilate(src, tempdst, kernel);
	vc_binary_erode(tempdst, dst, kernel);

	return 0;
}


/// <summary>
/// Remove conteudo de uma imagem atravwez de uma mascara binaria
/// </summary>
/// <param name="mask"></param>
/// <param name="image"></param>
/// <param name="dst"></param>
/// <returns></returns>
int vc_binary_mask_to_binary_remove(IVC* mask, IVC* image,IVC* dst) {
	int x, y;
	long int posmask, posdst;
	if (mask->height <= 0 || mask->width <= 0 || mask->channels != 1 || mask->data == NULL) return 0;
	if (dst->height <= 0 || dst->width <= 0 || dst->channels != 1 || dst->data == NULL) return 0;

	for (y = 0; y < mask->height; y++) {
		for (x = 0; x < mask->width; x++) {
			posmask = y * mask->bytesperline + x * mask->channels;
			posdst = y * dst->bytesperline + x * dst->channels;
			if (mask->data[posmask] == 1) dst->data[posdst] = 0;
			else dst->data[posdst] = image->data[posmask];
		}
	}
	return 1;
}

/// <summary>
/// Mantem o conteudo da imagem original onde tambem existe na mascara binaria
/// </summary>
/// <param name="mask"></param>
/// <param name="image"></param>
/// <param name="dst"></param>
/// <returns></returns>
int vc_binary_mask_to_gray_maintain(IVC* mask, IVC* image, IVC* dst) {
	int x, y;
	long int posmask, posdst;
	if (mask->height <= 0 || mask->width <= 0 || mask->channels != 1 || mask->data == NULL) return 0;
	if (image->height <= 0 || image->width <= 0 || image->channels != 1|| image->data == NULL) return 0;
	if (dst->height <= 0 || dst->width <= 0 || dst->channels != 1|| dst->data == NULL) return 0;
	for (y = 0; y < mask->height; y++) {
		for (x = 0; x < mask->width; x++) {
			posmask = y * mask->bytesperline + x * mask->channels;
			posdst = y * image->bytesperline + x * image->channels;
			if (mask->data[posmask] == 0) {
				dst->data[posdst] = 0;
			}
			else {
				dst->data[posdst] = image->data[posdst];
			}
		}
	}
	return 1;
}

#pragma region MEU LABELLING
/*
int vc_binary_blob_labelling(IVC* src, IVC* dst) {
	int x, y, label = 1;
	long int posA,posB,posC,posD,posX, posdst;
	int menor;
	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			menor = 256;
			posA = (y - 1) * src->bytesperline + (x - 1) * src->channels;
			posB = (y - 1) * src->bytesperline + x * src->channels;
			posC = (y - 1) * src->bytesperline + (x + 1) * src->channels;
			posD = y * src->bytesperline + (x - 1) * src->channels;
			posX = y * src->bytesperline + x * src->channels;
			if (src->data[posX] == 255) {
				// 1º verifica se pixel esta nos limites da imagem 2º ve se nao pertence ao plano de fundo
				// 3º ve se o valor no destino é menor que o menor label 4º verifica se o valor da dst é diferente de 0
				if ((y - 1 > 0 && x - 1 > 0) && src->data[posA] != 0 && dst->data[posA] < menor && dst->data[posA] != 0) {
					menor = dst->data[posA];
				}
				if ((y - 1 > 0) && src->data[posB] != 0 && dst->data[posB] < menor && dst->data[posB] != 0) {
					menor = dst->data[posB];
				}
				if ((y - 1 > 0 && x + 1 <= src->width) && src->data[posC] != 0 && dst->data[posC] < menor && dst->data[posC] != 0) {
					menor = dst->data[posC];
				}
				if ((x - 1 > 0) && src->data[posD] != 0 && dst->data[posD] < menor && dst->data[posD] != 0) {
					menor = dst->data[posD];
				}
				if (menor > label) {
					dst->data[posX] = label;
					label++;
				}
				else {
					dst->data[posX] = menor;
				}
			}
			else {
				dst->data[posX] = 0;
			}
		}
	}
}*/
#pragma endregion 

/// <summary>
/// realiza o histograma de uma imagem em tons de cinzento
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <returns></returns>
int vc_gray_histogram_show(IVC* src, IVC* dst) {
	int x, y, white;
	long int pos;
	int ni[256] = { 0 };

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			ni[src->data[pos]]++;
		}
	}

	float pdf[256];
	int n = src->width * src->height;
	for (int i = 0; i < 256; i++) {
		pdf[i] = (float)ni[i] / (float)n;
	}

	float pdfmax = 0;
	
	for (int i = 0; i < 256; i++) {
		if (pdf[i] > pdfmax) pdfmax = pdf[i];
	}

	float pdfnorm[256];

	for (int i = 0; i < 256; i++) {
		pdfnorm[i] = pdf[i] / pdfmax;
	}

	for (int i = 0; i < 256 * 256; i++)
		dst->data[i] = 0;
	for (int x = 0; x < 256; x++) {
		for (y = (256 - 1); y >= (256 - 1) - pdfnorm[x] * 255; y--) {
			dst->data[y * 256 + x] = 255;
		}
	}
}

/// <summary>
/// Realiza a equalização de uma imagem em cinzento
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <returns></returns>
int vc_gray_histogram_equalization(IVC* src, IVC* dst) {
	int x, y, white;
	long int pos,possrc,posdst;
	int ni[256] = { 0 };

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			ni[src->data[pos]]++;
		}
	}

	float pdf[256];
	int n = src->width * src->height;
	for (int i = 0; i < 256; i++) {
		pdf[i] = (float)ni[i] / (float)n;
	}

	float cdf[256];
	float cdfmin = -1;
	cdf[0] = pdf[0];
	if (cdf[0] != 0) cdfmin = cdf[0];
	for (int i = 1; i < 256; i++) {
		cdf[i] = cdf[i - 1] + pdf[i];
		if (cdfmin == -1 && cdf[i] != 0) cdfmin = cdf[i];
	}

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;
			dst->data[posdst] = ((cdf[src->data[possrc]] - cdfmin) / (1 - cdfmin)) * 255;
		}
	}
}


/// <summary>
/// Realiza a equalização de uma imagem HSV
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <returns></returns>
int vc_hsv_histogram_equalization(IVC* src, IVC* dst) {
	int x, y, white;
	long int pos, possrc, posdst;
	int nis[256] = { 0 };
	int niv[256] = { 0 };
	int n = src->width * src->height;
	//sat
	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			nis[src->data[pos + 1]]++;
		}
	}

	float pdfs[256];
	for (int i = 0; i < 256; i++) {
		pdfs[i] = (float)nis[i] / (float)n;
	}

	float cdfs[256];
	float cdfmins = -1;
	cdfs[0] = pdfs[0];
	if (cdfs[0] != 0) cdfmins = cdfs[0];
	for (int i = 1; i < 256; i++) {
		cdfs[i] = cdfs[i - 1] + pdfs[i];
		if (cdfmins == -1 && cdfs[i] != 0) cdfmins = cdfs[i];
	}

	//value
	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			niv[src->data[pos + 2]]++;
		}
	}

	float pdfv[256];
	for (int i = 0; i < 256; i++) {
		pdfv[i] = (float)niv[i] / (float)n;
	}

	float cdfv[256];
	float cdfminv = -1;
	cdfv[0] = pdfv[0];
	if (cdfv[0] != 0) cdfminv = cdfv[0];
	for (int i = 1; i < 256; i++) {
		cdfv[i] = cdfv[i - 1] + pdfv[i];
		if (cdfminv == -1 && cdfv[i] != 0) cdfminv = cdfv[i];
	}

	//final
	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;
			dst->data[posdst] = src->data[posdst];
			dst->data[posdst+1] = ((cdfs[src->data[possrc+1]] - cdfmins) / (1 - cdfmins)) * 255;
			dst->data[posdst+2] = ((cdfv[src->data[possrc+2]] - cdfminv) / (1 - cdfminv)) * 255;
		}
	}
}

int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th){
	int x, y,yv,xv;
	long int pos, possrc, posdst;
	int fx, fy,mx,my;
	int a[8], count = 0;
	float magn;
	float dir;
	for (y = 1; y < src->height - 1; y++) {
		for (x = 1; x < src->width -1; x++) {
			possrc = y * src->bytesperline + x * src->channels;

			for (yv = -1; yv <= 1; yv++) {
				for (xv = -1; xv <= 1; xv++) {
					if(yv == 0 && xv == 0){
						xv++;
					}
					a[count] = src->data[(y - yv) * src->bytesperline + (x - xv) * src->channels];
					count++;
				}
			}
			/*
				0 1 2
				3 x 4
				5 6 7
			*/

			count = 0;
			mx = (a[2] + a[4] + a[7]) - (a[0] + a[3] + a[5]);
			my = (a[5] + a[6] + a[7]) - (a[0] + a[1] + a[2]);
			fx = src->data[possrc] * mx;
			fy = src->data[possrc] * my;
			magn = sqrt((fx*fx) + (fy*fy));
			if (magn >= th) dst->data[possrc] = 0;
			else dst->data[possrc] = 1;
		}
	}
}


#pragma endregion


#pragma region labelling
// Etiquetagem de blobs
// src		: Imagem binária de entrada
// dst		: Imagem grayscale (irá conter as etiquetas)
// nlabels	: Endereço de memória de uma variável, onde será armazenado o número de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. É necessário libertar posteriormente esta memória.
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[256] = { 0 };
	int labelarea[256] = { 0 };
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC* blobs; // Apontador para array de blobs (objectos) que será retornado desta função.

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	// Copia dados da imagem binária para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pixéis de plano de fundo devem obrigatóriamente ter valor 0
	// Todos os pixéis de primeiro plano devem obrigatóriamente ter valor 255
	// Serão atribuídas etiquetas no intervalo [1,254]
	// Este algoritmo está assim limitado a 254 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	// Limpa os rebordos da imagem binária
	for (y = 0; y < height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x < width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels; // B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels; // D
			posX = y * bytesperline + x * channels; // X

			// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else
				{
					num = 255;

					// Se A está marcado
					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					// Se B está marcado, e é menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					// Se C está marcado, e é menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					// Se D está marcado, e é menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posD]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	//printf("\nMax Label = %d\n", label);

	// Contagem do número de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que não hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++; // Conta etiquetas
		}
	}

	// Se não há blobs
	if (*nlabels == 0) return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC*)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a < (*nlabels); a++) blobs[a].label = labeltable[a];
	}
	else return NULL;

	return blobs;
}


int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Conta área de cada blob
	for (i = 0; i < nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y < height - 1; y++)
		{
			for (x = 1; x < width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// Área
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					// Perímetro
					// Se pelo menos um dos quatro vizinhos não pertence ao mesmo label, então é um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		// Bounding Box
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		// Centro de Gravidade
		//blobs[i].xc = (xmax - xmin) / 2;
		//blobs[i].yc = (ymax - ymin) / 2;
		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}

	return 1;
}
#pragma endregion