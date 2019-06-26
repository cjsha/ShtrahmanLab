/*
 * Prototype Behavior Code
 * Authors: Max Guerrero and Cris Sharp
 *
 * If template song is played, lick right else lick left.
 *
 * PIN LAYOUT:
 *
 * Valve and Lick
 * PE2 - Left Valve
 * PE1 - Right Valve
 *
 * PA2 - Left Lick
 * PA3 - Right Lick
 *
 * Tone and Filter Signals:
   PE4 M0PWM4, Filter Clock - PB4 M0PWM2.
 *
 * Amplifier Standby:
 * PF4 - Always HIGH
 *
 * Motor
 * PB6 (M0PWM0)
 *
 * Punishment LED
 * PC5 Input to shield and outputs 12V signal
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/pwm.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "driverlib/debug.h"

#define MAXSONGLENGTH 6
#define TONESPERSONG 6
#define TIMERESOLUTIONFACTOR 1000
#define TONEDURATION 0.25
#define TIMEBETWEENTONES 0.25
#define PLAYTONECOUNTER TIMERESOLUTIONFACTOR*(TONEDURATION + TIMEBETWEENTONES)
#define PHASE2COUNTERTIME (TONESPERSONG*(PLAYTONECOUNTER))
#define LICKWINDOWDURATION 3000
#define PHASE3COUNTERTIME PHASE2COUNTERTIME + LICKWINDOWDURATION //Convert to variable
#define MOTORLENGTH 1000
#define PHASE4COUNTERTIME (PHASE3COUNTERTIME + MOTORLENGTH) //Convert to variable
#define TONELENGTH 0.375              //Length of each tone in seconds
#define TONELENGTHLOAD 1/(TONELENGTH) //In order to obtain timer/counter cycles
#define OPENVALVETIME 0.25              //Duration of valve being open time
#define C4   (uint32_t)4186        //0
#define Db4  (uint32_t)4434       //1
#define Dnat4   (uint32_t)4698         //2
#define Eb4  (uint32_t)4978       //3
#define E4   (uint32_t)5274         //4
#define Fnat4   (uint32_t)5587        //5
#define Gb4  (uint32_t)5919        //6
#define G4   (uint32_t)6271         //7
#define Ab4  (uint32_t)6644         //8
#define A4   (uint32_t)7040
#define Bb4  (uint32_t)7459
#define B4   (uint32_t)7902
#define C5   (uint32_t)8372
#define Db5  (uint32_t)8870
#define Dnat5 (uint32_t)9397
#define Eb5  (uint32_t)9956
#define E5   (uint32_t)10548
#define MAXDEV 8
#define MINDEV 2
#define SONGBLOCKLENGTH 3
#define PIN_MASK 0x0C
//outerPhases (training phases) 0,1,2 and 3
unsigned int phase;
unsigned int outerPhase0 = 1;
unsigned int outerPhase1 = 0;
unsigned int outerPhase2 = 0;
unsigned int outerPhase3 = 0;
unsigned int outerPhase4 = 0;
unsigned int finalTask = 0;
unsigned int templatePlayed = 0;
unsigned int nonTemplatePlayed = 0;
unsigned int nonTemplateBlock = 1;
unsigned int breakPeriod = 500;
/*Training Phase 3 parameters*/
unsigned int atLeastOneCorrect = 0;
unsigned int biasCounter = 3;
/*Lick Window Variables*/
unsigned int leftValveOpen = 0;
unsigned int rightValveOpen = 0;
unsigned int openedOnce = 0;
/*Correctness*/
unsigned int correct = 0;
unsigned int incorrect = 0;
unsigned int licked;
unsigned int alreadyTransmitted = 0;


char takeInParametersFlag = 0;

unsigned int masterTimerCounter = 0;
unsigned int phase1 = 1;
unsigned int phase2 = 0;
unsigned int phase3 = 0;
unsigned int tone = 0;                   //Tone Index
unsigned int play = 0;                //Play = 1; Pause = 0 GUI Controlled
unsigned int cycleCounter = 1;
unsigned int minDev;                  //Minimum and maximum deviations
unsigned int maxDev;                  //for computing residual of tones from template song
unsigned int songIndeces[6];
char hexChars[26];
unsigned int trainingPhase;
unsigned int updatedVars = 0;
unsigned int noLick = 0;
double songRewardTime;
unsigned int noLickEncouragementCount;
unsigned int encouragementDrop;
/*
 * GUI -> MCU Edited Behavior Protocol Parameters:
 * These parameters can be adjusted via UART from the GUI to the MCU.e
 * we can determine the play tone, phase1, phase2, etc counter variables
 * (used by the Master Timer To keep track of the phases) by knowing the
 * tone duration and time between tones.
 * TODO: MAKE SURE TO TAKE INTO ACCOUNT GUI TIME CONVERSION FACTOR 0x01 = 10 ms !!
 */
