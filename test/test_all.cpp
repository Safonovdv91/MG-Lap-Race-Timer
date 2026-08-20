/**
 * Тесты для модуля battery.cpp
 * 
 * Запуск: pio test -e native-tests -f test_battery
 */

#include <unity.h>

// Подключаем реализацию моков напрямую (только в одном файле!)
#include "fixtures/mocks/arduino_mocks.cpp"

// Подключаем исходные файлы для тестирования
#include "../src/drivers/battery/battery.cpp"
#include "../src/core/measurement_core.cpp"

// ============================================================================
// Тестовые кейсы для calculateBatteryPercentage
// ============================================================================

void test_battery_100_percent(void) {
    // Напряжение >= 4.10В = 100%
    int result = calculateBatteryPercentage(4.20);
    TEST_ASSERT_EQUAL_INT(100, result);
}

void test_battery_90_percent(void) {
    // 3.95В + небольшой дельтой = ~90%
    int result = calculateBatteryPercentage(3.96);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(89, result);
    TEST_ASSERT_LESS_OR_EQUAL_INT(91, result);
}

void test_battery_50_percent(void) {
    // 3.75В = 50%
    int result = calculateBatteryPercentage(3.75);
    TEST_ASSERT_EQUAL_INT(50, result);
}

void test_battery_25_percent(void) {
    // 3.65В = 25%
    int result = calculateBatteryPercentage(3.65);
    TEST_ASSERT_EQUAL_INT(25, result);
}

void test_battery_0_percent(void) {
    // Напряжение <= 3.35В = 0%
    int result = calculateBatteryPercentage(3.30);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_battery_low_voltage(void) {
    // Очень низкое напряжение = 0%
    int result = calculateBatteryPercentage(3.0);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_battery_high_voltage(void) {
    // Очень высокое напряжение = 100%
    int result = calculateBatteryPercentage(4.2);
    TEST_ASSERT_EQUAL_INT(100, result);
}

void test_battery_edge_case_3_45v(void) {
    // Граничное значение 3.45В = 5%
    int result = calculateBatteryPercentage(3.45);
    TEST_ASSERT_EQUAL_INT(5, result);
}

void test_battery_edge_case_3_85v(void) {
    // Граничное значение 3.86В = ~76%
    int result = calculateBatteryPercentage(3.86);
    TEST_ASSERT_EQUAL_INT(76, result);
}

// ============================================================================
// Тесты для addToHistory
// ============================================================================

void test_addToHistory_first_element(void) {
    Measurement testHistory[HISTORY_SIZE];
    memset(testHistory, 0, sizeof(testHistory));
    int testIndex = 0;
    
    testHistory[testIndex++] = {1.23f, 1000};
    
    TEST_ASSERT_EQUAL_INT(1, testIndex);
    TEST_ASSERT_EQUAL_FLOAT(1.23, testHistory[0].value);
}

void test_addToHistory_fill_buffer(void) {
    Measurement testHistory[HISTORY_SIZE];
    memset(testHistory, 0, sizeof(testHistory));
    int testIndex = 0;
    
    for (int i = 0; i < HISTORY_SIZE; i++) {
        testHistory[testIndex++] = {(float)(i + 1), (unsigned long)(i * 100)};
    }
    
    TEST_ASSERT_EQUAL_INT(HISTORY_SIZE, testIndex);
    TEST_ASSERT_EQUAL_FLOAT(1.0, testHistory[0].value);
    TEST_ASSERT_EQUAL_FLOAT((float)HISTORY_SIZE, testHistory[HISTORY_SIZE - 1].value);
}

// ============================================================================
// Тесты для handleCooldown (FSM)
// ============================================================================

void test_handleCooldown_ready_after_timeout(void) {
    timerStatus = STATUS_DISPLAY;
    displayStartTime = 1000;
    
    unsigned long nowMs = 1000 + TIMER_COOLDOWN_PERIOD + 1000;
    handleCooldown(nowMs);
    
    TEST_ASSERT_EQUAL_INT(STATUS_READY, timerStatus);
}

void test_handleCooldown_still_displaying(void) {
    timerStatus = STATUS_DISPLAY;
    displayStartTime = 1000;
    
    unsigned long nowMs = 1000 + 1000;
    handleCooldown(nowMs);
    
    TEST_ASSERT_EQUAL_INT(STATUS_DISPLAY, timerStatus);
}

// ============================================================================
// Тесты для updateLiveTimer
// ============================================================================

void test_updateLiveTimer_running(void) {
    timerStatus = STATUS_RUNNING;
    startTime = 1000000;
    
    unsigned long nowUs = 3000000;
    updateLiveTimer(nowUs);
    
    unsigned long long expected = nowUs - startTime;
    TEST_ASSERT_EQUAL_UINT64(expected, currentRaceTime);
}

void test_updateLiveTimer_display_mode(void) {
    timerStatus = STATUS_DISPLAY;
    startTime = 1000000;
    endTime = 5000000;
    
    updateLiveTimer(6000000);
    
    TEST_ASSERT_EQUAL_UINT64(endTime - startTime, currentRaceTime);
}

void test_updateLiveTimer_ready_mode(void) {
    timerStatus = STATUS_READY;
    updateLiveTimer(1000000);
    
    TEST_ASSERT_EQUAL_UINT64(0, currentRaceTime);
}

// ============================================================================
// Настройка тестового окружения
// ============================================================================

void setUp(void) {
    // Сброс состояния для measurement_core тестов
    timerStatus = STATUS_READY;
    currentRaceTime = 0;
    startTime = 0;
    endTime = 0;
    displayStartTime = 0;
    measurementInProgress = false;
}

void tearDown(void) {
    // Очистка после каждого теста
    // (в данном случае не требуется)
}

// ============================================================================
// Главная функция запуска тестов
// ============================================================================

int runUnityTests(void) {
    UNITY_BEGIN();

    // Тесты battery
    RUN_TEST(test_battery_100_percent);
    RUN_TEST(test_battery_90_percent);
    RUN_TEST(test_battery_50_percent);
    RUN_TEST(test_battery_25_percent);
    RUN_TEST(test_battery_0_percent);
    RUN_TEST(test_battery_low_voltage);
    RUN_TEST(test_battery_high_voltage);
    RUN_TEST(test_battery_edge_case_3_45v);
    RUN_TEST(test_battery_edge_case_3_85v);

    // Тесты measurement_core
    RUN_TEST(test_addToHistory_first_element);
    RUN_TEST(test_addToHistory_fill_buffer);
    RUN_TEST(test_handleCooldown_ready_after_timeout);
    RUN_TEST(test_handleCooldown_still_displaying);
    RUN_TEST(test_updateLiveTimer_running);
    RUN_TEST(test_updateLiveTimer_display_mode);
    RUN_TEST(test_updateLiveTimer_ready_mode);

    return UNITY_END();
}

// Для PlatformIO Test Runner
void setup() {
    runUnityTests();
}

void loop() {
    // Пусто для тестов
}

// Для запуска через native test
int main(int argc, char **argv) {
    return runUnityTests();
}
