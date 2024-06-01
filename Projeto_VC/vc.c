//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de fun��es n�o seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#define MAX3(r,g,b) (r>g? (r>b ? r:b):(g>b?g:b))
#define MIN3(r,g,b) (r<g?(r<b?r:b):(g<b?g:b))

#define MAX(a,b) (a>b?a:b)
#define MIN(a,b) (a<b?a:b)
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "vc.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Alocar mem�ria para uma imagem
IVC* vc_image_new(int width, int height, int channels, int levels)
{
	IVC* image = (IVC*)malloc(sizeof(IVC));

	if (image == NULL) return NULL;
	if ((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char*)malloc(image->width * image->height * image->channels * sizeof(char));

	if (image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar mem�ria de uma imagem
IVC* vc_image_free(IVC* image)
{
	if (image != NULL)
	{
		if (image->data != NULL)
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
//    FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char* netpbm_get_token(FILE* file, char* tok, int len)
{
	char* t;
	int c;

	for (;;)
	{
		while (isspace(c = getc(file)));
		if (c != '#') break;
		do c = getc(file);
		while ((c != '\n') && (c != EOF));
		if (c == EOF) break;
	}

	t = tok;

	if (c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if (c == '#') ungetc(c, file);
	}

	*t = 0;

	return tok;
}


long int unsigned_char_to_bit(unsigned char* datauchar, unsigned char* databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char* p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
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
			if ((countbits > 8) || (x == width - 1))
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



void bit_to_unsigned_char(unsigned char* databit, unsigned char* datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char* p = databit;

	countbits = 1;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
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
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC* vc_read_image(char* filename)
{
	FILE* file = NULL;
	IVC* image = NULL;
	unsigned char* tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;

	// Abre o ficheiro
	if ((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if (strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if (strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if (strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
#endif

			fclose(file);
			return NULL;
		}

		if (levels == 1) // PBM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
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
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			size = image->width * image->height * image->channels;

			if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
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

int vc_write_image(char* filename, IVC* image)
{
	FILE* file = NULL;
	unsigned char* tmp;
	long int totalbytes, sizeofbinarydata;

	if (image == NULL) return 0;

	if ((file = fopen(filename, "wb")) != NULL)
	{
		if (image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
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

			if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
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

int vc_gray_negative(IVC* srcdst)
{

	if (srcdst->width < 0 || srcdst->height < 0 || (srcdst->levels > 255 && srcdst->levels < 0) || srcdst == NULL)
		return 0;
	if (srcdst->channels != 1)
		return 0;
	int x, y;
	long int pos;
	for (x = 0; x < srcdst->width; x++)
	{
		for (y = 0; y < srcdst->height; y++)
		{
			pos = y * srcdst->bytesperline + x * srcdst->channels;
			srcdst->data[pos] = 255 - srcdst->data[pos];
		}
	}
	if (vc_write_image("NegativeHsvPetNormal.pgm", srcdst) == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/// <summary>
/// Negativo de uma imagem rgb, apenas se subtrai 255 a cada componente de cor.
/// </summary>
/// <param name="srcdst"></param>
/// <returns></returns>
int vc_rgb_negative(IVC* srcdst)
{
	if (srcdst->width < 0 || srcdst->height < 0 || (srcdst->levels > 255 && srcdst->levels < 0) || srcdst == NULL)
		return 0;
	if (srcdst->channels != 3)
		return 0;
	int x, y;
	long int pos;


	for (x = 0; x < srcdst->width; x++)
	{
		for (y = 0; y < srcdst->height; y++)
		{
			pos = y * srcdst->bytesperline + x * srcdst->channels;
			srcdst->data[pos] = 255 - srcdst->data[pos];
			srcdst->data[pos + 1] = 255 - srcdst->data[pos + 1];
			srcdst->data[pos + 2] = 255 - srcdst->data[pos + 2];
		}
	}
	if (vc_write_image("testeRGBparaNegativo.ppm", srcdst) == 1)
	{
		vc_image_free(srcdst);
		return 1;
	}
	vc_image_free(srcdst);
	return 0;
}

int vc_rgb_get_red_gray(IVC* srcdst)
{
	if (srcdst->width < 0 || srcdst->height < 0 || (srcdst->levels > 255 && srcdst->levels < 0) || srcdst == NULL)
		return 0;
	if (srcdst->channels != 3)
		return 0;

	int x, y;
	long int pos;

	for (x = 0; x < srcdst->width; x++)
	{
		for (y = 0; y < srcdst->height; y++)
		{
			pos = y * srcdst->bytesperline + x * srcdst->channels;
			srcdst->data[pos + 1] = srcdst->data[pos]; //Componente verde fica com o valor da vermelha
			srcdst->data[pos + 2] = srcdst->data[pos]; //Componente azul fica com o valor da vermelha
		}
	}
	if (vc_write_image("testeRedIntensidadeCinza.ppm", srcdst) == 1)
	{
		vc_image_free(srcdst);
		return 1;
	}
	vc_image_free(srcdst);
	return 0;
}

/// <summary>
/// Converte uma imagem rgb para cinzento.
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
/// <returns></returns>
int vc_rgb_to_gray(IVC* src, IVC* dst)
{
	if (src->width < 0 || src->height < 0 || (src->levels > 255 && src->levels < 0) || src == NULL)
		return 0;
	if (src->channels != 3 || dst->channels != 1)
		return 0;
	int x, y;
	long int possrc, posdst;
	float r, g, b;
	for (x = 0; x < dst->width; x++)
	{
		for (y = 0; y < dst->height; y++)
		{
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;

			r = (float)src->data[possrc];
			g = (float)src->data[possrc + 1];
			b = (float)src->data[possrc + 2];



			dst->data[posdst] = (unsigned char)((r * 0.299) + (g * 0.587) + (b * 0.114));
			//Trabalha com as 3 cores, colocando todas com o mesmo valor criando o cinzento.
			//r = dst->data[pos]/* * 0.299*/;
			//g = dst->data[pos]/* * 0.587*/;
			//b = dst->data[pos]/* * 0.114*/;
			//dst->data[pos] = (r + g + b) ;
			//dst->data[pos+1] = (r + g + b) ;
			//dst->data[pos + 2] = (r + g + b);
		}
	}

	return 1;
}

int vc_rgb_to_hsv_aula(IVC* src, IVC* dst)
{
	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 3 || dst->channels != 3)
		return 0;

	int x, y;
	float matiz, sat, valor, max, min, r, g, b;
	long int possrc, posdst;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;

			max = min = src->data[possrc];

			r = src->data[possrc];
			g = src->data[possrc + 1];
			b = src->data[possrc + 2];
			max = MAX3(r, g, b);
			min = MIN3(r, g, b);


			valor = max;
			if (valor > 0 && (max - min) > 0)
				sat = 255.0f * (max - min) / valor;
			else
				sat = 0;

			if (sat == 0)
				matiz = 0;
			else
			{
				if ((max == src->data[possrc]) && (src->data[possrc + 1] > src->data[possrc + 2]))
					matiz = 60 * (src->data[possrc + 1] - src->data[possrc + 2]) / (max - min);
				else if (max == src->data[possrc] && src->data[possrc + 2] > src->data[possrc + 1])
					matiz = 360 + 60 * (src->data[possrc + 1] - src->data[possrc + 2]) / (max - min);
				else if (max == src->data[possrc + 1])
					matiz = 120 + 60 * (src->data[possrc + 2] - src->data[possrc]) / (max - min);
				else if (max == src->data[possrc + 2])
					matiz = 240 + 60 * (src->data[possrc] - src->data[possrc + 1]) / (max - min);
				else
					matiz = 0;
			}
			dst->data[posdst] = (int)(matiz * 255.0f) / 360;
			dst->data[posdst + 1] = (int)sat;
			dst->data[posdst + 2] = (int)valor;
		}
	}

	//if (vc_write_image("petNormalHsv.ppm", dst) == 1)
	//{
	//	vc_image_free(src);
	//	return 1;
	//}

	//vc_image_free(src);

	return 0;
}


int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{
	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 3 || dst->channels != 1)
		return 0;

	int x, y;
	long int possrc, posdst;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;

			if ((src->data[possrc] >= hmin && src->data[possrc] <= hmax) &&
				(src->data[possrc + 1] >= smin && src->data[possrc + 1] <= smax) &&
				(src->data[possrc + 2] >= vmin && src->data[possrc + 2] <= vmax))
			{
				dst->data[posdst] = 255;
			}
			else
			{
				dst->data[posdst] = 0;
			}
		}
	}

	return 1;
}

int vc_scale_gray_to_color_palette(IVC* src, IVC* dst)
{
	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 3)
		return 0;

	int x, y;
	long int possrc, posdst;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;

			if (src->data[possrc] <= 63)
			{
				dst->data[posdst] = 0;
				dst->data[posdst + 1] = (src->data[possrc] * 4);
				dst->data[posdst + 2] = 255;
			}
			else if (src->data[possrc] <= 127)
			{
				dst->data[posdst] = 0;
				dst->data[posdst + 1] = 255;
				dst->data[posdst + 2] = 255 - (src->data[possrc] - 64) * 4;
			}
			else if (src->data[possrc] <= 191)
			{
				dst->data[posdst] = (src->data[possrc] - 128) * 4;
				dst->data[posdst + 1] = 255;
				dst->data[posdst + 2] = 0;
			}
			else
			{
				dst->data[posdst] = 255;
				dst->data[posdst + 1] = 255 - (src->data[possrc] - 192) * 4;
				dst->data[posdst + 2] = 0;
			}
		}

	}
	if (vc_write_image("scale_gray_to_color_palette.ppm", dst))
	{
		vc_image_free(src);
		vc_image_free(dst);
		return 1;
	}
	vc_image_free(src);
	vc_image_free(dst);
	return 0;
}

int histograma_teste(IVC* src)
{

	//Imagem com 1 canal;

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1)
		return 0;

	int x, y;
	long int possrc;
	//De 50 em 50 niveis at� 255 
	int numLess50, numBetween50100, numBetween100150, numBetween150200, above200;
	numLess50 = numBetween50100 = numBetween100150 = numBetween150200 = above200 = 0;
	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			possrc = y * src->bytesperline + x * src->channels;
			if (src->data[possrc] < 50)
			{
				numLess50++;
			}
			else if (src->data[possrc] < 100)
			{
				numBetween50100++;
			}
			else if (src->data[possrc] < 150)
			{
				numBetween100150++;
			}
			else if (src->data[possrc] < 200)
			{
				numBetween150200++;
			}
			else
			{
				above200++;
			}
		}
	}

	printf("Total pixels:%d\n", (src->width * src->height));
	printf("Less50:%d\nBetween 50 and 100:%d\nBetween 100 and 150:%d\nBetween 150 and 200:%d\nAbove 200:%d", numLess50,
		numBetween50100, numBetween100150, numBetween150200, above200);
	printf("Soma de todos:%d\n", (numLess50 + numBetween50100 + numBetween100150 + numBetween150200 + above200));


	vc_image_free(src);
	return 1;
}


int Count_White_Pixels_Gray(IVC* src) {

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1)
		return 0;

	int x, y, countPixel = 0;
	long int pos;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			if (src->data[pos] > 0)
				countPixel++;
		}
	}

	return countPixel;
}


int Count_Black_Pixels_Gray(IVC* src) {

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1)
		return 0;

	int x, y, countPixel = 0;
	long int pos;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;
			if (src->data[pos] == 0)
				countPixel++;
		}
	}
	return countPixel;
}

