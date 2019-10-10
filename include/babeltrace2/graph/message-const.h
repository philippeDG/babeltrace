#ifndef BABELTRACE2_GRAPH_MESSAGE_CONST_H
#define BABELTRACE2_GRAPH_MESSAGE_CONST_H

/*
 * Copyright (c) 2010-2019 EfficiOS Inc. and Linux Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __BT_IN_BABELTRACE_H
# error "Please include <babeltrace2/babeltrace.h> instead."
#endif

#include <babeltrace2/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Message types. Unhandled message types should be ignored.
 */
typedef enum bt_message_type {
	BT_MESSAGE_TYPE_EVENT				= 1 << 0,
	BT_MESSAGE_TYPE_MESSAGE_ITERATOR_INACTIVITY	= 1 << 1,
	BT_MESSAGE_TYPE_STREAM_BEGINNING		= 1 << 2,
	BT_MESSAGE_TYPE_STREAM_END			= 1 << 3,
	BT_MESSAGE_TYPE_PACKET_BEGINNING		= 1 << 4,
	BT_MESSAGE_TYPE_PACKET_END			= 1 << 5,
	BT_MESSAGE_TYPE_DISCARDED_EVENTS		= 1 << 6,
	BT_MESSAGE_TYPE_DISCARDED_PACKETS		= 1 << 7,
} bt_message_type;

/**
 * Get a message's type.
 *
 * @param message	Message instance
 * @returns		One of #bt_message_type
 */
extern bt_message_type bt_message_get_type(const bt_message *message);

extern void bt_message_get_ref(const bt_message *message);

extern void bt_message_put_ref(const bt_message *message);

#define BT_MESSAGE_PUT_REF_AND_RESET(_var)		\
	do {						\
		bt_message_put_ref(_var);		\
		(_var) = NULL;				\
	} while (0)

#define BT_MESSAGE_MOVE_REF(_var_dst, _var_src)	\
	do {						\
		bt_message_put_ref(_var_dst);	\
		(_var_dst) = (_var_src);		\
		(_var_src) = NULL;			\
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* BABELTRACE2_GRAPH_MESSAGE_CONST_H */
