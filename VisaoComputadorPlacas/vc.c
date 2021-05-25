//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITECNICO DO CAVADO E DO AVE
//                          2020/2021
//             ENGENHARIA DE SISTEMAS INFORMATICOS
//                    VISAO POR COMPUTADOR
//
//                     [  Joao Fonseca  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de funcoes nao seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include "vc.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 // 	FUNCOES DA BIBLIOTECA FORNECIDA PELO PROFESSOR
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#pragma region LIB_VISAO_COMPUTADOR

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// FUNCOEES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#pragma region ALOCACAO_LIBERTACAO

// Alocar memoria para uma imagem
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

#pragma endregion ALOCACAO_LIBERTACAO

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// FUNCOES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#pragma region FUNCOES_DE_LEITURA

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

#pragma endregion FUNCOES_DE_LEITURA


#pragma endregion LIB_VISAO_COMPUTADOR



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 // 	FUNCOES DESENVOLVIDAS PARA O TRABALHO
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#pragma region LIB_DESENVOLVIDA_ALUNOS

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 // 	FUNCOES DE MANIPULACAO DE IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#pragma region FUNCOES_DE_MANIPULACAO


void vc_rgb_get_blue_component(IVC* srcdst) {

	for (int x = 0; x < srcdst->height; x++) {
		for (int y = 0; y < srcdst->width; y++) {
			int pos = x * srcdst->bytesperline + y * srcdst->channels;
			srcdst->data[pos] = 0;
			srcdst->data[pos + 1] = 0;
			srcdst->data[pos + 2] = srcdst->data[pos + 2];
		}
	}
}


void vc_rgb_to_gray(IVC* src, IVC* dst) {

	for (int x = 0; x < src->height; x++) {
		for (int y = 0; y < src->width; y++) {

			int pos = x * src->bytesperline + y * src->channels;
			int posDst = x * dst->bytesperline + y * dst->channels;

			float rf = src->data[pos];
			float gf = src->data[pos + 1];
			float bf = src->data[pos + 2];

			dst->data[posDst] = (unsigned char)((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
		}
	}
}

void vc_gray_to_binary_average_threshould(IVC* src) {
	int threshold = getThresholdByAverage(src);
	vc_gray_to_binary(src, threshold);
}

void vc_gray_to_binary_given_threshould(IVC* src, int threshold) {
	vc_gray_to_binary(src, threshold);
}

int getThresholdByAverage(IVC* src) {
	int vectorSize = getVectorSize(src);
	int total = 0;
	for (int x = 0; x < vectorSize; x += src->channels) {
		total += src->data[x];
	}
	int threshould = total / vectorSize;
	printf("\nThreshould Average Value = %i\n", threshould);
	return threshould;
}


void vc_gray_to_binary(IVC* src, int threshold) {
	int vectorSize = getVectorSize(src);
	for (int x = 0; x < vectorSize; x += src->channels) {
		src->data[x] = getValueByThreshold(src->data[x], threshold);;
	}
}

int getValueByThreshold(int colorValue, int threshold) {
	return threshold >= colorValue ? 0 : 255;
}

int getVectorSize(IVC* src) {
	return src->width * src->height * src->channels;
}


int vc_binary_open(IVC* src, int kernelErode, int kernelDilate) {
	IVC* destImageDilate = vc_image_new(src->width, src->height, 1, 255);

	vc_binary_erode(src, destImageDilate, kernelErode);

	vc_binary_dilate(destImageDilate, src, kernelDilate);

	return 1;
}

int vc_binary_close(IVC* src, int kernelErode, int kernelDilate) {
	IVC* destImageErode = vc_image_new(src->width, src->height, 1, 255);
	
	vc_binary_dilate(src, destImageErode, kernelErode);

	vc_binary_erode(destImageErode, src, kernelDilate);

	return 1;
}

void vc_binary_dilate(IVC* src, IVC* dst, int kernel) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;


	int offset = (kernel - 1) / 2;
	long int pos, posk;
	int existe = 0;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
	if (channels != 1) return 0;

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;
			//offset - posi��o da vizinhan�a do kernel, caso o kernel seja 3*3 ent�o ky=-1 0 1 e kx tb
			for (int ky = -offset; ky <= offset; ky++) {
				for (int kx = -offset; kx <= offset; kx++) {
					//Para ignorar as partes da Vizinhan�a do Kernel que ficam fora da imagem (nas pontas e estremidades da imagem)
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width)) {
						//posk = y * bytesperline + x * channels) + (ky * bytesperline + x * channels) --> (onde num kernel de 3*3 o ky = -1 0 1 e kx = -1 0 1)
						posk = (y + ky) * bytesperline + (x + kx) * channels;

						//se a vizinhan�a for branco -> o pixel origem fica a branco (ADICIONA PIXELS)
						if (src->data[posk] == 255) {
							existe = 1;
						}
					}
				}
			}
			if (existe == 1) {
				dst->data[pos] = 255;
			}
			else {
				dst->data[pos] = 0;
			}
			existe = 0;
		}
	}
}