float percent_pixels_brain(float areaCerb, float numbPixels)
{
	float percentPixels = (numbPixels * 100.0f) / areaCerb;

	return percentPixels;
}

int vc_gray_to_binary(IVC* src, IVC* dst, int threshold)
{

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y;
	long int pos;

	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			pos = y * src->bytesperline + x * src->channels;

			if (src->data[pos] > threshold)
			{
				dst->data[pos] = 255;
			}
			else
			{
				dst->data[pos] = 0;
			}
		}
	}

	return 1;
}

//Somat�rio do brilho todo da imagem, depois � feito a divis�o pela resolu��o para obter a m�dia de brilho.
int vc_gray_to_binary_global_mean(IVC* src, IVC* dst)
{

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y, t, brilho = 0;
	long int pos;

	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			pos = y * src->bytesperline + x * src->channels;
			brilho += src->data[pos];
		}
	}
	t = brilho / (src->width * src->height);
	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			pos = y * src->bytesperline + x * src->channels;
			if (src->data[pos] > t)
			{
				dst->data[pos] = 255;
			}
			else
				dst->data[pos] = 0;
		}
	}
	return 1;
}

int vc_gray_to_binary_mid_point(IVC* src, IVC* dst, int N)
{

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y, T, vmin, vmax, v;
	long int posx, posy, posViz, pos;
	int tempx, tempy;

	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			vmin = 255;
			vmax = 0;

			for (tempx = -(N / 2); tempx <= N / 2; tempx++)
			{
				for (tempy = -(N / 2); tempy <= N / 2; tempy++)
				{
					//asdsa maior que 0 e menor que a largura.
					if (y + tempy < 0 || y + tempy >= src->height || x + tempx < 0 || x + tempx >= src->width)
						continue;
					posx = (x + tempx) * src->channels;
					posy = (y + tempy) * src->bytesperline;
					pos = posx + posy;
					if (src->data[pos] > vmax)
						vmax = src->data[pos];
					if (src->data[pos] < vmin)
						vmin = src->data[pos];
				}
			}
			T = (vmin + vmax) / 2;
			posy = y * src->bytesperline;
			posx = x * src->channels;
			pos = posy + posx;
			if (T > src->data[pos])
			{
				dst->data[pos] = 255;
			}
			else
				dst->data[pos] = 0;
		}
	}
	return 1;
}

