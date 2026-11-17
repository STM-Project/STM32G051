#ifndef RADIO_H
#define RADIO_H
#include "main.h"
#define RADIO_STATION_COUNT 24U
#define RADIO_STEP_DIVIDER 0x12BFU
/* Frequencies use units of 0.01 MHz; no printf float support is needed. */
void Radio_Apply(void);
void Radio_Key(uint8_t key);
const char *Radio_Name(void);
uint16_t Radio_Frequency(void);
uint16_t Radio_Divider(void);
uint8_t Radio_Errors(void);
#endif
