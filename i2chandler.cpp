#include "i2chandler.h"
#include <iostream>
#include <unistd.h>

I2CHandler::I2CHandler(int device_address, int retry_attempts)
    : retry_attempts(retry_attempts) {

    // Initialize WiringPi
    if (wiringPiSetup() == -1) {
        throw std::runtime_error("WiringPi setup failed");
        
    }
    this->device_address = device_address;
    fd = wiringPiI2CSetup(device_address);
    if (fd < 0) {
        throw std::runtime_error("Failed to setup I2C device");
    }
}

void I2CHandler::reset() {
}

int I2CHandler::get_device_address() {return this->device_address;}

int I2CHandler::readByte(uint8_t reg_address) {  // return -1 if error, or 0-255 value read
    int attempts = 0;
    int result = -1;

    while (attempts < retry_attempts) {
        result = wiringPiI2CReadReg8(fd, reg_address);
        if (result >= 0) {
            break;
        }
        attempts++;
        usleep(50000); // Wait 50ms before retrying
    }

//    if (result < 0) {
        //std::cerr << "Failed to read register: " << static_cast<int>(reg_address) << std::endl;
//    }

    return result;
}

int I2CHandler::writeByte(uint8_t reg_address, uint8_t data) {
    int attempts = 0;
    int result = -1;

    while (attempts < retry_attempts) {
        result = wiringPiI2CWriteReg8(fd, reg_address, data);
        if (result >= 0) {
            break;
        }
        attempts++;
        usleep(20000); // Wait 20ms before retrying
    }

    if (result < 0) {
//        std::cerr << "Failed to write register: " << static_cast<int>(reg_address) << std::endl;
	
        return -1;
    }

    return attempts;
}

bool I2CHandler::writeBytes(uint8_t reg, const uint8_t* data, size_t data_length) {
    for (int retries = 0; retries < this->retry_attempts; retries++) {
        bool success = true;
        for (size_t i = 0; i < data_length; i++) {
            if (wiringPiI2CWriteReg8(fd, reg + i, data[i]) == -1) {
                success = false;
                break;
            }
        }

        if (success) {
            return true;
        }
        usleep(1000);  // Wait for 1ms before retrying
    }

//    perror("Failed to write bytes in I2CHandler_write_bytes");

    return false;
}