int vc_gray_to_binary_mid_point_cMin(IVC* src, IVC* dst, int N)
{

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y, T, vmin, vmax, v;
	long int posx, posy, posViz, pos, cMin = 15;
	int tempx, tempy;

	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			vmin = 255;
			vmax = 0;

			for (tempx = -(N / 2); tempx <= N / 2; tempx++)
			{
				for (tempy = -(N / 2); tempy <= N / 2; tempy++)
				{
					//asdsa maior que 0 e menor que a largura.
					if (y + tempy < 0 || y + tempy >= src->height || x + tempx < 0 || x + tempx >= src->width)
						continue;
					posx = (x + tempx) * src->channels;
					posy = (y + tempy) * src->bytesperline;
					pos = posx + posy;
					if (src->data[pos] > vmax)
						vmax = src->data[pos];
					if (src->data[pos] < vmin)
						vmin = src->data[pos];
				}
			}
			if ((vmax - vmin) < cMin)
				T = src->levels / 2;
			else
				T = (vmin + vmax) / 2;
			posy = y * src->bytesperline;
			posx = x * src->channels;
			pos = posy + posx;
			if (T > src->data[pos])
			{
				dst->data[pos] = 255;
			}
			else
				dst->data[pos] = 0;
		}
	}
	return 1;
}

