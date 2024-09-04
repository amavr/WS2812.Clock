#include <ESP8266WiFi.h>

const char *ssid = "amavr-d";
const char *password = "220666abba";

#include "main.h"
#include "effects.h"

// MSGEQ7
#include "MSGEQ7.h"
#define INP_PIN A0
#define RESET_PIN D3
#define STROBE_PIN D2
#define MSGEQ7_INTERVAL ReadsPerSecond(50)
#define MSGEQ7_SMOOTH 191 // Range: 0-255

CMSGEQ7<MSGEQ7_SMOOTH, RESET_PIN, STROBE_PIN, INP_PIN> MSGEQ7;

XSOUND bass;
XSOUND middle;
XSOUND high;

int vals[3] = {0, 0, 0};

#include <FastLED.h>
#include <pixeltypes.h>
FASTLED_USING_NAMESPACE

#define NUM_LEDS 60
#define LED_PIN D4
#define LED_TYPE WS2811
#define BRIGHTNESS 96
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

uint8_t HUE_C = 160;
uint8_t SAT_C = 200;
uint8_t HUE_H = 0;
uint8_t HUE_M = 96;
uint8_t VAL_MIN = 30;
uint8_t VAL_MAX = 150;

uint8_t VAL_MIN_H = 130;
uint8_t VAL_MAX_H = 200;

#define BTN_PIN D1

bool volatile isMusicMode = false;

void ICACHE_RAM_ATTR isrBtn()
{
    isMusicMode = !isMusicMode;
}

void showClock(uint8_t val_bg)
{
    // show clock label
    for (int i = 0; i < NUM_LEDS; i++)
    {
        leds[i] = CHSV(HUE_C, SAT_C, (i % 5 == 0) ? VAL_MAX : max(VAL_MIN, val_bg));
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.flush();

    // This will set the IC ready for reading
    MSGEQ7.begin();

    // tell FastLED about the LED strip configuration
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    // set master brightness control
    FastLED.setBrightness(BRIGHTNESS);

    uint8_t val = VAL_MIN;
    uint8_t dir = 1;
    uint8_t hue = CRGB::DarkGreen;

    fill(leds, NUM_LEDS, hue, val);
    FastLED.show();

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        if (val >= 240)
            dir = -1;
        if (val <= 30)
            dir = 1;
        val += dir * 4;
        fill(leds, NUM_LEDS, hue, val);
        FastLED.show();

        Serial.print(".");
        delay(20);
    }
    Serial.println("done");

    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

    pinMode(BTN_PIN, INPUT);
    attachInterrupt(D1, isrBtn, RISING);

    // led setup
    pinMode(LED_PIN, OUTPUT);
}

volatile uint8_t val_h = VAL_MAX;
bool dir_h = true;

void showTime(int hh, int mm, int ss)
{
    val_h = val_h + (dir_h ? 5 : -5);
    if (val_h > VAL_MAX_H)
    {
        dir_h = !dir_h;
        val_h = VAL_MAX_H;
    }
    if (val_h < VAL_MIN_H)
    {
        dir_h = !dir_h;
        val_h = VAL_MIN_H;
    }

    if (hh >= 12)
    {
        hh -= 12;
    }

    // int dh = mm / 12;
    // // деление между 2-мя пикселами по яркости
    // int ofs = mm % 12;
    // int val1 = VAL_MAX * (12 - ofs) / 12;
    // int val2 = VAL_MAX - val1;

    // // show current hour
    // if (val1 > 20)
    // {
    //     leds[hh * 5 + dh] = CHSV(HUE_H, SAT_C, val1);
    // }
    // if (val2 > 20)
    // {
    //     leds[hh * 5 + dh + 1] = CHSV(HUE_H, SAT_C, val2);
    // }
    // show current minute
    leds[mm] = CHSV(HUE_M, SAT_C, VAL_MAX);
    // show current hour
    leds[hh * 5] = CHSV(HUE_H, SAT_C, val_h);
    // show current second
    leds[ss] = CRGB(250, 250, 250);
}

static int prev_day = 0;
static int prev_sec = 0;

int h, m, s = 0;
uint8_t b_val = VAL_MIN;

void test()
{
    s += 1;
    if (s >= 60)
    {
        s = 0;
        m++;
    }
    if (m >= 60)
    {
        m = 0;
        h++;
    }
    if (h >= 24)
    {
        h = 0;
    }

    showClock(VAL_MIN);
    showTime(h, m, s);
    FastLED.show();
    delay(10);
}

void clockMode()
{
    time_t now;
    time(&now);

    struct tm *timeinfo = localtime(&now);
    int hh = timeinfo->tm_hour;
    int mm = timeinfo->tm_min;
    int ss = timeinfo->tm_sec;

    if (prev_sec == ss)
    {
        // return;
    }
    prev_sec = ss;

    if (prev_day != timeinfo->tm_mday)
    {
        configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
        prev_day = timeinfo->tm_mday;
    }

    // Analyze without delay every interval
    bool newReading = MSGEQ7.read(MSGEQ7_INTERVAL);
    // Led output
    if (newReading)
    {
        // Read bass frequency
        int bass_val = MSGEQ7.get(MSGEQ7_BASS);
        b_val = calcSoundBass(&bass, bass_val);
    }

    showClock(b_val);
    showTime(hh, mm, ss);

    FastLED.show();

    Serial.print(hh);
    Serial.print(":");
    Serial.print(mm);
    Serial.print(":");
    Serial.println(ss);

    delay(100);
}

uint16_t middlePos = 0;
uint8_t hue = 0;

void musivMode()
{
    // Analyze without delay every interval
    bool newReading = MSGEQ7.read(MSGEQ7_INTERVAL);

    // Led output
    if (newReading)
    {
        // Read bass frequency
        vals[0] = MSGEQ7.get(MSGEQ7_0) + MSGEQ7.get(MSGEQ7_1) + MSGEQ7.get(MSGEQ7_2);
        vals[1] = MSGEQ7.get(MSGEQ7_3);
        vals[2] = MSGEQ7.get(MSGEQ7_5) + MSGEQ7.get(MSGEQ7_6);

        uint8_t b_val = calcSoundBass(&bass, vals[0]);
        uint8_t m_val = calcSoundMedium(&middle, vals[1]);
        uint8_t h_val = calcSoundHigh(&high, vals[2]);

        fill_rainbow(leds, NUM_LEDS, hue++, 5, max(b_val, (uint8_t)40));
        // hue += 2;

        uint16_t l = 0;
        uint16_t r = 0;
        if (m_val == 0)
        {
            middlePos = getNewPosition(NUM_LEDS);
        }
        else
        {
            // showMiddle(middlePos, m_val, leds, NUM_LEDS);

            uint16_t ledNum = map(m_val, 0, 0xff, 0, 30);
            l = max(middlePos - ledNum, 0);
            r = min(middlePos + ledNum, (int)NUM_LEDS);
            for (int i = l; i <= r; i++)
            {
                leds[i].maximizeBrightness();
            }
        }

        if (h_val * 2 > NOISE_LEVEL * 3)
        {
            // h_val = h_val / 24;
            h_val = h_val >> 5;
            for (uint8_t i = 0; i < h_val; i++)
            {
                uint16_t rnd = random16(NUM_LEDS);
                // запрет высоких в зоне отображения средних
                if (rnd < l || rnd > r)
                {
                    leds[rnd] = 0xB6B6B6;
                    // leds[rnd].maximizeBrightness();
                }
            }
        }

        FastLED.show();
    }
}

void loop()
{
    if (isMusicMode)
    {
        musivMode();
    }
    else
    {
        clockMode();
    }
}