double       delay;
double       punishDuration;
unsigned int lickWindowDuration = LICKWINDOWDURATION;
int toneDuration = 1;
double       timeBetweenTones = 1.0*TIMEBETWEENTONES;
unsigned int timeRes = TIMERESOLUTIONFACTOR;
unsigned int playToneCounter;
unsigned int phase2CounterTime;
unsigned int phase3CounterTime;
unsigned int phase4CounterTime =  (PHASE3COUNTERTIME + MOTORLENGTH);
unsigned int breakWindow = 500;
double       rightValveOpenTime = OPENVALVETIME;
double       leftValveOpenTime = OPENVALVETIME;
double       timeBetweenRewardPhase1;
double       noLickEncouragementTrials;
int totalTrials = 100;
int trial = 0;
int j = 0;

int poop;
double p = 2.0;

uint32_t ui32Length1;
uint32_t ui32Length2;
uint32_t ui32Length3;
uint32_t rightOpenDuration;
uint32_t leftOpenDuration;
uint32_t ui32PWMClock;
uint32_t  tones[] = {C4,Db4,Dnat4,Eb4,E4,Fnat4,Gb4,G4,Ab4,A4,Bb4,B4,C5,Db5,Dnat5,Eb5,E5};
uint32_t  song[] = {C4, E4, G4, C5};
uint32_t filterClockFreq[6];
uint8_t  templateSong1[] = {C4, E4, G4, C5};
uint8_t  templateSong[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint32_t ui32MotorClock;
uint8_t  lickData[4] = {0x74, 0x00, 0x00, 0x00};
uint8_t  clearArray[]  = {0x00,0x00,0x00};
uint8_t  tonesIndex[]  = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
size_t   noTones;       //used to keep track of how many tones less than 6 per song
size_t   tonesPerSong;

int songDeviation =  0;

/*PWM Load Values for PWM signals
  These load values are in units of PWM Clock ticks and needed in the
  PWMGenPeriodSet() function as arguments */


uint32_t pwmLoad[TONESPERSONG];
uint32_t filterPWMLoad[TONESPERSONG];
uint32_t motorLoad;
uint32_t driveFreq = 400; //400 Hz
double motorDutyCycle = 0.10; // 10% duty Cycle
double maxMotorDutyCycle = 0.25;
/*GUI Variables here: Tone Duration, Time between Tones etc*/

void setUpMasterTimer(void);
void setUpTonesTimers(void);
void playTone(unsigned int i);
void enablePeripherals(void);
void configurePWM(void);
void configureSinglePWM(void);
void enableUART(void);
void LickPortAIntHandler(void);
void valveInterruptHandler(void);
void openRightValve(void);
void openLeftValve(void);
void enableValveTimer(void);
void Tone1TimerHandler(void);
void Tone2TimerHandler(void);
void Tone3TimerHandler(void);
void Tone4TimerHandler(void);
void UART0IntHandler(void);
void RLSendMessage(void);
void shuffleSong(void);
void coinFlip(void);
void updateVariables(void);
uint8_t computeSongDeviation(void);
void transmitUART(void);
void configureMotorPWM(void);
void motorRun(void);
void motorOff(void);
void setUpRampTimer(void);
void motorTimerHandler(void);

int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_16|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
    SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

    playToneCounter = timeRes*(toneDuration + timeBetweenTones);
    phase2CounterTime = tonesPerSong*(playToneCounter);
    phase3CounterTime = (phase2CounterTime + lickWindowDuration);
    phase4CounterTime = (phase3CounterTime + MOTORLENGTH);
    ui32Length1 = (SysCtlClockGet()*toneDuration);
    rightOpenDuration = (SysCtlClockGet()*rightValveOpenTime);
    leftOpenDuration = (SysCtlClockGet()*leftValveOpenTime);

    //Configure System Clock to run at 12.5MHz
    //configure the PWM clock to run at same frequency

    enablePeripherals();
    setUpMasterTimer();
    setUpRampTimer();
    GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    while(1)
    {
        while(phase1)
        {

        }
        while(phase2)
        {

        }
        while(!play)
        {

        }
        while(phase3)
        {

        }
    }
}