void vc_binary_erode(IVC* src, IVC* dst, int kernel) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;

	//definicaoo de variaveis
	int offset = (kernel - 1) / 2; //(int) floor(((double) kernel) / 2.0);
	long int pos, posk;
	int existe = 0;

	// Verificacao de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
	if (channels != 1) return 0;

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;

			// NxM Vizinhos
			//offset - posicao da vizinhanca do kernel, caso o kernel seja 3*3 entao ky=-1 0 1 e kx tb
			for (int ky = -offset; ky <= offset; ky++) {
				for (int kx = -offset; kx <= offset; kx++) {
					//Para ignorar as partes da Vizinhanca do Kernel que ficam fora da imagem (nas pontas e estremidades da imagem)
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width)) {
						//posk = y * bytesperline + x * channels) + (ky * bytesperline + x * channels) --> (onde num kernel de 3*3 o ky = -1 0 1 e kx = -1 0 1)
						posk = (y + ky) * bytesperline + (x + kx) * channels;

						//se a vizinhanca for preto -> o pixel origem fica a preto (REMOVE PIXEIS (apaga-os))
						if (src->data[posk] == 0) {
							existe = 1;
						}
					}
				}
			}
			if (existe == 1) {
				dst->data[pos] = 0;
			}
			else {
				dst->data[pos] = 255;
			}
			existe = 0;
		}
	}
}

int extractImage(IVC* src, IVC* dst) {
	int vectorSize = getVectorSize(dst);

	int imagePixelsCount = 0;

	for (int line = 0; line < dst->height; line++) {
		for (int column = 0; column < dst->width; column++) {
			int pos = line * dst->bytesperline + column * dst->channels;
			if (dst->data[pos] == 0) {
				src->data[pos] = 0;
				src->data[pos+1] = 0;
				src->data[pos+2] = 0;
				imagePixelsCount++;
			}
		}
	}
	return imagePixelsCount;
}


#pragma endregion FUNCOES_DE_MANIPULACAO


