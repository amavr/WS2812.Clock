#include <Arduino.h>

#define NUM_LEDS 60
#define MAX_COEF 1.8
#define NOISE_LEVEL 40

struct XSOUND
{
    uint16_t avg_val = 0;
    uint16_t index = 0;
    uint16_t med[3] = {0, 0, 0};
    uint16_t med_val = 0;
    uint16_t avg_min = 0;
    uint16_t prev = 0;
    bool down = false;
};

uint8_t calcSoundBass(XSOUND *data, int val)
{
    if(val < NOISE_LEVEL)  return 0;
    // val = max(val - NOISE_LEVEL / 2, 0);

    // get median value
    data->med[data->index] = val;
    if (++data->index >= 3)
        data->index = 0;
    data->med_val = (max(data->med[0], data->med[1]) == max(data->med[1], data->med[2])) ? max(data->med[0], data->med[2]) : max(data->med[1], min(data->med[0], data->med[2]));

    // нет смысла в вычислении среднего
    // data->med_val = val;
    // calc average value
    data->avg_val = (7 * data->avg_val + val) >> 3;

    // спуск?
    if (data->prev > data->med_val)
    {
        data->down = true;
    }
    // подъем?
    else{
        // перед этим был спуск?
        if(data->down){
            // получить среднее
            data->avg_min = (7 * data->avg_min + data->prev) >> 3;
        }
        data->down = false;
    }
    data->prev = data->med_val;

    data->avg_min -= data->avg_min >> 4;

    // из сигнала вычитается среднее минимальное
    // return (uint8_t)constrain(map(data->med_val - data->avg_min, 0, (uint16_t)roundl(data->avg_val * 1.6), 0, 240), 0, 240);
    return (uint8_t)constrain(map(val - data->avg_min, 0, (uint16_t)roundl(data->avg_val * 1.6), 0, 240), 0, 240);
}

uint8_t calcSoundMedium(XSOUND *data, int val)
{
    if(val < NOISE_LEVEL)  return 0;
    val = max(val - NOISE_LEVEL / 2, 0);

    // нет смысла в вычислении среднего
    data->med_val = val;
    // calc average value
    data->avg_val = (7 * data->avg_val + val) >> 3;

    // спуск?
    if (data->prev > data->med_val)
    {
        data->down = true;
    }
    // подъем?
    else{
        // перед этим был спуск?
        if(data->down){
            // получить среднее
            data->avg_min = (7 * data->avg_min + data->prev) >> 3;
        }
        data->down = false;
    }
    data->prev = data->med_val;

    // data->avg_min -= data->avg_min >> 3;

    // из сигнала вычитается среднее минимальное
    return (uint8_t)constrain(map(data->med_val - data->avg_min, 0, (uint16_t)roundl(data->avg_val * 1.6), 0, 240), 0, 240);
}

uint8_t calcSoundHigh(XSOUND *data, int val)
{
    if(val < NOISE_LEVEL)  return 0;
    val = max(val - NOISE_LEVEL / 2, 0);

    // get median value
    data->med[data->index] = val;
    if (++data->index >= 3)
        data->index = 0;
    data->med_val = (max(data->med[0], data->med[1]) == max(data->med[1], data->med[2])) ? max(data->med[0], data->med[2]) : max(data->med[1], min(data->med[0], data->med[2]));

    // calc average value
    data->avg_val = (7 * data->avg_val + val) >> 3;

    // спуск?
    if (data->prev > data->med_val)
    {
        data->down = true;
    }
    // подъем?
    else{
        // перед этим был спуск?
        if(data->down){
            // получить среднее
            data->avg_min = (7 * data->avg_min + data->prev) >> 3;
        }
        data->down = false;
    }
    data->prev = data->med_val;
    data->avg_min -= data->avg_min >> 7;

    // из сигнала вычитается среднее минимальное
    // return (uint8_t)constrain(map(data->med_val - data->avg_min, 0, (uint16_t)roundl(data->avg_val * 1.6), 0, 240), 0, 240);
    return (uint8_t)constrain(map(val - data->avg_min, 0, (uint16_t)roundl(data->avg_val * 1.6), 0, 240), 0, 240);
}

// скользящее среднее
int smoothAvg(int avg, int len, int value)
{
    return avg + 2 * (value - avg) / (1 + len);
}

// басы должны реагировать на фронт и тыл волны, сила вспышки - высота волны
// медиана для гашения помех
int median(int newVal)
{
    static int buf[3];
    static byte count = 0;
    buf[count] = newVal;
    if (++count >= 3)
        count = 0;
    return (max(buf[0], buf[1]) == max(buf[1], buf[2])) ? max(buf[0], buf[2]) : max(buf[1], min(buf[0], buf[2]));
}
