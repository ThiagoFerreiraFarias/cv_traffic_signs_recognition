//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITeCNICO DO CaVADO E DO AVE
//                          2011/2012
//             ENGENHARIA DE SISTEMAS INFORMaTICOS
//                    VISaO POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <sys/timeb.h>
#include <windows.h>
#define VC_DEBUG

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                           MACROS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 * @brief Image and Vision Computing
 */
typedef struct imageVisionCumputing {
	unsigned char* data;
	int width, height;
	int channels;			// Binario/Cinzentos=1; RGB=3
	int levels;				// Binario=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// width * channels
} IVC;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UM BLOB (OBJECTO)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
	int x, y, width, height;	// Caixa Delimitadora (Bounding Box)
	int area;					// �rea
	int xc, yc;					// Centro-de-massa
	int perimeter;				// Per�metro
	int label;					// Etiqueta
} OVC;


typedef struct HsvSegmentationConstraints {
	int hmin;
	int hmax;
	int smin;
	int smax;
	int vmin;
	int vmax;
} HsvSC;

typedef struct PairCoordinate {
	int eixoXstart;
	int eixoXend;
	int eixoYstart;
	int eixoYend;

	int q1b;
	int q2b;
	int q3b;
	int q4b;

	int q1p;
	int q2p;
	int q3p;
	int q4p;
} Coord;

IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);

IVC* vc_read_image(char* filename);
int vc_write_image(char* filename, IVC* image);

OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);

/**
 * @brief Inverte uma imagem em escala de cinza,
 * invertendo os valores de cada posiçao do vetor de pixels
 * @param srcdst IVC* -> apontador de estrutura IVC de imagem de entrada
 * @return void
 */
void vc_gray_negative(IVC* srcdst);

/**
 * @brief Dada uma imagem colorida (RGB), percorre o vetor pixels da imagem e
 *  isola a componente azul transformando os canais vermelho e verde em preto.
 * @param srcdst IVC* -> apontador de estrutura IVC de imagem de entrada
 * @return void
 */
void vc_rgb_get_blue_component(IVC* srcdst);

/**
 * @brief Dada uma imagem colorida (RGB), percorre o vetor pixels da imagem e
 *  converte em escalas de cinza.
 * @param src IVC* -> apontador de estrutura IVC de imagem de entrada
 * @param dst IVC* -> apontador de estrutura IVC de imagem de saida
 * @return void
 */
void vc_rgb_to_gray(IVC* src, IVC* dst);

/**
 * @brief Dada uma imagem em escala de cinza, percorre o vetor de pixels da imagem e
 * calcula a média dos valores para obter o valor de threshould.
 * A seguir binariza a imagem atribuindo valor 255 a todos os pixels com valor superior
 * a média e valor 0 a todos com valor inferior.
 * @param src IVC* -> apontador de estrutura IVC de imagem de entrada
 * @return void
 */
void vc_gray_to_binary_average_threshould(IVC* src);

/**
 * @brief Dada uma imagem em escala de cinza, percorre o vetor de pixels da imagem e
 * calcula a média dos valores para obter o valor de threshould.
 * @param src IVC* -> apontador de estrutura IVC de imagem de entrada
 * @return int -> threshould medio
 */
int getThresholdByAverage(IVC* src);

/**
 * @brief Dada uma imagem em escala de cinza, e um valor arbitrario de threshould,
 * percorre o vetor de pixels da imagem atribuindo valor 255 a todos os pixels com valor superior
 * a média e valor 0 a todos com valor inferior.
 * @param src IVC* -> apontador de estrutura IVC de imagem de entrada
 * @param threshold int -> valor arbitrario de threshould
 * @return void
 */
void vc_gray_to_binary_given_threshould(IVC* src, int threshold);


/**
 * @brief Dada uma imagem em escala de cinza, e um valor definiido de threshould,
 * percorre o vetor de pixels da imagem atribuindo valor 255 a todos os pixels com valor superior
 * a média e valor 0 a todos com valor inferior.
 * @param src IVC* -> apontador de estrutura IVC de imagem de entrada
 * @param threshold int -> valor arbitrario de threshould
 * @return void
 */
void vc_gray_to_binary(IVC* src, int threshold);