// Etiquetagem de blobs
// src		: Imagem binaria de entrada
// dst		: Imagem grayscale (ira conter as etiquetas)
// nlabels	: Endereco de memoria de uma variavel, onde sera armazenado o numero de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. E necessario libertar posteriormente esta memoria.
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
	int labeltable[5000] = { 0 };
	int labelarea[5000] = { 0 };
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC* blobs; // Apontador para array de blobs (objectos) que sera retornado desta funcao.

	int isOk = 0;
	for (int pos=0; pos < 256; pos++) {
		if (labeltable[pos]==0) isOk++;
	}

	if (isOk == 256) {

		// Verificacao de erros
		if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
		if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
		if (channels != 1) return NULL;

		// Copia dados da imagem bin�ria para imagem grayscale
		memcpy(datadst, datasrc, bytesperline * height);

		// Todos os pixeis de plano de fundo devem obrigatoriamente ter valor 0
		// Todos os pixeis de primeiro plano devem obrigatoriamente ter valor 255
		// Serao atribuidas etiquetas no intervalo [1,254]
		// Este algoritmo esta assim limitado a 255 labels
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
						//printf("\nlabel teste 1 %d", labeltable[label]);
						labeltable[label] = label;
						label++;
					}
					else
					{
						num = 255;

						// Se A esta marcado
						if (datadst[posA] != 0) num = labeltable[datadst[posA]];
						// Se B esta marcado, e o menor que a etiqueta "num"
						if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
						// Se C esta marcado, e o menor que a etiqueta "num"
						if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
						// Se D esta marcado, e o menor que a etiqueta "num"
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
								for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
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
					//printf("\nlabel teste 2 %d", labeltable[datadst[posX]]);
					datadst[posX] = labeltable[datadst[posX]];
				}
			}
		}

		//printf("\nMax Label = %d\n", label);

		// Contagem do namero de blobs
		// Passo 1: Eliminar, da tabela, etiquetas repetidas
		for (a = 1; a < label - 1; a++)
		{
			for (b = a + 1; b < label; b++)
			{
				//printf("\nlabel teste 3 %d", labeltable[b]);
				//printf("\nlabel teste 4 %d", labeltable[a]);
				if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
			}
		}
		// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que nao hajam valores vazios (zero) entre etiquetas
		*nlabels = 0;
		for (a = 1; a < label; a++)
		{
			if (labeltable[a] != 0)
			{
				//printf("\nlabel teste 5 %d", labeltable[a]);
				labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
				(*nlabels)++; // Conta etiquetas
			}
		}

		// Se nao ha blobs
		if (*nlabels == 0) return NULL;

		// Cria lista de blobs (objectos) e preenche a etiqueta
		blobs = (OVC*)calloc((*nlabels), sizeof(OVC));

		if (blobs != NULL)
		{	
			
			for (a = 0; a < (*nlabels); a++) {
				if (labeltable[a] != 0 && labeltable[a] != NULL) {
					blobs[a].label = labeltable[a];
				}
				//printf("\nfinal label%d", labeltable[a]);
			}
		}
		else return NULL;

		return blobs;
	}
	return NULL;
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

	// Verificacao de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Conta area de cada blob
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

		// Centro de Gravidade
		//blobs[i].xc = (xmax - xmin) / 2;
		//blobs[i].yc = (ymax - ymin) / 2;
		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}

	printBlobsInfo(blobs, nblobs);

	return 1;
}

/**
 * Imprime todas as informações relativas aos blobs identificados,
 * conforme a quantidade de blobs encontrados
 * */
void printBlobsInfo(OVC* blobs, int nblobs) {
	for (int position = 0; position < nblobs; position++) {
		printf("\n******************************************\n");
		printf("Area: %d\n", blobs[position].area);
		printf("Perimetro: %d\n", blobs[position].perimeter);
		printf("Centro de massa eixo X: %d\n", blobs[position].xc);
		printf("Centro de massa eixo Y: %d\n", blobs[position].yc);
		printf("Caixa limitadora eixo X: %d\n", blobs[position].x);
		printf("Caixa limitadora eixo Y: %d\n", blobs[position].y);
		printf("Caixa limitadora dimensões:\n");
		printf("\t\t\twidht %d\n", blobs[position].width);
		printf("\t\t\theight: %d\n", blobs[position].height);
	}
}


