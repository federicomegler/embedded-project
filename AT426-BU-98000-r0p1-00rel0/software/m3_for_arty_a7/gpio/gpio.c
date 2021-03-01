/*
 * Copyright:
 * ----------------------------------------------------------------
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 *   (C) COPYRIGHT 2014, 2018 ARM Limited
 *       ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 * ----------------------------------------------------------------
 * File:     gpio.c
 * Release Information : Cortex-M3 DesignStart-r0p1-00rel0
 * ----------------------------------------------------------------
 *
 */

/*
 * --------Included Headers--------
 */

#include "gpio.h"
#include "xparameters.h"        // Project memory and device map
#include "xgpio.h"              // Xilinx GPIO routines
#include "peripherallink.h"     // IRQ definitions
#include <string.h>
#include "core_cm3.h"

/************************** Variable Definitions **************************/
/*
 * The following are declared static to this module so they are zeroed and so they are
 * easily accessible from a debugger
 *
 * Also they are initialised in main, but accessed by the IRQ routines
 */
 
static XGpio Gpio_Led_DIPSw;   /* The driver instance for GPIO Device 0 */
static XGpio Gpio_RGBLed_PB;   /* The driver instance for GPIO Device 1 */
static XGpio Gpio_DAPLink;     /* The driver instance for the DAPLink GPIO */

//Global Variables

char word[200] = "";
char encoded_sentence[200] = "";
char character[5] = "";
int iterator=0;
uint32_t lastButton;
int isLong = 0;
int begin = 0;


/*****************************************************************************/

// Initialise the GPIO and zero the outputs
int InitialiseGPIO( void )
{
    // Define local variables
    int status;

    /*
     * Initialize the GPIO driver so that it's ready to use,
     * specify the device ID that is generated in xparameters.h
    */
    status = XGpio_Initialize(&Gpio_Led_DIPSw, XPAR_AXI_GPIO_0_DEVICE_ID);
    if (status != XST_SUCCESS)  {
        return XST_FAILURE;
    }

    status = XGpio_Initialize(&Gpio_RGBLed_PB, XPAR_AXI_GPIO_1_DEVICE_ID);
    if (status != XST_SUCCESS)  {
        return XST_FAILURE;
    }

    status = XGpio_Initialize(&Gpio_DAPLink, XPAR_DAPLINK_IF_0_AXI_GPIO_0_DEVICE_ID);
    if (status != XST_SUCCESS)  {
        return XST_FAILURE;
    }

    // GPIO0
    // Port0 drives led_4bits.  Set bottom 4 UART ports to be outputs.
    XGpio_SetDataDirection(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0xFFFFFFF0);
//    ARTY_A7_GPIO0->TRI0 = 0xfffffff0;

    // Port 1 inputs the dip switches. Set to be inputs
    XGpio_SetDataDirection(&Gpio_Led_DIPSw, ARTY_A7_DIP_CHANNEL, 0xFFFFFFFF);
//    ARTY_A7_GPIO0->TRI1 = 0xffffffff;

    // GPIO1
    // Port0 drives led_rgb.  Set 12 UART ports to be outputs.
    XGpio_SetDataDirection(&Gpio_RGBLed_PB, ARTY_A7_RGB_CHANNEL, 0xfffff000);
//    ARTY_A7_GPIO1->TRI0 = 0xfffff000;
    // Port 1 inputs the push button switches. Set to be inputs
    XGpio_SetDataDirection(&Gpio_RGBLed_PB, ARTY_A7_PB_CHANNEL, 0xffffffff);
//    ARTY_A7_GPIO1->TRI1 = 0xffffffff;
    
    // DAPLink GPIO
    // Single channel
    XGpio_SetDataDirection(&Gpio_DAPLink, ARTY_A7_DAPLINK_GPIO_CHANNEL, 0x00000000);


    // Default value of LEDs
    XGpio_DiscreteWrite(&Gpio_RGBLed_PB, ARTY_A7_RGB_CHANNEL, 0x0);
//    ARTY_A7_GPIO1->DATA0 = 0x0;
    
    // Write 0xA to LEDs
    XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x0);
//    ARTY_A7_GPIO0->DATA0 = 0x0;

    return XST_SUCCESS;
    
}

