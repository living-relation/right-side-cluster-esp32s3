/**
 * dash_bridge.c — TrackCluster Run_2 UART encode/decode + CRC-8
 *
 * Implements the fixed 32-byte bridge protocol declared in right-dash_data.h.
 * Right display decodes RIGHT frames and remains a dumb renderer of center-owned
 * state. Encode helpers are retained for ABI/test symmetry.
 */

#include "right-dash_data.h"
#include <string.h>

volatile dash_data_t dash = {0};

uint8_t dash_crc8(const uint8_t *data, size_t n) {
    uint8_t c = 0xFF;
    for (size_t i = 0; i < n; i++) {
        c ^= data[i];
        for (int b = 0; b < 8; b++) {
            c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x07) : (uint8_t)(c << 1);
        }
    }
    return c;
}

static inline void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v & 0xFFu); p[1] = (uint8_t)(v >> 8); }
static inline void put_i16(uint8_t *p, int16_t v)  { put_u16(p, (uint16_t)v); }
static inline void put_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static inline uint16_t get_u16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static inline int16_t  get_i16(const uint8_t *p) { return (int16_t)get_u16(p); }
static inline uint32_t get_u32(const uint8_t *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static inline uint16_t clamp_u16_from_float(float value, float scale) {
    if (value <= 0.0f) return 0u;
    float scaled = value * scale;
    if (scaled >= 65535.0f) return 65535u;
    return (uint16_t)scaled;
}

static inline int16_t clamp_i16_from_float(float value, float scale) {
    float scaled = value * scale;
    if (scaled > 32767.0f) return 32767;
    if (scaled < -32768.0f) return -32768;
    return (int16_t)scaled;
}

static inline uint8_t pack_left_state_byte(const dash_data_t *d) {
    return (uint8_t)((d->headlight_dim ? 0x01u : 0x00u) |
                     ((d->ecu_protect_level & 0x03u) << 1) |
                     (d->ecu_limp_mode ? 0x08u : 0x00u));
}

static inline void unpack_left_state_byte(uint8_t packed, dash_data_t *d) {
    d->headlight_dim = (uint8_t)(packed & 0x01u);
    d->ecu_protect_level = (uint8_t)((packed >> 1) & 0x03u);
    d->ecu_limp_mode = (uint8_t)((packed >> 3) & 0x01u);
}

void dash_encode_left(uint8_t out[UART_BRIDGE_FRAME_LEN], const dash_data_t *d, uint16_t seq) {
    memset(out, 0, UART_BRIDGE_FRAME_LEN);
    out[0] = UART_BRIDGE_SOF;
    out[1] = UART_BRIDGE_TYPE_LEFT;
    put_u16(&out[2], seq);

    uint8_t *p = &out[4];
    put_u16(&p[0],  clamp_u16_from_float(d->mph, 10.0f));
    put_i16(&p[2],  clamp_i16_from_float(d->oil_temp, 10.0f));
    put_u16(&p[4],  clamp_u16_from_float(d->oil_press, 10.0f));
    put_u16(&p[6],  clamp_u16_from_float(d->fuel_press, 10.0f));
    put_u16(&p[8],  clamp_u16_from_float(d->fuel_level, 10.0f));
    put_u16(&p[10], clamp_u16_from_float(d->rpm, 1.0f));
    p[12] = (uint8_t)d->gear;
    p[13] = d->odo_mode;

    float active_trip = (d->odo_mode == DASH_TRIP_B) ? d->trip_b : d->trip_a;
    put_u32(&p[14], (uint32_t)(d->odo * 10.0f));
    put_u32(&p[18], (uint32_t)(active_trip * 10.0f));
    put_u16(&p[22], d->flags);
    p[24] = d->display_mode;
    p[25] = pack_left_state_byte(d);

    out[30] = dash_crc8(&out[1], 29);
    out[31] = UART_BRIDGE_EOF;
}

void dash_encode_right(uint8_t out[UART_BRIDGE_FRAME_LEN], const dash_data_t *d, uint16_t seq) {
    memset(out, 0, UART_BRIDGE_FRAME_LEN);
    out[0] = UART_BRIDGE_SOF;
    out[1] = UART_BRIDGE_TYPE_RIGHT;
    put_u16(&out[2], seq);

    uint8_t *p = &out[4];
    put_u16(&p[0],  clamp_u16_from_float(d->lambda, 1000.0f));
    put_u16(&p[2],  clamp_u16_from_float(d->boost + 30.0f, 100.0f));
    put_i16(&p[4],  clamp_i16_from_float(d->coolant_temp, 10.0f));
    put_i16(&p[6],  clamp_i16_from_float(d->ign_adv, 10.0f));
    put_i16(&p[8],  clamp_i16_from_float(d->iat, 10.0f));
    put_u16(&p[10], clamp_u16_from_float(d->rpm, 1.0f));
    put_u16(&p[12], d->flags);
    p[14] = d->display_mode;
    p[15] = d->headlight_dim;
    p[16] = d->boost_menu_index;
    p[17] = d->tc_menu_index;
    put_u32(&p[18], d->ecu_protect_flags);
    p[22] = d->ecu_limp_mode;
    p[23] = d->ecu_protect_level;
    p[24] = d->ecu_last_protect_reason;
    p[25] = (uint8_t)(d->ecu_warning_flags & 0xFFu);

    out[30] = dash_crc8(&out[1], 29);
    out[31] = UART_BRIDGE_EOF;
}

bool dash_decode_left(const uint8_t in[UART_BRIDGE_FRAME_LEN], dash_data_t *d, uint16_t *seq_out) {
    if (in[0] != UART_BRIDGE_SOF || in[31] != UART_BRIDGE_EOF) return false;
    if (in[1] != UART_BRIDGE_TYPE_LEFT) return false;
    if (dash_crc8(&in[1], 29) != in[30]) return false;

    *seq_out = get_u16(&in[2]);

    const uint8_t *p = &in[4];
    d->mph        = get_u16(&p[0]) * 0.1f;
    d->oil_temp   = get_i16(&p[2]) * 0.1f;
    d->oil_press  = get_u16(&p[4]) * 0.1f;
    d->fuel_press = get_u16(&p[6]) * 0.1f;
    d->fuel_level = get_u16(&p[8]) * 0.1f;
    d->rpm        = (float)get_u16(&p[10]);
    d->gear       = (int8_t)p[12];
    d->odo_mode   = p[13];
    d->odo        = get_u32(&p[14]) * 0.1f;

    float trip_val = get_u32(&p[18]) * 0.1f;
    if (d->odo_mode == DASH_TRIP_B) d->trip_b = trip_val;
    else d->trip_a = trip_val;

    d->flags = get_u16(&p[22]);
    d->display_mode = p[24];
    unpack_left_state_byte(p[25], d);

    return true;
}

bool dash_decode_right(const uint8_t in[UART_BRIDGE_FRAME_LEN], dash_data_t *d, uint16_t *seq_out) {
    if (in[0] != UART_BRIDGE_SOF || in[31] != UART_BRIDGE_EOF) return false;
    if (in[1] != UART_BRIDGE_TYPE_RIGHT) return false;
    if (dash_crc8(&in[1], 29) != in[30]) return false;

    *seq_out = get_u16(&in[2]);

    const uint8_t *p = &in[4];
    d->lambda = get_u16(&p[0]) * 0.001f;
    d->boost = (get_u16(&p[2]) * 0.01f) - 30.0f;
    d->coolant_temp = get_i16(&p[4]) * 0.1f;
    d->ign_adv = get_i16(&p[6]) * 0.1f;
    d->iat = get_i16(&p[8]) * 0.1f;
    d->rpm = (float)get_u16(&p[10]);
    d->flags = get_u16(&p[12]);
    d->display_mode = p[14];
    d->headlight_dim = p[15];
    d->boost_menu_index = p[16];
    d->tc_menu_index = p[17];
    d->ecu_protect_flags = get_u32(&p[18]);
    d->ecu_limp_mode = p[22];
    d->ecu_protect_level = p[23];
    d->ecu_last_protect_reason = p[24];
    d->ecu_warning_flags = p[25];

    return true;
}
