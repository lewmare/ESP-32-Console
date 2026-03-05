// ============================================================
// IR_Clone.h — Modular IR Clone Module
// Pure IR logic only: receive, store, send
// No buzzer, no display — semua di main.cpp
// Library: IRremoteESP8266 by crankyoldgit
// ============================================================

#pragma once

#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>

// ===== Storage limits =====
#define IR_MAX_SIGNALS 5
#define IR_MAX_RAW_LEN 200

// ============================================================
// STRUCTS & ENUMS
// ============================================================

struct IRSignalData
{
    decode_type_t protocol;
    uint64_t value;
    uint16_t bits;
    uint16_t rawData[IR_MAX_RAW_LEN];
    uint16_t rawLen;
    bool isRaw;
    bool valid;
};

enum IRCaptureResult
{
    IR_RESULT_NONE,      // Tidak ada sinyal baru
    IR_RESULT_OK,        // Sinyal berhasil direkam
    IR_RESULT_DUPLICATE, // Duplikat, diabaikan
    IR_RESULT_REPEAT,    // Repeat flag, diabaikan
};

enum IRSendResult
{
    IR_SEND_OK,    // Berhasil dikirim
    IR_SEND_EMPTY, // Tidak ada sinyal
};

// ============================================================
// IRManager CLASS
// ============================================================

class IRManager
{
public:
    // ── begin() ───────────────────────────────────────────────
    // Panggil di setup(). Pin dari Config.h diteruskan ke sini.
    void begin(uint8_t rxPin, uint8_t txPin, uint16_t bufSize = 1024)
    {
        _irrecv = new IRrecv(rxPin, bufSize, 15, true);
        _irsend = new IRsend(txPin);
        _irrecv->enableIRIn();
        _irsend->begin();
        Serial.printf("[IR] Ready | RX:%d TX:%d\n", rxPin, txPin);
    }

    // ── update() ──────────────────────────────────────────────
    // Panggil setiap frame di loop().
    // Return IRCaptureResult — main.cpp yang urus buzzer/display.
    IRCaptureResult update()
    {
        if (!_irrecv || !_irrecv->decode(&_results))
            return IR_RESULT_NONE;

        IRCaptureResult res = IR_RESULT_NONE;

        if (_results.repeat)
        {
            res = IR_RESULT_REPEAT;
        }
        else if ((millis() - _lastRecvMs) < DUPLICATE_WINDOW_MS)
        {
            res = IR_RESULT_DUPLICATE;
        }
        else
        {
            _lastRecvMs = millis();
            int slot = _allocSlot();
            _store(slot, _results);
            _selectedIdx = slot;
            res = IR_RESULT_OK;
            Serial.printf("[IR] Captured slot#%d — %s\n",
                          slot + 1, describeSelected().c_str());
        }

        _irrecv->resume();
        return res;
    }

    // ── send() ────────────────────────────────────────────────
    // Kirim sinyal slot terpilih.
    // Return IRSendResult — main.cpp yang urus buzzer/display.
    IRSendResult send()
    {
        if (!_irsend || _count == 0 || !_signals[_selectedIdx].valid)
            return IR_SEND_EMPTY;

        _irrecv->disableIRIn();

        const IRSignalData &s = _signals[_selectedIdx];
        if (s.isRaw)
            _irsend->sendRaw(s.rawData, s.rawLen, 38);
        else
            _irsend->send(s.protocol, s.value, s.bits);

        delay(100);
        _irrecv->enableIRIn();

        Serial.printf("[IR] Sent slot#%d — %s\n",
                      _selectedIdx + 1, describeSelected().c_str());
        return IR_SEND_OK;
    }

    // ── nextSlot() ────────────────────────────────────────────
    void nextSlot()
    {
        if (_count > 1)
        {
            _selectedIdx = (_selectedIdx + 1) % _count;
            Serial.printf("[IR] Slot → #%d\n", _selectedIdx + 1);
        }
    }

    void previousSlot()
    {
        if (_count > 1)
        {
            _selectedIdx = (_selectedIdx - 1 + _count) % _count;
            Serial.printf("[IR] Slot → #%d\n", _selectedIdx + 1);
        }
    }

    // ── Getters ───────────────────────────────────────────────
    bool hasSignal() const { return _count > 0; }
    int getCount() const { return _count; }
    int getSelectedIdx() const { return _selectedIdx; }

    // "SAMSUNG 32bit" atau "RAW 67pulses"
    String describeSelected() const
    {
        const IRSignalData &s = _signals[_selectedIdx];
        if (!s.valid)
            return "(empty)";
        if (s.isRaw)
        {
            char b[20];
            snprintf(b, sizeof(b), "RAW %d pulses", s.rawLen);
            return String(b);
        }
        char b[28];
        snprintf(b, sizeof(b), "%s %dbit",
                 typeToString(s.protocol, false).c_str(), s.bits);
        return String(b);
    }

    // "2/5"
    String slotLabel() const
    {
        char b[8];
        snprintf(b, sizeof(b), "%d/%d", _selectedIdx + 1, IR_MAX_SIGNALS);
        return String(b);
    }

private:
    IRrecv *_irrecv = nullptr;
    IRsend *_irsend = nullptr;
    decode_results _results;

    IRSignalData _signals[IR_MAX_SIGNALS];
    int _count = 0;
    int _selectedIdx = 0;
    unsigned long _lastRecvMs = 0;

    static const uint16_t DUPLICATE_WINDOW_MS = 300;

    // FIFO slot allocator
    int _allocSlot()
    {
        if (_count < IR_MAX_SIGNALS)
            return _count++;
        for (int i = 0; i < IR_MAX_SIGNALS - 1; i++)
            _signals[i] = _signals[i + 1];
        return IR_MAX_SIGNALS - 1;
    }

    void _store(int idx, const decode_results &r)
    {
        IRSignalData &s = _signals[idx];
        s.valid = true;
        if (r.decode_type != UNKNOWN)
        {
            s.protocol = r.decode_type;
            s.value = r.value;
            s.bits = r.bits;
            s.isRaw = false;
            s.rawLen = 0;
        }
        else
        {
            s.isRaw = true;
            s.protocol = UNKNOWN;
            s.value = 0;
            s.bits = 0;
            uint16_t len = min((uint16_t)r.rawlen, (uint16_t)IR_MAX_RAW_LEN);
            s.rawLen = len;
            for (uint16_t i = 0; i < len; i++)
                s.rawData[i] = r.rawbuf[i] * RAWTICK;
        }
    }
};