void enablePeripherals(void)
{
    //Enable GPIOF/A Peripheral and configure the pins connected to the LED's as outputs
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); // Valve (and PWM)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);

    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_2); //Left Valve
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_1); //Right Valve
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE,  GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_5); //PC5 punishment LED strip pulled to +12V in shield
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_4); //standby pin PF4
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6|GPIO_PIN_7);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, GPIO_PIN_6);

    /*TEST PIN ENABLE*/
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_6);
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_7);

    //enable standby pin
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4);

    //configurePWM();
    configureSinglePWM();
    configureMotorPWM();
    setUpTonesTimers();

    //Enable PA2  and 3's weak pull up resistor
    GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_3,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);

    //Enable PWM, Pin Interrupt Register
    GPIOIntRegister(GPIO_PORTA_BASE, LickPortAIntHandler);
    GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3, GPIO_FALLING_EDGE);
    //Enable UART
    enableUART();
    //Enable Pin Interrupt
//    GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3);

}

void configureSinglePWM(void){
     ui32PWMClock = SysCtlClockGet();
     //SIGNAL 1: Tone - PE4 M0PWM4, Filter Clock - PB4 M0PWM2
     GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_4);
     GPIOPinTypePWM(GPIO_PORTE_BASE, GPIO_PIN_4);
     GPIOPinConfigure(GPIO_PB4_M0PWM2);
     GPIOPinConfigure(GPIO_PE4_M0PWM4);

     PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN); //for M0 PWM4 PE4

     PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN); //for M0 PWM2 PB4

}

void configurePWM(void){
   //SIGNAL 1: Tone - PF1 M1PWM5, Filter Clock - PA7 M1PWM3
   GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);
   GPIOPinTypePWM(GPIO_PORTA_BASE, GPIO_PIN_7);
   GPIOPinConfigure(GPIO_PF1_M1PWM5);
   GPIOPinConfigure(GPIO_PA7_M1PWM3);

   //Configure M0PWM3 for PB5
   GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_5);
   GPIOPinConfigure(GPIO_PB5_M0PWM3);

   //Configure M0PWM4 for PE4
   GPIOPinTypePWM(GPIO_PORTE_BASE, GPIO_PIN_4);
   GPIOPinConfigure(GPIO_PE4_M0PWM4);

   ui32PWMClock = SysCtlClockGet();


   PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN); //for M1PWM5

   PWMGenConfigure(PWM1_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN); //for M1 PWM3

   PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN);

   PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);

}
void configureMotorPWM(void){
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_6);
    GPIOPinConfigure(GPIO_PB6_M0PWM0);

    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
    ui32MotorClock = SysCtlClockGet();

    motorLoad = ui32MotorClock/driveFreq;
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, motorLoad);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 0.25*motorLoad);

    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, false);
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);

    //Configure M1PWM0 for PD0 and M0PWM7 for PD1
//    GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_1);
//    GPIOPinConfigure(GPIO_PD1_M0PWM7);
//    PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
//    ui32MotorClock = SysCtlClockGet();
//
//    motorLoad = ui32MotorClock/driveFreq;
//    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, motorLoad);
//    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, motorDutyCycle*motorLoad); //Initially only at 10% D-C
//
//
//    PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, false);
//    PWMGenEnable(PWM0_BASE, PWM_GEN_3);
}

void motorRun(void){
    /*Timer for Ramping Enable*/
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, motorLoad);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, motorDutyCycle*motorLoad); //Initially only at 10% D-C
    TimerEnable(TIMER2_BASE,TIMER_A);
    TimerIntEnable(TIMER2_BASE,TIMER_A);
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);
}
void motorOff(void){
    TimerIntDisable(TIMER2_BASE,TIMER_TIMA_TIMEOUT);
    PWMGenDisable(PWM0_BASE, PWM_GEN_0);
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, false);
    motorDutyCycle = 0.10;
}
/*
 * For outer phases 0 and 1, the sequence is 3 Block songs
 */
void shuffleSong(void){
    int k, j;

    if(nonTemplateBlock){
       do{
          for(j = 0; j < tonesPerSong; ++j){
              tonesIndex[j] = (uint8_t)(rand()%16);
          }
          for(j = 0; j < tonesPerSong; ++j){
              song[j] = tones[tonesIndex[j]];
              filterClockFreq[j] = 75*song[j];
          }
          songDeviation = computeSongDeviation();
       }while(songDeviation > maxDev || songDeviation < minDev);

       for(k = 0; k < tonesPerSong; ++k){
           pwmLoad[k] = ui32PWMClock/song[k] - 1;
           filterPWMLoad[k] = ui32PWMClock/filterClockFreq[k] - 1;
       }
       nonTemplatePlayed = 1;
       //non-template GPIO Pin HI, template LOW
       templatePlayed = 0;
    }
    else{
        for(k = 0; k < tonesPerSong; ++k){
            pwmLoad[k] = ui32PWMClock/tones[templateSong[k]] - 1;
            filterPWMLoad[k] = ui32PWMClock/(75*tones[templateSong[k]]) - 1;
        }
        templatePlayed = 1;
        //non-template GPIO Pin LOW, template HI
        nonTemplatePlayed = 0;
        songDeviation = 0;
    }


    /* commented out for single PWM Code */

//    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, pwmLoad2);
//    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0.5*pwmLoad2);
//
//    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, pwmLoad3);
//    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 0.5*pwmLoad3);
//
//    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, pwmLoad4);
//    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 0.5*pwmLoad4);

}

