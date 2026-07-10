#include "framer7b.h"

#include <stddef.h>
#include <errno.h>

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/// Вычислить размер буфера, который будет после BASE128 кодирования данных размером in_size.
#define FRAMER7B_ENCODE_SIZE(in_size) ((in_size * 8 + 6) / 7)

/// Вычислить размер буфера, который будет после декодирования BASE128 кодированных данных размером in_size.
#define FRAMER7B_DECODE_SIZE(in_size) ((in_size * 7) / 8)

enum {
    FRAMER7B_START = 0xD4,
    FRAMER7B_END = 0x81,
    FRAMER7B_MARK_MASK = 0x80,
};

// Декодирование BASE128 in-place (с начала к концу)
static uint16_t decode_inplace(uint8_t* const buf, uint16_t const in_size) {
    uint16_t const k = in_size / 8;
    uint16_t const r = in_size % 8;

    uint16_t in_pos = 0;
    uint16_t out_pos = 0;

    if (in_size < 2) {
        return 0;
    }

    // 1. Обрабатываем полные блоки по 8 байт (forwards)
    for (uint16_t i = 0; i < k; ++i) {
        // Маскируем 0x7F, чтобы игнорировать 8-й "маркерный" бит
        uint8_t o0 = buf[in_pos + 0] & 0x7F;
        uint8_t o1 = buf[in_pos + 1] & 0x7F;
        uint8_t o2 = buf[in_pos + 2] & 0x7F;
        uint8_t o3 = buf[in_pos + 3] & 0x7F;
        uint8_t o4 = buf[in_pos + 4] & 0x7F;
        uint8_t o5 = buf[in_pos + 5] & 0x7F;
        uint8_t o6 = buf[in_pos + 6] & 0x7F;
        uint8_t o7 = buf[in_pos + 7] & 0x7F;
        
        buf[out_pos + 0] = (o0 << 1) | (o1 >> 6);
        buf[out_pos + 1] = (o1 << 2) | (o2 >> 5);
        buf[out_pos + 2] = (o2 << 3) | (o3 >> 4);
        buf[out_pos + 3] = (o3 << 4) | (o4 >> 3);
        buf[out_pos + 4] = (o4 << 5) | (o5 >> 2);
        buf[out_pos + 5] = (o5 << 6) | (o6 >> 1);
        buf[out_pos + 6] = (o6 << 7) | o7;

        in_pos += 8;
        out_pos += 7;
    }

    // 2. Обрабатываем остаток (от 2 до 7 байт)
    if (r > 1) {
        uint8_t o[7] = {0};
        for (uint16_t i = 0; i < r; ++i) {
            o[i] = buf[in_pos + i] & 0x7F;
        }
        
        buf[out_pos + 0] = (o[0] << 1) | (o[1] >> 6);
        if (r > 2) buf[out_pos + 1] = (o[1] << 2) | (o[2] >> 5);
        if (r > 3) buf[out_pos + 2] = (o[2] << 3) | (o[3] >> 4);
        if (r > 4) buf[out_pos + 3] = (o[3] << 4) | (o[4] >> 3);
        if (r > 5) buf[out_pos + 4] = (o[4] << 5) | (o[5] >> 2);
        if (r > 6) buf[out_pos + 5] = (o[5] << 6) | (o[6] >> 1);
    }

    return FRAMER7B_DECODE_SIZE(in_size);
}

int framer7b_receiver__init(struct Framer7bReceiver* const self, uint8_t* const buf, uint16_t const buf_size) {
    if (NULL == self) {
        return -EINVAL;
    }

    if (NULL == buf) {
        return -EINVAL;
    }

    if (0 == buf_size) {
        return -EINVAL;
    }

    self->buf = buf;
    self->buf_size = buf_size;

    framer7b_receiver__reset(self);

    return 0;
}

void framer7b_receiver__reset(struct Framer7bReceiver* const self) {
    if (NULL != self) {
        self->buf_ptr = 0;
        self->is_start_received = false;
    }
}

int framer7b_receiver__push(struct Framer7bReceiver* const self, uint8_t const byte) {
    if (byte & FRAMER7B_MARK_MASK) {
        if (FRAMER7B_START == byte) {
            framer7b_receiver__reset(self);
            self->is_start_received = true;
        } else if (FRAMER7B_END == byte) {
            if (self->is_start_received) {
                uint16_t const decoded_size = decode_inplace(self->buf, self->buf_ptr);
                framer7b_receiver__reset(self);
                if (0 == decoded_size) {
                    return -EINVAL;
                }
                return decoded_size;
            }
        } else {
            framer7b_receiver__reset(self);
            return -EINVAL;
        }
    } else {
        if (self->is_start_received) {
            if (self->buf_ptr < self->buf_size) {
                self->buf[self->buf_ptr] = byte;
                self->buf_ptr += 1;
            } else {
                framer7b_receiver__reset(self);
                return -EOVERFLOW;
            }
        }
    }

    return 0;
}