int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int N)
{

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y, T, vmin, vmax, v, aux = 0, tempx, tempy;
	long int posx, posy, posViz, pos, brilho = 0, media = 0, valor = 0, nVizs = 0;
	float k = -0.2, desvioPadrao, valorTotal = 0;
	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			valorTotal = 0;
			nVizs = 0;
			T = 0;
			media = 0;
			brilho = 0;
			for (tempx = -(N / 2); tempx <= N / 2; tempx++)
			{
				for (tempy = -(N / 2); tempy <= N / 2; tempy++)
				{
					if (y + tempy < 0 || y + tempy >= src->height || x + tempx < 0 || x + tempx >= src->width)
						continue;
					posy = (y + tempy) * src->bytesperline;
					posx = (x + tempx) * src->channels;
					pos = posy + posx;
					brilho += src->data[pos];
				}
			}
			media = brilho / (N * N - 1);
			for (tempx = -(N / 2); tempx <= N / 2; tempx++)
			{
				for (tempy = -(N / 2); tempy <= N / 2; tempy++)
				{
					if (y + tempy < 0 || y + tempy >= src->height || x + tempx < 0 || x + tempx >= src->width)
						continue;

					posy = (y + tempy) * src->bytesperline;
					posx = (x + tempx) * src->channels;
					pos = posy + posx;
					aux = src->data[pos];
					valorTotal += (aux - media) * (aux - media);
					nVizs++;
				}
			}
			desvioPadrao = sqrt((valorTotal / nVizs - 1));
			T = media + k * desvioPadrao;
			posy = y * src->bytesperline;
			posx = x * src->channels;
			pos = posy + posx;
			if (T > src->data[pos])
			{
				dst->data[pos] = 255;
			}
			else
				dst->data[pos] = 0;
		}
	}
	return 1;
}

int vc_binary_dilate(IVC* src, IVC* dst, int kernel)
{

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y, tempx, tempy, posx, posy, teste = 0;
	long int possrc, posdst, pos;

	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;
			for (tempx = -(kernel / 2); tempx <= kernel / 2; tempx++)
			{
				for (tempy = -(kernel / 2); tempy <= kernel / 2; tempy++)
				{
					teste = 0;
					//asdsa maior que 0 e menor que a largura.
					if (y + tempy < 0 || y + tempy >= src->height || x + tempx < 0 || x + tempx >= src->width)
						continue;
					posx = (x + tempx) * src->channels;
					posy = (y + tempy) * src->bytesperline;
					pos = posx + posy;
					if (src->data[pos] == 255)
					{
						dst->data[posdst] = 255;
						teste = 1;
						break;
					}
				}
				if (teste == 1)
					break;
			}
			if (teste == 0)
			{
				dst->data[posdst] = 0;
			}
		}
	}
	return 1;
}

int vc_binary_erode(IVC* src, IVC* dst, int kernel)
{

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y, tempx, tempy, posx, posy, teste = 0;
	long int possrc, posdst, pos;

	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;
			for (tempx = -(kernel / 2); tempx <= kernel / 2; tempx++)
			{
				for (tempy = -(kernel / 2); tempy <= kernel / 2; tempy++)
				{
					teste = 0;
					//asdsa maior que 0 e menor que a largura.
					if (y + tempy < 0 || y + tempy >= src->height || x + tempx < 0 || x + tempx >= src->width)
						continue;
					posx = (x + tempx) * src->channels;
					posy = (y + tempy) * src->bytesperline;
					pos = posx + posy;
					if (src->data[pos] == 0)
					{
						dst->data[posdst] = 0;
						teste = 1;
						break;
					}
				}
				if (teste == 1)
					break;
			}
			if (teste == 0)
			{
				dst->data[posdst] = 255;
			}
		}
	}
	return 1;
}

int vc_binary_open(IVC* src, IVC* dst, int kernelErode, int kernelDilate)
{

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	IVC* tempImage;
	tempImage = vc_image_new(src->width, src->height, 1, src->levels);
	vc_binary_erode(src, tempImage, kernelErode);
	vc_binary_dilate(tempImage, dst, kernelDilate);
	vc_image_free(tempImage);
	return 1;
}


