#include "pwmcontroller.h"
#include <wiringPi.h>


//TMP:
#include <cstdio>


// Constructor
PWMController::PWMController(uint8_t pin, float periodInNs, float voltage_max) {
    pinNumber = pin;
    supplyVoltage = voltage_max;

    // calculate pwmRange and pwmClock based on desired period
//    setPeriod(periodInNs);

    // Setup the GPIO
    wiringPiSetupGpio();
    pinMode(pinNumber, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);
//    pwmSetClock(pwmClock);
//    pwmSetRange(pwmRange);
    setPeriod(periodInNs);
}

// sets/stores supply/system voltage for calc.
void PWMController::setSupplyVoltage(float voltage) {
    supplyVoltage = voltage;
}


// Method to set PWM duty cycle
void PWMController::setDutyCycleRaw(uint32_t dutyCycle) {
    pwmWrite(pinNumber, dutyCycle);
}

void PWMController::setDutyCycle(uint16_t dutyCycle) {
    float divisor = 65536/(pwmRange);
//    printf("SD: divisor is: %.3f   duty result %u/%u\n",divisor,(int)((dutyCycle/divisor)),pwmRange);
    setDutyCycleRaw((int)(dutyCycle/divisor));
}


// Method to set PWM period
float PWMController::setPeriod(float periodInNs) {
    return 1.0e9 / setFrequency(1.0 / (periodInNs / 1e9));
}


// Method to set PWM frequency

float PWMController::setFrequency(float frequencyInHz, uint8_t precision) {
    uint32_t R, C;
    float actual_F;
    float F = frequencyInHz;
//    printf("SF: Target frequency is %.3f Hz\n",F);
    // If precision is not zero, calculate R as 2^precision - 1
    if (precision != 0) {
        R = (1 << precision) - 1;
    } else {
        // Start with max R
        R = 65535;
    }

    // Calculate C based on the formula C = B / (F * R)
    C = round(INTERNAL_CLOCK_FREQUENCY / (F * R));
// printf("SF: First Clock : %u\n",C);

    // C does not work well when 1, and C=0 is pwm disable. 
    if (C < 2) {
        C = 2;
        // If C is lower than the minimum valid value, recalculate R for the best possible frequency
        R = round(INTERNAL_CLOCK_FREQUENCY / (F * C));
    }
//  printf("SF: final Clock: %u  final Range: %u\n",C,R);
    // Call pwmSetClock and pwmSetRange with calculated C and R
    pwmSetRange(R);
//    usleep(100);
    pwmSetClock(C);

    // Calculate the actual set frequency F = B / (C * R)
    actual_F = INTERNAL_CLOCK_FREQUENCY / (C * R);
//  printf("SF: final Freq: %.3f\n",actual_F);

    // set global variables:
    pwmRange = R;
    pwmClock = C;

    return actual_F;
}


// Method to set voltage
void PWMController::setVoltage(float voltage) {
    // calculate duty cycle based on desired voltage and supply voltage
    double divisor = supplyVoltage/pwmRange; // supply voltage piece.
    setDutyCycleRaw((int)(voltage/divisor));
}

// method to retrieve the current frequency. 
float PWMController::getFrequency() {
    return INTERNAL_CLOCK_FREQUENCY / (pwmClock * pwmRange);
}
