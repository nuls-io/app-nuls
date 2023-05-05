#pragma once

#include <stdbool.h>  // bool

/**
 * Callback to reuse action with approve/reject in step FLOW.
 */
typedef void (*action_validate_cb)(bool);


/**
 * Callback to reuse action with approve/reject in step FLOW.
 */
typedef bool (*step_flow_cb)(bool);


/**
 * Display address on the device and ask confirmation to export.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_public_key(void);

/**
 * Display transaction information on the device and ask confirmation to sign.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_sign_tx(void);