// Set GPIO interrupts
void EnableGPIOInterrupts( void )
{
    // Push buttons and DIP switches are on Channel 2
    XGpio_InterruptEnable(&Gpio_RGBLed_PB, XGPIO_IR_CH2_MASK);
    XGpio_InterruptEnable(&Gpio_Led_DIPSw, XGPIO_IR_CH2_MASK);

    // Having enabled the M1 to handle the interrupts, now enable the GPIO to send the interrupts
    XGpio_InterruptGlobalEnable(&Gpio_RGBLed_PB);
    XGpio_InterruptGlobalEnable(&Gpio_Led_DIPSw);
}


// Define the GPIO interrupt handlers
void GPIO0_Handler ( void )
{
    volatile uint32_t gpio_dip_switches;

    // Read dip switches, change LEDs to match
    gpio_dip_switches = XGpio_DiscreteRead(&Gpio_Led_DIPSw, ARTY_A7_DIP_CHANNEL);   // Capture DIP status
    XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, gpio_dip_switches);   // Set LEDs

    // Clear interrupt from GPIO
    XGpio_InterruptClear(&Gpio_Led_DIPSw, XGPIO_IR_MASK);
    // Clear interrupt in NVIC
    NVIC_ClearPendingIRQ(GPIO0_IRQn);
}

/* Note : No interrupt handler for DAPLink GPIO, it does not have the IRQ line connected
          No requirement as it is only a toggle between QSPI XIP and QSPI normal controllers
          Instead, standard routine provided
*/

void SetDAPLinkQSPIMode( u32 mode )
{
    // Set the qspi_sel line
    XGpio_DiscreteWrite(&Gpio_DAPLink, ARTY_A7_DAPLINK_GPIO_CHANNEL, mode);
}

void IncLeds( void )
{
    // Routine to allow other blocks to indicate activity
    volatile uint32_t gpio_dip_switches;

    // Read dip switches, change LEDs to match
    gpio_dip_switches = XGpio_DiscreteRead(&Gpio_Led_DIPSw, ARTY_A7_DIP_CHANNEL);   // Capture DIP status
    XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, (gpio_dip_switches+1));   // Set LEDs
    
}

void SysTick_Handler(void){        
				isLong = 1;
}

void timer(){
	 uint32_t status = SysTick_Config(800);
	if(status == 1)
		XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x8);
}

void blink(int led){
		int i = 0;
    XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, led);
    for(i=0; i<100; ++i);
    XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x0);
		for(i=0; i<100; ++i);
}


void longDelay(){
	int i = 0;
	for(i=0; i < 120; i++);
}
void mediumDelay(){
	int i = 0;
	for(i=0; i < 60; i++);
}
void shortDelay(){
	int i = 0;
	for(i=0; i < 20; i++);
}

void GPIO1_Handler ( void )
{
	if(XGpio_DiscreteRead(&Gpio_RGBLed_PB, ARTY_A7_PB_CHANNEL) == 0x4){
			nextCharacter();	
	}
	else if(XGpio_DiscreteRead(&Gpio_RGBLed_PB, ARTY_A7_PB_CHANNEL) == 0x8){
			printWord();
	}
	else{
		if(begin == 1){
			begin = 0;
			if(isLong == 1){
					XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x02);
					longDelay();
					XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x00);	
					addSignal(isLong);
			}
			else{
					XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x01);
					shortDelay();
					XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x00);
					addSignal(isLong);
			}
		}
		//Start counting the length of the signal only if button 1 is high
		if(begin == 0 && XGpio_DiscreteRead(&Gpio_RGBLed_PB, ARTY_A7_PB_CHANNEL) == 0x1){
			isLong = 0;
			begin = 1;
			timer();
		}
	}
	
    // Clear interrupt from GPIO
    XGpio_InterruptClear(&Gpio_RGBLed_PB, XGPIO_IR_MASK);
    // Clear interrupt in NVIC
    NVIC_ClearPendingIRQ(GPIO1_IRQn);
}

//Insert the long/short signal into character[]
void addSignal(int isLong){
		if(isLong == 1){
			character[iterator] = 'l';
			++iterator;
		}
		else{
			character[iterator] = 's';
			++iterator;
		}
}

