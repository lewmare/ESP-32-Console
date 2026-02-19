#ifndef BUZZER_ENGINE_H
#define BUZZER_ENGINE_H

#include <Arduino.h>

// ==================== BUZZER ENGINE ====================
enum BuzzerState
{
    BUZZER_IDLE,
    BUZZER_PLAYING,
    BUZZER_WAITING
};

struct BuzzerEngine
{
    int pin;
    const int *melody;
    const int *duration;
    int length, currentNote, cachedVolume;
    int singleMelody[1], singleDuration[1];
    unsigned long lastTime;
    BuzzerState state;

    void init(int p);
    void play(const int *m, const int *d, int len);
    void update();
    void playOnceTone(int freq, int dur);
    bool isPlaying();
};

void BuzzerEngine::playOnceTone(int freq, int dur)
{
    singleMelody[0] = freq;
    singleDuration[0] = dur;
    play(singleMelody, singleDuration, 1);
}
void BuzzerEngine::init(int p)
{
    pin = p;
    state = BUZZER_IDLE;
}
void BuzzerEngine::play(const int *m, const int *d, int len)
{
    melody = m;
    duration = d;
    length = len;
    currentNote = 0;
    state = BUZZER_PLAYING;
    lastTime = millis();
    int volumePercent = VolumeLevelValues[currentSettings.volumeIndex];
    cachedVolume = map(volumePercent, 0, 100, 0, 255);
    if (cachedVolume == 0)
    {
        state = BUZZER_IDLE;
        return;
    }
    ledcWriteTone(Buzzer_Channel, melody[currentNote]);
    ledcWrite(Buzzer_Channel, cachedVolume);
}
void BuzzerEngine::update()
{
    if (state == BUZZER_IDLE)
        return;
    unsigned long now = millis();
    if (state == BUZZER_PLAYING)
    {
        if (now - lastTime >= (unsigned long)duration[currentNote])
        {
            ledcWriteTone(Buzzer_Channel, 0);
            lastTime = now;
            state = BUZZER_WAITING;
        }
    }
    else if (state == BUZZER_WAITING)
    {
        if (now - lastTime >= 20)
        {
            currentNote++;
            if (currentNote >= length)
            {
                ledcWriteTone(Buzzer_Channel, 0);
                state = BUZZER_IDLE;
            }
            else
            {
                if (melody[currentNote] != 0)
                {
                    ledcWriteTone(Buzzer_Channel, melody[currentNote]);
                    ledcWrite(Buzzer_Channel, cachedVolume);
                }
                lastTime = now;
                state = BUZZER_PLAYING;
            }
        }
    }
}
bool BuzzerEngine::isPlaying() { return state != BUZZER_IDLE; }

#include <Buzzer/BuzzerNote.h>

const int melody_Startup[] = {NOTE_E5, NOTE_G5, NOTE_C6, NOTE_E6};
const int duration_Startup[] = {120, 120, 180, 300};

#endif