int vc_binary_close(IVC* src, IVC* dst, int kernelDilate, int kernelErode)
{

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	IVC* tempImage;
	tempImage = vc_image_new(src->width, src->height, 1, src->levels);
	vc_binary_dilate(src, tempImage, kernelDilate);
	vc_binary_erode(tempImage, dst, kernelErode);
	vc_image_free(tempImage);
	return 1;
}

int vc_subtract_image(IVC* src, IVC* srcSub, IVC* dst)
{
	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y;
	long int possrc, posdst;

	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;

			dst->data[posdst] = src->data[possrc] - srcSub->data[possrc];
		}
	}
	return 1;
}

int vc_copy_src_to_dst(IVC* src, IVC* dst)
{
	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y;
	long int possrc, posdst;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;

			if (dst->data[posdst] == 255)
				continue;
			dst->data[possrc] = src->data[possrc];
		}
	}
	return 1;
}


int vc_copy_src_to_dst_rgb(IVC* src, IVC* dst)
{
	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 3 || dst->channels != 3)
		return 0;

	int x, y;
	long int possrc, posdst;

	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			possrc = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;

			dst->data[possrc] = src->data[possrc];
			dst->data[possrc + 1] = src->data[possrc + 1];
			dst->data[possrc + 2] = src->data[possrc + 2];
		}
	}
	return 1;
}

// Usada agora a fun��o do professor.
//int vc_binary_blob_labelling(IVC* src, IVC* dst)
//{
//	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
//		return 0;
//	if (src->channels != 1 || dst->channels != 1)
//		return 0;
//
//	unsigned char etiqueta = 1;
//	
//	int x, y, tempx, tempy, posx, posy;
//	long int possrc, posdst,posPixA, posPixB, posPixC, posPixD, pos;
//
//	for (y = 0; y < src->height; y++)
//	{
//		for (x = 0; x < src->width; x++)
//		{
//			posPixA = (y - 1) * src->bytesperline + (x - 1) * src->channels;
//			posPixB = (y - 1) * src->bytesperline + (x)*src->channels;
//			posPixC = (y - 1) * src->bytesperline + (x + 1) * src->channels;
//			posPixD = (y)*src->bytesperline + (x - 1) * src->channels;
//			possrc = y * src->bytesperline + x * src->channels;
//			if (src->data[possrc] == 255)
//			{
//				if (posPixA < 0 || posPixB < 0 || posPixC < 0 || posPixD <0)
//					continue;
//				//printf("A: %d, B: %d, C: %d, D: %d, X: %d\n", src->data[posPixA], src->data[posPixB], src->data[posPixC], src->data[posPixD], src->data[possrc]);
//				if (src->data[posPixA] == 0 && src->data[posPixB] == 0 && src->data[posPixC] == 0 && src->data[posPixD] == 0)
//				{
//					dst->data[possrc] = etiqueta;
//					etiqueta++;
//				}
//				else
//				{
//					int min = 255;
//					if (dst->data[posPixA] < min && dst->data[posPixA] != 0)
//						min = dst->data[posPixA];
//					if (dst->data[posPixB] < min && dst->data[posPixB] != 0)
//						min = dst->data[posPixB];
//					if (dst->data[posPixC] < min && dst->data[posPixC] != 0)
//						min = dst->data[posPixC];
//					if (dst->data[posPixD] < min && dst->data[posPixD] != 0)
//						min = dst->data[posPixD];
//					
//					dst->data[possrc] = min;
//				}
//			}
//			else
//				dst->data[possrc] = 0;
//		}
//	}
//	return 1;
//}

// Etiquetagem de blobs
// src		: Imagem bin�ria de entrada
// dst		: Imagem grayscale (ir� conter as etiquetas)
// nlabels	: Endere�o de mem�ria de uma vari�vel, onde ser� armazenado o n�mero de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. � necess�rio libertar posteriormente esta mem�ria.
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
	OVC* blobs; // Apontador para array de blobs (objectos) que ser� retornado desta fun��o.

	// Verifica��o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	// Copia dados da imagem bin�ria para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pix�is de plano de fundo devem obrigat�riamente ter valor 0
	// Todos os pix�is de primeiro plano devem obrigat�riamente ter valor 255
	// Ser�o atribu�das etiquetas no intervalo [1,254]
	// Este algoritmo est� assim limitado a 254 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	// Limpa os rebordos da imagem bin�ria
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

					// Se A est� marcado
					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					// Se B est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					// Se C est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					// Se D est� marcado, e � menor que a etiqueta "num"
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

	// Contagem do n�mero de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que n�o hajam valores vazios (zero) entre etiquetas
	if (!*nlabels > 0)
		*nlabels = 0;

	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++; // Conta etiquetas
		}
	}

	// Se n�o h� blobs
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

	// Verifica��o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Conta �rea de cada blob
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
					// �rea
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					// Per�metro
					// Se pelo menos um dos quatro vizinhos n�o pertence ao mesmo label, ent�o � um pixel de contorno
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
		blobs[i].fimX = (xmin)+(xmax - xmin) + 1;
		blobs[i].fimY = (ymin)+(ymax - ymin) + 1;
		// Centro de Gravidade
		//blobs[i].xc = (xmax - xmin) / 2;
		//blobs[i].yc = (ymax - ymin) / 2;
		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}

	return 1;
}

