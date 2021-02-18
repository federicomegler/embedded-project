///*
// * Copyright:
// * ----------------------------------------------------------------
// * This confidential and proprietary software may be used only as
// * authorised by a licensing agreement from ARM Limited
// *   (C) COPYRIGHT 2014, 2016 ARM Limited
// *       ALL RIGHTS RESERVED
// * The entire notice above must be reproduced on all authorised
// * copies and copies may only be made to the extent permitted
// * by a licensing agreement from ARM Limited.
// * ----------------------------------------------------------------
// * File:     main.c
// * Release Information : Cortex-M3 DesignStart-r0p1-00rel0
// * ----------------------------------------------------------------
// *
// */
///*
// * --------Included Headers--------
// */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


//// Xilinx specific headers
#include "xparameters.h"
#include "xgpio.h"

#include "m3_for_arty.h"        // Project specific header
#include "gpio.h"
#include "uart.h"
#include "spi.h"
#include "atomic.h"
#include "xuartlite_l.h"

//// Atomic test function
//uint32_t atomic_test(uint32_t *mem, uint32_t val);

///*******************************************************************/
void scan(u8 *DataBufferPtr, unsigned int NumByte);
void codingMorse(char *b);


int started = 0;
int len = 0;
char character[5] = {};
char string[40] = {};




