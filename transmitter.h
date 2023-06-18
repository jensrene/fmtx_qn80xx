#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include "i2chandler.h"
#include "rdsencoder.h"
#include "pwmcontroller.h"

class Transmitter {
public:
    /**
     * @brief Transmitter constructor.
     * @param device_address The device address.
     * @param retries Number of retries.
     * @param pwmPin  Pin number of PWM, 0 for no PWM
     */
    Transmitter(uint8_t device_address, int retries, uint8_t pwmPin = 0);
    ~Transmitter();

    /**
     * @brief Shutdown the transmitter.
     */
    void shutdown();

    /**
     * @brief Startup the transmitter.
     * @return 0 if all is fine, otherwise neg error.
     */
    int8_t startup();

    /**
     * @brief Set the frequency of the transmitter.
     * @param frequency The frequency to set.
     */
    int8_t set_frequency(double frequency);

    /**
     * @brief Reset the transmitter.
     */
    void reset();

    /**
     * @brief Get the status of the transmitter.
     * @return The status of the transmitter. Neg on error
     */
    int status();

    /**
     * @brief Transmit RDS data.
     * @param rds_bytes Pointer to the RDS data.
     * @param rds_bytes_length Length of the RDS data.
     * @return Result of the transmission.
     */
    int8_t transmit_rds(const uint8_t* rds_bytes, size_t rds_bytes_length);

    /**
     * @brief Transmit RDS data as 8byte msg (4*16bit word)
     * @param full8byte_msg RDS data as 4*16bit
     * @return Result of the transmission.
     */

    int8_t transmit_rds(const uint16_t full8byte_msg[4]);


    /**
     * @brief Disable auto-off feature.
     */
    void disable_autooff();

    /**
     * @brief Enable auto-off feature.
     */
    void enable_autooff();

    /**
     * @brief Set soft clipping.
     * @param enabled Whether soft clipping is enabled or not.
     */
    void set_softclipping(bool enabled);

    /**
     * @brief Set buffer gain.
     * @param gainBits The buffer gain value.
     */
    void set_buffergain(uint8_t gainBits);

    /**
     * @brief Set digital gain.
     * @param gainBits The digital gain value.
     */
    void set_digitalgain(uint8_t gainBits);

    /**
     * @brief Set input impedance.
     * @param impedance The input impedance value.
     */
    void set_inputimpedance(uint8_t impedance);

    /**
     * @brief Write a value to a register.
     * @param reg_address The register address.
     * @param value The value to write.
     */
    void write_register(uint8_t reg_address, uint8_t value);

    /**
     * @brief Read the value from a register.
     * @param reg_address The register address.
     * @return The value read from the register.
     */
    uint8_t read_register(uint8_t reg_address);

    // Declare getter functions
    /**
     * @brief Get the soft clipping status.
     * @return The soft clipping status.
     */
    bool get_softclipping() const;

    /**
     * @brief Get the buffer gain value.
     * @return The buffer gain value.
     */
    uint8_t get_buffergain() const;

    /**
     * @brief Get the digital gain value.
     * @return The digital gain value.
     */
    uint8_t get_digitalgain() const;

    /**
     * @brief Get the input impedance value.
     * @return The input impedance value.
     */
    uint8_t get_inputimpedance() const;
 /**
     * @brief Get the current frequency.
     * @return The current frequency.
     */
    double get_frequency() const;


    /**
     * @brief Set the power of the transmitter as a percentage.
     * @param percentage The power pseudo-percentage (0-10000)-> (0-100.00%)
     */
    void set_power(uint16_t percentage);

    /**
     * @brief Get the current power as a pseudo-percentage. (0-10000)
     * @return The current power pseudo-percentage. (0-10000)-> (0-100.00%)
     */
    uint16_t get_power() const;

    char* get_errmsg() const;

private:
    const uint8_t MAX_RETRIES = 5; // Maximum number of retries
    I2CHandler i2c_handler; // I2C handler object
    void reset_i2c_handler(); // Reset the I2C handler
    void set_reg_vga(); // Set the VGA register (internal use only)

    // Transmitter settings and parameters
    bool active = false; // Transmitter activation status
    bool autooff = false; // Auto-off feature status
    bool softClipping = false; // Soft clipping status
    uint8_t bufferGain = 0b101; // Buffer gain value (3 bits only, 0-7 in value)
    uint8_t digitalGain = 0b10; // Digital gain value (2 bits only, 0-2)
    uint8_t inputImpedance = 0b11; // Input impedance value (2 bits only, 0-3)
    double frequency; // Current frequency setting (in MHz)
    uint8_t pwmPin; // rasp pin used for pwm out (default: 18(p 12), disabled: 0)
    uint16_t power; // Current power setting (0-255)
    char* errmsg = NULL; // last error message

    // sets a new internal error msg.
    void set_errmsg(const char *format,...);

};

#endif // TRANSMITTER_H

// Wiring Connections:
// Raspberry Pi            FM Transmitter
// ------------------------------------
// PWM (PiPin12)      ->   Pin 5
// GND (PiPin9)       ->   Pin 2
// SCL (PiPin5)       ->   Pin 4
// SDA (PiPin3)       ->   Pin 3
// 3.3V (PiPin1)      ->   Pin 1