void encouragementDropRandom(void){
    double r;
    r =  (double)rand() / (double)RAND_MAX ;
 //   printf("%f\n",(double)(1.0/3.0));
    if(r <= (double)(1.0/p)){
        encouragementDrop = 1;
    }else{
        encouragementDrop = 0;
    }
    //printf("%f\n", r);
}

void coinFlip(void){
   if(outerPhase2){
      if(rand()%2 == 0){
          nonTemplateBlock = (!nonTemplateBlock)&1;
          biasCounter = 3;
      }
      else{
         biasCounter = 0;
      }
   }
   else if(outerPhase3||outerPhase4){
       if(rand()%2 == 0){
           nonTemplateBlock = !nonTemplateBlock;
       }
   }

}
uint8_t computeSongDeviation(void){
    int j;
    uint8_t sum = 0;
    for(j = 0; j < tonesPerSong; ++j){
        sum += abs(templateSong[j] - tonesIndex[j]);
    }
    return sum;
}
/*
 * READ ME:
 * We have edited code to only use module A timer 1 for all tones
 * instead of one timer to keep track of each tone. This is better
 * because the MCU will be enabling less General Purpose Timer Modules
 * and thus consume less power.
 * */
void setUpTonesTimers(void){
    /*Timer1 for Tone(LED) 1 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    TimerConfigure(TIMER1_BASE,TIMER_CFG_ONE_SHOT);
    //Maybe add function here for Setting up all timers and their corresponding
    //Interrupts
    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    /*Timer 2 for Tone(LED) 2*/
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
    TimerConfigure(TIMER2_BASE, TIMER_CFG_ONE_SHOT);
    IntEnable(INT_TIMER2A);
    TimerIntEnable(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
    /*Timer 3 for Tone(LED) 3*/
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
    TimerConfigure(TIMER3_BASE, TIMER_CFG_ONE_SHOT);
    IntEnable(INT_TIMER3A);
    TimerIntEnable(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
    /*Timer 4 for Tone 4 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER4);
    TimerConfigure(TIMER4_BASE, TIMER_CFG_ONE_SHOT);
    IntEnable(INT_TIMER4A);
    TimerIntEnable(TIMER4_BASE, TIMER_TIMA_TIMEOUT);
}

/*
 * Enables the timer for timing the duration of the tone length.
 * Sets the period of the PWM signal according to the value stored in
 * PWMLoad which is determined either  by the template song or the
 * random tone sorting algorithm
 */
void playTone(unsigned int i){
    if(i == 1){

         //We want the four tones played consecutively to last a total of
         //1.5 seconds, therefore the duration of each tone is such:
         TimerLoadSet(TIMER1_BASE, TIMER_A, ui32Length1 - 1);

         /*Enable Timers*/
         TimerEnable(TIMER1_BASE,TIMER_A);
         // Turn on tone
         // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);
         //Tone
         PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, pwmLoad[i-1]);
         PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 0.5*pwmLoad[i-1]);
         PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
         PWMGenEnable(PWM0_BASE, PWM_GEN_2);
         //Filter Clock

         PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, filterPWMLoad[i-1]);
         PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0.5*filterPWMLoad[i-1]);
         PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);
         PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    }
    else if(i == 2){
        /*Timer2 for Tone(LED) 2*/
        ui32Length2 = ui32Length1;

        TimerLoadSet(TIMER1_BASE,TIMER_A, ui32Length2-1);

        /*Enable Timers*/
        TimerEnable(TIMER1_BASE,TIMER_A);
        //Tone

        PWMGenPeriodSet(PWM1_BASE, PWM_GEN_1, pwmLoad[i-1]);

        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, pwmLoad[i-1]);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 0.5*pwmLoad[i-1]);
        PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
        PWMGenEnable(PWM0_BASE, PWM_GEN_2);
        //Filter Clock

        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, filterPWMLoad[i-1]);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0.5*filterPWMLoad[i-1]);
        PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);
        PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    }
    else if(i == 3){
        /*Timer3 for Tone(LED) 3*/
        ui32Length3 = ui32Length2;
        TimerLoadSet(TIMER1_BASE, TIMER_A, ui32Length3-1);
        /*Enable Timers*/
        TimerEnable(TIMER1_BASE,TIMER_A);
        //Tone


        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, pwmLoad[i-1]);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 0.5*pwmLoad[i-1]);
        PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
        PWMGenEnable(PWM0_BASE, PWM_GEN_2);
        //Filter Clock

        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, filterPWMLoad[i-1]);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0.5*filterPWMLoad[i-1]);
        PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);
        PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    }
    else if(i == 4){
        TimerLoadSet(TIMER1_BASE, TIMER_A, ui32Length3-1);
        /*Enable Timers*/
        TimerEnable(TIMER1_BASE,TIMER_A);


        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, pwmLoad[i-1]);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 0.5*pwmLoad[i-1]);
        PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
        PWMGenEnable(PWM0_BASE, PWM_GEN_2);
        //Filter Clock

        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, filterPWMLoad[i-1]);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0.5*filterPWMLoad[i-1]);
        PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);
        PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    }
    else if(i == 5){
        TimerLoadSet(TIMER1_BASE, TIMER_A, ui32Length3-1);
        /*Enable Timers*/
        TimerEnable(TIMER1_BASE,TIMER_A);

        //Tone

        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, pwmLoad[i-1]);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 0.5*pwmLoad[i-1]);
        PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
        PWMGenEnable(PWM0_BASE, PWM_GEN_2);
        //Filter Clock

        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, filterPWMLoad[i-1]);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0.5*filterPWMLoad[i-1]);
        PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);
        PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    }
    else{
        TimerLoadSet(TIMER1_BASE, TIMER_A, ui32Length3-1);
        /*Enable Timers*/
        TimerEnable(TIMER1_BASE,TIMER_A);
        //Tone

        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, pwmLoad[i-1]);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 0.5*pwmLoad[i-1]);
        PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
        PWMGenEnable(PWM0_BASE, PWM_GEN_2);
        //Filter Clock

        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, filterPWMLoad[i-1]);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0.5*filterPWMLoad[i-1]);
        PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);
        PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    }
}

