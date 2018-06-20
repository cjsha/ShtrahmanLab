#include "driverlib/pin_map.h"
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"

#define FREQ1 (uint32_t)1000
#define FREQ2 (uint32_t)(1000*75)
#define FREQ4 (uint32_t)(4020)

#define MAMAOSC (uint32_t)200000000
#define OSCDIV (uint32_t)16
#define PWMDIV (uint32_t)1

#define OSC (uint32_t)(MAMAOSC/OSCDIV)
#define PWMOSC (uint32_t)(OSC/PWMDIV)
#define PERIOD1 (uint32_t)(PWMOSC/FREQ1)
#define DUTY1 (uint32_t)(PERIOD1/2)
#define PERIOD2 (uint32_t)(PWMOSC/FREQ2)
#define DUTY2 (uint32_t)(PERIOD2/2)
#define PERIOD4 (uint32_t)(PWMOSC/FREQ4)
#define DUTY4 (uint32_t) (PERIOD4/2)
/*PWM Connector
 *
 */


int
main(void)
{

    //Set the clock
   SysCtlClockSet(SYSCTL_SYSDIV_16 | SYSCTL_USE_PLL |   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

   //Configure PWM Clock divide system clock by 64
   SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

   // Enable the peripherals used by this program.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);  //The Tiva Launchpad has two modules (0 and 1). Module 1 covers the LED pins
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    //Configure PF1,PF2,PF3 Pins as PWM
    GPIOPinConfigure(GPIO_PF1_M1PWM5);
    GPIOPinConfigure(GPIO_PD0_M1PWM0);
    GPIOPinConfigure(GPIO_PD1_M1PWM1);
    GPIOPinConfigure(GPIO_PE4_M0PWM4);

    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);
    GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPinTypePWM(GPIO_PORTE_BASE, GPIO_PIN_4);

    //Configure PWM Options
    PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM1_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    //Set the Period (expressed in clock ticks)
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, PERIOD1);// PWM For PF5
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, PERIOD2);//
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_1, PERIOD2);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PERIOD4);// PWM Period for PE4

    //Set PWM DUTY1
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, DUTY1);
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, DUTY2);
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_1, DUTY2);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, DUTY4);

    // Enable the PWM generator
    PWMGenEnable(PWM1_BASE, PWM_GEN_2);
    PWMGenEnable(PWM1_BASE, PWM_GEN_0);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);

    // Turn on the Output pins
    PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT | PWM_OUT_0_BIT | PWM_OUT_1_BIT, true);
    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);

    while(1){    }

}





















//#include <stdint.h>
//#include <stdbool.h>
//#include "inc/hw_memmap.h"
//#include "inc/hw_types.h"
//#include "driverlib/sysctl.h"
//#include "driverlib/gpio.h"
//#include "driverlib/debug.h"
//#include "driverlib/pwm.h"
//#include "driverlib/pin_map.h"
//#include "inc/hw_gpio.h"
//#include "driverlib/timer.h"
//#include"inc/tm4c123gh6pm.h"
//
//#define PWM_FREQUENCY 440
//#define PWM_FREQ_2 440*75
//
//
//int main(void)
//{
//    //The following variables will be used to program the PWM. They are defined as “volatile”
//    //to guarantee that the compiler will not eliminate them, regardless of the optimization setting
//    volatile uint32_t ui32Load;
//    volatile uint32_t ui32Load2;
//    volatile uint32_t ui32PWMClock;
//    volatile int adjust;
//    adjust = 83; //The scalar value in order to set number of ticks to be equal to
//                 //Desired Pulse Width
//    uint32_t ui32Period;
//
//
//    //CPU Clock running at 40MHz
//    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
//    //Set the PWM Clock to run at 625KHz
//    SysCtlPWMClockSet(SYSCTL_PWMDIV_32);
//    //Enable the PWM Module 1 and GPIO Port D module
//    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
//    //Must enable, for the PWM output on Port D.
//    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
//    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
//    //Port D pin 0 must be configured as a PWM Output for Module 1,
//    //using generator 0 see schematic.
//    //Store PWM Clock variable to be ticking at 625kHz times two
//    ui32PWMClock = SysCtlClockGet() / 32;
//    //Number of PWM clock ticks to be defined as one PWM Cycle
//    //We should end up with a 55 Hz Freq for PWM
//
//    GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0);
//    GPIOPinConfigure(GPIO_PD0_M1PWM0); //Motor A, PinD0
//
//    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_0);
//    GPIOPinConfigure(GPIO_PF0_M1PWM4); //Motor E, PinF0
//    ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;
//
//    ui32Load2 = (ui32PWMClock / PWM_FREQUENCY) - 1;
//    PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN |PWM_GEN_MODE_NO_SYNC);
//    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, ui32Load);
//    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_4, ui32Load/2);
////    PWMOutputState(PWM1_BASE, PWM_OUT_4_BIT, true);
//
//
//    ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;
//    PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN |PWM_GEN_MODE_NO_SYNC);
//    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, ui32Load);
//    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0,  ui32Load/2);
//
//    PWMGenEnable(PWM1_BASE, PWM_GEN_0);
//    PWMGenEnable(PWM1_BASE, PWM_GEN_2);
//
//    PWMOutputState(PWM1_BASE, PWM_OUT_0_BIT| PWM_OUT_4_BIT, true);
//
//
//
//    /*GPIO Configuration for generating square wave*/
//    /*TODO: Change to a different port */
//   // GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
//
////    /*Timer Configuration*/
////    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
////    TimerConfigure(TIMER0_BASE,TIMER_CFG_PERIODIC);
////    //Determine timer frequency
////    //for now just assume 1000Hz
////    ui32Period = (SysCtlClockGet()/1000)/2;
////    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);
////    /*Interrupt Enable*/
////    IntEnable(INT_TIMER0A);
////    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
////    IntMasterEnable();
////    /*Timer Enable */
////    TimerEnable(TIMER0_BASE, TIMER_A);
//
//    while(1)
//    {
//    }
//
//}
//
//void Timer0IntHandler(void)
//{
// // Clear the timer interrupt
// TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
// // Read the current state of the GPIO pin and
// // write back the opposite state
//if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2))
// {
//  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
// }
// else
// {
//  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 4);
// }
//}
