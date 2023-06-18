#include "transmitter.h"

#define CHKERR(fn,rv) do { if ((fn) < 0) return rv; } while(0)

// own print function, we want to be able to fill a char* with a formated text message. (GNU libc)
char* ptrprintf(const char *format,...) {
    va_list args;
    char* buffer = NULL;
    va_start(args, format);
    vasprintf(&buffer, format, args);
    va_end(args);

    return buffer;
}




Transmitter::Transmitter(uint8_t device_address, int retries, uint8_t pwmPin) : i2c_handler(device_address, retries) {
    this->pwmPin = pwmPin;
}

Transmitter::~Transmitter(){
    if (this->errmsg != NULL) { // we already have an old error msg
	free(this->errmsg);
    }
}

void Transmitter::set_errmsg(const char *format,...) {
    va_list args;
    if (this->errmsg != NULL) { // we already have an old error msg
	free(this->errmsg);
    }
    char* buffer = NULL;
    va_start(args, format);
    vasprintf(&buffer, format, args);
    va_end(args);

    this->errmsg = buffer;
}


char* Transmitter::get_errmsg() const{
    return this->errmsg;
}

void Transmitter::reset_i2c_handler() {
    //i2c_handler.set_retries(MAX_RETRIES);
    i2c_handler.reset();
}


void Transmitter::reset() {
    i2c_handler.writeByte(0x00,0b11100011);
    usleep(200000);

}

void Transmitter::shutdown() {
    i2c_handler.writeByte(0x00,0b00100011);
    this->active = false;
}


int8_t Transmitter::startup() {
    uint8_t tempReadValue;
    int i2c_tempresult = i2c_handler.readByte(0x06);
    if (i2c_tempresult < 0) { // error reading byte.
//	this->errmsg = ptrprintf("Cannot read data. Is the chip really at I2C address %02x. Please verify connection",i2c_handler.get_device_address());
	set_errmsg("Cannot read data. Is the chip really at I2C address %02x. Please verify connection",i2c_handler.get_device_address());
	return -1;
    }
    tempReadValue = i2c_tempresult >> 2;
    if (tempReadValue != 0b1101) {
	
	set_errmsg("Chip ID value is %d instead of 13. Is this a QN8066 chip?\n", tempReadValue);
        return -2;
    }

    // Reset everything
    this->reset();

    
    // Setup expected clock source and div
    CHKERR(i2c_handler.writeByte(0x02,0b00010000),-3);
    CHKERR(i2c_handler.writeByte(0x07,0b11101000),-4);
    CHKERR(i2c_handler.writeByte(0x08,0b00001011),-5);

    // Enable RDS TX and set pre-emphasis
    // Here, I am assuming a pre-emphasis of 50us and RDS enabled
    CHKERR(i2c_handler.writeByte(0x01, 0b00000000 | 1 << 6),-6);

    // Exit standby, enter TX
    CHKERR(i2c_handler.writeByte(0x00, 0b00001011),-7);
    usleep(200000);

    // Reset aud_pk
    CHKERR(i2c_handler.writeByte(0x24, 0b11111111),-8);
    CHKERR(i2c_handler.writeByte(0x24, 0b01111111),-9);

    // not in datasheet, but found online. Better try: disable AGC for improved sound:
    CHKERR(i2c_handler.writeByte(0x6e, 0b10110111),-10);
    this->set_reg_vga();
    this->active = true;
    return 0;
}

int8_t Transmitter::set_frequency(double frequency) {
    this->frequency = frequency;
    int tempFreq = (int)((frequency - 60) / 0.05);

//    wiringPiI2CWriteReg8(fd, 0x19, 0b00100000 | (tempFreq >> 8));
    CHKERR(i2c_handler.writeByte(0x19,    0b00100000 | (tempFreq >> 8)),-1); // with RDS enabled
//    wiringPiI2CWriteReg8(fd, 0x1b, 0b11111111 & tempFreq);
    CHKERR(i2c_handler.writeByte(0x1b, 0b11111111 & tempFreq),-2);
    return 0;
}

void Transmitter::disable_autooff() {
    this->autooff=false;
    i2c_handler.writeByte(0x27, 0b00111010); // also sets soft clip threshold to 3db of 0.5V and loudest 75kHz Pilot (10%)
}

void Transmitter::enable_autooff() {
    this->autooff=true;
    i2c_handler.writeByte(0x27, 0b00001010); // also sets soft clip threshold to 3db of 0.5V and loudest 75kHz Pilot (10%)
}

void Transmitter::set_reg_vga() {
    uint8_t reg_vga;
    reg_vga = (this->softClipping << 7) | ((this->bufferGain & 0x07) << 4) | ((this->digitalGain & 0x03) << 2) | (this->inputImpedance & 0x03);
    i2c_handler.writeByte(0x28, reg_vga);
}

