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

char word[20] = "";
char character[5] = "";



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

void GPIO1_Handler ( void )
{

    int mask, led_val, incr;
    volatile uint32_t gpio_push_buttons;
    volatile uint32_t gpio_leds_rgb;

    // For LEDs, cycle around colour each time respective push button is pressed
    // Only change if a pushbutton is pressed.
    // This prevents a double change as the button is released.
    if( XGpio_DiscreteRead(&Gpio_RGBLed_PB, ARTY_A7_PB_CHANNEL) != 0 )
    {
        // LEDs are on a 3 spacing.  So multiply button press by 2^3 to increment the correct LED
        gpio_push_buttons = XGpio_DiscreteRead(&Gpio_RGBLed_PB, ARTY_A7_PB_CHANNEL);
        gpio_leds_rgb     = XGpio_DiscreteRead(&Gpio_RGBLed_PB, ARTY_A7_RGB_CHANNEL);
        if ( gpio_push_buttons & 0x1 ) {
                mask = 0x7;
                incr = 0x1;
        } else if ( gpio_push_buttons & 0x2 ) {
                mask = (0x7 << 3);
                incr = (0x1 << 3);
        } else if ( gpio_push_buttons & 0x4 ) {
                mask = (0x7 << 6);
                incr = (0x1 << 6);
        } else if ( gpio_push_buttons & 0x8 ) {
                mask = (0x7 << 9);
                incr = (0x1 << 9);
        }

        led_val = gpio_leds_rgb & mask;
        led_val = (led_val+incr) & mask;
        gpio_leds_rgb = (gpio_leds_rgb & ~mask) | led_val;
        
        XGpio_DiscreteWrite(&Gpio_RGBLed_PB, ARTY_A7_RGB_CHANNEL, gpio_leds_rgb);
    }

    // Clear interrupt from GPIO
    XGpio_InterruptClear(&Gpio_RGBLed_PB, XGPIO_IR_MASK);
    // Clear interrupt in NVIC
    NVIC_ClearPendingIRQ(GPIO1_IRQn);
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

uint32_t buttonCheck(uint32_t lastButton){
	uint32_t buttonStates;
	int i = 0;
	buttonStates = XGpio_DiscreteRead(&Gpio_RGBLed_PB, ARTY_A7_PB_CHANNEL);
	
	if(buttonStates != lastButton){
		switch(buttonStates)
		{
			case 0x01: // short signal
					XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x01);
					shortDelay();
					XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x00);
					character[i] = 's';
					++i;
					//print("short");
					//mediumDelay();
			break;
			case 0x02: // long signal
					XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x01);
					mediumDelay();
					XGpio_DiscreteWrite(&Gpio_Led_DIPSw, ARTY_A7_LED_CHANNEL, 0x00);
					character[i] = 'l';
					++i;
				  //print("-");
					//shortDelay();
			break;
			case 0x04: // letter end
					longDelay();
					codingMorse(character);
					for(i=0; i<strlen(character); ++i){
						character[i] = '\0';
					}
					i=0;
			break;
			case 0x08: // word end
				longDelay();
				longDelay();
				print(word);
			break;
				default:
					shortDelay();
				break;
			}
	}	
	return buttonStates; 
}


void codingMorse(char *b){

if(b[0]=='s')
{
 if(b[1]=='s'){
  
    if(b[2]=='s'){
     
      if(b[3]=='s'){        
        if(b[4]=='s'){         
         word[strlen(word)] = '5';      
        }
        else if(b[4]=='l'){
         word[strlen(word)] = '4'; 
        }
        else word[strlen(word)] = 'h';        
      }
      else if(b[3]=='l'){
       if(b[4]=='s'){         
         blink(4); //error
        }
        else if(b[4]=='l'){
         word[strlen(word)] = '3'; 
        }
        else word[strlen(word)] = 'v';  
      }
      else word[strlen(word)] = 's'; 
       
   }
   else if(b[2]=='l'){ //ssl
    
    if(b[3]=='s'){        
        if(b[4]=='s'){         
         blink(4); //error      
        }
        else if(b[4]=='l'){
         blink(4); //error
        }
        else word[strlen(word)] = 'f';        
      }
      else if(b[3]=='l'){ //ssll
       if(b[4]=='s'){         
         blink(4); //error
        }
        else if(b[4]=='l'){
         word[strlen(word)] = '2'; 
        }
        else blink(4); 
      }
      else word[strlen(word)] = 'u'; 
    
   }
   else word[strlen(word)] = 'i'; 
  
 }
 
 else if(b[1]=='l'){ 
  
    if(b[2]=='s'){ 
     
      if(b[3]=='s'){       
        if(b[4]=='s'){        
         blink(4);      
        }
        else if(b[4]=='l'){
         blink(4);
        }
        else word[strlen(word)] = 'l';       
      }
      else if(b[3]=='l'){ 
       if(b[4]=='s'){         
         blink(4); //error
        }
        else if(b[4]=='l'){
         blink(4);
        }
        else blink(4); 
      }
      else word[strlen(word)] = 'r'; 
       
   }
   else if(b[2]=='l'){ 
    
    if(b[3]=='s'){      
        if(b[4]=='s'){         
         blink(4); //error      
        }
        else if(b[4]=='l'){
         blink(4); //error
        }
        else word[strlen(word)] = 'p';        
      }
      else if(b[3]=='l'){ 
       if(b[4]=='s'){         
         blink(4); //error
        }
        else if(b[4]=='l'){
         word[strlen(word)] = 'l'; 
        }
        else word[strlen(word)] = 'j'; 
      }
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
        word[strlen(word)] = 'z';        
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