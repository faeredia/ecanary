
#ifndef _MYRTCLIB_H_
#define _MYRTCLIB_H_

#include <Arduino.h>
#include <Time.h>

// Set your own pins with these defines !
// Arduino pin for the Serial Clock
// Arduino pin for the Data I/O
// Arduino pin for the Chip Enable
class DS1302{
public:
    DS1302(uint8_t SCLK=5, uint8_t IO=6, uint8_t CE=4);
    void setDS1302Time(int year=2018, int month=4, int dayofmonth=21, int dayofweek=6, int hours=9, int minutes=27, int seconds=30);
    void readDS1302Time();
    time_t getDS1302Time();
private:
    void DS1302_clock_burst_read( uint8_t *p);
    void DS1302_clock_burst_write( uint8_t *p);
    uint8_t DS1302_read(int address);
    void DS1302_write( int address, uint8_t data);
    void _DS1302_start( void);
    void _DS1302_stop(void);
    uint8_t _DS1302_toggleread( void);
    void _DS1302_togglewrite( uint8_t data, uint8_t release);
protected:
    uint8_t DS1302_SCLK_PIN;
    uint8_t DS1302_IO_PIN;
    uint8_t DS1302_CE_PIN;
};

#endif