Coord* drawBlobBox(IVC* src, OVC blobs, int nblobs) {

		int eixoXstart = blobs.x;
		int eixoXend = blobs.width + eixoXstart;
		int eixoYstart = blobs.y;
		int eixoYend = blobs.height + eixoYstart;

		Coord* coordinates = (Coord*)malloc(sizeof(Coord));
		coordinates->eixoXstart = eixoXstart;
		coordinates->eixoXend = blobs.width + eixoXstart;
		coordinates->eixoYstart = blobs.y;
		coordinates->eixoYend = blobs.height + eixoYstart;


		int cmX = blobs.xc;
		int cmY = blobs.yc;

		int distX = blobs.width * 2;
		int distY = blobs.height * 2;


		for (int line = 0; line < src->height; line++) {
			for (int column = 0; column <= src->width; column++) {
				int pos = line * src->bytesperline + column * src->channels;
				int pos_minus1 = (line-1) * src->bytesperline + (column-1) * src->channels;
				int pos_minus2 = (line-1) * src->bytesperline + (column-1) * src->channels;
				if (line >= eixoYstart && line <= eixoYend) {
					if ((column == eixoXstart || column == eixoXend) && distY >= 0) {
						src->data[pos] = 75;
						src->data[pos + 1] = 247;
						src->data[pos+2] = 49;	
						
						src->data[pos_minus1] = 75;
						src->data[pos_minus1 + 1] = 247;
						src->data[pos_minus1 +2] = 49;

						src->data[pos_minus2] = 75;
						src->data[pos_minus2 + 1] = 247;
						src->data[pos_minus2 +2] = 49;
					}
					if (column == cmX) {
						int pos_plus1 = (line + 1) * src->bytesperline + (column + 1) * src->channels;
						
						src->data[pos] = 199;
						src->data[pos + 1] = 0;
						src->data[pos + 2] = 255;

						src->data[pos_minus1] = 199;
						src->data[pos_minus1 + 1] = 0;
						src->data[pos_minus1 + 2] = 255;

						src->data[pos_plus1] = 199;
						src->data[pos_plus1 + 1] = 0;
						src->data[pos_plus1 + 2] = 255;
					}
				}
				if (column >= eixoXstart && column <= eixoXend) {
					if ((line == eixoYstart || line == eixoYend) && distX >= 0) {
						src->data[pos] = 75;
						src->data[pos + 1] = 247;
						src->data[pos + 2] = 49;

						src->data[pos_minus1] = 75;
						src->data[pos_minus1 + 1] = 247;
						src->data[pos_minus1 + 2] = 49;

						src->data[pos_minus2] = 75;
						src->data[pos_minus2 + 1] = 247;
						src->data[pos_minus2 + 2] = 49;
					}
					if (line == cmY) {

						int pos_plus1 = (line + 1) * src->bytesperline + (column + 1) * src->channels;

						src->data[pos] = 199;
						src->data[pos + 1] = 0;
						src->data[pos + 2] = 255;

						src->data[pos_minus1] = 199;
						src->data[pos_minus1 + 1] = 0;
						src->data[pos_minus1 + 2] = 255;

						src->data[pos_plus1] = 199;
						src->data[pos_plus1 + 1] = 0;
						src->data[pos_plus1 + 2] = 255;
					}
				}
			}
		}

	return coordinates;
}



void vc_rgb_to_hsv(IVC* src) {

	int vectorSize = src->width * src->height * src->channels;

	for (int x = 0; x < vectorSize; x += src->channels) {

		float red = (float)src->data[x];
		float green  = (float)src->data[x + 1];
		float blue = (float)src->data[x + 2];

		int max = vc_rgb_max(red, green, blue);
		int min = vc_rgb_min(red, green, blue);

		float value = (float)max;
		float saturation=0;
		float hue=0;

		if (value == 0) {
			saturation = 0;
			hue = 0;
		}
		else {
			saturation = ((max - min) / value) * 255;
			hue = getHue(red, green, blue, max, min);
		}

		src->data[x] = (unsigned char)hue;
		src->data[x + 1] = (unsigned char)saturation;
		src->data[x + 2] = (unsigned char)value;
	}
}


