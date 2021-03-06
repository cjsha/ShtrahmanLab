#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
//#include "drivers/pinout.h"


/**
 * main.c
 */
void PortAIntHandler(void);

int main(void)
{
    int32_t i32Val;
    //Enable the GPIOA Peripheral
    SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
    //
    // Configure the device pins.
    //

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    //
    //Wait for the GPIOA Module to be ready
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)){

    }
    //Register the port-level interrupt handler. This handler is the first
    //level interrupt handler for all the pin interrupts
    GPIOIntRegister(GPIO_PORTA_BASE, PortAIntHandler);
    //Initialize the GPIO pin configuration
    //Set pins2  SW Controlled
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_2);
    // Enable PA2's weak pull up resistor
    GPIOPadConfigSet(GPIO_PORTA_BASE,GPIO_PIN_2,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);

    //Set pins 1,2 and 3 as output, SW Controlled
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1| GPIO_PIN_2|GPIO_PIN_3);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_2);

    //Make pin 2 high level triggered interrupts.
    //

    GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_2, GPIO_HIGH_LEVEL);
    //
    // Read pin
    //
    i32Val = GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2);

    //
    //Write some pins. Eventhough pins 2,4,5 are specified, those pins
    //are unaffected by this write because they are configured as inputs.
    //At the end of this write, pin 0 is low, and pin 3 is high.
    //


    //
    // Enable the pin interrupts
    //
    GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2);

    GPIOIntClear(GPIO_PORTA_BASE, GPIO_INT_PIN_2);
	while(1){

	}


}

void PortAIntHandler(void){
//    GPIOIntClear(GPIO_PORTA_BASE, GPIOIntStatus(GPIO_PORTA_BASE,true));
    if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2)){
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
    }
    else{
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_2);
    }

}
