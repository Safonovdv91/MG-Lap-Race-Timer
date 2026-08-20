/**
 * Arduino/ESP32 Mocks для нативного тестирования
 * 
 * Этот файл предоставляет заглушки для Arduino/ESP32 API,
 * чтобы тесты могли запускаться на хосте (macOS/Linux/Windows)
 */

#ifndef ARDUINO_MOCKS_H
#define ARDUINO_MOCKS_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// ============================================================================
// Базовые типы Arduino
// ============================================================================

typedef uint8_t byte;
typedef bool boolean;

#ifndef PROGMEM
#define PROGMEM
#endif

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

// ============================================================================
// Serial
// ============================================================================

class SerialMock {
public:
    void begin(unsigned long baud);
    void printf(const char* fmt, ...);
    void print(const char* str);
    void print(int val);
    void print(float val);
    void println(const char* str);
    void println(int val);
    void println(float val);
    size_t write(uint8_t data);
};

extern SerialMock Serial;

// ============================================================================
// Время
// ============================================================================

unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void delayMicroseconds(unsigned long us);

// ============================================================================
// ADC
// ============================================================================

#define ADC_11db 0

void analogSetPinAttenuation(int pin, int attenuation);
void analogReadResolution(int bits);
int analogRead(int pin);

// ============================================================================
// FreeRTOS критические секции (мок)
// ============================================================================

typedef int portMUX_TYPE;

#define portMUX_INITIALIZER_UNLOCKED 0

#define portENTER_CRITICAL(mux) \
    do { (void)(mux); } while(0)

#define portEXIT_CRITICAL(mux) \
    do { (void)(mux); } while(0)

#define portENTER_CRITICAL_ISR(mux) \
    do { (void)(mux); } while(0)

#define portEXIT_CRITICAL_ISR(mux) \
    do { (void)(mux); } while(0)

// ============================================================================
// Preferences
// ============================================================================

class Preferences {
public:
    bool begin(const char* name, bool readOnly = false);
    void end();
    size_t putString(const char* key, const char* value);
    size_t getString(const char* key, char* value, size_t maxLen);
    int getInt(const char* key, int defaultValue = 0);
    bool putInt(const char* key, int value);
    void clear();
};

// ============================================================================
// Макросы для памяти
// ============================================================================

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

// ============================================================================
// GPIO
// ============================================================================

#define LOW 0
#define HIGH 1

#define INPUT 0x01
#define OUTPUT 0x03

void pinMode(int pin, int mode);
void digitalWrite(int pin, int val);
int digitalRead(int pin);

// ============================================================================
// Для измерений
// ============================================================================

#define HISTORY_SIZE 5

#endif // ARDUINO_MOCKS_H
