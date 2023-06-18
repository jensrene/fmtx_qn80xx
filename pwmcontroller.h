#ifndef PWMCONTROLLER_H
#define PWMCONTROLLER_H

#include <wiringPi.h>
#include <stdint.h>
#include <math.h>

#define INTERNAL_CLOCK_FREQUENCY 19200000.0 // freq of PWM 

class PWMController {

private:
    uint8_t pinNumber;    ///< The pin number
    uint32_t pwmRange;    ///< The PWM range
    uint32_t pwmClock;    ///< The PWM clock
    float supplyVoltage;  ///< Supply voltage, defaults to 3.3V

public:
    /**
     * @brief Constructs a new PWMController object
     * 
     * @param pin The pin number (defaults to 18)
     * @param periodInNs The period in nanoseconds (defaults to 18300.0)
     * @param voltage The voltage (defaults to 3.3V)
     */
    PWMController(uint8_t pin = 18, float periodInNs = 18300.0, float voltage = 3.3);

    /**
     * @brief informs the class about the supply/max pwm voltage for calculation. (default is 3.3V)
     * 
     * @param dutyCycle The voltage (=3.3V)
     */
    void setSupplyVoltage(float voltage);

    /**
     * @brief Sets the raw duty cycle of the PWM signal
     * 
     * @param dutyCycle The duty cycle to set
     */
    void setDutyCycleRaw(uint32_t dutyCycle);

    /**
     * @brief Sets the duty cycle of the PWM signal
     * 
     * @param dutyCycle The duty cycle to set (0..65535)
     */
    void setDutyCycle(uint16_t dutyCycle);

    /**
     * @brief Sets the period of the PWM signal
     * 
     * @param periodInNs The desired period in nanoseconds
     * @return The actual period set in seconds
     */
    float setPeriod(float periodInNs);

    /**
     * @brief Sets the frequency of the PWM signal
     * 
     * @param frequencyInHz The desired frequency in Hertz
     * @param precision The precision (defaults to 0, meaning highest possible)
     * @return The actual frequency set in Hertz
     */
    float setFrequency(float frequencyInHz,uint8_t precision = 0);

    /**
     * @brief Sets the voltage by adjusting the PWM duty cycle. Maximum is the supply voltage from class initalization.
     * 
     * @param voltage The voltage to set, relative to the supply voltage.
     */
    void setVoltage(float voltage);

    /**
     * @brief Gets the currently used frequency. 
     *
     */
    float getFrequency();

};

#endif /* PWMCONTROLLER_H */
