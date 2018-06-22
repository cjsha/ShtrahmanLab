/*
 * Prototype Behavior Code
 * Authors: Max Guerrero and Cris Sharp
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"

unsigned int masterTimerCounter = 0;
unsigned int phase1 = 1;
unsigned int phase2 = 0;
unsigned int phase3 = 0;

uint32_t ui32Length1;
uint32_t ui32Length2;
uint32_t ui32Length3;

void setUpMasterTimer(void);
void setUpTonesTimer(void);
void beginTones(void);
void enablePeripherals(void);
void enableUART(void);
void LickPortAIntHandler(void);
void Tone1TimerHandler(void);
void Tone2TimerHandler(void);
void Tone3TimerHandler(void);
void Tone4TimerHandler(void);
void UART0IntHandler(void);
void RLSendMessage(void);

int main(void)
{
    //Configure System Clock to run at 40MHz
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
    enablePeripherals();
    setUpMasterTimer();
   // setUpTonesTimer();

    while(1)
    {
        while(phase1)
        {

        }
        while(phase2)
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
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //Enable PA2  and 3's weak pull up resistor
    GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_3,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);

    //Enable PWM, Pin Interrupt Register
    GPIOIntRegister(GPIO_PORTA_BASE, LickPortAIntHandler);
    GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3, GPIO_FALLING_EDGE);
    //Enable UART
    enableUART();
    //Enable Pin Interrupt
    GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3);

}

void setUpTonesTimer(void)
{
    /*Timer1 for Tone(LED) 1 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    TimerConfigure(TIMER1_BASE,TIMER_CFG_ONE_SHOT);
    //We want the four tones played consecutively to last a total of
    //1.5 seconds, therefore the duration of each tone is such:
    ui32Length1= (SysCtlClockGet()/2.66667);
    TimerLoadSet(TIMER1_BASE, TIMER_A, ui32Length1 - 1);
    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

     /*Timer2 for Tone(LED) 2*/
     ui32Length2 = ui32Length1;
     SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
     TimerConfigure(TIMER2_BASE, TIMER_CFG_ONE_SHOT);
     TimerLoadSet(TIMER2_BASE,TIMER_A, 2*ui32Length2-1);
     IntEnable(INT_TIMER2A);
     TimerIntEnable(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
     /*Timer3 for Tone(LED) 3*/
     ui32Length3 = ui32Length2;
     SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
     TimerConfigure(TIMER3_BASE, TIMER_CFG_ONE_SHOT);
     TimerLoadSet(TIMER3_BASE, TIMER_A, 3*ui32Length3-1);
     IntEnable(INT_TIMER3A);
     TimerIntEnable(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
     /*Enable Timers*/
     TimerEnable(TIMER1_BASE,TIMER_A);
     TimerEnable(TIMER2_BASE,TIMER_A);
     TimerEnable(TIMER3_BASE,TIMER_A);
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

void beginTones(void){

}


void MasterTimerIntHandler(void)
{
    //Clear master timer Interrupt
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    ++masterTimerCounter;
    if(phase1 && masterTimerCounter == 2000){
     //begin tones timer (enable Timer for Tones)
     setUpTonesTimer();
    }
    else if(masterTimerCounter == 4150){
       phase2 = 1;
       phase1 = 0;
    }
    else if(masterTimerCounter == 8150){
       phase3 = 1;
       phase2 = 0;
    }
    else if(masterTimerCounter == 10150){
       phase3 = 0;
       phase1 = 1;
       masterTimerCounter = 0;
    }

}

void Tone1TimerHandler(void)
{
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_2, 4);

}

void Tone2TimerHandler(void)
{
    TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
    if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2)){
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 8);
    }

}
void Tone3TimerHandler(void)
{
    TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
    if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_3)){
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 2);
        //Re-Enable and Set one more length of time
        //Before turning off light
        TimerEnable(TIMER3_BASE, TIMER_A);
        TimerLoadSet(TIMER3_BASE, TIMER_A, ui32Length1 - 1);
    }   //Turn off after its length of duration
    else if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1)){
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1,0);
        //Offset master timer counter to zero to start tracking
        //lick window duration
    }

}
void Tone4TimerHandler(void){


}

void LickPortAIntHandler(void)
{
    GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_2| GPIO_PIN_3);
    if(!GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2)){
        //Left port licked flag (LED FOR NOW)
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);
        RLSendMessage();
        //TODO: enable valve one-shot timer for valve open/close

    }
    else if( !GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_3)){
        //Right Port Licked (LED FOR NOW)
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
    }
    else if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1)){
        //Toggle Off
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
    }
    else if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2)){
        //Toggle off
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
    }
}

void enableUART(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);  // PA6 as the Reciever Line
    GPIOPinConfigure(GPIO_PA1_U0TX);   // PA7 as the Transmitter Line
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
    IntEnable(INT_UART0);
    // Receiver Interrupts: When a single character has been received
    // Receiver Timeout: Generated when a character has been received,
    // and a second character has not been received within a 32-Bit Period
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

}

void UART0IntHandler(void){
    uint32_t ui32Status;
    ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status
    UARTIntClear(UART0_BASE, ui32Status); //clear the asserted interrupts
    while(UARTCharsAvail(UART0_BASE)) //loop while there are chars
    {
    UARTCharPutNonBlocking(UART0_BASE, UARTCharGetNonBlocking(UART0_BASE)); //echo character
    }
}

void RLSendMessage(void){
    UARTCharPut(UART0_BASE, 'R');
    UARTCharPut(UART0_BASE, 'i');
    UARTCharPut(UART0_BASE, 'g');
    UARTCharPut(UART0_BASE, 'h');
    UARTCharPut(UART0_BASE, 't');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, 'l');
    UARTCharPut(UART0_BASE, 'i');
    UARTCharPut(UART0_BASE, 'c');
    UARTCharPut(UART0_BASE, 'k');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '\n');
}