void setUpMasterTimer(void){
    uint32_t ui32MasterPeriod;
    /*Timer Configuration*/
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    /*Delay Calculations*/
    //a 0.001 Second resolution for master timer
    ui32MasterPeriod = (SysCtlClockGet()/1000);
    TimerLoadSet(TIMER0_BASE,TIMER_A, ui32MasterPeriod - 1);
    /*Interrupt Enable*/
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE,TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
    /*Timer Enable*/
    TimerEnable(TIMER0_BASE,TIMER_A);
}

void setUpRampTimer(void){
    uint32_t ui32MotorInterval;
    /*Timer Configuration*/
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
    TimerConfigure(TIMER2_BASE, TIMER_CFG_PERIODIC);
    /*Delay Calculations*/
    //a 0.01 Second resolution for increasing duty cycle
    ui32MotorInterval = (SysCtlClockGet()/100);
    TimerLoadSet(TIMER2_BASE,TIMER_A, ui32MotorInterval - 1);
    /*Interrupt Enable*/
    IntEnable(INT_TIMER2A);
    TimerIntEnable(TIMER2_BASE,TIMER_TIMA_TIMEOUT);
   // TimerEnable(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
}
void transmitUART(void){
   int j;
   GPIOIntDisable(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3);

   if(phase1){
      UARTCharPut(UART0_BASE, 0x72);
      UARTCharPut(UART1_BASE, 0x72);
      if(!templatePlayed){
         // make this a function
         UARTSendData(tonesIndex, 6);
      }
      else{
         UARTSendData(templateSong, 6);
      }
   } //The following licks where performed during the lick Window
   else if(phase3 /*&& !alreadyTransmitted */){
      lickData[2] = (char)songDeviation;
      UARTSendData(lickData, sizeof(lickData));
      memcpy(&lickData[1], clearArray, sizeof(clearArray));
   }

   GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3);
}

