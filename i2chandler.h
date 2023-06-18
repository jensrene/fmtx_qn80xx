#ifndef I2CHANDLER_H
#define I2CHANDLER_H

#include <cstdint>
#include <stdexcept>
#include <wiringPi.h>
#include <wiringPiI2C.h>

class I2CHandler {
public:
    I2CHandler(int device_address, int retry_attempts = 3);
    void reset();
    int readByte(uint8_t reg_address);
    int writeByte(uint8_t reg_address, uint8_t data);  //returns number of attempts needed (max=retry_attempts set at class init)
    bool writeBytes(uint8_t reg_address, const uint8_t* data, size_t data_length);
    int get_device_address();

private:
    int fd;
    int retry_attempts;
    int device_address;
};

#endif // I2CHANDLER_H