int main( void){


// Define local variables
    int     status;
    int     DAPLinkFittedn;
    int     i;
    int     readbackError;
    char    debugStr[256];
    
    // Illegal location
    volatile u32 emptyLoc;
    volatile u32 QSPIbase;
        
    // DTCM test location
    uint32_t dtcmTest;
    
    // BRAM base
    // Specify as volatile to ensure processor reads values back from BRAM
    // and not local storage
    volatile u32 *pBRAMmemory = (u32 *)XPAR_BRAM_0_BASEADDR;

    // CPU ID register
    volatile u32 *pCPUId = (u32 *)0xE000ED00;
    volatile u32 CPUId;
    volatile u32 alias_test;
    int          CPU_part;
    int          CPU_rev;
    int          CPU_var;
    char         CPU_name[20];
        
    // Enable the following if you wish to test illegal accesses
    // and debug lock-up conditions
/*    
    u32 *pLegalAddr   = (u32 *)0x40120000;
    u32 *pIllegalAddr = (u32 *)0x40200000;
*/


    // Test data for SPI
    u8 spi_tx_data[8] = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef};
    u8 spi_rx_data[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    
    // Test data for BRAM
    u32 bram_data[8] = {0x01234567, 0x89abcdef, 0xdeadbeef, 0xfeebdaed, 0xa5f03ca5, 0x87654321, 0xfedc0ba9, 0x01020408};

    // Initialise the UART
    InitialiseUART();

    // Clear all interrupts
    NVIC_ClearAllPendingIRQ();
    
    // Initialise the GPIO
    status = InitialiseGPIO();
    if (status != XST_SUCCESS)  {
        print("Error - Xilinx GPIO failed to initialise\n");
    }

    // Enable GPIO Interrupts
    NVIC_EnableIRQ(GPIO0_IRQn);
    NVIC_EnableIRQ(GPIO1_IRQn);
    EnableGPIOInterrupts();

    // Enable UART Interrupts
    NVIC_EnableIRQ(UART0_IRQn);
    EnableUARTInterrupts();

//    // Read the DAPLinkFitted input, (assigned to IRQ[31]).
//    // Note the IRQ is never enabled, so polling the pending register will indicate the status
//    NVIC_DisableIRQ(DAPLinkFittedn_IRQn);
//    DAPLinkFittedn = NVIC_GetPendingIRQ( DAPLinkFittedn_IRQn );


//    // Initialise the SPI
//    status = InitialiseSPI(DAPLinkFittedn);
//    if (status != XST_SUCCESS)  {
//        print("Error - Xilinx SPI controllers failed to initialise\n");
//    }

//    DisableSPIInterrupts();


//    // Set DAPLink QSPI to the normal read-write controller
//    // Do NOT do this for code running from the DAPLink QSPI.  This will switch from the XIP QSPI
//    // controller to the standard controller, so the processor will not be able to access it's code image
//    // This should only be done if the XIP QSPI is used to copy code to internal TCM, then boot-load from that TCM
////    SetDAPLinkQSPIMode( QSPI_QSPIMODE );

//    // Read the CPU ID register to auto-detect the CPU and revision
//    // Note however that code is compiled for a specific processor, so even though
//    // the processor can be auto-detected, if the compiled code has extended commands not
//    // supported by the processor, then runtime issues can occur
//    CPUId    = *pCPUId;
//    CPU_var  = ((CPUId & 0x00F00000) >> 20);
//    CPU_part = ((CPUId & 0x0000FFF0) >> 4);
//    CPU_rev  = CPUId & (0x0000000F);
//    
//    switch (CPU_part)
//    {
//      case 0xC21 : strcpy(  CPU_name, "Cortex-M1" ); break;
//      case 0xC23 : strcpy(  CPU_name, "Cortex-M3" ); break;
//      default    : sprintf( CPU_name, "Unknown %x", CPU_part );
//    }
//    
//    sprintf (debugStr, "Arm %s Revision %i Variant %i\r\n\n", CPU_name, CPU_rev, CPU_var );
//    
//#ifndef SIM_BUILD    
//    // Use Xilinx version print command
//    print ("************************************\r\n");
//    print ( debugStr );
//    print ("Example design for Digilent A7 board\r\n");
//    if ( DAPLinkFittedn )
//      print ("\nV2C-DAPLink board not detected\r\n");
//    else
//      print ("\nV2C-DAPLink board detected\r\n");
//    print ("Use DIP switches and push buttons to\r\ncontrol LEDS\r\n");
//    print (" Version 1.1\r\n");
//    print ("************************************\r\n");
//#else
//    print ( debugStr );
//#endif    

//        // *****************************
//        // Test the code memory aliasing
//        // *****************************
//        // Uses the reserved vector table location 0x0000001c and its alias
//        // This is only writeable in TCM, a write to QSPI will hang
//        // Any access to an unmapped region will hang

//        // DAPLINK fitted: (Low QSPI, High BRAM)
//        // DAPLINK absent: (Low BRAM, High BRAM)
//        status = 0;

//        if(*(uint32_t*)0x0000001c != 0x55){
//          status += 1;
//           }
//        if(*(uint32_t*)0x1000001c != 0x55){
//          status += 2;
//          }

//        // Write to ITCM upper alias
//        *(uint32_t*)0x1000001c = 12;

//        if(DAPLinkFittedn){
//        //Lower alias should have changed
//          if(*(uint32_t*)0x0000001c != 12){
//            status += 4;
//            }
//            // Write to ITCM lower alias
//          *(uint32_t*)0x0000001c = 11;
//          if(*(uint32_t*)0x0000001c != 11){
//            status += 4;
//            }
//          if(*(uint32_t*)0x1000001c != 11){
//            status += 8;
//            }
//          } else {
//          if(*(uint32_t*)0x0000001c != 0x55){
//            status += 4;
//            }
//          if(*(uint32_t*)0x1000001c != 12){
//            status += 8;
//            }
//          }


//          if (status == 3){
//            print("Upper and lower regions written since BRAM initialised\r\n");
//                                                status = 0;
//            }
//                                        if (status == 2){
//            print("Upper alias writen since BRAM initialised\r\n");
//                                                status = 0;
//                                        }                   
//          if (status == 4 && (*(uint32_t*)0x0000001c == 12)){
//            print("ITCM also aliased high\r\n");
//                                          status = 0;
//            }
//                                        if(status != 0){
//          print("Unexpected alias behaviour");
//          // 1: QSPI content unexpected
//          // 2: ITCM content unexpected (maybe written by download)
//          // 4: Lower alias overwritten or wrong
//          // 8: ITCM Upper alias not changed
//          sprintf(debugStr, " %d\r\n", status);
//          print(debugStr);
//         } else {
//           print ("Aliasing OK\r\n");
//           }
//    // *****************************************************
//    // Test the BRAM
//    // *****************************************************
//    
//    // Write to BRAM
//    for( i=0; i< (sizeof(bram_data)/sizeof(u32)); i++)
//        *pBRAMmemory++ = bram_data[i];
//    readbackError = 0;
//    // Reset the pointer
//    pBRAMmemory = (u32 *)XPAR_BRAM_0_BASEADDR;

//    // Readback
//    for( i=0; i< (sizeof(bram_data)/sizeof(u32)); i++)
//    {
//      if ( *pBRAMmemory++ != bram_data[i] )
//        readbackError++;
//      }

//    if ( readbackError )
//        print( "ERROR - Bram readback corrupted.\r\n" );
//    else
//        print( "Bram readback correct\r\n" );


//    // *****************************************************
//    // Test the SPI
//    // *****************************************************
//    
//    // Initialise the base QSPI to the correct mode
//    status = InitQSPIBaseFlash();
//    status = WriteQSPIBaseFlash( spi_tx_data, sizeof(spi_tx_data)/sizeof(u8), 0x0 );
//    status = ReadQSPIBaseFlash ( spi_rx_data, sizeof(spi_rx_data)/sizeof(u8), 0x0 );


//    // Manually type out, print does work when called back to back from a loop
///*
//    sprintf( debugStr, "%x %x %x %x %x %x %x %x\r\n", spi_rx_data[0], spi_rx_data[1], spi_rx_data[2], spi_rx_data[3], 
//                                                   spi_rx_data[4], spi_rx_data[5], spi_rx_data[6], spi_rx_data[7] );
//    print( debugStr );
//*/    
//    // Compare buffers
//    readbackError = 0;
//    for( i=0; i<(sizeof(spi_rx_data)/sizeof(u8)); i++ )
//    {
//        if( spi_rx_data[i] != spi_tx_data[i] )
//            readbackError++;
//    }

//    if ( readbackError )
//        print( "ERROR - Base SPI readback corrupted.\r\n" );
//    else
//        print( "Base SPI readback correct\r\n" );
//   

//    // ******************************************************
//    // Test exceptions.  Write to legal and illegal addresses
//    // ******************************************************
///*    
//    // Do an access to an legal location
//    emptyLoc = *pLegalAddr;

//    // Do an access to an illegal location
//    emptyLoc = *pIllegalAddr;

//*/
//   // ******************************************************
//   // Test exclusive transactions
//   // ******************************************************
//   
//   // Data TCM supports exclusive access
//   status = atomic_access(&dtcmTest,0x12341230);
//   if (status == 1) {
//   print( "STREX DTCM failed unexpectedly\r\n");
//   }
//   // Instruction TCM supports exclusive access (ARTY connected)
//   status = atomic_access((uint32_t*)0x10001000,0x12341231);
//   if (status == 1) {
//   print( "STREX ITCM high alias failed unexpectedly\r\n");
//   }
//   // Instruction QSPI region, no exclusive monitor
//   // Unused vector table entry after Usage fault handler
//   status = atomic_access((uint32_t*)0x0000001c,0x12341232);
//   if (status == 1 && DAPLinkFittedn) {
//   print( "STREX ITCM low alias failed unexpectedly\r\n");
//   }
//   if (status == 0 && !DAPLinkFittedn) {
//   print( "STREX QSPI success (unexpected)\r\n");
//   }               
//   // BRAM region is external, no exclusive monitor
//   status = atomic_access((uint32_t*)XPAR_BRAM_0_BASEADDR,0x12341233);
//   if (status == 0 && !DAPLinkFittedn) {
//   print( "STREX BRAM success (unexpected)\r\n");
//   }
//   print( "Atomic transaction test completed\r\n" );
//   
// print( "Startup complete, entering main interrupt loop\r\n" );
	
		
		print("ciao");
    while ( 1 )
    {
				if(getButton() == 0x1){
						blink(0x1);
				}
				if(getButton() == 0x2){
						blink(0x2);
				}
				if(getButton() == 0x4){
						blink(0x4);
				}
				if(getButton() == 0x8){
						blink(0x8);
				}
    }
}


