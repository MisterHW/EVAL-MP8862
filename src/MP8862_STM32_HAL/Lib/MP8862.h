/*
* MP8862 - 2.8V-22V VIN, 2A IOUT, 4-Switch, Integrated Buck-Boost Converter with I2C Interface
*
* Author  : Helge B. Wurst
* Created : 2021-07-14
* License : Creative Commons - Attribution - ShareAlike 3.0
*/

#ifndef HW_MP8862_H
#define HW_MP8862_H

#include "stm32f7xx_hal.h"

// MP8862 device addresses
enum MP8862_address {
    MP8862_ADDR_0x69 = 0x69, // ADD = 0.00 .. 0.25 * VCC
    MP8862_ADDR_0x6B = 0x6B, // ADD = 0.25 .. 0.50 * VCC
    MP8862_ADDR_0x6D = 0x6D, // ADD = 0.50 .. 0.75 * VCC
    MP8862_ADDR_0x6F = 0x6F, // ADD = 0.75 .. 1.00 * VCC
};

// Register bits (Handling of "Reserved" bits is unclear, writing as "0" seems plausible, however they can read as 0 or 1 unless explicitly specified (see 0x06).

enum MP8862_REG_VOUT_GO_bits {
    MP8862_GO_BIT      = 1 << 0 , // Set bit to 1 to engage voltage change after updating VOUT_L, VOUT_H registers. Resets to 0 when new setpoint is applied.
    MP8862_PG_DELAY_EN = 1 << 1 , // When PG_DELAY_EN D[1] is 1, PG experiences a 100μs rising delay. (default 0)
};

enum MP8862_REG_CTL1_bits {
    MP8862_FREQ_00_500kHz   = 0x0 << 2, // switching frequency (default)
    // MP8862_FREQ bits 01, 10, 11 : reserved
    MP8862_MODE             = 1 << 4 , // switching mode: 0: Enables auto PFM/PWM mode, 1: Sets forced PWM mode (default)
    MP8862_DISCH_EN         = 1 << 5 , // 1: Output discharge function during EN or VIN shutdown (default)
    MP8862_HICCUP_OCP_OVP   = 1 << 6 , // Over-current and over-voltage protection mode: 0: Latch-off, 1: Hiccup (default)
    MP8862_EN               = 1 << 7 , // I2C on/off control (EN is high, the EN bit takes over), soft EN : I2C register does not reset,
                                     // HW EN: reset to defaults, delay to reset depending on DISCH_EN. )
    MP8862_CTL1_DEFAULT_OUTPUT_OFF = 0x70,
    MP8862_CTL1_DEFAULT_OUTPUT_ON  = 0xF0,
};

enum MP8862_REG_CTL2_bits {
    // MP8862_REG_CTL2[3:0] : reserved
    MP8862_SS_00_300us           = 0x0 << 4 , // output start-up soft-start timer
    MP8862_SS_01_500us           = 0x1 << 4 ,
    MP8862_SS_10_700us           = 0x2 << 4 ,
    MP8862_SS_11_900us           = 0x3 << 4 , // (SS default 900 µs)
    MP8862_LINE_COMP_00_OFF      = 0x0 << 6 , // output voltage compensation vs. load current (default OFF)
    MP8862_LINE_COMP_01_2A_100mV = 0x1 << 6 ,
    MP8862_LINE_COMP_10_2A_200mV = 0x2 << 6 ,
    MP8862_LINE_COMP_11_2A_400mV = 0x3 << 6 ,

};

enum MP8862_REG_STATUS_bits {
    MP8862_STATUS_CC_CV = 1 << 4 , // CC/!CV : constant-current output mode (1) or constant voltage output mode (0)
    MP8862_STATUS_OTW   = 1 << 5 , // Over-temperature warning indication.
    MP8862_STATUS_OTP   = 1 << 6 , // Over-temperature protection indication.
    MP8862_STATUS_PG    = 1 << 7 , // Output power good indication.
};