uint8_t* framer7b_receiver__buf(struct Framer7bReceiver* const self) {
    return self ? self->buf : NULL;
}

int framer7b__encode_in_place(uint8_t* const data_buf, uint16_t const data_size, uint16_t const buf_size) {
    uint16_t const encoded_size = FRAMER7B_ENCODE_SIZE(data_size);
    uint16_t const total_size = encoded_size + 2; // +2 для START и END

    // Проверка: помещаются ли данные в буфер
    if (total_size > buf_size) {
        return -EOVERFLOW; // Ошибка: недостаточно места
    }

    // Особый случай: пустые данные
    if (data_size == 0) {
        data_buf[0] = FRAMER7B_START;
        data_buf[1] = FRAMER7B_END;
        return 2;
    }

    // 1. Записываем END в конец буфера
    data_buf[encoded_size + 1] = FRAMER7B_END;

    // 2. Кодируем данные с конца к началу
    // Закодированные данные займут позиции от 1 до encoded_size
    uint16_t const k = data_size / 7;
    uint16_t const r = data_size % 7;

    uint16_t in_pos = data_size;
    uint16_t out_pos = encoded_size + 1; // Начинаем с позиции после END

    // Обрабатываем остаток (от 1 до 6 байт) - СНАЧАЛА, так как кодируем с конца к началу
    if (r > 0) {
        in_pos -= r;
        uint16_t out_block_len = (r * 8 + 6) / 7;
        out_pos -= out_block_len;
        uint8_t b[6] = {0};
        for (uint16_t i = 0; i < r; ++i) {
            b[i] = data_buf[in_pos + i];
        }
        
        data_buf[out_pos + 0] = (b[0] >> 1) & 0x7F;
        if (r > 1) data_buf[out_pos + 1] = ((b[0] & 0x01) << 6) | ((b[1] >> 2) & 0x7F);
        if (r > 2) data_buf[out_pos + 2] = ((b[1] & 0x03) << 5) | ((b[2] >> 3) & 0x7F);
        if (r > 3) data_buf[out_pos + 3] = ((b[2] & 0x07) << 4) | ((b[3] >> 4) & 0x7F);
        if (r > 4) data_buf[out_pos + 4] = ((b[3] & 0x0F) << 3) | ((b[4] >> 5) & 0x7F);
        if (r > 5) data_buf[out_pos + 5] = ((b[4] & 0x1F) << 2) | ((b[5] >> 6) & 0x7F);
        
        // Записываем последний байт (хвостовые биты)
        if (r == 1) data_buf[out_pos + 1] = (b[0] & 0x01) << 6;
        else if (r == 2) data_buf[out_pos + 2] = (b[1] & 0x03) << 5;
        else if (r == 3) data_buf[out_pos + 3] = (b[2] & 0x07) << 4;
        else if (r == 4) data_buf[out_pos + 4] = (b[3] & 0x0F) << 3;
        else if (r == 5) data_buf[out_pos + 5] = (b[4] & 0x1F) << 2;
        else if (r == 6) data_buf[out_pos + 6] = (b[5] & 0x3F) << 1;
    }

    // Обрабатываем полные блоки по 7 байт (backwards) - ПОТОМ
    for (uint16_t i = 0; i < k; ++i) {
        uint8_t b[7] = {0};
        in_pos -= 7;
        out_pos -= 8;
        
        b[0] = data_buf[in_pos + 0];
        b[1] = data_buf[in_pos + 1];
        b[2] = data_buf[in_pos + 2];
        b[3] = data_buf[in_pos + 3];
        b[4] = data_buf[in_pos + 4];
        b[5] = data_buf[in_pos + 5];
        b[6] = data_buf[in_pos + 6];
        
        data_buf[out_pos + 0] = (b[0] >> 1) & 0x7F;
        data_buf[out_pos + 1] = ((b[0] & 0x01) << 6) | ((b[1] >> 2) & 0x7F);
        data_buf[out_pos + 2] = ((b[1] & 0x03) << 5) | ((b[2] >> 3) & 0x7F);
        data_buf[out_pos + 3] = ((b[2] & 0x07) << 4) | ((b[3] >> 4) & 0x7F);
        data_buf[out_pos + 4] = ((b[3] & 0x0F) << 3) | ((b[4] >> 5) & 0x7F);
        data_buf[out_pos + 5] = ((b[4] & 0x1F) << 2) | ((b[5] >> 6) & 0x7F);
        data_buf[out_pos + 6] = ((b[5] & 0x3F) << 1) | ((b[6] >> 7) & 0x7F);
        data_buf[out_pos + 7] = b[6] & 0x7F;
    }

    // 3. Записываем START в начало буфера
    data_buf[0] = FRAMER7B_START;
    
    return total_size;
}
