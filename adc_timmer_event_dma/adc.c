#include "stm32f4xx.h"                  // Device header

#define NUM_AMOST 4096

void ADC_IRQHandler(void);
void DMA2_Stream0_IRQHandler(void);

static inline void startProcess(void);
static inline void stopProcess(void);

ADC_TypeDef *adc = ADC3;
GPIO_TypeDef *gpio = GPIOA;
volatile uint32_t data[NUM_AMOST];
volatile uint32_t size = 0;
volatile uint32_t overrun = 0;

int main(void)
{
	//led cfg
 	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
	GPIOD->MODER |= GPIO_MODER_MODER12_0|GPIO_MODER_MODER13_0;//GPIO_MODER_MODER14_0|GPIO_MODER_MODER15_0; //12gr 13og 14rd 15bl
	GPIOD->ODR &= ~(GPIO_ODR_OD12|GPIO_ODR_OD13);//|GPIO_ODR_OD14|GPIO_ODR_OD15);
# if 1
	//dma cfg
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
	DMA2_Stream0->CR &= ~DMA_SxCR_EN;
	DMA2_Stream0->NDTR = NUM_AMOST;
	DMA2_Stream0->PAR = (uint32_t)((uint32_t *)&adc->DR);
	DMA2_Stream0->M0AR = (uint32_t)((uint32_t *)&data[0]);
	DMA2_Stream0->CR = (2 << DMA_SxCR_CHSEL_Pos)|//Channel selection = 2 (for adc3)
										 (0 << DMA_SxCR_PL_Pos)|//Priority level high
										 (0 << DMA_SxCR_DBM_Pos)|//double mode disable (nao suporta transferencias memory-memory)
										 (2 << DMA_SxCR_MSIZE_Pos)|//Memory data size = word (32-bit)
										 (2 << DMA_SxCR_PSIZE_Pos)|//Peripheral data size = word (32-bit)
										 (1 << DMA_SxCR_MINC_Pos)|//Memory increment mode = Memory address pointer is incremented after each data transfer(depends on Memory data size)
										 (0 << DMA_SxCR_PINC_Pos)|//Peripheral increment mode = disable
									   (0 << DMA_SxCR_CIRC_Pos)|//Circular mode Off
										 (0 << DMA_SxCR_DIR_Pos)|//Data transfer direction = Peripheral-to-memory
										 (1 << DMA_SxCR_TCIE_Pos);//Transfer complete interrupt able
	NVIC_EnableIRQ(DMA2_Stream0_IRQn);
	
#endif
	//adc pin cfg
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	GPIOA->MODER |= (3 << GPIO_MODER_MODE0_Pos);//PA 0 analog mode
	
	//adc cfg
	RCC->APB2ENR |= RCC_APB2ENR_ADC3EN;
	adc->CR2 |= ADC_CR2_ADON; 
	adc->CR1 |= (0 << ADC_CR1_RES_Pos)| //ADC 12BITS
							(1 << ADC_CR1_DISCEN_Pos);//| //Discontinuous mode on regular channels
//							(1 << ADC_CR1_OVRIE_Pos)|
//							(1 << ADC_CR1_EOCIE_Pos); // INTERRUPT ABLE
	adc->SQR1 |= (0 << ADC_SQR1_L_Pos); //1 CONV
	adc->SQR3 |= (0 << ADC_SQR3_SQ1_Pos); //CHANEL 0
	adc->SMPR2 = (7 << ADC_SMPR2_SMP1_Pos); //SAMMPLING TIME 
	adc->CR2 |= (1 << ADC_CR2_EXTEN_Pos)| //EXTEN RISING EDGE
							(3 << ADC_CR2_EXTSEL_Pos)| //EXSEL TIMER CANAL 2 EVENT
//							(1 << ADC_CR2_EOCS_Pos)| // End of conversion selection = the EOC bit is set at the end of each regular conversion
							(1 << ADC_CR2_DMA_Pos);//Direct memory access mode ABLE
//	ADC123_COMMON->CCR |= ADC_CCR_TSVREFE;
//	NVIC_EnableIRQ(ADC_IRQn);
	
	//confg pin timmer 
	#if 1
	RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	gpio-> MODER |= GPIO_MODER_MODER1_1; //PA1 ALTERNATE FUNC
	gpio->AFR[0] |= 1 << GPIO_AFRL_AFSEL1_Pos; 
	gpio->OSPEEDR |= 3 << GPIO_OSPEEDR_OSPEED1_Pos;
	#endif
	
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	
	TIM2->PSC = 1000 - 1;//VALOR PELO QUAL O CLOCK INICIAL � DIVIDIDO
	TIM2->ARR = 16 - 1;
	
	TIM2->CCMR1 = (TIM2->CCMR1 & ~TIM_CCMR1_OC2M_Pos)| (6 << TIM_CCMR1_OC2M_Pos);
	TIM2->CR1 |= TIM_CR1_ARPE;
	TIM2->CCR2 = 8 - 1;//tempo alto
	TIM2->CCER = (TIM2->CCER & ~TIM_CCER_CC2E)| (1 << TIM_CCER_CC2E_Pos);

	startProcess();
	while(1);
}

static inline void startProcess(void)
{
	DMA2_Stream0->CR |= DMA_SxCR_EN;
	adc->CR2 |= ADC_CR2_SWSTART;
	
	TIM2->CR1 |= TIM_CR1_CEN;
	GPIOD->ODR |= GPIO_ODR_OD13;
}

static inline void stopProcess(void)
{
	DMA2_Stream0->CR &= ~DMA_SxCR_EN;
	adc->CR2 &= ~(ADC_CR2_SWSTART|ADC_CR2_ADON);
	TIM2->CR1 &= ~TIM_CR1_CEN;
	GPIOD->ODR ^= GPIO_ODR_OD13;
}

#if 0
void ADC_IRQHandler(void)
{
	if(ADC123_COMMON->CSR & ADC_CSR_EOC3)
	{
		GPIOD->ODR ^= GPIO_ODR_OD12;
	}
	if(adc->SR & ADC_SR_OVR)
	{
		overrun++;
	}
	size++;
}
#endif
void DMA2_Stream0_IRQHandler(void)
{
	if(DMA2->LISR & DMA_LISR_TCIF0)
	{
		DMA2->LIFCR |= DMA_LIFCR_CTCIF0;
		stopProcess();
	}
}