float vc_rgb_max(int r, int g, int b) {
	return (r > g ? (r > b ? r : b) : (g > b ? g : b));
}
float vc_rgb_min(int r, int g, int b) {
	return (r < g ? (r < b ? r : b) : (g < b ? g : b));
}

float getHue(float red, float green, float blue, float max, float min) {
	float divisor = max - min;

	if (divisor <= 0) return 0;

	float hue;
	
	if (max == red) {
		if (green >= blue) {
			hue = 60.0 * (green - blue) / divisor;
		} else {
			hue = 360.0 + 60.0 * (green - blue) / divisor;
		}
	} else if (max == green) {
		hue = 120.0 + 60.0 * (blue - red) / divisor;
	}
	else {
		hue = 240.0 + 60.0 * (red - green) / divisor;
	}

	return hue / 360 * 255;
}


void vc_hsv_segmentation(IVC* src, int hmin, int hmax, int smin, int smax, int vmin, int vmax) {

	int vectorSize = src->width * src->height * src->channels;

	for (int x = 0; x < vectorSize; x += src->channels) {

		float channel_01 = (float)src->data[x];
		float channel_02 = (float)src->data[x + 1];
		float channel_03 = (float)src->data[x + 2];

		int hue = channel_01 / 255 * 360;
		int saturation = channel_02 / 255 * 100;
		int value = channel_03 / 255 * 100;

		int realValue;

		realValue = (hue > hmin && hue <= hmax 
			&& saturation >= smin && saturation <= smax
			&& value >= vmin && value <= vmax) ? 255 : 0;

		src->data[x] = (unsigned char)realValue;
		src->data[x + 1] = (unsigned char)realValue;
		src->data[x + 2] = (unsigned char)realValue;
	}

}


void rgb2bgrinvert(IVC* imagem) {

	int vectorSize = imagem->width * imagem->height * imagem->channels;

	for (int x = 0; x < vectorSize; x += imagem->channels) {
		int r = imagem->data[x];
		int g = imagem->data[x+1];
		int b = imagem->data[x+2];

		imagem->data[x] = b;
		imagem->data[x + 1] = g;
		imagem->data[x + 2] = r;
	}
}

int vc_rgb_to_hsv2(IVC* srcdst) {
	float hue, saturation, value;
	float rgb_max, rgb_min;
	float max_min_diff;

	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (srcdst->channels != 3) return 0;

	int size = srcdst->width * srcdst->height * srcdst->channels;

	for (int i = 0; i < size; i += srcdst->channels) {
		float r = (float)srcdst->data[i];
		float g = (float)srcdst->data[i + 1];
		float b = (float)srcdst->data[i + 2];


		// Max and min RGB
		rgb_max = vc_rgb_max(r, g, b);
		rgb_min = vc_rgb_min(r, g, b);

		// Default values
		hue = 0;
		saturation = 0;


		// Value toma valores entre [0,255]
		value = rgb_max;
		if (value != 0) {
			max_min_diff = (rgb_max - rgb_min);

			// Saturation toma valores entre [0,255]
			saturation = (max_min_diff / value) * 255;

			if (saturation != 0) {
				// Hue toma valores entre [0,360]
				if ((rgb_max == r) && (g >= b)) {
					hue = 60 * (g - b) / max_min_diff;
				}
				else if ((rgb_max == r) && (b > g)) {
					hue = 360 + 60 * (g - b) / max_min_diff;
				}
				else if (rgb_max == g) {
					hue = 120 + 60 * (b - r) / max_min_diff;
				}
				else /* rgb_max == b*/ {
					hue = 240 + 60 * (r - g) / max_min_diff;
				}
			}
		}

		// Atribui valores entre [0,255]
		srcdst->data[i] = (unsigned char)(hue / 360 * 255);
		srcdst->data[i + 1] = (unsigned char)(saturation);
		srcdst->data[i + 2] = (unsigned char)(value);
	}

	return 1;
}