void nextCharacter(){
		codingMorse(character);
		for(iterator=0; iterator<5; ++iterator){
			character[iterator] = '\0';
		}
		iterator=0;
}

void printWord(){
		codingMorse(character);
		for(iterator=0; iterator<5; ++iterator){
			character[iterator] = '\0';
		}
		iterator=0;
		print(word);
}

uint32_t buttonCheck(int lastButton){
	uint32_t buttonStates;
	buttonStates = XGpio_DiscreteRead(&Gpio_RGBLed_PB, ARTY_A7_PB_CHANNEL);
	
	if(buttonStates != lastButton){
		switch(buttonStates)
		{
			case 0x01: // short signal
//					XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x01);
//					shortDelay();
//					XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x00);
					character[iterator] = 's';
					++iterator;
					//print("short");
					//mediumDelay();
			break;
			case 0x02: // long signal
//					XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x01);
//					mediumDelay();
//					XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x00);
					character[iterator] = 'l';
					++iterator;
				  //print("-");
					//shortDelay();
			break;
			case 0x04: // letter end
//					XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x04);
//					shortDelay();
//					XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x00);
					longDelay();
					decodingMorse(character);
					for(iterator=0; iterator<5; ++iterator){
						character[iterator] = '\0';
					}
					iterator=0;
			break;
			case 0x08: // word end
//				XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x08);
//				shortDelay();
//				XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x00);
					codingMorse(character);
					for(iterator=0; iterator<5; ++iterator){
								character[iterator] = '\0';
							}
					iterator=0;
					print(word);
			break;
				default:
					shortDelay();
				break;
			}
	}	
	return buttonStates; 
}

void encodingMorse(char *b){
	int internal_iterator = 0;
	strncpy(encoded_sentence,"",strlen(encoded_sentence)); // reset the string each time the encodingMorse function is called;
	for(int i = 0; i < strlen(b); i++){
		switch(b[i]){
			case 'a': 
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'b':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'c':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'd':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'e':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'f':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'g':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'h':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'i':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'j':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'k':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'l':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'm':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'n':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'o':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'p':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'q':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'r':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 's':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 't':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'u':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'v':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'w':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'x':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'y':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case 'z':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case '0':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case '1':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case '2':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case '3':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case '4':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case '5':
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case '6':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case '7':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case '8':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case '9':
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 'l';
				internal_iterator++;
				encoded_sentence[internal_iterator] = 's';
				internal_iterator++;
				encoded_sentence[internal_iterator] = ':';
				internal_iterator++;
				break;
			case ' ':
				encoded_sentence[internal_iterator] = '_';
				internal_iterator++;
				break;
		}
	}
}


