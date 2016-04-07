#include <stdint.h>
#include "keyboard.h"
#include "action.h"
#include "util.h"
#include "action_layer.h"

#ifdef DEBUG_ACTION
#include "debug.h"
#else
#include "nodebug.h"
#endif


/* 
 * Default Layer State
 */
uint32_t default_layer_state = 0;

static void default_layer_state_set(uint32_t state)
{
    debug("default_layer_state: ");
    default_layer_debug(); debug(" to ");
    default_layer_state = state;
    default_layer_debug(); debug("\n");
    clear_keyboard_but_mods(); // To avoid stuck keys
}

void default_layer_debug(void)
{
    dprintf("%08lX(%u)", default_layer_state, biton32(default_layer_state));
}

void default_layer_set(uint32_t state)
{
    default_layer_state_set(state);
}

#ifndef NO_ACTION_LAYER
void default_layer_or(uint32_t state)
{
    default_layer_state_set(default_layer_state | state);
}
void default_layer_and(uint32_t state)
{
    default_layer_state_set(default_layer_state & state);
}
void default_layer_xor(uint32_t state)
{
    default_layer_state_set(default_layer_state ^ state);
}
#endif


#ifndef NO_ACTION_LAYER
/* 
 * Keymap Layer State
 */
uint32_t layer_state = 0;

static void layer_state_set(uint32_t state)
{
    dprint("layer_state: ");
    layer_debug(); dprint(" to ");
    layer_state = state;
    layer_debug(); dprintln();
    clear_keyboard_but_mods(); // To avoid stuck keys
}

void layer_clear(void)
{
    layer_state_set(0);
}

void layer_move(uint8_t layer)
{
    layer_state_set(1UL<<layer);
}

void layer_on(uint8_t layer)
{
    layer_state_set(layer_state | (1UL<<layer));
}

void layer_off(uint8_t layer)
{
    layer_state_set(layer_state & ~(1UL<<layer));
}

void layer_invert(uint8_t layer)
{
    layer_state_set(layer_state ^ (1UL<<layer));
}

void layer_or(uint32_t state)
{
    layer_state_set(layer_state | state);
}
void layer_and(uint32_t state)
{
    layer_state_set(layer_state & state);
}
void layer_xor(uint32_t state)
{
    layer_state_set(layer_state ^ state);
}

void layer_debug(void)
{
    dprintf("%08lX(%u)", layer_state, biton32(layer_state));
}
#endif

#if !defined(NO_ACTION_LAYER) && defined(PREVENT_STUCK_MODIFIERS)
uint8_t source_layers_cache[MAX_LAYER_BITS][(MATRIX_ROWS * MATRIX_COLS + 7) / 8] = {0};

void update_source_layers_cache(keypos_t key, uint8_t layer)
{
    const uint8_t key_number = key.col + (key.row * MATRIX_COLS);
    const uint8_t storage_row = key_number / 8;
    const uint8_t storage_bit = key_number % 8;

    for (uint8_t bit_number = 0; bit_number < MAX_LAYER_BITS; bit_number++) {
        source_layers_cache[bit_number][storage_row] ^=
            (-((layer & (1U << bit_number)) != 0)
             ^ source_layers_cache[bit_number][storage_row])
            & (1U << storage_bit);
    }
}

uint8_t read_source_layers_cache(keypos_t key)
{
    const uint8_t key_number = key.col + (key.row * MATRIX_COLS);
    const uint8_t storage_row = key_number / 8;
    const uint8_t storage_bit = key_number % 8;
    uint8_t layer = 0;

    for (uint8_t bit_number = 0; bit_number < MAX_LAYER_BITS; bit_number++) {
        layer |=
            ((source_layers_cache[bit_number][storage_row]
              & (1U << storage_bit)) != 0)
            << bit_number;
    }

    return layer;
}

int8_t find_source_layer(keypos_t key)
{
    uint32_t master_layer_state = layer_state | default_layer_state;
    /* check top layer first */
    for (int8_t layer = 31; layer >= 0; layer--) {
        if (master_layer_state & (1UL << layer)) {
            action_t action = action_for_key(layer, key);
            if (action.code != ACTION_TRANSPARENT) {
                return layer;
            }
        }
    }
    /* fall back to layer 0 */
    return 0;
}
/*
 * Make sure the action triggered when the key is released is the same
 * one as the one triggered on press. It's important for the mod keys
 * when the layer is switched after the down event but before the up
 * event as they may get stuck otherwise.
 */
uint8_t get_source_layer(keypos_t key, bool pressed)
{
    if (pressed || disable_action_cache) {
        uint8_t layer = find_source_layer(key);
        if (!disable_action_cache) {
            update_source_layers_cache(key, layer);
        }
        return layer;
    } else {
        return read_source_layers_cache(key);
    }
}

#else
action_t layer_switch_get_action(keypos_t key)
{
#ifndef NO_ACTION_LAYER
    uint32_t master_layer_state = layer_state | default_layer_state;
    /* check top layer first */
    for (int8_t layer = 31; layer >= 0; layer--) {
        if (master_layer_state & (1UL << layer)) {
            action_t action = action_for_key(layer, key);
            if (action.code != ACTION_TRANSPARENT) {
                return action;
            }
        }
    }
    /* fall back to layer 0 */
    return action_for_key(0, key);
#else
    return action_for_key(biton32(default_layer_state), key);
#endif
}
#endif
