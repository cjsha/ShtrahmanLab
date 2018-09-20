#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
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

#define PWM_FREQUENCY 1000
#define FREQ1  261.63
#define CLOCK1 FREQ1*75
#define FREQ2  320.63
#define CLOCK2 FREQ2*75
#define FREQ3  392.00
#define CLOCK3 FREQ3*75
#define FREQ4  466.16
#define CLOCK4 FREQ4*75

volatile uint32_t ui32FreqLoad[4];
volatile uint32_t ui32ClockLoad[4];
volatile uint32_t ui32PWMClock;

double frequencies[] = {FREQ1,FREQ2,FREQ3,FREQ4};
double clock[] = {CLOCK1, CLOCK2, CLOCK3, CLOCK4};
volatile uint8_t toneDutyCycle[] = {0.5*FREQ1, 0.5*FREQ2, 0.5*FREQ3, 0.5*FREQ4};
volatile uint8_t clockDutyCycle[] = {0.5*CLOCK1,0.5*CLOCK2,0.5*CLOCK3,0.5*CLOCK4};

void pwmSysPeripheralSetup();
void GPIOPeriphEnable();
void GPIOPWMPinConfigure();
void PWMFreqConfig();
void GenEnablePWM();

int main(void)
{

  //ui8Adjust = 500000
  SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
//  SysCtlPWMClockSet(SYSCTL_PWMDIV_64);
//  GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0);
//  GPIOPinConfigure(GPIO_PD0_M1PWM0);
//
//  ui32PWMClock = SysCtlClockGet() / 64;
//  ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;
//  PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
//  PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, ui32Load);
//  PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, 5000000 * ui32Load / 1000);
//  PWMOutputState(PWM1_BASE, PWM_OUT_0_BIT, true);
//  PWMGenEnable(PWM1_BASE, PWM_GEN_0);
  GPIOPeriphEnable();
  pwmSysPeripheralSetup();
  GPIOPWMPinConfigure();
  PWMFreqConfig();
  GenEnablePWM();

  while(1){
  }

}

void GPIOPeriphEnable( void ){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
}

void pwmSysPeripheralSetup( void ){
    SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0|GPIO_PIN_1);
    GPIOPinTypePWM(GPIO_PORTE_BASE, GPIO_PIN_4);
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_4|GPIO_PIN_7);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_3);
    GPIOPinTypePWM(GPIO_PORTA_BASE, GPIO_PIN_7);
}

void GPIOPWMPinConfigure( void ){
    GPIOPinConfigure(GPIO_PD0_M1PWM0);
    GPIOPinConfigure(GPIO_PD1_M0PWM7);
    GPIOPinConfigure(GPIO_PE4_M0PWM4);
    GPIOPinConfigure(GPIO_PF1_M1PWM5);
    GPIOPinConfigure(GPIO_PF3_M1PWM7);
    GPIOPinConfigure(GPIO_PB4_M0PWM2);
    GPIOPinConfigure(GPIO_PB7_M0PWM1);
    GPIOPinConfigure(GPIO_PA7_M1PWM3);
}

void PWMFreqConfig( void ){
    //loop this
    int i;
    ui32PWMClock = SysCtlClockGet() / 64;
    for(i = 0; i < 4; ++i){
       ui32FreqLoad[i] = (ui32PWMClock / frequencies[i]) - 1;
       ui32ClockLoad[i] = (ui32PWMClock / clock[i]) - 1;
    }

    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN); //PE4 TONE 1
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, ui32FreqLoad[0]);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, toneDutyCycle[0]);

    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN); //PB4 Clock 1
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1,  ui32ClockLoad[0]);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, clockDutyCycle[1]);

    PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN); //PD0 Tone 2
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, ui32FreqLoad[1]);
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, toneDutyCycle[1]);

    PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN); //PD1 Clock 2
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3,  ui32ClockLoad[1]);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, clockDutyCycle[1]);

    PWMGenConfigure(PWM1_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN); //PA7 Tone3
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_1, ui32FreqLoad[2]);
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_3, toneDutyCycle[2]);

    PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN); //PF1 Clock 3
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2,  ui32ClockLoad[2]);
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, clockDutyCycle[2]);

    PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN); //PF3 Tone 4
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, ui32FreqLoad[3]);
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, toneDutyCycle[3]);

    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN); //PB7 Clock 4
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0,  ui32ClockLoad[3]);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, clockDutyCycle[3]);


}

void GenEnablePWM( void ){

    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);

    PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);

    PWMOutputState(PWM1_BASE, PWM_OUT_0_BIT, true);
    PWMGenEnable(PWM1_BASE, PWM_GEN_0);

    PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, true);
    PWMGenEnable(PWM0_BASE, PWM_GEN_3);

    PWMOutputState(PWM1_BASE, PWM_OUT_3_BIT, true);
    PWMGenEnable(PWM1_BASE, PWM_GEN_1);

    PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);
    PWMGenEnable(PWM1_BASE, PWM_GEN_2);

    PWMOutputState(PWM1_BASE, PWM_OUT_7_BIT, true);
    PWMGenEnable(PWM1_BASE, PWM_GEN_3);

    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, true);
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
}