void MasterTimerIntHandler(void)
{
    //Clear master timer Interrupt
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    if(masterTimerCounter == 0 &&play){
        alreadyTransmitted = 0; //clear the 'Already Transmitted' Flag to transmit
                                //lick data
        UARTCharPut(UART0_BASE, 0x71);
        UARTCharPut(UART1_BASE, 0x71);
        UARTCharPut(UART0_BASE, (char)(trial>>8));
        UARTCharPut(UART0_BASE, (char)(trial));
        UARTCharPut(UART1_BASE, (char)(trial>>8));
        UARTCharPut(UART1_BASE, (char)(trial));
    }

    if(play){
       ++masterTimerCounter;
    }
    if((phase1 && play && masterTimerCounter%(int)(playToneCounter) == 0) || masterTimerCounter == 1){
       motorDutyCycle = 0.10;
       tone++;
       shuffleSong();
       if(templatePlayed){
           GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, GPIO_PIN_6);
           GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0);
       }
       else{
           GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);
           GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, 0);
       }
       playTone(tone);
       if(tone == tonesPerSong){
           transmitUART();
           phase1 = 0;
       }

    }
    else if(masterTimerCounter == phase2CounterTime+1 && play){
       /*TURN OFF TESTING PINS HERE*/
       GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0);
       GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, 0);
       phase2 = 1;

       encouragementDropRandom();
       if((outerPhase0 && templatePlayed )|| (encouragementDrop && !outerPhase0 && templatePlayed /*(noLickEncouragementCount == noLick))*/ )){ // Free Drops
          openRightValve();
          lickData[3] = (1|3<<4);
       }
       else if((outerPhase0 && !templatePlayed) ||( encouragementDrop && !outerPhase0 && !templatePlayed)){
          openLeftValve();
          lickData[3] |= (2|3<<4);
       }
    }
    else if(masterTimerCounter == (phase3CounterTime) && play){
       phase3 = 1;
       phase2 = 0;
       transmitUART();
       if(lickData[1] == 0){ //if neither lick was detected, then transmit this info to GUI
//          transmitUART();          //Because not need to re-send info to GUI, also increment noLick Count
          if( noLickEncouragementCount == noLick){
              noLick = 0;
          }else{
             ++noLick;
          }
       }//turn motor off if it ran during lick window period

       //Make motor run here (only if mouse licked correctly!)
       if(correct){
           motorRun();
       }

    } //Punishment if mouse licked incorrectly, not punished for no licking
    else if(masterTimerCounter == (phase4CounterTime) && play && (!correct&&!noLick)){
        poop = 1;
        poop = 3;
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_5, GPIO_PIN_5);
    }
    else if(masterTimerCounter == (phase4CounterTime) && play && !(!correct&&!noLick)){
       //turn motor off
       motorOff();

    }else if(masterTimerCounter == (phase4CounterTime + breakPeriod)&&!(!correct&&!noLick)){
        tone = 0;
        openedOnce = 0; // Reset this flag for the next Lick Window
        correct = 0;
        phase3 = 0;
        phase1 = 1;
        masterTimerCounter = 0;
        ++cycleCounter;
        GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3);
        if(cycleCounter%(SONGBLOCKLENGTH) == 0 && (outerPhase0||outerPhase1)){
            nonTemplateBlock = !nonTemplateBlock;
            cycleCounter = 0;
        }
        else if(outerPhase2 && (cycleCounter%(biasCounter+1) == 0)){
            cycleCounter = 0;
            if(atLeastOneCorrect){
                atLeastOneCorrect = 0;
                coinFlip();
            }
            else{
                biasCounter = 0;
            }
        }
        else if(outerPhase3){
            if(atLeastOneCorrect){
               atLeastOneCorrect = 0;
               coinFlip();
            }
        }
        else if(outerPhase4){
            coinFlip();
        }
        ++trial;
        if(trial == totalTrials){
            play = 0;
        }

    }
    else if(masterTimerCounter == (phase4CounterTime + punishDuration) &&play){ //punishment over and back to phase 1
        motorOff();
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_5, 0);

    }else if(masterTimerCounter == (phase4CounterTime + punishDuration + breakPeriod) && play && (!correct&&!noLick)){
        tone = 0;
        openedOnce = 0; // Reset this flag for the next Lick Window
        correct = 0;
        phase3 = 0;
        phase1 = 1;
        masterTimerCounter = 0;
        ++cycleCounter;
        GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3);
        if(cycleCounter%(SONGBLOCKLENGTH) == 0 && (outerPhase0||outerPhase1)){
            nonTemplateBlock = !nonTemplateBlock;
            cycleCounter = 0;
        }
        else if(outerPhase2 && (cycleCounter%(biasCounter+1) == 0)){
            cycleCounter = 0;
            if(atLeastOneCorrect){
                atLeastOneCorrect = 0;
                coinFlip();
            }
            else{
                biasCounter = 0;
            }
        }
        else if(outerPhase3){
            if(atLeastOneCorrect){
               atLeastOneCorrect = 0;
               coinFlip();
            }
        }
        ++trial;
        if(trial == totalTrials){
           play = 0;
        }

    }
}


void Tone1TimerHandler(void)
{
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, false);
    PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, false);
}

