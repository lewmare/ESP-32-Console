#pragma once
// ================================================================
//  bitmap_anim.h  —  Sistem Animasi Bitmap Modular
//  Untuk ESP32 + U8g2 (OLED SH1106 128x64)
// ================================================================
//
//  KONSEP UTAMA
//  ─────────────
//  Satu "sprite sheet" = kumpulan frame yang disusun secara:
//    • HORIZONTAL : frame berjajar ke kanan (paling umum)
//    • VERTICAL   : frame berjajar ke bawah
//
//  Format byte tiap frame sama dengan format U8g2 drawXBM:
//    - 1 bit per pixel, LSB first
//    - byte pertama = kolom 0-7, baris 0
//
//  TIPE ANIMASI
//  ─────────────
//    ANIM_ONCE      : putar sekali, berhenti di frame terakhir
//    ANIM_LOOP      : ulangi terus dari awal
//    ANIM_PINGPONG  : maju → mundur → maju → ...
//    ANIM_REVEAL_H  : reveal kolom per kolom dari kiri (startup effect)
//    ANIM_REVEAL_V  : reveal baris per baris dari atas
//
//  PENGGUNAAN CEPAT
//  ─────────────────
//    #include "bitmap_anim.h"
//
//    // 1. Deklarasi animator
//    BitmapAnim myAnim;
//
//    // 2. Inisialisasi (sprite sheet, lebar 1 frame, tinggi, jumlah frame,
//    //                  fps, posX, posY, tipe, arah)
//    myAnim.init(frames_data, 16, 16, 4, 8, 0, 0, ANIM_LOOP, ANIM_DIR_H);
//
//    // 3. Di dalam loop():
//    myAnim.update();      // hitung frame
//    myAnim.draw(u8g2);    // gambar ke buffer
//
//    // Opsional:
//    myAnim.setFPS(12);
//    myAnim.pause();
//    myAnim.resume();
//    myAnim.gotoFrame(0);
//    bool done = myAnim.isFinished(); // untuk ANIM_ONCE
// ================================================================

#include <Arduino.h>
#include <U8g2lib.h>

// ── Tipe animasi ─────────────────────────────────────────────────
enum AnimType
{
    ANIM_ONCE,     // Putar 1x, stop di frame terakhir
    ANIM_LOOP,     // Loop tak terbatas
    ANIM_PINGPONG, // Maju-mundur
    ANIM_REVEAL_H, // Reveal kolom per kolom (kiri → kanan)
    ANIM_REVEAL_V  // Reveal baris per baris (atas → bawah)
};

// ── Arah susunan frame dalam sprite sheet ────────────────────────
enum AnimDir
{
    ANIM_DIR_H, // Frame berjajar horizontal (default)
    ANIM_DIR_V  // Frame berjajar vertikal
};

// ── Urutan bit dalam tiap byte ────────────────────────────────────
// ANIM_LSB : bit 0 = pixel kiri  → format XBM / U8g2 default
// ANIM_MSB : bit 7 = pixel kiri  → format bitmap Arduino/Processing
//            (format yang dipakai data frames[][288])
enum AnimBitOrder
{
    ANIM_LSB, // LSB first — default U8g2 XBM
    ANIM_MSB  // MSB first — format byte biasa (0x80 >> col%8)
};

// ── Status animator ──────────────────────────────────────────────
enum AnimState
{
    ANIM_STATE_PLAYING,
    ANIM_STATE_PAUSED,
    ANIM_STATE_FINISHED
};

// ================================================================
//  Kelas utama BitmapAnim
// ================================================================
class BitmapAnim
{
public:
    // ── Konstruktor default ──────────────────────────────────────
    BitmapAnim() { _reset(); }

