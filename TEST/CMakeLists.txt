cmake_minimum_required(VERSION 3.10)
project(framer7b_test C)

set(CMAKE_C_STANDARD 17)

# Добавляем тест как обычное приложение
add_executable(framer7b_test test.c)

# Линкуем с основной библиотекой
target_link_libraries(framer7b_test framer7b-c)

target_compile_options(framer7b_test PRIVATE -Werror -Wno-attributes)
