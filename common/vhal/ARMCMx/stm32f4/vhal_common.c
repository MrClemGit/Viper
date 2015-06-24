#include "vhal_common.h"


//RM rcc chapter, register APB1ENR and APB2ENR
void rcc_enable_tim(int x){
	x++;
	if (x>=2 && x<=5){
		//TIM2..5
		RCC->APB1ENR |= (1<<(x-2));
	} else if(x==1){
		RCC->APB2ENR |= 1;
	}else if (x>=9 && x<=11){
		//TIM9.11
		RCC->APB2ENR |= (1<<(16+x-9));
	}
}

void rcc_disable_tim(int x){
	x++;
	if (x>=2 && x<=5){
		//TIM2..5
		RCC->APB1ENR &= ~(1<<(x-2));
	} else if(x==1){
		RCC->APB2ENR &= ~1;
	}else if (x>=9 && x<=11){
		//TIM9.11
		RCC->APB2ENR &= ~(1<<(16+x-9));
	}
}
