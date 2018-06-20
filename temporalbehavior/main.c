#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

char phase=0;
int counter=0;

void ISR(void){
    counter++;
    if (counter>10)
    {
        phase++;
    }
    if (phase>2)
    {
        phase=0;
    }

}

int main(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE,TIMER_CFG_A_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, 3000);
    TimerIntRegister(TIMER0_BASE,TIMER_A,*ISR);
    TimerIntEnable(TIMER0_BASE,TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER0_BASE,TIMER_A);


	while(1){
	    while(phase==0){}
	    while(phase==1){}
	    while(phase==2){}
	}
}
