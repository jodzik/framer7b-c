#include <framer7b.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>

void print_buf(char const* const name, uint8_t const* const buf, uint16_t const size) {
    printf("%s=[", name);
    for (uint16_t i = 0; i < size; i++) {
        printf(" %02X", buf[i]);
    }
    printf("]\n");
}

int _encode_decode(
    struct Framer7bReceiver* const framer,
    uint8_t const* const data,
    uint16_t const data_size,
    uint16_t const encode_buf_size)
{
    int check = 0;
    uint16_t encoded_size = 0;
    uint8_t data_buf[UINT16_MAX] = {0};

    printf("Test encoding and decoding data in loop with data_size=%u encode_buf_size=%u\n",
        data_size, encode_buf_size);

    memcpy(data_buf, data, data_size);
    check = framer7b__encode_in_place(data_buf, data_size, encode_buf_size);
    if (check < 0) {
        return check;
    } else if (0 == check) {
        fprintf(stderr, "%s: framer7b__encode_in_place(,%u,%u) return zero\n", __func__, data_size, encode_buf_size);
        return -1000;
    } else if (check > UINT16_MAX) {
        fprintf(stderr, "%s: framer7b__encode_in_place(,%u,%u) return value greater than UINT16_MAX\n",
            __func__, data_size, encode_buf_size);

        return -1001;
    }

    encoded_size = (uint16_t)check;

    for (uint16_t i = 0; i < encoded_size; i++) {
        int const decoded_size = framer7b_receiver__push(framer, data_buf[i]);
        if (decoded_size < 0) {
            return decoded_size;
        } else if (decoded_size > 0) {
            if (i == encoded_size - 1) {
                if (data_size != decoded_size) {
                    fprintf(stderr, "%s: initial and decoded data sizes mismatch: initial=%u decoded=%i\n",
                        __func__, data_size, decoded_size);
                    return -1002;
                }
                if (0 == memcmp(data, framer7b_receiver__buf(framer), decoded_size)) {
                    return 0;
                } else {
                    fprintf(stderr, "%s: initial and decoded data mismatch\n", __func__);
                    print_buf("initial", data, data_size);
                    print_buf("decoded", framer7b_receiver__buf(framer), data_size);
                    return -1003;
                }
            } else {
                fprintf(stderr, "%s: framer7b__push return non zero on non last encoded data byte: %u\n", __func__, i);
                return -1004;
            }
        }
    }

    fprintf(stderr, "%s: framer7b__push() does'nt return non zero value for all encoded buffer (%u)\n",
        __func__, encoded_size);

    return -1004;
}

