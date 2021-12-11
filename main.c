// CONFIG1L
#pragma config FEXTOSC = HS     // External Oscillator mode Selection bits (HS (crystal oscillator) above 8 MHz; PFM set to high power)
#pragma config RSTOSC = EXTOSC_4PLL// Power-up default value for COSC bits (EXTOSC with 4x PLL, with EXTOSC operating per FEXTOSC bits)

// CONFIG3L
#pragma config WDTCPS = WDTCPS_31// WDT Period Select bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = OFF        // WDT operating mode (WDT enabled regardless of sleep)

#include <xc.h>
#include <stdio.h>
#include "dc_motor.h"
#include "color.h"
#include "i2c.h"
#include "serial.h"
#include "interrupts.h"
#include "timers.h"

#define _XTAL_FREQ 64000000 //note intrinsic _delay function is 62.5ns at 64,000,000Hz  

//initialise variables
//static volatile int movements = 0;
//int timerArray[30] = {};
//int movementArray[30] = {}; //define overall empty array

void main(void){
    // Initialise Helper Scripts
    initDCmotorsPWM(99);
    color_click_init();
    initUSART4(); 
    Timer0_init();
    __delay_ms(300);
    Interrupts_init();

    // setup buttons for input
    TRISFbits.TRISF2=1; //set TRIS value for pin (input)
    ANSELFbits.ANSELF2=0; //turn off analogue input on pin
    TRISFbits.TRISF3=1; //set TRIS value for pin (input)
    ANSELFbits.ANSELF3=0; //turn off analogue input on pin
    
    struct DC_motor motorL, motorR; 		//declare two DC_motor structures 
    unsigned int PWMcycle = 99;
    motorL.power=0; 						//zero power to start
    motorL.direction=1; 					//set default motor direction
    motorL.dutyHighByte=(unsigned char *)(&PWM6DCH);	//store address of PWM duty high byte
    motorL.dir_LAT=(unsigned char *)(&LATE); 		//store address of LAT
    motorL.dir_pin=4; 						//pin RE4 controls direction
    motorL.PWMperiod=PWMcycle; 			//store PWMperiod for motor
    
    motorR.power=0; 						//zero power to start
    motorR.direction=1; 					//set default motor direction
    motorR.dutyHighByte=(unsigned char *)(&PWM7DCH);	//store address of PWM duty high byte
    motorR.dir_LAT=(unsigned char *)(&LATG); 		//store address of LAT
    motorR.dir_pin=6; 						//pin RG6 controls direction
    motorR.PWMperiod=PWMcycle; 			//store PWMperiod for motor
    
    //definition of RGB structure
    struct RGB_val test;
    test.C = 0;
    test.R = 0;
    test.G = 0;
    test.B = 0;
    
    // Initialise Front LEDs 
    LATGbits.LATG1=0;   //set initial output state
    TRISGbits.TRISG1=0; //set TRIS value for pin (output)
    LATAbits.LATA4=0;   //set initial output state
    TRISAbits.TRISA4=0; //set TRIS value for pin (output)
    LATFbits.LATF7=0;   //set initial output state
    TRISFbits.TRISF7=0; //set TRIS value for pin (output)
    
    // LEDs on board
    TRISDbits.TRISD7 = 0;
    LATDbits.LATD7 = 0;
    TRISHbits.TRISH3 = 0;
    LATHbits.LATH3 = 0;
//    
//    char string[30];
//    char string0[30];
//    char string1[30];
//    char string2[30];
//    char string3[30];
    unsigned int RedRatio, GreenRatio, BlueRatio;
   
    // Turn on Front White LED Lights
    LATGbits.LATG1=1;   
    LATAbits.LATA4=1;   
    LATFbits.LATF7=1; 
    
    // Calibration done against White Card
    unsigned int cal = 0;
    while(cal==0){
        LATDbits.LATD7 = 1;
        while (PORTFbits.RF2); //empty while loop (wait for button press)
        if (!PORTFbits.RF2){
            LATDbits.LATD7 = 0;
            calibrateW(&test);
            __delay_ms(300);
            }   
        
        LATDbits.LATD7 = 1;
        while (PORTFbits.RF2); //empty while loop (wait for button press)
        if (!PORTFbits.RF2){
            LATDbits.LATD7 = 0;
            calibrateB(&test);
            __delay_ms(300);
            }   
        
        LATHbits.LATH3 = 1;
        while (PORTFbits.RF3);
        if (!PORTFbits.RF3){
            LATHbits.LATH3 = 0;
            cal = 1;
        }
    }
    
    unsigned int check1 = 9;
    unsigned int check2 = 9;
    unsigned int check3 = 9;
    unsigned int check4 = 9;
    unsigned int count = 0;
    
    while(1){          
        unsigned int detected_colour;  
        read_colours(&test);
        if (count==0) {check1 = determine_color_new(&test);}
        if (count==1) {check2 = determine_color_new(&test);}
        if (count==2) {check3 = determine_color_new(&test);}
        if (count==3) {check4 = determine_color_new(&test);count=0;}
        else (count += 1);
        
        // 3 checks to detect colour
        if (check1==check2 && check2==check3 && check3==check4){
            detected_colour = check1;
            check1=9; 
            check2=9; 
            check3=9; 
            check4=9;// makes sure function only occurs once
        }
        
        if (detected_colour == 0){ turnRight90(&motorL,&motorR);__delay_ms(100);} // Red
        if (detected_colour == 1){ turnLeft90(&motorL,&motorR);__delay_ms(100);} // Blue
        if (detected_colour == 2){ turnRight180(&motorL,&motorR);__delay_ms(100);} // Green 
        if (detected_colour == 3){ reverseTurnRight90(&motorL,&motorR);__delay_ms(100);} // Yellow
        if (detected_colour == 4){ reverseTurnLeft90(&motorL,&motorR);__delay_ms(100);} // Pink
        if (detected_colour == 5){ turnRight135(&motorL,&motorR);__delay_ms(100);} // Orange 
        if (detected_colour == 6){ turnLeft135(&motorL,&motorR);__delay_ms(100);} // Light Blue
        if (detected_colour == 7){ turnRight180(&motorL,&motorR);__delay_ms(100);} // White - need to alter
        if (detected_colour == 8){ turnRight90(&motorL,&motorR);__delay_ms(100);} // Black - need to alter
        if (detected_colour == 9){ forward(&motorL,&motorR);} // Ambient
        //__delay_ms(200); 
        
        
        // For debugging/reviewing purposes
//        RedRatio = ((float)(test.R - test.blackR) / (float)(test.whiteR - test.blackR)) * 10000;
//        GreenRatio = ((float)(test.G - test.blackG) / (float)(test.whiteG - test.blackG)) * 10000;
//        BlueRatio = ((float)(test.B - test.blackB) / (float)(test.whiteB - test.blackB)) * 10000;
        //brightness = lumin(&test);
//        
//        sprintf(string1," R:%d ",RedRatio);
//        TxBufferedString(string1);
//        sendTxBuf();
//        __delay_ms(150);
//        
//        sprintf(string2," G:%d ",GreenRatio);
//        TxBufferedString(string2);
//        sendTxBuf();
//        __delay_ms(150);
//
//        sprintf(string3," B:%d ",BlueRatio);
//        TxBufferedString(string3);
//        sendTxBuf();
//        __delay_ms(150);
//        
//        sprintf(string," Color:%d ",detected_colour);
//        TxBufferedString(string);
//        sendTxBuf();
//        __delay_ms(50);
    }
}