void Tone2TimerHandler(void)
{
    TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
    PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, false);

}
void Tone3TimerHandler(void)
{
    TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);
    PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, false);

}
void Tone4TimerHandler(void){
    TimerIntClear(TIMER4_BASE, TIMER_TIMA_TIMEOUT);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);
    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, false);


}

// Ramps up the motor by linearly incrementing
// the the duty cycle of the PWM signal to reach a
// maximum desired RPM.
void motorTimerHandler(void){
    TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
    if(motorDutyCycle >= maxMotorDutyCycle){ //if reached max duty cycle, disable this interrupt
       TimerIntDisable(TIMER2_BASE,TIMER_TIMA_TIMEOUT);
       motorDutyCycle = 0.10;
    }else{
       motorDutyCycle += 0.03; //increase by 0.3%
       PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, motorDutyCycle*motorLoad);
    }
}
/*
 * Direction of lick: LSB
 * Correctness: MSB
 */
void LickPortAIntHandler(void)
{
    uint8_t direction;
    uint8_t buffer = ((~GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_3|GPIO_PIN_2))&PIN_MASK)>>2;
    GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_2| GPIO_PIN_3);
    if( buffer>>1 == 1) {
        UARTCharPut(UART0_BASE, 0xfe);
    }
    else if(buffer & 1 == 1) {
        UARTCharPut(UART0_BASE, 0xfd);
    }
    if(!openedOnce){
        lickData[1] = buffer;
        if(lickData[1]>>1 == 1 && !templatePlayed){
           lickData[1] = 2;
           correct = 0;
           correct = 1;
           if((outerPhase2||outerPhase3) && !atLeastOneCorrect){
              atLeastOneCorrect = 1;
           }
           lickData[1] |= 1<<4;
           openRightValve();
        }
        else if(lickData[1]>>1 == 1 && templatePlayed){
           correct = 1;
           if((outerPhase2||outerPhase3) && !atLeastOneCorrect){
              atLeastOneCorrect = 1;
           }
           lickData[1] = 2;
           lickData[1] |= 1<<5;
           openRightValve();
        }
        else if(lickData[1]&1 == 1 && templatePlayed == 1){  // left lick and template played
            lickData[1] = 1;
            correct = 1;
            if((outerPhase2||outerPhase3) && !atLeastOneCorrect){
               atLeastOneCorrect = 1;
            }
            openLeftValve();
            lickData[1] |= 1<<4;
            //if incorrect, make lick window end early.
            //we do this by "fast-forwarding" to time where lick window is over
            //TODO: Update correct/incorrect percentage
       }
       else if(lickData[1]&1 == 1 && templatePlayed == 0) {  // left lick and non-template played
          lickData[1] = 1;
          correct = 0;
          lickData[1] |= 1<<5;
       }
       if(phase1){

       }
       else{
           lickData[3] = 0x00;
       }

     }
     openedOnce = 1;
     GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3);
}

void openRightValve(void){
   rightValveOpen = 1;
   leftValveOpen = 0;
   GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_PIN_1);
   enableValveTimer();
}
void openLeftValve(void){
   rightValveOpen = 0;
   leftValveOpen = 1;
   GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, GPIO_PIN_2);
   enableValveTimer();
}

void enableValveTimer(void){
   SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER5);
   TimerConfigure(TIMER5_BASE, TIMER_CFG_ONE_SHOT);
   if(leftValveOpen){
      TimerLoadSet(TIMER5_BASE, TIMER_A, leftOpenDuration);
   }
   else{
      TimerLoadSet(TIMER5_BASE, TIMER_A, rightOpenDuration);
   }
   IntEnable(INT_TIMER5A);
   TimerIntEnable(TIMER5_BASE, TIMER_TIMA_TIMEOUT);
   /*Enable Timer*/
   TimerEnable(TIMER5_BASE,TIMER_A);

}
void valveInterruptHandler(void){
    //close valve after a predetermined amount of time
    if(leftValveOpen){
       TimerIntClear(TIMER5_BASE, TIMER_TIMA_TIMEOUT);
       GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, 0);
    }
    else if(rightValveOpen){
        TimerIntClear(TIMER5_BASE, TIMER_TIMA_TIMEOUT);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 0);

    }
}

