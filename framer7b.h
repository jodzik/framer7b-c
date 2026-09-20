#ifndef FRAMER7B_H_
#define FRAMER7B_H_

#include <stdint.h>
#include <stdbool.h>

/// Вычислить размер фрейма, который будет создан для in_data_size данных.
#define FRAMER7B_FRAME_SIZE(in_data_size) ((((in_data_size) * 8 + 6) / 7) + 2)

typedef struct Framer7bReceiver {
    uint8_t* buf;
    uint16_t buf_size;
    uint16_t buf_ptr;
    bool is_start_received;
} Framer7bReceiver;

/// @brief Инициализация объекта - приемника framer7b кадров.
/// @param self - объект Framer7bReceiver.
/// @param buf - буфер в который будет писать побайтно фрейм функция framer7b__push.
/// @param buf_size - размер buf.
/// @return 0 - успех, иначе ошибка.
int framer7b_receiver__init(struct Framer7bReceiver* self, uint8_t* buf, uint16_t buf_size);

/// @brief Сбросить внутреннее состояние фреймера.
/// @param self - объект framer7b.
void framer7b_receiver__reset(struct Framer7bReceiver* self);

/// @brief Обработка очередного принятого байта.
/// @param self - объект Framer7bReceiver.
/// @param byte - принятый байт.
/// @return 0 - байт принят, ожидание следующего, < 0 - ошибка потока, > 0 - фрейм принят и успешно декодирован, 
///         возвращается длина полезных данных, буфер с полезными данными можно получить вызвав framer7b__buf().
int framer7b_receiver__push(struct Framer7bReceiver* self, uint8_t byte);

/// @brief Получить буфер указанный при инициализации.
/// @param self - объект Framer7bReceiver.
/// @return Буфер указанный при инициализации.
uint8_t* framer7b_receiver__buf(struct Framer7bReceiver* self);

/// @brief Кодировать данные в тот же буфер и вставить управляющие байты.
/// @param data_buf - буфер с данными, в который также будет записан фрейм.
/// @param data_size - размер полезных данных в буфере.
/// @param buf_size - размер буфера, должен быть больше чем data_size, см. FRAMER7B_ENCODE_SIZE().
/// @return Размер кодированных данных (> 0) иначе ошибка.
int framer7b__encode_in_place(uint8_t* data_buf, uint16_t data_size, uint16_t buf_size);

#endif // FRAMER7B_H_