/**
 * @brief Dada uma imagem binarizada de entrada, uma estrutura de imagem de saida,
 * realiza operacoes morfologicas de erosao e dilatacao baseado em tamanhos de kernel
 * para cada operacao.
 * @param src IVC* -> apontador de estrutura IVC de imagem de entrada
 * @param dst IVC* -> apontador de estrutura IVC de imagem de saida
 * @param kernelErode int -> tamanho do kernel a ser aplicado na operacao de erosao
 * @param kernelDilate int -> tamanho do kernel a ser aplicado na operacao de dilatacao
 * @return void
 */
int vc_binary_open(IVC* src, IVC* dst, int kernelErode, int kernelDilate);

/**
 * @brief operação morfologica de erosao. Reduz tamanho e elimina excessos atraves da conversao dos valores
 * dos pixels de bordas da imagem em 255.
 * @param src IVC* -> apontador de estrutura IVC de imagem de entrada
 * @param dst IVC* -> apontador de estrutura IVC de imagem de saida
 * @param kernel int -> tamanho do kernel a ser aplicado na operacao de erosao
 * @return void
 */
void vc_binary_erode(IVC* src, IVC* dst, int kernel);

/**
 * @brief operação morfologica de dilatacao. Expande as imagems e elimina ruidos
 * (internos e externos) atraves da conversao dos valores dos pixels de bordas da imagem em 0.
 * @param src IVC* -> apontador de estrutura IVC de imagem de entrada
 * @param dst IVC* -> apontador de estrutura IVC de imagem de saida
 * @param kernel int -> tamanho do kernel a ser aplicado na operacao de dilatacao
 * @return void
 */
void vc_binary_dilate(IVC* src, IVC* dst, int kernel);

/**
 * @brief Percorremos as duas estruturas de imagem e compar pixel a pixel,
 * todos os pixels com valor zero na imagem tratada são passados a zep na imagem original,
 * os demais mantém seus valores originais.
 * @param src IVC* -> apontador de estrutura IVC de imagem de entrada (original)
 * @param dst IVC* -> apontador de estrutura IVC de imagem de tratada.
 * @return int -> contagem de pixels relativos a imagem resultante.
 */
int extractImage(IVC* src, IVC* dst);

/**
 * @brief Obtem tamanho do vetor de pixels da estrutura de imagem de entrada,
 * valor obtido atraves do calculo: heigh X width X channels
 * @param src IVC* -> apontador de estrutura IVC de imagem de entrada.
 * @return int -> tamanho do vetor de pixels.
 */
int getVectorSize(IVC* src);

/**
 * @brief dado um apontador para array de blobs (objectos), itera o array
 * imprimindo todas as informações relativas aos diversos campos de cada objeto.
 * @param blobs OVC* -> apontador para array de blobs (objectos).
 * @param nblobs int -> quantidade de blobs (tamanho do array).
 * @return void
 */
void printBlobsInfo(OVC* blobs, int nblobs);

/**
 * @brief dados uma estrura de imagem de entrada (a qual foi usada para tratamento e identificacao
 * de blobs) e quantidade de blobs identificados, percorre as linhas e colunas (array de pixels) da estrutura
 * de imagem e atribui o valor 255 aos pixels que estiverem dentro dos limites estabelecidos.
 * Para bonding box temos o a posicao inicial da linha e da coluna e sabemos a quantidade de pixels seguintes a converter o valor
 * através das propriedades do objeto, limitamos as colunas e linhas e convertemos as posicoes de interesse com valor 255
 * O mesmo é aplicado para os eixos dos centros de massa, obtemos a coluna e a linha e respeitando os limites da bounding boxes, converte-se
 * os pixels em valor 255 (para imagens coloridas, apenas um canal será convertido)
 * @param src IVC* -> apontador de estrutura IVC de imagem de entrada/saida.
 * @param blobs OVC* -> apontador para array de blobs (objectos).
 * @param nblobs int -> quantidade de blobs (tamanho do array).
 * @return void
 */
Coord* drawBlobBox(IVC* src, OVC blobs);


void vc_rgb_to_hsv(IVC* src);

float getHue(float red, float green, float blue, float max, float min);

void vc_hsv_segmentation(IVC* src, int hmin, int hmax, int smin, int smax, int vmin, int vmax);

void rgb2bgrinvert(IVC* src);


float vc_rgb_max(int r, int g, int b);

float vc_rgb_min(int r, int g, int b);

int vc_rgb_to_hsv2(IVC* srcdst);