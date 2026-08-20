# Скрипт добавляет arduino_mocks.cpp в сборку тестов
Import("env")

# Добавляем исходный файл моков в сборку
env.Append(SRC_FILTER=["+<test/mocks/arduino_mocks.cpp>"])

