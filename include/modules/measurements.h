/**
 * measurements.h
 * 
 * Заголовочный файл для обработчиков побочных эффектов.
 * Разделение core логики и side effects
 */

#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include <Arduino.h>
#include "core/measurement_core.h"

// ============================================================================
// LED Indicators
// ============================================================================

/**
 * Управление статусным светодиодом датчика.
 * Показывает текущее состояние луча.
 */
void handleStatusLED();

// ============================================================================
// Side Effects Handlers
// ============================================================================

/**
 * Обработка WebSocket рассылки данных.
 * @param nowMs Текущее время в миллисекундах
 * @param lapFinished Флаг завершения замера
 */
void handleWebsocketBroadcast(unsigned long nowMs, bool lapFinished);

/**
 * Вывод отладочной информации в Serial.
 * @param nowMs Текущее время в миллисекундах
 * @param lapFinished Флаг завершения замера
 */
void handleSerialOutput(unsigned long nowMs, bool lapFinished);

// ============================================================================
// Main Processing Wrapper
// ============================================================================

/**
 * Основная функция обработки с side effects.
 * Вызывается из loop() приемника.
 * 
 * Обёртка над processMeasurements() + side effects
 */
void processMeasurementsWithSideEffects();

// ============================================================================
// Legacy API (для совместимости)
// ============================================================================

// processMeasurements() теперь определяется в core/measurement_core.h
// Для полного цикла с side effects использовать processMeasurementsWithSideEffects()

#endif // MEASUREMENTS_H
