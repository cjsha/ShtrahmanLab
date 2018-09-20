#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

unsigned int timerCounter = 0;
unsigned int freq = 1;

int main(void)
{
   uint32_t ui32Period1;
   uint32_t ui32Period2;

   SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
   /*PE0,PE1,PE2*/
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
   GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2);
   /*PC4, PC5*/
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
   GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE,GPIO_PIN_4|GPIO_PIN_5);
   /*PF0*/
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
   GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);
   /*PA4*/
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
   GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_4);


   SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
   //Full width 32 bit Timer
   SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

   TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
   TimerConfigure(TIMER1_BASE, TIMER_CFG_ONE_SHOT);

   ui32Period1 = (SysCtlClockGet() /freq) / 2;
   ui32Period2 = 20000000;

   TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period1 -1);
   TimerLoadSet(TIMER1_BASE, TIMER_A, ui32Period2  - 1);
   IntEnable(INT_TIMER0A);
   IntEnable(INT_TIMER1A);

   TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
   TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
   IntMasterEnable();
   TimerEnable(TIMER0_BASE, TIMER_A);


   while(1){}
}
void Timer0IntHandler(void){
// Clear the timer interrupt
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
// Read the current state of the GPIO pin and
// write back the opposite state
    if(GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_2))
    {
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2, 0);
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4|GPIO_PIN_5, 0);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_4, 0);
    }
    else
    {
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2 );
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_4|GPIO_PIN_5);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_4, GPIO_PIN_4);
    }

    timerCounter++;
    if (timerCounter % 29 == 0){
        TimerEnable(TIMER1_BASE, TIMER_A);
    }

}

void Timer1IntHandler(void){
// Clear the timer interrupt
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
// Read the current state of the GPIO pin and
// write back the opposite state
    if(GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_2))
    {
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0);
    }
    else
    {
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 2);
    }
}
