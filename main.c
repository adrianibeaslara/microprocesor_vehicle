/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

int usart_on; //para saber si estamos ejecutando un comando de la usart
uint8_t texto[7] = "      ";
int comando_usart = 0 ;
int recepcion_ya_atendida = 0;

int medicion_tiempo_1;
int medicion_tiempo_2;

int nueva_distacia_medida;//para la distancia
unsigned int distancia;//para la distancia

unsigned short valor_adc = 0; //para el potenciometro

unsigned  int DC = 100; //velocidad ruedas

int valor_trigger;//para trigger sensor ultrasonidos

int velocidad_max_mitad = 0;//para reducir la veloxidad a la mitad con el potenciometro

int estado ;

int tiempo_inicio_espera;     //esperas toc
int tiempo_final_espera;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void zumbador_intermitente();
void zumbador_on();
void zumbador_off();

void avanzar_alante();
void retroceder_atras();
void rueda_izquierda_alante();
void rueda_izquierda_atras();
void rueda_derecha_alante();
void rueda_derecha_atras();


unsigned int DC_en_funcion_de_Distancia(unsigned int distancia);

void calculo_velocidad_alante(int v);
void calculo_velocidad_atras(int v);

void evaluar_potencia_maxima();// con el uso de el potenciometro

void tiempo_de_secuencia(int tiempo);