void readMorseChar(){
		
			uint32_t button = getButton();
		if(button & 0xe){
				disableButton(0x1);
				if(started){
						if(len < 5){
								character[len] = 's'; //s means SHORT
								++len;
						}
						else{
								codingMorse(character);
								int i=0;
								for(i=0; i<5; ++i) character[i] = '-';
								started = 0;
								len = 0;
						}
				}
		}
		else if(button & 0xd){
				disableButton(0x2);
				if(started){
						if(len < 5){
								character[len] = 'l'; //1 means LONG
								++len;
						}
						else{
								codingMorse(character);
								int i=0;
								for(i=0; i<5; ++i) character[i] = '-';
								started = 0;
								len = 0;
						} 
				}
		}
		else if(button & 0xb){
			disableButton(0x4);
			if(started == 0){
					started = 1;
				}
		}
		else if(button & 0x7){
				if(started){
							codingMorse(character);
							int i=0;
							for(i=0; i<5; ++i) character[i] = '-';
							started = 0;
							len = 0;
				}
		}
}



void scan(u8 *DataBufferPtr, unsigned int NumBytes) {
		int i=0;		
		while(i<NumBytes){
				DataBufferPtr[i] = XUartLite_RecvByte(STDIN_BASEADDRESS);
		}
		return;
}