int analyzesQuadrants(IVC* src, OVC blobs, int segmentColor) {


	int eixoXstart = blobs.x;
	int eixoXend = blobs.width + eixoXstart;
	int eixoYstart = blobs.y;
	int eixoYend = blobs.height + eixoYstart;

	int cmX = blobs.xc;
	int cmY = blobs.yc;

	int q1b = 0;
	int q2b = 0;
	int q3b = 0;
	int q4b = 0;

	for (int line = 0; line < src->height; line++) {
		for (int column = 0; column <= src->width; column++) {
			int pos = line * src->bytesperline + column * src->channels;
		
			if (line >= eixoYstart && line <= eixoYend && column >= eixoXstart && column <= eixoXend) {
				//primeiro quadrante
				if (column <= cmX && line <= cmY) {
					if (src->data[pos] == 255) {
						q1b++;
					}
				}
				//segundo quadrante
				else if (column <= cmX && line > cmY) {
					if (src->data[pos] == 255) {
						q2b++;
						
					}
				}
				//terceiro quadrante
				else if (column > cmX && line <= cmY) {
					if (src->data[pos] == 255) {
						q3b++;
						;
					}
				}
				//quarto quadrante
				else {
					if (src->data[pos] == 255) {
						q4b++;
					}
				}
			}
		}
	}

	int widthQ1 = cmX - eixoXstart;
	int heigthQ1 = cmY - eixoYstart;
	int dimQ1 = widthQ1 * heigthQ1;

	int widthQ2 = cmX - eixoXstart;
	int heigthQ2 = eixoYend - cmY;
	int dimQ2 = widthQ2 * heigthQ2;

	int widthQ3 = eixoXend - cmX;
	int heigthQ3 = cmY - eixoYstart;
	int dimQ3 = widthQ3 * heigthQ3;

	int widthQ4 = eixoXend - cmX;
	int heigthQ4 = eixoYend - cmY;
	int dimQ4 = widthQ4 * heigthQ4;

	float ratioQ1 = (float)q1b / dimQ1;
	float ratioQ2 = (float)q2b / dimQ2;
	float ratioQ3 = (float)q3b / dimQ3;
	float ratioQ4 = (float)q4b / dimQ4;

	return getSignType(ratioQ1, ratioQ2, ratioQ3, ratioQ4, segmentColor);
}