void TIM2_IRQHandler(void);
void TIM3_IRQHandler(void);
void TIM4_IRQHandler(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//* * * * * * * * FUNCIONES ZUMBADOR * * * * * * * * * * * * * * * * *
void zumbador_intermitente(){
	TIM2->CCMR1 = 0x3000;     //para el toggle
}

void zumbador_on(){
	TIM2->CCMR1 = 0x4000;    //forzar 0
}

void zumbador_off(){
	TIM2->CCMR1 = 0x5000;  //forzar 1
}


unsigned int DC_en_funcion_de_Distancia(unsigned int x) {
	 float proporcion = (x - 10.0f) / 10.0f;
	    float dc = 98.0f-proporcion*(98.0f - 2.0f); // va de 98 a 2

	    //lo redondeo para que sea un entero
	    unsigned int dc_final = (unsigned int)(dc + 0.5f); //lo convierto en unsigned

	    return dc_final; // el dc para poner en los motores de las ruedas
}

void calculo_velocidad_atras( int v){
	DC = v;

	if(velocidad_max_mitad == 1){
		DC -= 50;  //bajas el duty cycle porque es hacia atras
	}
	 // Se lo paso a las ruedas
	 TIM4->CCR3 = DC;
	 TIM4->CCR4 = DC;
}

void calculo_velocidad_alante( int v){
	DC = v;

	if(velocidad_max_mitad == 1){
		DC += 50;
	}
    // Se lo paso a las ruedas
	 TIM4->CCR3 = DC;
	 TIM4->CCR4 = DC;
}


void evaluar_potencia_maxima(){

	valor_adc = ADC1->DR; // guardo el valor del ADC

     if(valor_adc<31){ //valor medio ( 0 - 31 ) o ( 32 - 64)
    	 velocidad_max_mitad = 1;  //menos de la mitad --> velocidad max a la mitad
     }else{
    	 velocidad_max_mitad = 0; //mas de la mitad --> velocidad max al maximo
     }
}

void avanzar_alante(){
	GPIOA ->BSRR |= (1 << (11));//0
	GPIOA ->BSRR |= (1 << (12));//0

}



void retroceder_atras(){
	GPIOA ->BSRR |= (1 << (11+16));//0
	GPIOA ->BSRR |= (1 << (12+16));//0

}


void tiempo_de_secuencia(int tiempo){
	tiempo_inicio_espera = TIM3->CNT; // mide comienzo
	tiempo_final_espera = tiempo_inicio_espera + tiempo;
	if(tiempo_final_espera >65535){tiempo_final_espera=  65535;}
	while(TIM3->CNT<tiempo_final_espera);

	tiempo_inicio_espera = 0;
		tiempo_final_espera = 0;
}




void rueda_izquierda_alante(){
	GPIOA ->BSRR |= (1 << (11));//0
	GPIOA ->BSRR |= (1 << (12));//0

	DC = 1; //izquierda a tope

		if(velocidad_max_mitad == 1){
				DC = 50;  // A D C
			}

			 TIM4->CCR3 = DC; //aqui
			 TIM4->CCR4 = 99; // esta esta parada
}

void rueda_derecha_alante(){
	GPIOA ->BSRR |= (1 << (11));//0
	GPIOA ->BSRR |= (1 << (12));//0

	DC = 1; //derecha a tope

	if(velocidad_max_mitad == 1){
			DC = 50; // A D C
		}

		 TIM4->CCR3 = 99; // esta esta parada
		 TIM4->CCR4 = DC; // aqui
}

void rueda_izquierda_atras(){
	GPIOA ->BSRR |= (1 << (11+16));//1
	GPIOA ->BSRR |= (1 << (12+16));//1

	DC = 99; // a tope

		if(velocidad_max_mitad == 1){
				DC = 50; // A D C
			}

			 TIM4->CCR3 = DC; //aqui
			 TIM4->CCR4 = 1; // paro
}

void rueda_derecha_atras(){
	GPIOA ->BSRR |= (1 << (11+16));//0
	GPIOA ->BSRR |= (1 << (12+16));//0
	DC = 99; // a tope

			if(velocidad_max_mitad == 1){
					DC = 50; // A D C
				}

				 TIM4->CCR3 = 1; //paro
				 TIM4->CCR4 = DC; //aqui
}

void stop(){
	GPIOA ->BSRR |= (1 << (11));//0
	GPIOA ->BSRR |= (1 << (12));//0
     //paro ambos
	 TIM4->CCR3 = 99;
	 TIM4->CCR4 = 99;
}
//* * * * * * * * * * T I M E R   2 * * * * * * * * * * * * * * * * * *

void TIM2_IRQHandler(void) {
	if ((TIM2->SR & 0x0002)!=0) { //CANAL 1 ( medir distancia)  TIC

		 if ((GPIOA->IDR&0x00000020)!=0) { //rising edge
			medicion_tiempo_1 = TIM2->CCR1; // tiempo cuando  comienzo
		 }
		 else{ //falling edge
		 	 medicion_tiempo_2 = (TIM2->CCR1)-medicion_tiempo_1; //diferencia de tiempo
		 	 if (medicion_tiempo_2<0) medicion_tiempo_2 += 0xFFFF; //en caso de que se desborde
		 	 distancia= medicion_tiempo_2/58; // calculo distancia
		 	 nueva_distacia_medida=1; // para el main
		 	}
		 TIM2->SR &= ~0x02; //borrar flag
		 }


	 if((TIM2->SR & 0x04) == 0x04){//CANAL 2 (para el zumbador intermitente) TOC+TOGGLE
		TIM2->CCR2 += 64000;        //+ 250 ms
		TIM2->SR &= ~0x04;     //borrar flag
	    }


   if((TIM2->SR & 0x08) == 0x08){//CANAL 3  ( trigger del ultrasonidos)  TOC
	   GPIOD -> BSRR |= (1<<(2+16));//0 trigger
	   valor_trigger = 0;
		TIM2->SR &= ~0x08;     //borrar flag
	    }







   if ((TIM2->SR & 0x10) == 0x10) { // Interrupcion CH4

       switch (estado) {
           case 1:
               rueda_derecha_atras();
               estado = 2;
               TIM3->CCR4 = (TIM3->CNT + 10000); // 500ms
               break;
           case 2:
               if (distancia <= 20) {
                   stop();
                   rueda_derecha_alante();
                   estado = 3;
                   TIM3->CCR4 = (TIM3->CNT + 10000); // 500ms
               } else {
                   estado = 0;
               }
               break;
           case 3:
               if (distancia <= 20) {
                   rueda_derecha_alante();
                   estado = 4;
                   TIM3->CCR4 = (TIM3->CNT + 10000); // 500ms
               } else {
                   estado = 0;
               }
               break;
           case 4:
               if (distancia <= 20) {
                   rueda_izquierda_atras();
                   estado = 5;
                   TIM3->CCR4 = (TIM3->CNT + 10000); // 500ms
               } else {
                   estado = 0;
               }
               break;
           case 5:
               if (distancia <= 20) {
                   rueda_izquierda_atras();
                   estado = 0;
                   TIM3->CCR4 = (TIM3->CNT + 10000); // 500ms
               } else {
                   estado = 0;
               }
               break;
           default:
               break;
       }
       TIM2->SR &= ~0x10; // clear flag
   }
  /* if((TIM2->SR & 0x10) == 0x10){//interrupcion CH4

	   if(estado == 1){
		   turn_right_backwards();
		   estado =2;
		   TIM3->CCR4 += 500; //500ms

	   }
	   else if(estado == 2){
		   if(distancia<=20){
		  stop();
		  turn_right_forward();
		  estado =3;
		  TIM3->CCR4 += 500; //500ms
		   }else{
			   estado =0;
		   }
	   }
	   else if(estado == 3){
	   		   if(distancia<=20){
	   			turn_right_forward();
	   		   estado =4;
	   		   TIM3->CCR4 += 500; //500ms
	   		   }else{
	   			   estado =0;
	   		   }
	   	   }
	   else if(estado == 4){
		   if(distancia<=20){
		   turn_left_backwards();
		   estado =5;
		   TIM3->CCR4 += 500; //500ms
		   }else{
			   estado =0;
		   }
	   }
	   else if(estado == 5){
		   if(distancia<=20){
		   turn_left_backwards();
		   TIM3->CCR4 += 500; //500ms
		   estado =0;
		   }else{
			   estado =0;
		   }
	   }



  		TIM2->SR &= ~0x10;     //clear flag
  	    }
    */

   /*
     				 if(distancia<=10){
     			      turn_right_backwards();
     		        	wait3(500);
     					estado =1;
     				 }

     			          if(distancia<=20){
     						 stop();
     						 wait3(500);
     				         turn_right_forward();
     				         wait3(500);
     				         estado =2;
     					                  }

     					  if(distancia<=20){
     	                    turn_left_backwards();
     		          	    wait3(500);
     		                estado =3;
     					                   }

     					   if(distancia<=20 ){
     					     turn_left_backwards();
     				         wait3(500);
     				         estado =0;
   	                                     }*/

	  }



//* * * * * * * * TIMER 3 * * * * * * * * * * * * * * * * *

void TIM3_IRQHandler(void){
	if ((TIM3->SR & 0x0002)!=0){ // CANAL 1
			GPIOD -> BSRR |= (1<<2);//1 trigger
			valor_trigger = 1;
			TIM3->CCR1 = TIM3->CNT + 300;// (300 ms)
			TIM2->CCR3 = TIM2->CNT + 13;//  (13 us)
		}
	TIM3->SR = 0x0000;  //borrar flag
	}

//* * * * * * * * TIMER 4 * * * * * * * * * * * * * * * * *





/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  // * * * * * * * * * * * * * * * * * * * C O N F I G U R A C I O N     D E     P I N E S* * * * * * * * * * * * * * * * * * * * *



  //* * * M O T O R E S   ( R U E D A S ) * * * * * * * * * * * * * * * * * * * * * *


  //Rueda izquierda PB8,PA11
   // corresponde a IN1 PB8
       GPIOB-> MODER &= ~(0x03 <<(2*8));
       GPIOB-> MODER |= (1<<(8*2+1));
       GPIOB->AFR[1]|=(0x02 << (0*4));
    //atras
       GPIOA->MODER &= ~(1 << ((11*2)+1));//0
       GPIOA->MODER |= (1 << (11*2));//1
   //rueda derecha PB9,PA12
       // funcion alterna ( PWM ) al IN3 PB9
       	GPIOB-> MODER &= ~(0x03 <<(2*9));
       	GPIOB-> MODER |= (1<<(9*2+1));
           GPIOB->AFR[1]|=(0x02 <<(1*4));
    //hacia atras
       GPIOA->MODER &= ~(1 << ((12*2)+1));//0
       GPIOA->MODER |= (1 << (12*2));//1



  //* * *  Z U M B A D O R * * * * * * * * * * * * * * * * * * * * * *

      GPIOA->MODER&=~(0x03 <<(2*1)); //00
      GPIOA->MODER|=(1<<3); //1
      GPIOA->AFR[0]|=(0x01<<(1*4));

//* * * P D 2    U L T R A S O N I D O S   T R I G G E R * * * * * * * * * * * * * * * * * * * * * *
    GPIOD->MODER &=~(0x03<<(2*2)); //00
    GPIOD->MODER |=(1<<(4));

//* * * P C 1 4   T I M 3     U L T R A S O N I D O S    T R I G G E R * * * * * * * * * * * * * * * * * * * * * *
    GPIOC->MODER&=~(0x03 <<(2*14)); //00
    GPIOC->MODER|=(1<<(29)); //1

    GPIOC->AFR[1]|=(0x02 <<(6*4));

//* * * P A 5     U L T R A S O N I D O S     E C H O * * * * * * * * * * * * * * * * * * * * * *
    GPIOA->MODER&=~(0x03 <<(2*5));//00
    GPIOA->MODER|=(1<<(11));
    GPIOA->AFR[0]|=(0x01 <<(5*4));


    //* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    //* * * * * *  * *  * *  * *  * *  * *[ T I M E R      2 ]* * * * *  * * * ** * * * * * * * * * * *
    //* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


    //* * * PD2 TIM2 ULTRASONIDOS ECHO * * * * * * * * * * * * * * * * * * * * * *
    TIM2->CR1 |= 0x0001;
    TIM2->CR2 = 0;
    TIM2->SMCR = 0;
    TIM2->PSC = 31;
    TIM2->CNT = 0;
    TIM2->ARR = 0xFFFF;
    TIM2->DIER = 0x001E;//habilita 4 canales //:
    TIM2->CCMR1 = 0x5001;
    TIM2->CCMR2 = 0x0000;
    TIM2->CCER = 0x011B; // flanco de subida y bajada
    TIM2->CCR1=10;
    TIM2->CCR2=64000;
    TIM2->CCR3=13;
    TIM4->EGR |= 0x01;
    TIM2->SR = 0;
    NVIC->ISER[0] |= (1 << 28);


  	  /* * * * TOC  DEL TRIGGER * * * */
      // * * * * TENEMOS QUE ENVIAR UNA SEÑAL CADA 10us * * * *



    //* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    //* * * * * *  * *  * *  * *  * *  * *[ T I M E R      3 ]* * * * *  * * * ** * * * * * * * * * * *
    //* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

  	  TIM3->CR1 = 0;
  	  TIM3->CR2 = 0;
  	  TIM3->SMCR = 0;
  	  TIM3->CCMR1=0;
  	  TIM3->CCMR2=0;
  	  TIM3->CCER = 0;
  	  TIM3->PSC = 31999; //1ms
  	  TIM3->EGR=0;//TOC
  	  TIM3->ARR = 0xFFFF;
  	  TIM3->CCR1 += 300; //300ms
  	  TIM3-> CNT = 0;
  	  TIM3->DIER |= 2;
  	  TIM3->CR1|=1;
  	  NVIC->ISER[0] |= (1 << 29);



  	//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   //* * * * * *  * *  * *  * *  * *  * *[ T I M E R      4 ]* * * * *  * * * ** * * * * * * * * * * *
   //* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

  	/* * * * P W M     R U E D A S      D E L       C O C H E  * * * */



          	  TIM4->CR1 = 0x0080;
        	  TIM4->CR2 = 0x0000;
          	  TIM4->SMCR = 0x0000;
          	  TIM4->PSC = 3199;
          	  TIM4->CNT = 0;
          	  TIM4->ARR = 99;
          	  TIM4->CCR3 = DC; //set signal DC in the channel we are using
          	  TIM4->CCR4 = DC; //set signal DC in the other channel we are using
          	  TIM4->DIER = 0x0000;
          	  TIM4->CCMR2 = 0x6868; //quiero habilitar canales 3 y 4
          	  TIM4->CCER = 0x1100; //canales 3 y 4
          	  TIM4->CR1 |= 0x0001; //counter enable
          	  TIM4->EGR |= 0x0001; // UG
          	  TIM4->SR = 0;

          	/* * * * P O T E N C I O M E T R O     P A 4 * * * */

          	  // Configuración ADC
          	  GPIOA->MODER |= 0x00000300;    // PA4 como analógico
          	  ADC1->CR2 &= ~(0x00000001);    // ADON = 0 (ADC apagado)
          	  ADC1->CR1 = 0x03000000;        // RES = 00 (resolución = 12 bits)
          	                                 // SCAN = 0 (modo scan deshabilitado)
          	                                 // EOCIE = 0 (deshabilitada la interupción por
          	 // EOC)

          	  ADC1->CR2 = 0x00000412;        // EOCS = 1 (activado el bit EOC al acabar cada
          	 // conversión)
          	                                 // DELS = 001 (retardo de la conversión hasta
          	 // que se lea el dato anterior)
          	                                 // CONT = 1 (conversión continua)
          	  ADC1->SMPR1 = 0;               // Sin sampling time (4 cycles)
          	  ADC1->SMPR2 = 0;
          	  ADC1->SMPR3 = 0;
          	  ADC1->SQR1 = 0x00000000;       // 1 elemento solo en la secuencia
          	  ADC1->SQR5 = 0x00000004;       // El elemento es el canal AIN4
          	  ADC1->CR2 |= 0x00000001;       // ADON = 1 (ADC activado)

          	  while ((ADC1->SR&0x0040)==0);  // Si ADCONS = 0, o sea no estoy para convertir,espero


          	  ADC1->CR2 |= 0x40000000;       // Si ADCONS = 1, arranco la conversion (SWSTART
          	 // = 1)





            	/* * * * U S A R T  * * * */

       	HAL_UART_Receive_IT(&huart1, texto, 1);
        //   	HAL_UART_Receive_IT(&huart2, texto, 1);




          	  avanzar_alante();
           	calculo_velocidad_alante(1);


          	//  move_backwards();
  	//* * * I N I C I A L I Z O    V A R I A B L E S  * * * * * * * * * * * * * * * * * * * * * *
  	 medicion_tiempo_1 = 0;
  	 medicion_tiempo_2 = 0;
  	 nueva_distacia_medida = 0;
  	 distancia = 0;
  	 valor_trigger = 0;
     valor_adc = 0;
     usart_on = 0;
     estado = 0;

  	  /* * * * * * * * * * * * * * * */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  	while (1)
  	  {

  	 bnv	evaluar_potencia_maxima(); // pase lo que pase atendemos al potenciometro


  		if(usart_on == 1){
  			if (recepcion_ya_atendida == 0){
  			switch(comando_usart) {
  			  case 1:
  			    HAL_UART_Transmit(&huart1, "Voy hacia delante\n  ", 19, 10000);
  			    usart_on = 1;
  			    recepcion_ya_atendida = 1; // para que no entre todo el rato en la misma accion mientras esta activa
  			    calculo_velocidad_alante(1);
  			    avanzar_alante();
  			    break;
  			  case 2:
  			    HAL_UART_Transmit(&huart1, "Voy hacia atras\n  ", 17, 10000);
  			    usart_on = 1;
  			  recepcion_ya_atendida = 1; // para que no entre todo el rato en la misma accion mientras esta activa
  			    retroceder_atras();
  			    calculo_velocidad_atras(99);
  			    break;
  			  case 3:
  			    HAL_UART_Transmit(&huart1, "Voy a la izquierda\n  ", 20, 10000);
  			    usart_on = 1;
  			  recepcion_ya_atendida = 1; // para que no entre todo el rato en la misma accion mientras esta activa
  			    calculo_velocidad_alante(1);
  			    rueda_derecha_alante();
  			    break;
  			  case 4:
  			    HAL_UART_Transmit(&huart1, "Giro a la derecha\n  ", 19, 10000);
  			    usart_on = 1;
  			  recepcion_ya_atendida = 1; // para que no entre todo el rato en la misma accion mientras esta activa
  			    calculo_velocidad_alante(1);
  			    rueda_izquierda_alante();
  			    break;
  			  case 5:
  			    HAL_UART_Transmit(&huart1, "Me paro\n  ", 9, 10000);
  			    usart_on = 1;
  			  recepcion_ya_atendida = 1; // para que no entre todo el rato en la misma accion mientras esta activa
  			    stop();
  			    break;
  			  case 6:
  			    HAL_UART_Transmit(&huart1, "vuelvo al modo autonomo\n  ", 25, 10000);
  			    usart_on = 0;
  			  recepcion_ya_atendida = 1; // para que no entre todo el rato en la misma accion mientras esta activa
  			    //modo automatico
  			    break;
  			  default:

  			    break;
  			}
  		}
  		}





  		if(usart_on == 0){  // para que no entre si le estoy enviando una orden


  		  if(nueva_distacia_medida==1){                   //Distancia minima
  			  if(distancia<=10  ){
  				  zumbador_on();


/*
          if(estado==0){
              move_forward();
        	  velocidad_alante(99);
	          estado =1;
	          TIM3->CCR4 += 10000; //5000ms

               }

*/



  				 if(distancia<=10){ //caso 1 distancia pequeña
  			      rueda_derecha_atras();
  		        	tiempo_de_secuencia(500); //espera
  					estado =1;
  				 }

  			          if(distancia<=20){
  						 stop();
  						 tiempo_de_secuencia(500);//Espera
  				         rueda_derecha_alante();
  				         tiempo_de_secuencia(500);//Espera
  				         estado =2;
  					                  }

  					  if(distancia<=20){
  	                    rueda_izquierda_atras();
  		          	    tiempo_de_secuencia(500);//Espera
  		                estado =3;
  					                   }

  					   if(distancia<=20 ){
  					     rueda_izquierda_atras();
  				         tiempo_de_secuencia(500);//Espera
  				         estado =0;
	                                     }


  				  nueva_distacia_medida=0;

  			  }else if(distancia>10 && distancia<=20){//caso 2 Distancia media
  				  avanzar_alante();
  				  zumbador_intermitente();
  				  DC = DC_en_funcion_de_Distancia(distancia); // relacciona la distancia con el DC
  				  calculo_velocidad_alante(DC);
  				  nueva_distacia_medida=0;

  			  }else{// caso 3 Distancia maxima
  				 avanzar_alante();
  				  zumbador_off();
  				  calculo_velocidad_alante(1); // a tope
  				  nueva_distacia_medida=0;  //libero esta variable
  			  }
  		  }
  		}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc.Init.LowPowerAutoWait = ADC_AUTOWAIT_DISABLE;
  hadc.Init.LowPowerAutoPowerOff = ADC_AUTOPOWEROFF_DISABLE;
  hadc.Init.ChannelsBank = ADC_CHANNELS_BANK_A;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.NbrOfConversion = 1;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = DISABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_4CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65535;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PD2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	//esto actualiza variables que seran evaluadas en el while

	if(texto[0] =='1'){
		comando_usart = 1; //para saber que accion realizar
		usart_on = 1;     //no entra en el bucle
		recepcion_ya_atendida = 0; //para que en el while se busque realizar una nueva accion
	}else if(texto[0] == '2' ){
		comando_usart = 2;
		usart_on = 1;     //no entra en el bucle
		recepcion_ya_atendida = 0; //para que en el while se busque realizar una nueva accion
	}else if(texto[0] == '3' ){
		comando_usart = 3; //para saber que accion realizar
		usart_on = 1;     //no entra en el bucle
		recepcion_ya_atendida = 0; //para que en el while se busque realizar una nueva accion
	}else if(texto[0] == '4' ){
		comando_usart = 4; //para saber que accion realizar
		usart_on = 1;     //no entra en el bucle
		recepcion_ya_atendida = 0;//para que en el while se busque realizar una nueva accion
	}else if(texto[0] == '5' ){
		comando_usart = 5; //para saber que accion realizar
		usart_on = 1;     //no entra en el bucle
		recepcion_ya_atendida = 0;//para que en el while se busque realizar una nueva accion
	}else if(texto[0] == '6' ){
		comando_usart = 6;  //para saber que accion realizar
		 usart_on = 0;  //ahora si entra en el modo autonomo
			recepcion_ya_atendida = 0; //para que en el while se busque realizar una nueva accion
		//modo automatico
	}
	HAL_UART_Receive_IT(huart, texto, 1); // Vuelve a activar Rx por haber acabado el buffer

}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