///* Interrupt handler for DAPLink Fitted */
//// This routine should never be called as the signal is used as IO
//// Routine created to prevent exceptions in the case the IRQ is enabled
//void DAPLinkFittedn ( void )
//{
//    // Clear the IRQ and disable any future IRQs
//    NVIC_ClearPendingIRQ(DAPLinkFittedn_IRQn);
//    NVIC_DisableIRQ(DAPLinkFittedn_IRQn);
//}



void codingMorse(char *b){

if(b[0]=='s')
{
 if(b[1]=='s'){
  
    if(b[2]=='s'){
     
      if(b[3]=='s'){        
        if(b[4]=='s'){         
         print("5");       
        }
        else if(b[4]=='l'){
         print("4");
        }
        else print("h");       
      }
      else if(b[3]=='l'){
       if(b[4]=='s'){         
         blink(1); //error
        }
        else if(b[4]=='l'){
         print("3");
        }
        else print("v"); 
      }
      else print("s");
       
   }
   else if(b[2]=='l'){ //ssl
    
    if(b[3]=='s'){        
        if(b[4]=='s'){         
         blink(1); //error      
        }
        else if(b[4]=='l'){
         blink(1); //error
        }
        else print("f");       
      }
      else if(b[3]=='l'){ //ssll
       if(b[4]=='s'){         
         blink(1); //error
        }
        else if(b[4]=='l'){
         print("2");
        }
        else blink(1); 
      }
      else print("u");
    
   }
   else print("i");
  
 }
 
 else if(b[1]=='l'){ 
  
    if(b[2]=='s'){ 
     
      if(b[3]=='s'){       
        if(b[4]=='s'){        
         blink(1);      
        }
        else if(b[4]=='l'){
         blink(1);
        }
        else print("l");       
      }
      else if(b[3]=='l'){ 
       if(b[4]=='s'){         
         blink(1); //error
        }
        else if(b[4]=='l'){
         blink(1);
        }
        else blink(1); 
      }
      else print("r");
       
   }
   else if(b[2]=='l'){ 
    
    if(b[3]=='s'){      
        if(b[4]=='s'){         
         blink(1); //error      
        }
        else if(b[4]=='l'){
         blink(1); //error
        }
        else print("p");       
      }
      else if(b[3]=='l'){ 
       if(b[4]=='s'){         
         blink(1); //error
        }
        else if(b[4]=='l'){
         print("1");
        }
        else print("j"); 
      }
      else print("w");
    
   }
   else print("a");
  
 }
 else print("e");
}
else if(b[0]=='l'){
 
 if(b[1]=='s'){
  
    if(b[2]=='s'){
     
      if(b[3]=='s'){        
        if(b[4]=='s'){         
         printf("6");       
        }
        else if(b[4]=='l'){
         blink(1);
        }
        else print("b");       
      }
      else if(b[3]=='l'){ 
       if(b[4]=='s'){         
         blink(1); //error
        }
        else if(b[4]=='l'){
         blink(1);
        }
        else print("x"); 
      }
      else print("d");
       
   }
   else if(b[2]=='l'){ //lsl
    
    if(b[3]=='s'){        
        if(b[4]=='s'){         
         blink(1); //error      
        }
        else if(b[4]=='l'){
         blink(1); //error
        }
        else print("c");       
      }
      else if(b[3]=='l'){ // lsll
       if(b[4]=='s'){         
         blink(1); //error
        }
        else if(b[4]=='l'){
         blink(1);
        }
        else print("y"); 
      }
      else print("k");
    
   }
   else print("n");
  
 }
 
 else if(b[1]=='l'){ //l l 
  
    if(b[2]=='s'){ // l l s
     
      if(b[3]=='s'){ // llss   
        if(b[4]=='s'){  //llsss      
         print("7");      
        }
        else if(b[4]=='l'){
         blink(1);
        }
        else print("z");       
      }
      else if(b[3]=='l'){ //l l s l
       if(b[4]=='s'){         
         blink(1); //error
        }
        else if(b[4]=='l'){
         blink(1);
        }
        else print("q"); 
      }
      else print("g");
       
   }
   else if(b[2]=='l'){ // l l l
    
    if(b[3]=='s'){      
        if(b[4]=='s'){         
         print("8");       
        }
        else if(b[4]=='l'){
         blink(1); //error
        }
        else blink(1);       
      }
      else if(b[3]=='l'){ //llll
       if(b[4]=='s'){         
         print("9"); //error
        }
else if(b[4]=='l'){
         print("0");
        }
        else blink(1); 
      }
      else print("o");
    
   }
   else print("m");
  
 }
 else print("t");
}
else blink(1);
}
