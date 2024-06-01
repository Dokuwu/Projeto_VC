#define CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <string>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>
extern "C" {
#include "vc.h"
}


int main(void) {
	// V�deo
	char videofile[20] = "video_resistors.mp4";
	cv::VideoCapture capture;
	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;
	// Outros
	std::string str;
	int key = 0;

	/* Leitura de v�deo de um ficheiro */
	/* NOTA IMPORTANTE:
	O ficheiro video.avi dever� estar localizado no mesmo direct�rio que o ficheiro de c�digo fonte.
	*/
	capture.open(videofile);

	/* Em alternativa, abrir captura de v�deo pela Webcam #0 */
	//capture.open(0, cv::CAP_DSHOW); // Pode-se utilizar apenas capture.open(0);

	/* Verifica se foi poss�vel abrir o ficheiro de v�deo */
	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de v�deo!\n";
		return 1;
	}

	/* N�mero total de frames no v�deo */
	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	/* Frame rate do v�deo */
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	/* Resolu��o do v�deo */
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);
	/* Cria uma janela para exibir o v�deo */
	cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

	cv::Mat frame;

	IVC* images[12] = { NULL };
	IVC* dilatarImagem = NULL, * imagemFinal = NULL, * imagemFinalAux = NULL, * imageVerde = NULL, * imageAzul = NULL, * imagePreto = NULL,
		* imageVermelho = NULL, * imageCastanho = NULL, * imagemCoresFinal = NULL, * auxBlobSegmentation = NULL, * imageLaranja = NULL,
		* imagemBoundingBox = NULL, * resistenciasJuntas = NULL;

	OVC* arrayVerde[6] = { NULL }, * arrayPreto[6] = { NULL }, * arrayVermelho[6] = { NULL }, * arrayAzul[6] = { NULL }, * arrayCastanho[6] = { NULL }, * arrayResistencia[6] = { NULL }, * arrayLaranja[6] = { NULL };
	OVC* blobSegmentation = NULL, * blobVerde = NULL, * blobPreto = NULL, * blobVermelho = NULL, * blobAzul = NULL, * blobCastanho = NULL, * blobCoresJuntas = NULL, * blobLaranja = NULL;

	while (key != 'q') {
		/* Leitura de uma frame do v�deo */
		capture.read(frame);
		int iteradorAzul, iteradorVermelho, iteradorCastanho, iteradorPreto, iteradorVerde, iteradorLaranja;
		int nBlobsSegmentation, nBlobsSegVerde, nBlobsSegPreto, nBlobsSegVermelho, nBlobsSegAzul, nBlobsSegCastanho;
		/* Verifica se conseguiu ler a frame */
		if (frame.empty()) break;

		/* N�mero da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		/* Exemplo de inser��o texto na frame */


		// Cria uma nova imagem IVC
		IVC* image = vc_image_new(video.width, video.height, 3, 255);
		// Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
		memcpy(image->data, frame.data, video.width * video.height * 3);
		// Executa uma fun��o da nossa biblioteca vc
		IVC* imageRGB = vc_image_new(image->width, image->height, 3, image->levels);

		//Converter imagem de BGR para RGB, porque o opencv le em BGR.
		vc_convert_bgr_to_rgb(image, imageRGB);

		imagemFinal = vc_image_new(image->width, image->height, 1, image->levels);
		images[0] = vc_image_new(image->width, image->height, 3, image->levels);
		vc_rgb_to_hsv_aula(imageRGB, images[0]);
		resistenciasJuntas = vc_image_new(image->width, image->height, 1, image->levels);


		//Obter corpo resistencia
		images[1] = vc_image_new(image->width, image->height, 1, image->levels);
		//Parte clara
		vc_hsv_segmentation(images[0], images[1], (29 * 255) / 360, (46 * 255) / 360, (31 * 255) / 100, (62 * 255) / 100, (54 * 255) / 100, (91 * 255) / 100);
		vc_copy_src_to_dst(images[1], imagemFinal);

		//Parte escura.
		vc_hsv_segmentation(images[0], images[1], (29 * 255) / 360, (38 * 255) / 360, (33 * 255) / 100, (46 * 255) / 100, (46 * 255) / 100, (56 * 255) / 100);
		vc_copy_src_to_dst(images[1], imagemFinal);

		//Obter Faixa Verde
		images[2] = vc_image_new(image->width, image->height, 1, image->levels);
		vc_hsv_segmentation(images[0], images[2], (79 * 255) / 360, (105 * 255) / 360, (28 * 255) / 100, (45 * 255) / 100, (35 * 255) / 100, (50 * 255) / 100);

		nBlobsSegVerde = 0;
		imageVerde = vc_image_new(image->width, image->height, 1, image->levels);
		blobVerde = vc_binary_blob_labelling(images[2], imageVerde, &nBlobsSegVerde);
		iteradorVerde = 0;
		if (blobVerde != NULL)
		{
			vc_binary_blob_info(imageVerde, blobVerde, nBlobsSegVerde);
			for (int i = 0; i < nBlobsSegVerde; i++)
			{
				blobVerde[i].valorCor = 5;
				if (blobVerde[i].area >= 125 && blobVerde[i].area <= 1000)
				{
					arrayVerde[iteradorVerde] = (OVC*)malloc(sizeof(OVC));
					memcpy(arrayVerde[iteradorVerde], &blobVerde[i], sizeof(OVC));
					iteradorVerde++;
				}
			}
		}

		//Copiar para imagens auxiliares.
		vc_copy_src_to_dst(images[2], imagemFinal);
		vc_copy_src_to_dst(images[2], resistenciasJuntas);

		//Obter Faixa Azul
		images[3] = vc_image_new(image->width, image->height, 1, image->levels);
		vc_hsv_segmentation(images[0], images[3], (155 * 255) / 360, (200 * 255) / 360, (16 * 255) / 100, (40 * 255) / 100, (36 * 255) / 100, (52 * 255) / 100);
		nBlobsSegAzul = 0;
		imageAzul = vc_image_new(image->width, image->height, 1, image->levels);
		blobAzul = vc_binary_blob_labelling(images[3], imageAzul, &nBlobsSegAzul);
		iteradorAzul = 0;
		if (blobAzul != NULL)
		{
			vc_binary_blob_info(imageAzul, blobAzul, nBlobsSegAzul);
			for (int i = 0; i < nBlobsSegAzul; i++)
			{
				blobAzul[i].valorCor = 6;
				if (blobAzul[i].area >= 90 && blobAzul[i].area <= 1000)
				{
					arrayAzul[iteradorAzul] = (OVC*)malloc(sizeof(OVC));
					memcpy(arrayAzul[iteradorAzul], &blobAzul[i], sizeof(OVC));
					iteradorAzul++;
				}
			}
		}

		//Copiar para imagens auxiliares.
		vc_copy_src_to_dst(images[3], imagemFinal);
		vc_copy_src_to_dst(images[3], resistenciasJuntas);


		//Obter Faixa Vermelha
		IVC* auxVermelho = vc_image_new(image->width, image->height, 1, image->levels);
		images[4] = vc_image_new(image->width, image->height, 1, image->levels);
		vc_hsv_segmentation(images[0], images[4], (0 * 255) / 360, (11 * 255) / 360, (45 * 255) / 100, (69 * 255) / 100, (55 * 255) / 100, (89 * 255) / 100);

		//Copiar para imagens auxiliares.
		vc_copy_src_to_dst(images[4], imagemFinal);
		vc_copy_src_to_dst(images[4], resistenciasJuntas);
		vc_copy_src_to_dst(images[4], auxVermelho);

		//Obter faixa vermelha 2
		vc_hsv_segmentation(images[0], images[4], (354 * 255) / 360, (360 * 255) / 360, (45 * 255) / 100, (75 * 255) / 100, (55 * 255) / 100, (75 * 255) / 100);

		//Copiar para imagens auxiliares.
		vc_copy_src_to_dst(images[4], imagemFinal);
		vc_copy_src_to_dst(images[4], resistenciasJuntas);
		vc_copy_src_to_dst(images[4], auxVermelho);

		nBlobsSegVermelho = 0;
		imageVermelho = vc_image_new(image->width, image->height, 1, image->levels);
		blobVermelho = vc_binary_blob_labelling(auxVermelho, imageVermelho, &nBlobsSegVermelho);
		iteradorVermelho = 0;
		if (blobVermelho != NULL)
		{
			vc_binary_blob_info(imageVermelho, blobVermelho, nBlobsSegVermelho);
			for (int i = 0; i < nBlobsSegVermelho; i++)
			{
				blobVermelho[i].valorCor = 2;
				if (blobVermelho[i].area >= 105 && blobVermelho[i].area <= 1000)
				{
					arrayVermelho[iteradorVermelho] = (OVC*)malloc(sizeof(OVC));
					memcpy(arrayVermelho[iteradorVermelho], &blobVermelho[i], sizeof(OVC));
					iteradorVermelho++;
				}
			}
		}
		//Obter faixa castanha
		IVC* juntarCastanhos = vc_image_new(image->width, image->height, 1, image->levels);
		images[5] = vc_image_new(image->width, image->height, 1, image->levels);
		//Escuro
		vc_hsv_segmentation(images[0], images[5], (12 * 255) / 360, (28 * 255) / 360, (25 * 255) / 100, (44 * 255) / 100, (31 * 255) / 100, (49 * 255) / 100);
		vc_copy_src_to_dst(images[5], imagemFinal);
		vc_copy_src_to_dst(images[5], juntarCastanhos);

		//Claro
		vc_hsv_segmentation(images[0], images[5], (11 * 255) / 360, (23 * 255) / 360, (42 * 255) / 100, (58 * 255) / 100, (41 * 255) / 100, (58 * 255) / 100);
		vc_copy_src_to_dst(images[5], imagemFinal);
		vc_copy_src_to_dst(images[5], juntarCastanhos);
		vc_copy_src_to_dst(juntarCastanhos, resistenciasJuntas);


		nBlobsSegCastanho = 0;
		imageCastanho = vc_image_new(image->width, image->height, 1, image->levels);
		blobCastanho = vc_binary_blob_labelling(juntarCastanhos, imageCastanho, &nBlobsSegCastanho);
		iteradorCastanho = 0;

		if (blobCastanho != NULL)
		{
			vc_binary_blob_info(imageCastanho, blobCastanho, nBlobsSegCastanho);
			for (int i = 0; i < nBlobsSegCastanho; i++)
			{
				blobCastanho[i].valorCor = 1;
				if (blobCastanho[i].area >= 95 && blobCastanho[i].area <= 1000)
				{
					arrayCastanho[iteradorCastanho] = (OVC*)malloc(sizeof(OVC));
					memcpy(arrayCastanho[iteradorCastanho], &blobCastanho[i], sizeof(OVC));
					iteradorCastanho++;
				}
			}
		}


		//Obter faixa Preta

		images[6] = vc_image_new(image->width, image->height, 1, image->levels);
		vc_hsv_segmentation(images[0], images[6], (35 * 255) / 360, (200 * 255) / 360, (3 * 255) / 100, (19 * 255) / 100, (15 * 255) / 100, (37 * 255) / 100);
		vc_copy_src_to_dst(images[6], imagemFinal);

		nBlobsSegPreto = 0;
		imagePreto = vc_image_new(image->width, image->height, 1, image->levels);
		blobPreto = vc_binary_blob_labelling(images[6], imagePreto, &nBlobsSegPreto);
		iteradorPreto = 0;

		if (blobPreto != NULL)
		{
			vc_binary_blob_info(imagePreto, blobPreto, nBlobsSegPreto);

			for (int i = 0; i < nBlobsSegPreto; i++)
			{
				blobPreto[i].valorCor = 0;
				if (blobPreto[i].area >= 150 && blobPreto[i].area <= 1000)
				{
					arrayPreto[iteradorPreto] = (OVC*)malloc(sizeof(OVC));
					memcpy(arrayPreto[iteradorPreto], &blobPreto[i], sizeof(OVC));
					iteradorPreto++;
				}
			}
		}
		vc_copy_src_to_dst(images[6], imagemFinal);
		vc_copy_src_to_dst(images[6], resistenciasJuntas);


		//Obter faixa laranja
		images[7] = vc_image_new(image->width, image->height, 1, image->levels);
		vc_hsv_segmentation(images[0], images[7], (6 * 255) / 360, (12 * 255) / 360, (68 * 255) / 100, (78 * 255) / 100, (80 * 255) / 100, (92 * 255) / 100);
		vc_copy_src_to_dst(images[7], imagemFinal);
		int nBlobsSegLaranja = 0;
		imageLaranja = vc_image_new(image->width, image->height, 1, image->levels);
		blobLaranja = vc_binary_blob_labelling(images[7], imageLaranja, &nBlobsSegLaranja);
		iteradorLaranja = 0;
		if (blobLaranja != NULL)
		{
			vc_binary_blob_info(imageLaranja, blobLaranja, nBlobsSegLaranja);

			for (int i = 0; i < nBlobsSegLaranja; i++)
			{
				blobLaranja[i].valorCor = 3;
				if (blobLaranja[i].area >= 150 && blobLaranja[i].area <= 1000)
				{
					arrayLaranja[iteradorLaranja] = (OVC*)malloc(sizeof(OVC));
					memcpy(arrayLaranja[iteradorLaranja], &blobLaranja[i], sizeof(OVC));
					iteradorLaranja++;
				}
			}
		}
		vc_copy_src_to_dst(images[7], imagemFinal);
		vc_copy_src_to_dst(images[7], resistenciasJuntas);
		vc_copy_src_to_dst(resistenciasJuntas, imagemFinal);
		imagemFinalAux = vc_image_new(imagemFinal->width, imagemFinal->height, 1, imagemFinal->levels);
		vc_binary_dilate(imagemFinal, imagemFinalAux, 7);


		auxBlobSegmentation = vc_image_new(image->width, image->height, 1, image->levels);
		nBlobsSegmentation = 0;
		imagemBoundingBox = vc_image_new(image->width, image->height, 3, image->levels);
		//Blob resist�ncias.
		blobSegmentation = vc_binary_blob_labelling(imagemFinalAux, auxBlobSegmentation, &nBlobsSegmentation);
		int numResis = 0;
		if (blobSegmentation != NULL)
		{
			vc_binary_blob_info(auxBlobSegmentation, blobSegmentation, nBlobsSegmentation);
			for (int b = 0; b < nBlobsSegmentation; b++)
			{
				if ((blobSegmentation[b].area <= 6900 || blobSegmentation[b].area >= 10000) || (blobSegmentation[b].height > 200))
					continue;
				int numFaixas = 3, totalOhmResis = 0, helperResis = 0;

				for (int i = 0; i < iteradorVerde; i++)
				{
					if (arrayVerde && arrayVerde[0] != NULL)
					{
						if (arrayVerde[i] != NULL && (arrayVerde[i]->x >= blobSegmentation[b].x &&
							arrayVerde[i]->y >= blobSegmentation[b].y &&
							arrayVerde[i]->fimX <= blobSegmentation[b].fimX &&
							arrayVerde[i]->fimY <= blobSegmentation[b].fimY))
						{
							arrayResistencia[helperResis] = (OVC*)malloc(sizeof(OVC));
							memcpy(arrayResistencia[helperResis], arrayVerde[i], sizeof(OVC));
							helperResis++;
						}
					}
				}

				for (int i = 0; i < iteradorAzul; i++)
				{
					if (arrayAzul && arrayAzul[0] != NULL)
					{
						if (arrayAzul[i] != NULL && (arrayAzul[i]->x >= blobSegmentation[b].x &&
							arrayAzul[i]->y >= blobSegmentation[b].y &&
							arrayAzul[i]->fimX <= blobSegmentation[b].fimX &&
							arrayAzul[i]->fimY <= blobSegmentation[b].fimY))
						{
							arrayResistencia[helperResis] = (OVC*)malloc(sizeof(OVC));
							memcpy(arrayResistencia[helperResis], arrayAzul[i], sizeof(OVC));
							helperResis++;
						}
					}
				}

				for (int i = 0; i < iteradorVermelho; i++)
				{
					if (arrayVermelho && arrayVermelho[0] != NULL)
					{
						if (arrayVermelho[i] != NULL && (arrayVermelho[i]->x >= blobSegmentation[b].x &&
							arrayVermelho[i]->y >= blobSegmentation[b].y &&
							arrayVermelho[i]->fimX <= blobSegmentation[b].fimX &&
							arrayVermelho[i]->fimY <= blobSegmentation[b].fimY))
						{
							arrayResistencia[helperResis] = (OVC*)malloc(sizeof(OVC));
							memcpy(arrayResistencia[helperResis], arrayVermelho[i], sizeof(OVC));
							helperResis++;
						}
					}
				}

				if (arrayPreto && arrayPreto[0] != NULL)
				{
					for (int i = 0; i < iteradorPreto; i++)
					{
						if (arrayPreto[i] != NULL && (arrayPreto[i]->x >= blobSegmentation[b].x &&
							arrayPreto[i]->y >= blobSegmentation[b].y &&
							arrayPreto[i]->fimX <= blobSegmentation[b].fimX &&
							arrayPreto[i]->fimY <= blobSegmentation[b].fimY))
						{
							arrayResistencia[helperResis] = (OVC*)malloc(sizeof(OVC));

							memcpy(arrayResistencia[helperResis], arrayPreto[i], sizeof(OVC));
							helperResis++;
						}
					}
				}

				if (arrayCastanho && arrayCastanho[0] != NULL)
				{
					for (int i = 0; i < iteradorCastanho; i++)
					{
						if (arrayCastanho[i] != NULL && (arrayCastanho[i]->x >= blobSegmentation[b].x &&
							arrayCastanho[i]->y >= blobSegmentation[b].y &&
							arrayCastanho[i]->fimX <= blobSegmentation[b].fimX &&
							arrayCastanho[i]->fimY <= blobSegmentation[b].fimY))
						{
							arrayResistencia[helperResis] = (OVC*)malloc(sizeof(OVC));
							memcpy(arrayResistencia[helperResis], arrayCastanho[i], sizeof(OVC));
							helperResis++;
						}
					}
				}
				if (arrayLaranja && arrayLaranja[0] != NULL)
				{
					for (int i = 0; i < iteradorLaranja; i++)
					{
						if (arrayLaranja[i] != NULL && (arrayLaranja[i]->x >= blobSegmentation[b].x &&
							arrayLaranja[i]->y >= blobSegmentation[b].y &&
							arrayLaranja[i]->fimX <= blobSegmentation[b].fimX &&
							arrayLaranja[i]->fimY <= blobSegmentation[b].fimY))
						{
							arrayResistencia[helperResis] = (OVC*)malloc(sizeof(OVC));
							memcpy(arrayResistencia[helperResis], arrayLaranja[i], sizeof(OVC));
							helperResis++;
						}
					}
				}

				if (helperResis > 0) {
					qsort(arrayResistencia, helperResis, sizeof(OVC*), compare);

					if (helperResis >= 3)
					{
						for (int i = 0; i < 3; i++) {
							int first = arrayResistencia[i]->x;
							int valueCor = arrayResistencia[i]->valorCor;
							if (numFaixas > 1) {
								appendDigit(&totalOhmResis, valueCor);
								numFaixas--;
							}
							else {
								int multiplicador = vc_table_resistors_multiplier(valueCor);
								totalOhmResis = totalOhmResis * multiplicador;
							}
						}
					}
					for (int i = 0; i < helperResis; i++) {
						arrayResistencia[i] = NULL;
					}
				}

				vc_copy_src_to_dst_rgb(imageRGB, imagemBoundingBox);

				std::string strOhms = "Ohms: " + std::to_string(totalOhmResis);
				std::string strXcGravidade = "Xc: " + std::to_string(blobSegmentation[b].xc);
				std::string strYcGravidade = "Yc: " + std::to_string(blobSegmentation[b].yc);
				std::string strArea = "Area: " + std::to_string(blobSegmentation[b].area);

				cv::rectangle(frame, cv::Point(blobSegmentation[b].x, blobSegmentation[b].y), cv::Point(blobSegmentation[b].fimX, blobSegmentation[b].fimY), cv::Scalar(0, 0, 255), 2);
				cv::circle(frame, cv::Point(blobSegmentation[b].xc, blobSegmentation[b].yc), 2, cv::Scalar(0, 0, 0), -1);

				cv::putText(frame, strOhms, cv::Point(blobSegmentation[b].x - 10, blobSegmentation[b].y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 2);
				cv::putText(frame, strOhms, cv::Point(blobSegmentation[b].x - 10, blobSegmentation[b].y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);

				cv::putText(frame, strXcGravidade, cv::Point(blobSegmentation[b].x + 100, blobSegmentation[b].y - 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 2);
				cv::putText(frame, strXcGravidade, cv::Point(blobSegmentation[b].x + 100, blobSegmentation[b].y - 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);

				cv::putText(frame, strYcGravidade, cv::Point(blobSegmentation[b].x + 100, blobSegmentation[b].y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 2);
				cv::putText(frame, strYcGravidade, cv::Point(blobSegmentation[b].x + 100, blobSegmentation[b].y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);

				cv::putText(frame, strArea, cv::Point(blobSegmentation[b].x + 100, blobSegmentation[b].y + 70), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 2);
				cv::putText(frame, strArea, cv::Point(blobSegmentation[b].x + 100, blobSegmentation[b].y + 70), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
			}
		}
		str = std::string("RESOLUCAO: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("TOTAL DE FRAMES: ").append(std::to_string(video.ntotalframes));
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("FRAME RATE: ").append(std::to_string(video.fps));
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("N. DA FRAME: ").append(std::to_string(video.nframe));
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		// Copia dados de imagem da estrutura IVC para uma estrutura cv::Mat
		//memcpy(frame.data, image->data, video.width * video.height * 3);
		// Liberta a mem�ria da imagem IVC que havia sido criada


		// +++++++++++++++++++++++++

		/* Exibe a frame */
		cv::imshow("VC - VIDEO", frame);
		/* Sai da aplica��o, se o utilizador premir a tecla 'q' */
		key = cv::waitKey(1);
	}
	vc_image_free(imagemFinal);
	vc_image_free(imagemFinalAux);
	vc_image_free(auxBlobSegmentation);
	for (int i = 0; i < 8; i++)
	{
		vc_image_free(images[i]);
	}

	free(blobSegmentation);
	free(blobAzul);
	free(blobCastanho);
	free(blobPreto);
	free(blobVermelho);
	free(blobVerde);
	/* Fecha a janela */
	cv::destroyWindow("VC - VIDEO");

	/* Fecha o ficheiro de v�deo */
	capture.release();

	return 0;
}