void enableUART(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    GPIOPinConfigure(GPIO_PA0_U0RX);   // PB0 as the Reciever Line
    GPIOPinConfigure(GPIO_PA1_U0TX);   // PB1 as the Transmitter Line
    GPIOPinConfigure(GPIO_PB0_U1RX);   // PB0 as the Reciever Line
    GPIOPinConfigure(GPIO_PB1_U1TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 19200,
        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
    UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 19200,
           UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
    IntEnable(INT_UART0);
    // Receiver Interrupts: When a single character has been received
    // Receiver Timeout: Generated when a character has been received,
    // and a second character has not been received within a 32-Bit Period
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

}
void updateVariables(void){
    int i;

        if(hexChars[0] == 0){
           outerPhase0 = 1;
           outerPhase1 = 0;
           outerPhase2 = 0;
           outerPhase3 = 0;
           outerPhase4 = 0;
        }
        else if(hexChars[0] == 1){
            outerPhase0 = 0;
            outerPhase1 = 1;
            outerPhase2 = 0;
            outerPhase3 = 0;
            outerPhase4 = 0;
        }
        else if(hexChars[0] == 2){
            outerPhase0 = 0;
            outerPhase1 = 0;
            outerPhase2 = 1;
            outerPhase3 = 0;
            outerPhase4 = 0;
        }
        else if(hexChars[0] == 3){
            outerPhase0 = 0;
            outerPhase1 = 0;
            outerPhase2 = 0;
            outerPhase3 = 1;
            outerPhase4 = 0;
        }
        else if(hexChars[0] == 4){
            outerPhase0 = 0;
            outerPhase1 = 0;
            outerPhase2 = 0;
            outerPhase3 = 0;
            outerPhase4 = 1;
        }


        //min/max deviations
        minDev = (int)hexChars[1];
        maxDev = (int)hexChars[2];
        //determine number of tones in songs
        for(i = 3; i < 9; ++i){
            if(hexChars[i] == 0xFF){
                ++noTones;
            }
        }
        tonesPerSong = MAXSONGLENGTH - noTones;
        //store song index into array
        for(i = 0; i < tonesPerSong; ++i) {
            templateSong[i] = hexChars[i + 3];
        }
        totalTrials = (((int)hexChars[9])<<8)|hexChars[10];
        delay = ((int)hexChars[11]<<8)|hexChars[12];
        delay = 10*delay;
        punishDuration = ((int)hexChars[13]<<8)|hexChars[14];
        punishDuration = 10*punishDuration;
        lickWindowDuration = ((int)hexChars[15]<<8)|hexChars[16]; // Units of 10 ms.
        lickWindowDuration = 10*lickWindowDuration; //Convert to clock units (ms)
        /*
         * A note on units
         * */
        toneDuration = 10*(int)hexChars[17];        //Units of 10 ms. eg. 100 units = 100 ms = 1 Second
        timeBetweenTones = 10*(int)hexChars[18];
        rightValveOpenTime = (double)hexChars[19]/100;
        leftValveOpenTime = (double)hexChars[20]/100;
        songRewardTime = (double)hexChars[21]/100;
        noLickEncouragementCount = (int)hexChars[22];
        p = (double)hexChars[23];

        rightOpenDuration = (SysCtlClockGet()*rightValveOpenTime);
        leftOpenDuration = (SysCtlClockGet()*leftValveOpenTime);

        playToneCounter =  toneDuration + timeBetweenTones; //
        phase2CounterTime = tonesPerSong*(playToneCounter);
        phase3CounterTime = (phase2CounterTime + lickWindowDuration);
        phase4CounterTime = (phase3CounterTime + delay);
        ui32Length1 = (SysCtlClockGet()*((double)toneDuration/1000));
}
void UART0IntHandler(void){
    char incomingChar = 0;

    uint32_t ui32Status;
    ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status
    UARTIntClear(UART0_BASE, ui32Status); //clear the asserted interrupts

       //UARTCharPutNonBlocking(UART0_BASE, UARTCharGetNonBlocking(UART0_BASE)); //echo character
       incomingChar = UARTCharGetNonBlocking(UART0_BASE);
       if (takeInParametersFlag == 1)
       {
           hexChars[j] = incomingChar;
           j++;
           if (j >= 24)
           {
               updateVariables();
               updatedVars = 1; //We are now allowed to toggle play/pause
               takeInParametersFlag = 0;

               j = 0;
           }
       }
       else if (incomingChar == 0x11)
       {
           takeInParametersFlag = 1;
       }
       else if (incomingChar == 0x56)
       {
           play = 1;
       }
       else if (incomingChar == 0x55)
       {
           play = 0;
       }
       else if (!play && incomingChar == 0x88)
       {
           GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_PIN_1);
           GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, GPIO_PIN_2);
       }
       else if (!play && incomingChar == 0x89)
       {
           GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1|GPIO_PIN_2 , 0);
       }

}

void UARTSendData(uint8_t *buf, size_t len)
{
   int idx;
   char sendChar;
   for(idx = 0; idx < len; ++idx) {
     sendChar = *buf;
     UARTCharPut(UART0_BASE, (char)sendChar);
     ++buf;
   }
}