    // ── init() — wajib dipanggil sebelum digunakan ───────────────
    // @param sheet    : pointer ke array PROGMEM / RAM berisi sprite sheet
    // @param fw       : lebar satu frame (pixel)
    // @param fh       : tinggi satu frame (pixel)
    // @param numFrames: jumlah frame total
    // @param fps      : frame per detik (1–60), 0 = tampilkan frame statis
    // @param x, y     : posisi gambar di layar
    // @param type     : tipe animasi (default ANIM_LOOP)
    // @param dir      : arah susunan frame (default ANIM_DIR_H)
    // @param progmem  : true jika data ada di PROGMEM (default true)
    // @param bitOrder : ANIM_LSB (XBM default) atau ANIM_MSB (format Arduino)
    void init(const uint8_t *sheet,
              int fw, int fh, int numFrames,
              int fps,
              int x, int y,
              AnimType type = ANIM_LOOP,
              AnimDir dir = ANIM_DIR_H,
              bool progmem = true,
              AnimBitOrder bitOrder = ANIM_LSB)
    {
        _sheet = sheet;
        _fw = fw;
        _fh = fh;
        _numFrames = max(1, numFrames);
        _x = x;
        _y = y;
        _type = type;
        _dir = dir;
        _progmem = progmem;
        _bitOrder = bitOrder;
        _state = ANIM_STATE_PLAYING;
        _curFrame = 0;
        _pingDir = 1;
        _revealPx = 0;
        _lastTick = millis();
        setFPS(fps);
    }

    // ── Ubah FPS setelah init ────────────────────────────────────
    void setFPS(int fps)
    {
        _fps = max(0, fps);
        _frameMs = (_fps > 0) ? (1000UL / _fps) : 0xFFFFFFFFUL;
    }

    // ── Kontrol ─────────────────────────────────────────────────
    void pause() { _state = ANIM_STATE_PAUSED; }
    void resume()
    {
        _state = ANIM_STATE_PLAYING;
        _lastTick = millis();
    }
    void stop() { _state = ANIM_STATE_FINISHED; }
    void restart()
    {
        _curFrame = 0;
        _pingDir = 1;
        _revealPx = 0;
        _state = ANIM_STATE_PLAYING;
        _lastTick = millis();
    }
    void gotoFrame(int f) { _curFrame = constrain(f, 0, _numFrames - 1); }
    void setPosition(int x, int y)
    {
        _x = x;
        _y = y;
    }
    void setType(AnimType t)
    {
        _type = t;
        restart();
    }

    // ── Query ────────────────────────────────────────────────────
    bool isFinished() const { return _state == ANIM_STATE_FINISHED; }
    bool isPlaying() const { return _state == ANIM_STATE_PLAYING; }
    int currentFrame() const { return _curFrame; }
    int totalFrames() const { return _numFrames; }
    AnimState state() const { return _state; }

    // ── update() — panggil sekali per loop() ────────────────────
    void update()
    {
        if (_state != ANIM_STATE_PLAYING)
            return;
        if (_fps == 0)
            return;

        unsigned long now = millis();
        if (now - _lastTick < _frameMs)
            return;
        _lastTick = now;

        switch (_type)
        {
        case ANIM_ONCE:
            _tickOnce();
            break;
        case ANIM_LOOP:
            _tickLoop();
            break;
        case ANIM_PINGPONG:
            _tickPingpong();
            break;
        case ANIM_REVEAL_H:
            _tickRevealH();
            break;
        case ANIM_REVEAL_V:
            _tickRevealV();
            break;
        }
    }

    // ── draw() — gambar frame saat ini ke display ────────────────
    void draw(U8G2 &u8g) const
    {
        if (_sheet == nullptr)
            return;

        if (_type == ANIM_REVEAL_H)
        {
            _drawRevealH(u8g);
            return;
        }
        if (_type == ANIM_REVEAL_V)
        {
            _drawRevealV(u8g);
            return;
        }
        _drawFrame(u8g, _curFrame);
    }

    // ── drawFrame() — gambar frame tertentu (tanpa animasi) ──────
    void drawFrame(U8G2 &u8g, int frameIndex) const
    {
        if (_sheet == nullptr)
            return;
        _drawFrame(u8g, constrain(frameIndex, 0, _numFrames - 1));
    }