int Transmitter::status() {
    int tmpRead;
    tmpRead = i2c_handler.readByte(0x1a);
    if (tmpRead < 0) {
	set_errmsg("Cannot read data for status. Check for interference!");
	return -1;
    }
    uint8_t aud_pk = (tmpRead >> 3) & 0b1111;
    tmpRead = i2c_handler.readByte(0x0a);
    if (tmpRead < 0) {
	set_errmsg("Cannot read data for status. Check for interference!");
	return -2;
    }
    uint8_t fsm = ( tmpRead >> 4) & 0b1111;
//debug    printf("fsm: %i, aud_pk: %i\n",fsm,aud_pk);

    // reset aud_pk
    CHKERR(i2c_handler.writeByte(0x24,0b11111111),-3);
    CHKERR(i2c_handler.writeByte(0x24,0b01111111),-4);
    return aud_pk | (fsm << 4); // lower 4 bits is aud_pk, higher 4 bits is fsm. fam should be 10 and aud_pk < 14
}

int8_t Transmitter::transmit_rds(const uint16_t full8byte_msg[4]){
    int tmpRead;
    tmpRead = i2c_handler.readByte(0x01);
    if (tmpRead < 0) return -2;
    uint8_t rds_status_byte = tmpRead;
    uint8_t rds_send_toggle_bit = rds_status_byte >> 1 & 0b1;
    tmpRead = i2c_handler.readByte(0x1a);
    if (tmpRead < 0) return -3;
    uint8_t rds_sent_status_toggle_bit = tmpRead >> 2 & 0b1;
    
    // write rds data into rds registers starting at 0x1c:
//    i2c_handler.writeBytes(0x1c,rds_bytes,rds_bytes_length);

    for (int counter=0;counter<4;counter++) {
	CHKERR(i2c_handler.writeByte(0x1c+(counter*2),(uint8_t)(full8byte_msg[counter] >>8)),-4);
	CHKERR(i2c_handler.writeByte(0x1d+(counter*2),(uint8_t)(full8byte_msg[counter] & 0xFF)),-5);
    }

    // toggle status bit
    CHKERR(i2c_handler.writeByte(0x01, rds_status_byte ^ 0b10),-6);
    usleep(87000);
    
    // WAIT FOR TOGGLE

     if ((i2c_handler.readByte(0x1a) >> 2 & 1) == rds_sent_status_toggle_bit) {
        int i = 0;
        while ((i2c_handler.readByte(0x1a) >> 2 & 1) == rds_sent_status_toggle_bit) {
            usleep(10000);
            i++;
            if (i > 50) {
               set_errmsg("rdsSentStatusToggleBit failed to flip.\n");
                // RDS has failed to update, reset the device (e.g., reset())
		return -1;
//	           break;
            }
        }
    }
    return 0;
}

void Transmitter::set_softclipping(bool enabled) {
    this->softClipping = enabled; // set class variable
    this->set_reg_vga();
}

void Transmitter::set_buffergain(uint8_t gainBits) { // 0->5, see datasheet
    if (gainBits > 5) {
	printf("Can not set TX input buffer gain higher then 5. Consider changing Input Impedance for further changes");
	return;
    }
    this->bufferGain = gainBits;
    this->set_reg_vga();
}

void Transmitter::set_digitalgain(uint8_t gainBits) { // 0->2, see datasheet
    if (gainBits > 2) {
	printf("Can not set TX digital gain higher then 2! Consider not using digital gain.");
	return;
	}
    this->digitalGain = gainBits;
    this->set_reg_vga();
}

void Transmitter::set_inputimpedance(uint8_t impedance) { // 0=10k,1=20k,2=40k,3=80k Ohm
    if (impedance > 3) {
	printf("Can not set TX input impedance higher then 3 (80kOhm).");
	return;
    }
    this->inputImpedance = impedance;
    this->set_reg_vga();
}

void Transmitter::set_power(uint16_t power) {
    // to implement rasppi PWM
    this->power = power;
}

void Transmitter::write_register(uint8_t reg_address, uint8_t value) {
    i2c_handler.writeByte(reg_address, value);
}

uint8_t Transmitter::read_register(uint8_t reg_address) {
    return i2c_handler.readByte(reg_address);
}


// Define getter functions (generic)
bool Transmitter::get_softclipping() const {
    return softClipping;
}

uint8_t Transmitter::get_buffergain() const {
    return bufferGain;
}

uint8_t Transmitter::get_digitalgain() const {
    return digitalGain;
}

uint8_t Transmitter::get_inputimpedance() const {
    return inputImpedance;
}

double Transmitter::get_frequency() const {
    return frequency;
}

uint16_t Transmitter::get_power() const {
    return power;
}
