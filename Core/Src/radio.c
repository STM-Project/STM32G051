#include "radio.h"
#include "pcf8575.h"

static const struct { const char *name; uint16_t frequency; } stations[] = {
  {"Radio Krakow", 10160},
  {"RMF Classic", 8780},
  {"Jedynka", 8940},
  {"Eska 2", 8880},
  {"Trojka", 9940},
  {"RMF FM", 9600},
  {"Radio Zet", 10410},
  {"RMF MAXXX", 9670},
  {"Radio Plus", 10610},
  {"Radio Maryja", 9060},
  {"Zlote przeboje", 9250},
  {"Radio Pogoda", 10240},
  {"Radio Katowice", 10300},
  {"TOK FM", 10290},
  {"Eska", 9770},
  {"Chillizet", 9370},
  {"Eska Rock", 10490},
  {"VOX FM", 10700},
  {"AntyRadio", 10100},
  {"Dwojka", 10200},
  {"Polskie Radio 24", 9720},
  {"Rock Radio", 10380},
  {"Radio Wnet", 9520},
  {"Radio Famka", 10050},
};
static uint8_t selected_station;
static int16_t offsets[RADIO_STATION_COUNT];
static uint8_t errors;

const char *Radio_Name(void) { return stations[selected_station].name; }
uint16_t Radio_Frequency(void)
{
  return (uint16_t)(stations[selected_station].frequency + offsets[selected_station]);
}
uint16_t Radio_Divider(void)
{
  /* G031 formula: (step+1) * (f - 10.7 - 75) / (2 * 12) - 1.
   * Convert 0.01 MHz units exactly with integer arithmetic. */
  return (uint16_t)(((uint32_t)RADIO_STEP_DIVIDER + 1U) *
                     (Radio_Frequency() - 8570U) / 2400U - 1U);
}
uint8_t Radio_Errors(void) { return errors; }
void Radio_Apply(void)
{
  errors = PCF8575_SetValues(RADIO_STEP_DIVIDER, Radio_Divider());
}
void Radio_Key(uint8_t key)
{
  switch (key) {
    case 1: if (selected_station + 1U < RADIO_STATION_COUNT) ++selected_station; break;
    case 2: if (selected_station > 0) --selected_station; break;
    case 4: if (offsets[selected_station] < 100) ++offsets[selected_station]; break;
    case 8: if (offsets[selected_station] > -100) --offsets[selected_station]; break;
    default: return;
  }
  Radio_Apply();
}
