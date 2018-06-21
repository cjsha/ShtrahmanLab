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
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

unsigned int masterTimerCounter = 0;
unsigned int phase1 = 1;

void setUpMasterTimer(void);

int main(void)
{
    setUpMasterTimer();

    while(1){
        while(phase1){

        }
    }
}

void setUpMasterTimer(void){
    uint32_t ui32MasterPeriod;
    //Configure System Clock to run at 40MHz
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

    //Enable GPIO Peripheral and configure the pins connected to the LED's as outputs
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);

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

void MasterTimerIntHandler(void)
{
    //Clear master timer Interrupt
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    ++masterTimerCounter;
    if(masterTimerCounter == 1000){
      GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 4);
    }
    else if(masterTimerCounter == 2000){
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_3,8);
    }

}

