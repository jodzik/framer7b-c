#!/bin/bash

set -e  # Остановка при ошибке

# Получаем абсолютный путь к директории, где находится скрипт
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Переходим в директорию скрипта
cd "$SCRIPT_DIR"

echo "=== Building tests ==="
echo "Script directory: $SCRIPT_DIR"

# Создаем директорию для сборки (всегда в TEST)
mkdir -p build
cd build

# Конфигурация CMake с явным включением тестов
cmake -DBUILD_TESTS=ON ../..

# Сборка
make

echo ""
echo "=== Running tests ==="
echo ""

# Запуск теста
set +e
./TEST/framer7b_test
RET="$?"
set -e

if [[ 0 = "$RET" ]]; then
    echo ""
    echo "=== Tests successfully completed ==="
else
    echo ""
    echo "=== Tests failed ==="
fi