int vc_draw_bounding_box(IVC* src, IVC* dst, OVC* blobs/*, int firstIteration*/)
{
	if (src->height < 0 || src->width < 0 || (src->levels < 0 || src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y;
	long int posy, posx, posTotal;

	//if(!firstIteration)
		//memcpy(dst->data, src->data, src->bytesperline * src->height);

	for (x = blobs->x; x < blobs->width + blobs->x; x++)
	{
		for (y = blobs->y; y < blobs->height + blobs->y; y++)
		{
			posTotal = y * src->bytesperline + x * src->channels;
			if (x == blobs->x || x == blobs->x + blobs->width - 1 ||
				y == blobs->y || y == blobs->y + blobs->height - 1)
			{
				dst->data[posTotal] = 255;
			}
		}
	}

	return 1;
	//// Bounding Box
	//blobs[i].x = xmin;
	//blobs[i].y = ymin;
	//blobs[i].width = (xmax - xmin) + 1;
	//blobs[i].height = (ymax - ymin) + 1;

	//// Centro de Gravidade
	////blobs[i].xc = (xmax - xmin) / 2;
	////blobs[i].yc = (ymax - ymin) / 2;
	//blobs[i].xc = sumx / MAX(blobs[i].area, 1);
	//blobs[i].yc = sumy / MAX(blobs[i].area, 1);

}


int vc_draw_bounding_box_rgb(IVC* src, IVC* dst, OVC* blobs)
{
	if (src->height < 0 || src->width < 0 || (src->levels < 0 || src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 3)
		return 0;

	int x, y;
	long int posy, posx, posTotal, posdst;

	for (x = blobs->x; x < blobs->width + blobs->x; x++)
	{
		for (y = blobs->y; y < blobs->height + blobs->y; y++)
		{
			if (x < 0 || x >= src->width || y < 0 || y >= src->height) {
				continue;
			}
			posTotal = y * src->bytesperline + x * src->channels;
			posdst = y * dst->bytesperline + x * dst->channels;
			if (x == blobs->x || x == blobs->x + blobs->width - 1 ||
				y == blobs->y || y == blobs->y + blobs->height - 1)
			{
				dst->data[posdst] = 255;
				dst->data[posdst + 1] = 255;
				dst->data[posdst + 2] = 255;
			}
		}
	}

	int xc = blobs->xc;
	int yc = blobs->yc;

	if (xc >= 0 && xc < src->width && yc >= 0 && yc < src->height)
	{
		posdst = yc * dst->bytesperline + xc * dst->channels;
		dst->data[posdst] = 0;
		dst->data[posdst + 1] = 0;
		dst->data[posdst + 2] = 0;
	}


	return 1;
	//// Bounding Box
	//blobs[i].x = xmin;
	//blobs[i].y = ymin;
	//blobs[i].width = (xmax - xmin) + 1;
	//blobs[i].height = (ymax - ymin) + 1;

	//// Centro de Gravidade
	////blobs[i].xc = (xmax - xmin) / 2;
	////blobs[i].yc = (ymax - ymin) / 2;
	//blobs[i].xc = sumx / MAX(blobs[i].area, 1);
	//blobs[i].yc = sumy / MAX(blobs[i].area, 1);

}



int vc_gray_histogram_show(IVC* src, IVC* dst)
{
	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y;
	long int pos;
	int ni[256] = { 0 };

	for (int x = 0; x < src->width; x++)
	{
		for (int y = 0; y < src->height; y++)
		{
			pos = y * src->bytesperline + x * src->channels;
			unsigned char brilho = src->data[pos];
			ni[brilho] = ni[brilho] + 1;
		}
	}

	float pdf[256];
	int n = src->width * src->height;

	for (int i = 0; i < 256; i++)
	{
		pdf[i] = (float)ni[i] / (float)n;
		printf("Nivel:%d pdf:%f\n", i, pdf[i]);
	}

	float pdfMax = 0;
	for (int i = 0; i < 256; i++)
	{
		if (pdf[i] > pdfMax)
			pdfMax = pdf[i];
	}
	printf("%f", pdfMax);

	float pdfNorm[256];
	for (int i = 0; i < 256; i++)
	{
		pdfNorm[i] = pdf[i] / pdfMax;
	}

	for (int i = 0; i < dst->width * dst->height; i++)
	{
		dst->data[i] = 0;
	}
	for (int x = 0; x < 256; x++) {
		for (y = (256 - 1); y >= (256 - 1) - pdfNorm[x] * 255; y--) {
			dst->data[y * 256 + x] = 255;
		}
	}
	return 1;
}


int vc_gray_histogram_equalization(IVC* src, IVC* dst)
{

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y;
	long int pos;

	int ni[256] = { 0 };

	for (int x = 0; x < src->width; x++)
	{
		for (int y = 0; y < src->height; y++)
		{
			pos = y * src->bytesperline + x * src->channels;
			unsigned char brilho = src->data[pos];
			ni[brilho] = ni[brilho] + 1;
		}
	}



	float pdf[256];
	int n = src->width * src->height;

	for (int i = 0; i < 256; i++)
	{
		pdf[i] = (float)ni[i] / (float)n;
		printf("Nivel:%d pdf:%f\n", i, pdf[i]);
	}

	float cdf[256];

	for (int i = 0; i < 256; i++)
	{
		if (i == 0)
			cdf[i] = pdf[i];
		else
			cdf[i] = cdf[i - 1] + pdf[i];
	}

	float cdfMin = 1;

	for (int i = 0; i < 256; i++)
	{
		if (cdf[i] != 0)
		{
			cdfMin = cdf[i];
			break;
		}
	}

	for (int x = 0; x < src->width; x++)
	{
		for (int y = 0; y < src->height; y++)
		{
			pos = y * src->bytesperline + x * src->channels;
			unsigned char brilho = src->data[pos];
			dst->data[pos] = (unsigned char)((cdf[brilho] - cdfMin) / (1.0f - cdfMin) * (src->levels - 1));
		}
	}
	return 1;
}


int vc_hsv_histogram_equalization(IVC* src, IVC* dst)
{
	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 3 || dst->channels != 3)
		return 0;

	int x, y;
	long int pos;

	int ni[256] = { 0 };


	for (int x = 0; x < src->width; x++)
	{
		for (int y = 0; y < src->height; y++)
		{
			pos = y * src->bytesperline + x * src->channels;
			unsigned char brilho = src->data[pos];
			unsigned char brilho2 = src->data[pos + 1];
			unsigned char brilho3 = src->data[pos + 2];
			ni[brilho] = ni[brilho] + 1;
			ni[brilho2] = ni[brilho2] + 1;
			ni[brilho3] = ni[brilho3] + 1;
		}
	}

	return 1;
}

int vc_gray_edge_prewitt(IVC* src, IVC* dst, float thold)
{

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int x, y, primeiraDerX, primeiraDerY, mascaraX, mascaraY, magnVector;
	long int pos, tempPos, tempPosX, tempPosY, c = 1;



	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			pos = y * src->bytesperline + x * src->channels;
			for (int tempx = -1; tempx <= 1; tempx++)
			{
				for (int tempy = -1; tempy <= 1; tempy++)
				{
					if (y + tempy < 0 || y + tempy >= src->height || x + tempx < 0 || x + tempx >= src->width)
						continue;
					tempPosX = (tempx + x) * src->channels;
					tempPosY = (tempy + y) * src->bytesperline;

					tempPos = tempPosX + tempPosY;

					mascaraX = ((src->data[tempPos - src->bytesperline - 2] + (c * src->data[tempPos + src->bytesperline + 1])) + src->data[tempPos + src->bytesperline + 4]) - ((src->data[tempPos - src->bytesperline - 4] + (c * src->data[tempPos - src->bytesperline - 1]) + src->data[tempPos + src->bytesperline + 2]));
					mascaraY = ((src->data[tempPos + src->bytesperline + 2] + (c * src->data[tempPos + src->bytesperline + 3]) + src->data[tempPos + src->bytesperline + 4]) - (src->data[tempPos - src->bytesperline - 4] + (c * src->data[tempPos - src->bytesperline - 3]) + src->data[tempPos - src->bytesperline - 2]));
					//mascaraX = mascaraX /3;
					//mascaraY = mascaraY /3;
					primeiraDerX = src->data[tempPos] * mascaraX;
					primeiraDerY = src->data[tempPos] * mascaraY;

					magnVector = sqrt(pow((double)primeiraDerX, 2) + (pow((double)primeiraDerY, 2)));

					//magnVector = (magnVector * 255) / 100;
					//printf("magnVector: %d\n", magnVector);
					if (magnVector > thold)
						dst->data[pos] = 255;
					else
						dst->data[pos] = 0;
				}
			}
		}
	}
	return 1;
}
int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernelsize)
{
	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;


	int x, y, media = 0;
	long int pos, tempPos, tempPosX, tempPosY, resFinal = 0, mascaraX = 0, mascaraY = 0, c = 1;
	int div = kernelsize * kernelsize;

	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			pos = y * src->bytesperline + x * src->channels;
			media = 0;
			for (int tempx = -(kernelsize / 2); tempx <= (kernelsize / 2); tempx++)
			{
				for (int tempy = -(kernelsize / 2); tempy <= (kernelsize / 2); tempy++)
				{
					if (y + tempy < 0 || y + tempy >= src->height || x + tempx < 0 || x + tempx >= src->width)
						continue;
					tempPosX = (tempx + x) * src->channels;
					tempPosY = (tempy + y) * src->bytesperline;

					tempPos = tempPosX + tempPosY;


					media += src->data[tempPos];
				}
			}
			if (media != 0)
			{
				resFinal = (media / div);
				dst->data[pos] = resFinal;
			}
		}
	}
	return 1;
}