enum MP8862_REG_INTERRUPT_bits{
    MP8862_PG_RISING        = 1 << 0 , // Output power good rising edge.
    MP8862_OT_WARNING_EXIT  = 1 << 1 , // Die temperature early warning exit bit.
    MP8862_OTEMPP_EXIT      = 1 << 2 , // Over-temperature protection exit
    MP8862_UVP_FALLING      = 1 << 3 , // Output voltage is in under-voltage protection.
    MP8862_OC_RECOVER       = 1 << 4 , // Recovery from CC current-limit mode
    MP8862_OC_ENTER         = 1 << 5 , // Entry of OC or CC current-limit mode.
    MP8862_OT_WARNING_ENTER = 1 << 6 , // Die temperature early warning entry bit
    MP8862_OTEMPP_ENTER     = 1 << 7 , // Over-temperature protection entry indication.
};

enum MP8862_REG_INT_MASK{
    MP8862_PG_MSK  = 1 << 0 , // Masks off the PG indication function on ALT. Setting  MSK to 1 disables this interrupt source.
    MP8862_UVP_MSK = 1 << 1 , // Masks off the output UVP interrupt. Setting  MSK to 1 disables this interrupt source.
    MP8862_OC_MSK  = 1 << 2 , // Masks off both OC/CC entry and recovery. Setting  MSK to 1 disables this interrupt source.
    MP8862_OTW_MSK = 1 << 3 , // Masks off the over-temperature warning. Setting  MSK to 1 disables this interrupt source.
    MP8862_OTP_MSK = 1 << 4 , // Masks off the OTP alert. Setting  MSK to 1 disables this interrupt source.
};

// MP8862 Revision 1 register map
enum MP8862_register {
    MP8862_REG_VOUT_L    = 0x00, // R[2:0] : VOUT[2:0] - default 0x04
    MP8862_REG_VOUT_H    = 0x01, // R[7:0] : VOUT[10:3] - default 0x3E ( 0x3E << 3 | 0x04 = 500 decimal = 5.00V )
    MP8862_REG_VOUT_GO   = 0x02, // R[1] : PG_DELAY_EN , R[0] : GO_BIT
    MP8862_REG_IOUT_LIM  = 0x03, // R[6:0] : output current limit threshold (0 .. 4 A (ROC 21.5k : 50 mA steps) - defaut 0x3C = 3.0 A
    MP8862_REG_CTL1      = 0x04, // R[7] : EN, R[6]: HICCUP_OCP_OVP, R[5] : DISCHG_EN, R[4] : MODE, R[3:2]: FREQ - default 0xF0
    MP8862_REG_CTL2      = 0x05, // R[7:6] : LINE_DROP_COMP, R[5:4] : SS - default 0x30
    // 0x07 .. 0x08 : Reserved
    MP8862_UNK_0x06      = 0x06, // 0x06 [7:1] is denoted "Reserved, ALL “0”" , [0] is denoted "Reserved", structurally an odd remnant. (variable)
    MP8862_UNK_0x07      = 0x07, // undocumented (variable)
    MP8862_UNK_0x08      = 0x08, // undocumented (variable)
    MP8862_REG_STATUS    = 0x09, // R[7] : PG, R[6] : OTP, R[5] : OTW, R[4] : CC_CV (instantaneous values)
    MP8862_REG_INTERRUPT = 0x0A, // R[7] : OTEMPP_ENTER, R[6] : OT_WARNING_ENTER, R[5] : OC_ENTER, R[4] : OC_RECOVER, R[3]: UVP_FALLING, R[2] : OTEMP_EXIT, R[1] : OT_WARNING_EXIT, R[0] : PG_RISING
                                 // Write 0xFF to MP8862_REG_INTERRUPT to reset the interrupt (ALT) pin state.
    MP8862_REG_INT_MASK  = 0x0B, // R[4] : OTPMSK, R[3] : OTWMSK, R[2] : OC_MSK, R[1] : UVP_MSK, R[0] : PG_MSK
    MP8862_REG_ID1       = 0x0C, // 0x00: standard MP8862-0000Q, 0x01: MP8862-0001 part number etc.
    MP8862_UNK_0x0D      = 0x0D, // undocumented (0x30)
    // 0x0E .. 0x26 : unused / undocumented
    MP8862_REG_MFR_ID    = 0x27, // Manufacturer ID (0x09)
    MP8862_REG_DEV_ID    = 0x28, // Device ID       (0x58)
    MP8862_REG_IC_REV    = 0x29, // IC revision     (0x01)
    // 0x40 .. 0x4F : ROM defaults? / unused
    MP8862_UNK_0x40      = 0x40, // undocumented (0x04)
    MP8862_UNK_0x41      = 0x41, // undocumented (0x3E)
    MP8862_UNK_0x42      = 0x42, // undocumented (0x00)
    MP8862_UNK_0x43      = 0x43, // undocumented (0x3C)
    MP8862_UNK_0x44      = 0x44, // undocumented (0xF0)
    MP8862_UNK_0x45      = 0x45, // undocumented (0x30)
    // 0x50 .. 0x5F : calibration coefficients? / unused
    MP8862_UNK_0x50      = 0x50, // undocumented (variable)
    MP8862_UNK_0x51      = 0x51, // undocumented (variable)
    MP8862_UNK_0x52      = 0x52, // undocumented (variable)
    // 0x60 .. 0xFF : unused / undocumented
};