    // ── private ─────────────────────────────────────────────────────
private:
    const uint8_t *_sheet = nullptr;
    int _fw = 0; // lebar 1 frame
    int _fh = 0; // tinggi 1 frame
    int _numFrames = 1;
    int _fps = 8;
    int _x = 0;
    int _y = 0;
    int _curFrame = 0;
    int _pingDir = 1;  // +1 maju, -1 mundur (pingpong)
    int _revealPx = 0; // progress reveal (kolom atau baris)
    unsigned long _frameMs = 125;
    unsigned long _lastTick = 0;
    AnimType _type = ANIM_LOOP;
    AnimDir _dir = ANIM_DIR_H;
    AnimState _state = ANIM_STATE_FINISHED;
    AnimBitOrder _bitOrder = ANIM_LSB;
    bool _progmem = true;

    // Baca 1 byte dari sheet (PROGMEM atau RAM)
    inline uint8_t _readByte(int idx) const
    {
        return _progmem ? pgm_read_byte(&_sheet[idx]) : _sheet[idx];
    }

    // Test satu pixel berdasarkan bit order
    inline bool _testPixel(uint8_t b, int col) const
    {
        if (_bitOrder == ANIM_MSB)
            return b & (0x80 >> (col % 8)); // MSB-first: bit 7 = pixel kiri
        else
            return b & (1 << (col % 8)); // LSB-first: bit 0 = pixel kiri (XBM)
    }

    // Hitung byte-offset awal untuk sebuah frame
    inline int _frameOffset(int f) const
    {
        int bytesPerFrame = (((_fw + 7) / 8)) * _fh; // byte per frame (lebar dibulatkan ke byte)
        if (_dir == ANIM_DIR_H)
        {
            // Frame berjajar horizontal: offset = f * lebar frame dalam byte
            // Tapi format XBM horizontal: byte pertama = baris 0, kolom 0-7
            // Untuk horizontal sheet, tiap frame dimulai setelah frame sebelumnya
            return f * bytesPerFrame;
        }
        else
        {
            // Frame berjajar vertikal: offset = f * (tinggi * bytes_per_baris)
            return f * bytesPerFrame;
        }
    }

    // Gambar satu frame penuh
    void _drawFrame(U8G2 &u8g, int f) const
    {
        int bytesW = (_fw + 7) / 8;
        int offset = _frameOffset(f);
        for (int row = 0; row < _fh; row++)
        {
            for (int col = 0; col < _fw; col++)
            {
                int byteIdx = offset + row * bytesW + (col / 8);
                if (_testPixel(_readByte(byteIdx), col))
                    u8g.drawPixel(_x + col, _y + row);
            }
        }
    }

    // Gambar partial untuk reveal horizontal (kolom 0 .. _revealPx-1)
    void _drawRevealH(U8G2 &u8g) const
    {
        int bytesW = (_fw + 7) / 8;
        for (int row = 0; row < _fh; row++)
        {
            for (int col = 0; col < _revealPx; col++)
            {
                int byteIdx = row * bytesW + (col / 8);
                if (_testPixel(_readByte(byteIdx), col))
                    u8g.drawPixel(_x + col, _y + row);
            }
        }
    }

    // Gambar partial untuk reveal vertikal (baris 0 .. _revealPx-1)
    void _drawRevealV(U8G2 &u8g) const
    {
        int bytesW = (_fw + 7) / 8;
        for (int row = 0; row < _revealPx; row++)
        {
            for (int col = 0; col < _fw; col++)
            {
                int byteIdx = row * bytesW + (col / 8);
                if (_testPixel(_readByte(byteIdx), col))
                    u8g.drawPixel(_x + col, _y + row);
            }
        }
    }

    // ── Tick functions ──────────────────────────────────────────
    void _tickOnce()
    {
        _curFrame++;
        if (_curFrame >= _numFrames)
        {
            _curFrame = _numFrames - 1;
            _state = ANIM_STATE_FINISHED;
        }
    }
    void _tickLoop()
    {
        _curFrame = (_curFrame + 1) % _numFrames;
    }
    void _tickPingpong()
    {
        _curFrame += _pingDir;
        if (_curFrame >= _numFrames - 1)
        {
            _curFrame = _numFrames - 1;
            _pingDir = -1;
        }
        if (_curFrame <= 0)
        {
            _curFrame = 0;
            _pingDir = 1;
        }
    }
    void _tickRevealH()
    {
        _revealPx++;
        if (_revealPx >= _fw)
        {
            _revealPx = _fw;
            _state = ANIM_STATE_FINISHED;
        }
    }
    void _tickRevealV()
    {
        _revealPx++;
        if (_revealPx >= _fh)
        {
            _revealPx = _fh;
            _state = ANIM_STATE_FINISHED;
        }
    }