//Analisa apenas as placas azuis
int getSignType(float percQ1, float percQ2, float percQ3, float percQ4, int segmentColor) {

	float minRatioLimit = 0.85;
	float maxRatioLimit = 1.15;

	if (segmentColor == 0) {

		//Forbidden
		float ratioQ1Forbidden = percQ1 / 0.30;
		float ratioQ2Forbidden = percQ2 / 0.30;
		float ratioQ3Forbidden = percQ3 / 0.30;
		float ratioQ4Forbidden = percQ4 / 0.30;

		if (ratioQ1Forbidden >= minRatioLimit && ratioQ1Forbidden <= maxRatioLimit &&
			ratioQ2Forbidden >= minRatioLimit && ratioQ2Forbidden <= maxRatioLimit &&
			ratioQ3Forbidden >= minRatioLimit && ratioQ3Forbidden <= maxRatioLimit &&
			ratioQ4Forbidden >= minRatioLimit && ratioQ4Forbidden <= maxRatioLimit
			) return 5;


		//Stop
		float ratioQ1Stop = percQ1 / 0.24;
		float ratioQ2Stop = percQ2 / 0.22;
		float ratioQ3Stop = percQ3 / 0.26;
		float ratioQ4Stop = percQ4 / 0.24;

		if (ratioQ1Stop >= minRatioLimit && ratioQ1Stop <= maxRatioLimit &&
			ratioQ2Stop >= minRatioLimit && ratioQ2Stop <= maxRatioLimit &&
			ratioQ3Stop >= minRatioLimit && ratioQ3Stop <= maxRatioLimit &&
			ratioQ4Stop >= minRatioLimit && ratioQ4Stop <= maxRatioLimit
			) return 6;
	}
	else {
		//ARROW LEFT
		float ratioQ1ArrowLeft = percQ1 / 0.38;
		float ratioQ2ArrowLeft = percQ2 / 0.38;
		float ratioQ3ArrowLeft = percQ3 / 0.31;
		float ratioQ4ArrowLeft = percQ4 / 0.31;


		//ARROW RIGHT
		float ratioQ1ArrowRight = percQ1 / 0.31;
		float ratioQ2ArrowRight = percQ2 / 0.31;
		float ratioQ3ArrowRight = percQ3 / 0.38;
		float ratioQ4ArrowRight = percQ4 / 0.38;

		int moreLeft = 0;
		int moreRight = 0;

		ratioQ1ArrowLeft < ratioQ1ArrowRight ? moreLeft++ : moreRight++;
		ratioQ2ArrowLeft < ratioQ2ArrowRight ? moreLeft++ : moreRight++;
		ratioQ3ArrowLeft < ratioQ3ArrowRight ? moreLeft++ : moreRight++;
		ratioQ4ArrowLeft < ratioQ4ArrowRight ? moreLeft++ : moreRight++;

		//ARROW LEFT
		if (ratioQ1ArrowLeft >= minRatioLimit && ratioQ1ArrowLeft <= maxRatioLimit &&
			ratioQ2ArrowLeft >= minRatioLimit && ratioQ2ArrowLeft <= maxRatioLimit &&
			ratioQ3ArrowLeft >= minRatioLimit && ratioQ3ArrowLeft <= maxRatioLimit &&
			ratioQ4ArrowLeft >= minRatioLimit && ratioQ4ArrowLeft <= maxRatioLimit &&
			moreLeft>=moreRight
			) return 1;

		//ARROW RIGHT
		if (ratioQ1ArrowRight >= minRatioLimit && ratioQ1ArrowRight <= maxRatioLimit &&
			ratioQ2ArrowRight >= minRatioLimit && ratioQ2ArrowRight <= maxRatioLimit &&
			ratioQ3ArrowRight >= minRatioLimit && ratioQ3ArrowRight <= maxRatioLimit &&
			ratioQ4ArrowRight >= minRatioLimit && ratioQ4ArrowRight <= maxRatioLimit &&
			moreRight >= moreLeft
			) return 2;

		//CAR
		float ratioQ1Car = percQ1 / 0.20;
		float ratioQ2Car = percQ2 / 0.24;
		float ratioQ3Car = percQ3 / 0.21;
		float ratioQ4Car = percQ4 / 0.25;

		if (ratioQ1Car >= minRatioLimit && ratioQ1Car <= maxRatioLimit &&
			ratioQ2Car >= minRatioLimit && ratioQ2Car <= maxRatioLimit &&
			ratioQ3Car >= minRatioLimit && ratioQ3Car <= maxRatioLimit &&
			ratioQ4Car >= minRatioLimit && ratioQ4Car <= maxRatioLimit
			) return 3;

		//HIGHWAY
		float ratioQ1highway = percQ1 / 0.15;
		float ratioQ2highway = percQ2 / 0.35;
		float ratioQ3highway = percQ3 / 0.14;
		float ratioQ4highway = percQ4 / 0.35;

		if (ratioQ1highway >= minRatioLimit && ratioQ1highway <= maxRatioLimit &&
			ratioQ2highway >= minRatioLimit && ratioQ2highway <= maxRatioLimit &&
			ratioQ3highway >= minRatioLimit && ratioQ3highway <= maxRatioLimit &&
			ratioQ4highway >= minRatioLimit && ratioQ4highway <= maxRatioLimit
			) return 4;

	}

	return -1;
}



#pragma endregion LIB_DESENVOLVIDA_ALUNOS