/* Approximate number of isDeviceReady() attempts after hardware EN _/  before output rises above 2.5V (assume 150 µs).
 * If unsuccessful (NACK), hardware EN needs to be disabled again immediately.
 */
enum MP8862_retry_count {
    /* time-critical power-up not supported on 100 kHz I2C -
     * at least 3 Bytes (DEVICE ADDR, REG ADDR, REG VALUE) need to be transferred within 150 µs.
     * If needed, an attempt can be made to delay by 50 .. 80µs, then start CTL1 write. If NACK, hardware EN needs
     * to be disabled as a Microcontroller GPIO, I2C GPIO expanders are not acceptable in this case.
     */
    MP8862_RETRY_I2C_400kHz  =  4, // ~25µs per byte: 150µs - 3 * 25µs leaves times for addtional 3 failed attempts.
                                   // If unsuccessful, the remaining 2 bytes transferred go e.g. to a GPIO expander update.
    MP8862_RETRY_I2C_1000kHz = 13, // ~ 10µs per byte
    MP8862_RETRY_I2C_3400kHz = 48, // ~  3µs per byte
};


class MP8862 {
    I2C_HandleTypeDef* hI2C {nullptr};
    MP8862_address deviceAddress{};
public:
    bool initialized {false};
    uint16_t VOUT_soft_limit_mV {}; // Prevent setVoltageSetpoint_mV() from setting values exceeding this limit.
    uint16_t IOUT_soft_limit_mA {}; // Prevent setCurrentLimit_mA()    from setting values exceeding this limit.

    bool init(I2C_HandleTypeDef *_hI2C, MP8862_address addr);
    bool isReady( );
    bool write( MP8862_register reg, uint8_t *data, uint8_t len );
    bool read ( MP8862_register reg, uint8_t *data, uint8_t len );
    bool write( MP8862_register reg, uint8_t value );

    bool hardwarePowerUp(bool (*callback_set_enable_pin)(uint16_t ID, uint8_t state), uint16_t ID, MP8862_retry_count trials); // time-critical power-up sequence
    bool setEnable ( bool soft_EN ); // soft_EN when hardware_EN is on

    bool setCurrentLimit_mA  ( uint16_t  current_mA );
    bool readCurrentLimit_mA ( uint16_t& current_mA );
    bool setVoltageSetpoint_mV  ( uint16_t  voltage_mV );
    bool readVoltageSetpoint_mV ( uint16_t& voltage_mV );
    bool readPG();
};


#endif //HW_MP8862_H