void decodingMorse(char *b){
	if(b[0]=='s'){ //s****
		if(b[1]=='s'){ //ss***
			if(b[2]=='s'){ //sss**
			 	if(b[3]=='s'){ //ssss*
					if(b[4]=='s'){ //sssss=5        
					 word[strlen(word)] = '5';      
					}
					else if(b[4]=='l'){ //ssssl = 4
					 word[strlen(word)] = '4'; 
					}
					else word[strlen(word)] = 'h'; //ssss = h     
				}
				else if(b[3]=='l'){ //sssl*
				 if(b[4]=='s'){         
					 blink(4); //error
					}
					else if(b[4]=='l'){ //sssll = 3
					 word[strlen(word)] = '3'; 
					} //sssl = v
					else word[strlen(word)] = 'v';  
				} //sss**
				else word[strlen(word)] = 's'; 
		 } 
		 else if(b[2]=='l'){ //ssl**		
			if(b[3]=='s'){  //ssls*      
					if(b[4]=='s'){         
					 blink(4); //error      
					}
					else if(b[4]=='l'){
					 blink(4); //error
					}
					else word[strlen(word)] = 'f'; //ssls =  f     
				}
				else if(b[3]=='l'){ //ssll*
				 if(b[4]=='s'){         
					 blink(4); //error
					}
					else if(b[4]=='l'){
					 word[strlen(word)] = '2';  //sslll = 2
					}
					else blink(4); 
				}
				else word[strlen(word)] = 'u'; //ssl = u
		 }
		 else word[strlen(word)] = 'i'; //ss = i
	 }	 
	 else if(b[1]=='l'){ //sl***
			if(b[2]=='s'){ //sls**
				if(b[3]=='s'){ //slss*      
					if(b[4]=='s'){        
					 blink(4);      
					}
					else if(b[4]=='l'){
					 blink(4);
					}
					else word[strlen(word)] = 'l';  //slss = l     
				}
				else if(b[3]=='l'){ //slsl*
				 if(b[4]=='s'){         
					 blink(4); //error
					}
					else if(b[4]=='l'){
					 blink(4);
					}
					else blink(4); 
				}
				else word[strlen(word)] = 'r'; //sls
		 }
		 else if(b[2]=='l'){ //sll**
			if(b[3]=='s'){      
					if(b[4]=='s'){         
					 blink(4); //error      
					}
					else if(b[4]=='l'){
					 blink(4); //error
					}
					else word[strlen(word)] = 'p'; //slls = p     
				}
				else if(b[3]=='l'){ 
				 if(b[4]=='s'){         
					 blink(4); //error
					}
					else if(b[4]=='l'){
					 word[strlen(word)] = '1'; 
					}
					else word[strlen(word)] = 'j'; 
				}//sll
				else word[strlen(word)] = 'w'; 
			
		 }
		 else word[strlen(word)] = 'a'; 
		
	 }
	 else word[strlen(word)] = 'e'; 
	}
	else if(b[0]=='l'){
	 
	 if(b[1]=='s'){
		
			if(b[2]=='s'){
			 
				if(b[3]=='s'){        
					if(b[4]=='s'){         
					 word[strlen(word)] = '6';        
					}
					else if(b[4]=='l'){
					 blink(4);
					}
					else word[strlen(word)] = 'b';       
				}
				else if(b[3]=='l'){ 
				 if(b[4]=='s'){         
					 blink(4); //error
					}
					else if(b[4]=='l'){
					 blink(4);
					}
					else word[strlen(word)] = 'x'; 
				}
				else word[strlen(word)] = 'd';  
		 }
		 else if(b[2]=='l'){ //lsl
			
			if(b[3]=='s'){        
					if(b[4]=='s'){         
					 blink(4); //error      
					}
					else if(b[4]=='l'){
					 blink(4); //error
					}
					else word[strlen(word)] = 'c';       
				}
				else if(b[3]=='l'){ // lsll
				 if(b[4]=='s'){         
					 blink(4); //error
					}
					else if(b[4]=='l'){
					 blink(4);
					}
					else word[strlen(word)] = 'y';  
				}
				else word[strlen(word)] = 'k'; 
			
		 }
		 else word[strlen(word)] = 'n'; 
	 } 
	 else if(b[1]=='l'){ //l l 
		
			if(b[2]=='s'){ // l l s
			 
				if(b[3]=='s'){ // llss   
					if(b[4]=='s'){  //llsss      
					 word[strlen(word)] = '7';       
					}
					else if(b[4]=='l'){
					 blink(4);
					}
					else word[strlen(word)] = 'z';        
				}
				else if(b[3]=='l'){ //l l s l
				 if(b[4]=='s'){         
					 blink(4); //error
					}
					else if(b[4]=='l'){
					 blink(4);
					}
					else word[strlen(word)] = 'q';  
				}
				else word[strlen(word)] = 'g'; 		 
		 }
		 else if(b[2]=='l'){ // l l l	
			if(b[3]=='s'){      
				if(b[4]=='s'){         
				 word[strlen(word)] = '8';        
				}
				else if(b[4]=='l'){
				 blink(4); //error
				}
				else blink(4);       
			}
			else if(b[3]=='l'){ //llll
			 if(b[4]=='s'){         
				word[strlen(word)] = '9';  //error
				}
			else if(b[4]=='l'){
				 word[strlen(word)] = '0'; 
				}
				else blink(4); 
			}
			else word[strlen(word)] = 'o'; 		
		 }
		 else word[strlen(word)] = 'm'; 
	 }
	 else word[strlen(word)] = 't'; 
	}
	else blink(4);
}