int vc_gray_lowpass_median_filter(IVC* src, IVC* dst, int kernelsize)
{

	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;


	int x, y;
	long int pos, tempPos, tempPosX, tempPosY, resFinal = 0;
	int arraySize = kernelsize * kernelsize;
	int i;
	int* array = (int*)malloc(arraySize * sizeof(int));
	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			pos = y * src->bytesperline + x * src->channels;
			i = 0;
			for (int tempx = -(kernelsize / 2); tempx <= (kernelsize / 2); tempx++)
			{
				for (int tempy = -(kernelsize / 2); tempy <= (kernelsize / 2); tempy++)
				{
					if (y + tempy < 0 || y + tempy >= src->height || x + tempx < 0 || x + tempx >= src->width)
					{
						array[i] = 0;
						i++;
						continue;
					}
					tempPosX = (tempx + x) * src->channels;
					tempPosY = (tempy + y) * src->bytesperline;

					tempPos = tempPosX + tempPosY;

					array[i] = src->data[tempPos];
					i++;
				}
			}
			for (int z = 0; z < arraySize - 1; z++)
			{
				for (int h = z + 1; h < arraySize; h++)
				{
					if (array[z] > array[h])
					{
						int tempArray = array[h];
						array[h] = array[z];
						array[z] = tempArray;
					}
				}
			}
			int posArray = (arraySize / 2);

			dst->data[pos] = (unsigned char)array[posArray];
		}
	}
	free(array);
	return 1;
}
int vc_table_resistors_value(char* stringCor)
{
	if (stringCor != NULL)
	{
		if (strcmp(stringCor, "Preto") == 0)
			return 0;
		else if (strcmp(stringCor, "Castanho") == 0)
			return 1;
		else if (strcmp(stringCor, "Vermelha") == 0)
			return 2;
		else if (strcmp(stringCor, "Laranja") == 0)
			return 3;
		else if (strcmp(stringCor, "Amarelo") == 0)
			return 4;
		else if (strcmp(stringCor, "Verde") == 0)
			return 5;
		else if (strcmp(stringCor, "Azul") == 0)
			return 6;
		else if (strcmp(stringCor, "Roxo") == 0)
			return 7;
		else if (strcmp(stringCor, "Cinzento") == 0)
			return 8;
		else if (strcmp(stringCor, "Branco") == 0)
			return 9;
		else if (strcmp(stringCor, "Dourado") == 0)
			return 10;
	}
	return -1;
}

