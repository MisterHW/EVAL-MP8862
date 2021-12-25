/*
* MP8862 - 2.8V-22V VIN, 2A IOUT, 4-Switch, Integrated Buck-Boost Converter with I2C Interface
*
* Author  : Helge B. Wurst
* Created : 2021-07-14
* License : Creative Commons - Attribution - ShareAlike 3.0
*/

#include "MP8862.h"

bool MP8862::init(I2C_HandleTypeDef *_hI2C, MP8862_address addr) {
    hI2C = _hI2C;
    deviceAddress = addr;
    initialized   = isReady();
    VOUT_soft_limit_mV = 5500;
    IOUT_soft_limit_mA = 3000;
    return initialized;
}

bool MP8862::isReady() {
    return HAL_OK == HAL_I2C_IsDeviceReady(hI2C, deviceAddress << 1, 3, 5);
}


bool MP8862::write(MP8862_register reg, uint8_t *data, uint8_t len ) {
    // S
    // deviceAddress << 1 | 0 : ACK : REG_ADDR : ACK
    // DATA0 : ACK [ : DATA1* : ACK [ : ... ]]
    // P
    // *: Register addresses are post-incremented, and *** read-only registers will be skipped ***.
    return HAL_OK == HAL_I2C_Mem_Write( hI2C, deviceAddress << 1, reg, 1, data, len, 5 );
}

bool MP8862::write(MP8862_register reg, uint8_t value) {
    return write( reg, &value, 1 );
}



bool MP8862::read(MP8862_register reg, uint8_t *data, uint8_t len ) {
    // S
    // deviceAddress << 1 | 0 : ACK : REG_ADDR_K : ACK
    // Sr
    // deviceAddress << 1 | 1 : ACK : BYTE K [: ACK : BYTE K+1* [ : ... ]] : NACK
    // P
    // *: Datasheet p.23 I2C write and read examples do not mention multi-byte read, but experimental evidence shows:
    //    - when master ACK is sent instead of NACK, multiple bytes can be read from auto-incrementing reg addresses
    //    - unmapped register addresses 0x0D ... 0x26 and 0x2A .. 0xFF read as 0x00
    //    -
    return HAL_OK == HAL_I2C_Mem_Read( hI2C, deviceAddress << 1, reg, 1, data, len, 5 );
}

bool MP8862::hardwarePowerUp(bool (*callback_set_enable_pin)(uint16_t ID, uint8_t state), uint16_t ID, MP8862_retry_count trials) {
    if(*callback_set_enable_pin == nullptr){
        return false;
    }
    uint8_t reg_default_off = MP8862_CTL1_DEFAULT_OUTPUT_OFF;
    bool success = false;

    callback_set_enable_pin( ID , 1 ); // set hardware EN = HI (starts time-critical power-up)
    for(uint8_t i = 0; i < trials; i++){ // Keep trying to get ACK, then immediately proceed to set CTL1.
        success = HAL_OK == HAL_I2C_Mem_Write(hI2C, deviceAddress << 1, MP8862_REG_CTL1, 1, &reg_default_off, 1, 1);
        if( success ){
           break;
        }
    }

    if( not success ){
        callback_set_enable_pin( ID , 0 );
        return false;
    } else {
        /* Leave hardware_EN on, output is off via soft_EN in CTL1 and responds to I2C commands.
         * Usage:
         * if( MP8862_inst.hardwarePowerUp( MP8862_set_EN, MP8862_RETRY_I2C_400kHz ) ) {
         *   MP8862_inst.setVoltageSetpoint_mV( <new setpoint> );
         *   MP8862_inst.write( MP8862_REG_CTL1 , MP8862_CTL1_DEFAULT_OUTPUT_ON );
         * }
         */
        return true;
    }
}

bool MP8862::setEnable(bool soft_EN) {
    uint8_t reg;
    bool success = read( MP8862_REG_CTL1 , &reg, 1 );
    reg |= MP8862_REG_CTL1_bits::MP8862_EN;
    return success & write( MP8862_REG_CTL1 , &reg, 1 );
}

bool MP8862::setCurrentLimit_mA(uint16_t current_mA) {
    if( current_mA  > IOUT_soft_limit_mA ){
        return false;
    }
    if(current_mA > 4025){ // highest permitted value producing int part 80
        current_mA = 4025; // 4.0 A max (I_LIM = 0x50)
    }
    uint32_t limit_raw = (current_mA * 1311 + (24 * 1311)) >> 16; // * 0.02 (~ 1311/65536)
    uint8_t tmp = limit_raw & 0x7F;
    return write( MP8862_REG_IOUT_LIM , &tmp, 1 );
}

bool MP8862::readCurrentLimit_mA(uint16_t &current_mA) {
    bool success;
    uint8_t tmp;
    success = read( MP8862_REG_IOUT_LIM , &tmp, 1 );
    current_mA = (tmp & 0x7F) * 50; // scale by 50 mA / LSB
    return success;
}

bool MP8862::setVoltageSetpoint_mV(uint16_t voltage_mV) {
    if( voltage_mV  > VOUT_soft_limit_mV ){
        return false;
    }
    if( voltage_mV > 20480 ){ // highest permitted value producing int part 2047
        voltage_mV = 20480; // 20.47 V max (VOUT = 2047)
    }
    uint8_t tmp[3];
    uint32_t voltage_raw = (voltage_mV * 819 + (5 * 819)) >> 13; // * 0.1 (~ 819/8192)
    tmp[0] = voltage_raw & 0x07;                 // VOUT_L value = voltage_raw[ 2:0]
    tmp[1] = (voltage_raw >> 3) & 0xFF;          // VOUT_H value = voltage_raw[10:3]
    tmp[2] = MP8862_GO_BIT | MP8862_PG_DELAY_EN; // Apply VOUT changes by setting the GO bit right after updating VOUT.
    return write( MP8862_REG_VOUT_L , tmp, 3 );
}

bool MP8862::readVoltageSetpoint_mV(uint16_t &voltage_mV) {
    bool success;
    uint8_t tmp[2];
    success = read( MP8862_REG_VOUT_L , tmp, 2 );
    voltage_mV = (tmp[1] << 3 | (tmp[0] & 0x07)) * 10; // scale by 10 mV / LSB
    return success;
}

bool MP8862::readPG() {
    uint8_t SR;
    bool success = read(MP8862_REG_STATUS, &SR, 1);
    return success && ((SR & MP8862_STATUS_PG) != 0);
}
