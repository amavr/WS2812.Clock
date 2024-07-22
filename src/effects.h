#include <Arduino.h>
#include <FastLED.h>

uint16_t getNewPosition(uint16_t ledNums)
{
    int ofs = ledNums / 10;
    return (uint16_t)random(ofs, ledNums - ofs);
}

void showMiddle(uint16_t center, uint8_t val, CRGB *leds, uint16_t ledNums){
    uint16_t ledNum = map(val, 0, 0xff, 0, 30);
    uint16_t l = max(center - ledNum, 0);
    uint16_t r = min(center + ledNum, (int)ledNums);
    for(int i = l; i <= r; i++)
    {
        // leds[i] = CRGB(val / 2, val / 2, val);
        leds[i].maximizeBrightness();
    }
}

void fill(struct CRGB * targetArray, int numToFill, uint8_t hue, uint8_t val)
{
    for( int i = 0; i < numToFill; i++) {
        targetArray[i] = CHSV(hue, 240, val);
    }
}

void fill_rainbow( struct CRGB * targetArray, int numToFill,
                  uint8_t initialHue,
                  uint8_t deltaHue,
                  uint8_t val )
{
    for( int i = 0; i < numToFill; i++) {
        targetArray[i] = CHSV(initialHue + (i * deltaHue), 240, val);
    }
}