int test_encode_decode(void) {
    uint8_t const DATA0[] = {0xFF};
    uint8_t const DATA1[] = {0x00, 0x01};
    uint8_t const DATA2[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t const DATA3[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t const DATA4[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    uint8_t const DATA5[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0xFF, 0xFF};

    int rc = 0;
    int check = 0;
    struct Framer7bReceiver framer = {0};
    uint8_t framer_buf[128] = {0};

    framer7b_receiver__init(&framer, framer_buf, sizeof(framer_buf));

    // Ожидаемо успешные проходы:

    check = _encode_decode(&framer, DATA0, sizeof(DATA0), FRAMER7B_FRAME_SIZE(sizeof(DATA0)));
    if (0 != check) {
        rc = check;
        fprintf(stderr, "encode/decode on DATA0 failed: %i\n", check);
    } else {
        printf("encode/decode on DATA0 OK\n");
    }

    check = _encode_decode(&framer, DATA1, sizeof(DATA1), FRAMER7B_FRAME_SIZE(sizeof(DATA1)));
    if (0 != check) {
        rc = check;
        fprintf(stderr, "encode/decode on DATA1 failed: %i\n", check);
    } else {
        printf("encode/decode on DATA1 OK\n");
    }

    check = _encode_decode(&framer, DATA2, sizeof(DATA2), FRAMER7B_FRAME_SIZE(sizeof(DATA2)));
    if (0 != check) {
        rc = check;
        fprintf(stderr, "encode/decode on DATA2 failed: %i\n", check);
    } else {
        printf("encode/decode on DATA2 OK\n");
    }

    check = _encode_decode(&framer, DATA3, sizeof(DATA3), FRAMER7B_FRAME_SIZE(sizeof(DATA3)));
    if (0 != check) {
        rc = check;
        fprintf(stderr, "encode/decode on DATA3 failed: %i\n", check);
    } else {
        printf("encode/decode on DATA3 OK\n");
    }

    check = _encode_decode(&framer, DATA4, sizeof(DATA4), FRAMER7B_FRAME_SIZE(sizeof(DATA4)));
    if (0 != check) {
        rc = check;
        fprintf(stderr, "encode/decode on DATA4 failed: %i\n", check);
    } else {
        printf("encode/decode on DATA4 OK\n");
    }

    check = _encode_decode(&framer, DATA5, sizeof(DATA5), FRAMER7B_FRAME_SIZE(sizeof(DATA5)));
    if (0 != check) {
        rc = check;
        fprintf(stderr, "encode/decode on DATA5 failed: %i\n", check);
    } else {
        printf("encode/decode on DATA5 OK\n");
    }

    check = _encode_decode(&framer, DATA5, sizeof(DATA5), FRAMER7B_FRAME_SIZE(sizeof(DATA5)) + 1);
    if (0 != check) {
        rc = check;
        fprintf(stderr, "encode/decode on DATA5 with buffer + 1 failed: %i\n", check);
    } else {
        printf("encode/decode on DATA5 with buffer + 1 OK\n");
    }

    // Пустой кадр не позволен
    check = _encode_decode(&framer, DATA0, 0, FRAMER7B_FRAME_SIZE(sizeof(DATA0)));
    if (-EINVAL != check) {
        rc = check;
        fprintf(stderr, "encode/decode on NULL data failed: %i\n", check);
    } else {
        printf("encode/decode on NULL data OK\n");
    }

    // Слишком маленький буфер для кодированных данных
    check = _encode_decode(&framer, DATA5, sizeof(DATA5), FRAMER7B_FRAME_SIZE(sizeof(DATA5)) - 1);
    if (-EOVERFLOW != check) {
        rc = -2000;
        fprintf(stderr, "encode/decode on DATA5 with small encode buffer does'nt return -EOVERFLOW: %i\n", check);
    } else {
        printf("encode/decode on DATA5 with small encode buffer OK\n");
    }

    // Слишком маленький буфер для приема данных
    {
        struct Framer7bReceiver framer_small = {0};
        uint8_t framer_buf_small[FRAMER7B_FRAME_SIZE(sizeof(DATA5)) - 1] = {0};

        framer7b_receiver__init(&framer_small, framer_buf_small, sizeof(framer_buf_small));

        check = _encode_decode(&framer_small, DATA5, sizeof(DATA5), FRAMER7B_FRAME_SIZE(sizeof(DATA5)));
        if (-EOVERFLOW != check) {
            rc = -2001;
            fprintf(stderr, "encode/decode on DATA5 with small decode buffer does'nt return -EOVERFLOW: %i\n", check);
        } else {
            printf("encode/decode on DATA5 with small decode buffer OK\n");
        }
    }

    return rc;
}

static int decode_in_loop(struct Framer7bReceiver* const framer, uint8_t const* const encoded, uint16_t encoded_size) {
    for (uint16_t i = 0; i < encoded_size; i++) {
        int const decoded_size = framer7b_receiver__push(framer, encoded[i]);
        if (decoded_size < 0) {
            return decoded_size;
        } else if (decoded_size > 0) {
            return decoded_size;
        }
    }

    return -1000;
}

int test_manual_decode(void) {
    uint8_t const DATA_LEAD_TRASH[] = {0x00, 0xD4, 0x00, 0x00, 0x81};
    uint8_t const DATA_WITHOUT_START[] = {0x00, 0x00, 0x00, 0x81};
    uint8_t const DATA_WITHOUT_END[] = {0xD4, 0x00, 0x00};
    uint8_t const DATA_SMALL[] = {0xD4, 0x00, 0x81};

    int rc = 0;
    int check = 0;
    struct Framer7bReceiver framer = {0};
    uint8_t framer_buf[128] = {0};

    framer7b_receiver__init(&framer, framer_buf, sizeof(framer_buf));

    check = decode_in_loop(&framer, DATA_LEAD_TRASH, sizeof(DATA_LEAD_TRASH));
    if (1 != check) {
        rc = -1010;
        fprintf(stderr, "decode DATA_LEAD_TRASH does'nt return 1: %i\n", check);
    } else {
        printf("manual decode on DATA_LEAD_TRASH OK\n");
    }

    check = decode_in_loop(&framer, DATA_WITHOUT_START, sizeof(DATA_WITHOUT_START));
    if (-1000 != check) {
        rc = -1011;
        fprintf(stderr, "decode DATA_WITHOUT_START does'nt return -1000: %i\n", check);
    } else {
        printf("manual decode on DATA_WITHOUT_START OK\n");
    }

    check = decode_in_loop(&framer, DATA_WITHOUT_END, sizeof(DATA_WITHOUT_END));
    if (-1000 != check) {
        rc = -1012;
        fprintf(stderr, "decode DATA_WITHOUT_END does'nt return -1000: %i\n", check);
    } else {
        printf("manual decode on DATA_WITHOUT_END OK\n");
    }

    check = decode_in_loop(&framer, DATA_SMALL, sizeof(DATA_SMALL));
    if (-EINVAL != check) {
        rc = -1013;
        fprintf(stderr, "decode DATA_SMALL does'nt return -EINVAL: %i\n", check);
    } else {
        printf("manual decode on DATA_SMALL OK\n");
    }

    return rc;
}

int main(int argc, char const *argv[]) {
    int rc = 0;
    int check = 0;

    check = test_encode_decode();
    rc = rc ? rc : check;

    check = test_manual_decode();
    rc = rc ? rc : check;

    return rc;
}