    void _reset()
    {
        _sheet = nullptr;
        _fw = 0;
        _fh = 0;
        _numFrames = 1;
        _fps = 8;
        _x = 0;
        _y = 0;
        _curFrame = 0;
        _pingDir = 1;
        _revealPx = 0;
        _frameMs = 125;
        _lastTick = 0;
        _type = ANIM_LOOP;
        _dir = ANIM_DIR_H;
        _bitOrder = ANIM_LSB;
        _state = ANIM_STATE_FINISHED;
        _progmem = true;
    }
};

// ================================================================
//  AnimPlayer — memutar sequence BitmapAnim satu per satu
//  Berguna untuk cutscene / intro multi-langkah
// ================================================================
#define ANIM_PLAYER_MAX 8 // max animasi dalam satu sequence

class AnimPlayer
{
public:
    AnimPlayer() : _count(0), _current(0), _active(false) {}

    // Tambahkan animasi ke sequence (pointer ke BitmapAnim yang sudah di-init)
    // @return false jika sudah penuh
    bool add(BitmapAnim *anim)
    {
        if (_count >= ANIM_PLAYER_MAX)
            return false;
        _anims[_count++] = anim;
        return true;
    }

    // Mulai dari anim pertama
    void play()
    {
        if (_count == 0)
            return;
        _current = 0;
        _active = true;
        _anims[0]->restart();
    }

    // Panggil di loop()
    void update()
    {
        if (!_active || _count == 0)
            return;
        BitmapAnim *cur = _anims[_current];
        cur->update();
        if (cur->isFinished())
        {
            _current++;
            if (_current >= _count)
            {
                _active = false;
                return;
            }
            _anims[_current]->restart();
        }
    }

    void draw(U8G2 &u8g) const
    {
        if (!_active || _count == 0)
            return;
        _anims[_current]->draw(u8g);
    }

    bool isFinished() const { return !_active; }
    void reset()
    {
        _current = 0;
        _active = false;
    }
    int currentIndex() const { return _current; }

private:
    BitmapAnim *_anims[ANIM_PLAYER_MAX];
    int _count = 0;
    int _current = 0;
    bool _active = false;
};

// ================================================================
//  SpriteSheet helper — bantu hitung offset kolom/baris frame
//  untuk sheet yang frame-nya berjajar tidak merata
// ================================================================
class SpriteSheet
{
public:
    // @param sheet      : pointer data PROGMEM
    // @param sheetWidth : lebar total sheet dalam piksel
    // @param fw, fh     : lebar & tinggi satu frame
    // @param cols       : jumlah kolom frame dalam sheet
    SpriteSheet(const uint8_t *sheet, int sheetWidth, int fw, int fh, int cols, bool progmem = true)
        : _sheet(sheet), _sheetW(sheetWidth), _fw(fw), _fh(fh), _cols(cols), _progmem(progmem) {}

    // Gambar frame ke-f ke posisi (x, y)
    void drawFrame(U8G2 &u8g, int f, int x, int y) const
    {
        int col = f % _cols;
        int row = f / _cols;
        int ox = col * _fw; // piksel-X dalam sheet
        int oy = row * _fh; // piksel-Y dalam sheet
        int sheetBytesW = (_sheetW + 7) / 8;

        for (int py = 0; py < _fh; py++)
        {
            for (int px = 0; px < _fw; px++)
            {
                int sx = ox + px;
                int sy = oy + py;
                int byteI = sy * sheetBytesW + (sx / 8);
                uint8_t b = _progmem ? pgm_read_byte(&_sheet[byteI]) : _sheet[byteI];
                if (b & (1 << (sx % 8)))
                    u8g.drawPixel(x + px, y + py);
            }
        }
    }

private:
    const uint8_t *_sheet;
    int _sheetW, _fw, _fh, _cols;
    bool _progmem;
};