int vc_table_resistors_multiplier(int mult)
{
	if (mult != NULL)
	{
		if (mult == 0)
			return 1;
		if (mult == 1)
			return 10;
		if (mult == 2)
			return 100;
		if (mult == 3)
			return 1000;
		if (mult == 4)
			return 10000;
		if (mult == 5)
			return 100000;
		if (mult == 6)
			return 1000000;
		else
			return 0;
	}
	return -1;
}

void appendDigit(int* num, int digit)
{
	*num = (*num * 10) + digit;


}

int compare(const void* a, const void* b) {
	OVC* resA = *(OVC**)a;
	OVC* resB = *(OVC**)b;
	return (resA->x - resB->x);
}

int vc_saturate_gold(IVC* src, IVC* dst)
{
	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 3 || dst->channels != 3)
		return 0;

	int x, y;
	long int pos;


	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			pos = y * src->bytesperline + x * src->channels;

			dst->data[pos] = 255;
			dst->data[pos + 1] = 215;
			dst->data[pos + 2] = 0;
		}
	}
	return 1;
}


int vc_convert_bgr_to_rgb(IVC* src, IVC* dst)
{
	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 3 || dst->channels != 3)
		return 0;

	int x, y;
	long int pos;


	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;

			dst->data[pos] = src->data[pos + 2];
			dst->data[pos + 1] = src->data[pos + 1];
			dst->data[pos + 2] = src->data[pos];
		}
	}
	return 1;
}

int vc_convert_rgb_to_bgr(IVC* src, IVC* dst)
{
	if (src->height < 0 || src->width < 0 || (src->levels < 0 && src->levels>255))
		return 0;
	if (src->channels != 3 || dst->channels != 3)
		return 0;

	int x, y;
	long int pos;


	for (x = 0; x < src->width; x++)
	{
		for (y = 0; y < src->height; y++)
		{
			pos = y * src->bytesperline + x * src->channels;

			dst->data[pos + 2] = src->data[pos];
			dst->data[pos + 1] = src->data[pos + 1];
			dst->data[pos] = src->data[pos + 2];
		}
	}
	return 1;
}
