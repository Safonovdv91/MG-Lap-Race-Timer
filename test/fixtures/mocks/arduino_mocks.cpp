/**
 * Arduino/ESP32 Mocks - реализации для нативного тестирования
 */

#include "arduino_mocks.h"
#include <stdarg.h>

// ============================================================================
// Serial Mock Implementation
// ============================================================================

SerialMock Serial;

void SerialMock::begin(unsigned long baud) {
    (void)baud;
}

void SerialMock::printf(const char* fmt, ...) {
#ifdef DEBUG_TESTS
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#endif
}

void SerialMock::print(const char* str) {
#ifdef DEBUG_TESTS
    printf("%s", str);
#endif
}

void SerialMock::print(int val) {
#ifdef DEBUG_TESTS
    printf("%d", val);
#endif
}

void SerialMock::print(float val) {
#ifdef DEBUG_TESTS
    printf("%.3f", val);
#endif
}

void SerialMock::println(const char* str) {
#ifdef DEBUG_TESTS
    printf("%s\n", str);
#endif
}

void SerialMock::println(int val) {
#ifdef DEBUG_TESTS
    printf("%d\n", val);
#endif
}

void SerialMock::println(float val) {
#ifdef DEBUG_TESTS
    printf("%.3f\n", val);
#endif
}

size_t SerialMock::write(uint8_t data) {
#ifdef DEBUG_TESTS
    putchar(data);
#endif
    return 1;
}

// ============================================================================
// Time Functions
// ============================================================================

unsigned long millis() {
#ifdef __MACH__
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

unsigned long micros() {
#ifdef __MACH__
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long)(tv.tv_sec * 1000000 + tv.tv_usec);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
#endif
}

void delay(unsigned long ms) {
    (void)ms;
}

void delayMicroseconds(unsigned long us) {
    (void)us;
}

// ============================================================================
// ADC Functions
// ============================================================================

void analogSetPinAttenuation(int pin, int attenuation) {
    (void)pin;
    (void)attenuation;
}

void analogReadResolution(int bits) {
    (void)bits;
}

int analogRead(int pin) {
    (void)pin;
    return 2048;  // Среднее значение для 12-битного АЦП
}

// ============================================================================
// Preferences Implementation
// ============================================================================

bool Preferences::begin(const char* name, bool readOnly) {
    (void)name;
    (void)readOnly;
    return true;
}

void Preferences::end() {
}

size_t Preferences::putString(const char* key, const char* value) {
    (void)key;
    (void)value;
    return 0;
}

size_t Preferences::getString(const char* key, char* value, size_t maxLen) {
    (void)key;
    (void)value;
    (void)maxLen;
    return 0;
}

int Preferences::getInt(const char* key, int defaultValue) {
    (void)key;
    (void)defaultValue;
    return 0;
}

bool Preferences::putInt(const char* key, int value) {
    (void)key;
    (void)value;
    return true;
}

void Preferences::clear() {
}

// ============================================================================
// GPIO Functions
// ============================================================================

void pinMode(int pin, int mode) {
    (void)pin;
    (void)mode;
}

void digitalWrite(int pin, int val) {
    (void)pin;
    (void)val;
}

int digitalRead(int pin) {
    (void)pin;
    return LOW;
}
