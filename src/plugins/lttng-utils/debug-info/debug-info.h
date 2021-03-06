/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2015-2019 EfficiOS Inc.
 * Copyright (c) 2015 Antoine Busque <abusque@efficios.com>
 * Copyright (c) 2019 Francis Deslauriers francis.deslauriers@efficios.com>
 *
 * Babeltrace - Debug information Plugin
 */

#ifndef BABELTRACE_PLUGIN_DEBUG_INFO_H
#define BABELTRACE_PLUGIN_DEBUG_INFO_H

#include <stdint.h>
#include <stdbool.h>

#include <babeltrace2/babeltrace.h>

#include "common/macros.h"

#define VPID_FIELD_NAME		"vpid"
#define IP_FIELD_NAME		"ip"

BT_HIDDEN
bt_component_class_initialize_method_status debug_info_comp_init(
		bt_self_component_filter *self_comp,
		bt_self_component_filter_configuration *config,
		const bt_value *params, void *init_method_data);

BT_HIDDEN
void debug_info_comp_finalize(bt_self_component_filter *self_comp);

BT_HIDDEN
bt_message_iterator_class_initialize_method_status debug_info_msg_iter_init(
		bt_self_message_iterator *self_msg_iter,
		bt_self_message_iterator_configuration *config,
		bt_self_component_port_output *self_port);

BT_HIDDEN
bt_message_iterator_class_next_method_status debug_info_msg_iter_next(
		bt_self_message_iterator *self_msg_iter,
		const bt_message_array_const msgs, uint64_t capacity,
		uint64_t *count);

BT_HIDDEN
bt_message_iterator_class_can_seek_beginning_method_status
debug_info_msg_iter_can_seek_beginning(
		bt_self_message_iterator *message_iterator,
		bt_bool *can_seek);

BT_HIDDEN
bt_message_iterator_class_seek_beginning_method_status debug_info_msg_iter_seek_beginning(
		bt_self_message_iterator *message_iterator);

BT_HIDDEN
void debug_info_msg_iter_finalize(bt_self_message_iterator *it);

#endif /* BABELTRACE_PLUGIN_DEBUG_INFO_H */
