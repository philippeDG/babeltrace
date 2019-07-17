/*
 * Copyright 2017-2018 Philippe Proulx <pproulx@efficios.com>
 * Copyright 2013, 2014 Jérémie Galarneau <jeremie.galarneau@efficios.com>
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

#define BT_LOG_TAG "LIB/FIELD-CLASS"
#include "lib/logging.h"

#include "lib/assert-pre.h"
#include <babeltrace2/trace-ir/field-class.h>
#include <babeltrace2/trace-ir/field-class-const.h>
#include <babeltrace2/trace-ir/field-const.h>
#include <babeltrace2/trace-ir/field.h>
#include <babeltrace2/trace-ir/clock-class.h>
#include "lib/object.h"
#include "compat/compiler.h"
#include "compat/endian.h"
#include "common/assert.h"
#include "compat/glib.h"
#include <float.h>
#include <inttypes.h>
#include <stdlib.h>

#include "clock-class.h"
#include "field-class.h"
#include "field.h"
#include "field-path.h"
#include "utils.h"
#include "lib/func-status.h"
#include "lib/integer-range-set.h"

enum bt_field_class_type bt_field_class_get_type(
		const struct bt_field_class *fc)
{
	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	return fc->type;
}

static
void init_field_class(struct bt_field_class *fc, enum bt_field_class_type type,
		bt_object_release_func release_func)
{
	BT_ASSERT(fc);
	BT_ASSERT(release_func);
	bt_object_init_shared(&fc->base, release_func);
	fc->type = type;
}

static
void init_integer_field_class(struct bt_field_class_integer *fc,
		enum bt_field_class_type type,
		bt_object_release_func release_func)
{
	init_field_class((void *) fc, type, release_func);
	fc->range = 64;
	fc->base = BT_FIELD_CLASS_INTEGER_PREFERRED_DISPLAY_BASE_DECIMAL;
}

static
void destroy_integer_field_class(struct bt_object *obj)
{
	BT_ASSERT(obj);
	BT_LIB_LOGD("Destroying integer field class object: %!+F", obj);
	g_free(obj);
}

static inline
struct bt_field_class *create_integer_field_class(bt_trace_class *trace_class,
		enum bt_field_class_type type)
{
	struct bt_field_class_integer *int_fc = NULL;

	BT_ASSERT_PRE_NON_NULL(trace_class, "Trace class");
	BT_LOGD("Creating default integer field class object: type=%s",
		bt_common_field_class_type_string(type));
	int_fc = g_new0(struct bt_field_class_integer, 1);
	if (!int_fc) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to allocate one integer field class.");
		goto error;
	}

	init_integer_field_class(int_fc, type, destroy_integer_field_class);
	BT_LIB_LOGD("Created integer field class object: %!+F", int_fc);
	goto end;

error:
	BT_OBJECT_PUT_REF_AND_RESET(int_fc);

end:
	return (void *) int_fc;
}

struct bt_field_class *bt_field_class_unsigned_integer_create(
		bt_trace_class *trace_class)
{
	return create_integer_field_class(trace_class,
		BT_FIELD_CLASS_TYPE_UNSIGNED_INTEGER);
}

struct bt_field_class *bt_field_class_signed_integer_create(
		bt_trace_class *trace_class)
{
	return create_integer_field_class(trace_class,
		BT_FIELD_CLASS_TYPE_SIGNED_INTEGER);
}

uint64_t bt_field_class_integer_get_field_value_range(
		const struct bt_field_class *fc)
{
	const struct bt_field_class_integer *int_fc = (const void *) fc;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_IS_INT(fc, "Field class");
	return int_fc->range;
}

BT_ASSERT_PRE_FUNC
static
bool size_is_valid_for_enumeration_field_class(struct bt_field_class *fc,
		uint64_t size)
{
	// TODO
	return true;
}

void bt_field_class_integer_set_field_value_range(
		struct bt_field_class *fc, uint64_t size)
{
	struct bt_field_class_integer *int_fc = (void *) fc;

	BT_ASSERT_PRE_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_FC_IS_INT(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HOT(fc, "Field class");
	BT_ASSERT_PRE(size <= 64,
		"Unsupported size for integer field class's field value range "
		"(maximum is 64): size=%" PRIu64, size);
	BT_ASSERT_PRE(
		int_fc->common.type == BT_FIELD_CLASS_TYPE_UNSIGNED_INTEGER ||
		int_fc->common.type == BT_FIELD_CLASS_TYPE_SIGNED_INTEGER ||
		size_is_valid_for_enumeration_field_class(fc, size),
		"Invalid field value range for enumeration field class: "
		"at least one of the current mapping ranges contains values "
		"which are outside this range: %!+F, size=%" PRIu64, fc, size);
	int_fc->range = size;
	BT_LIB_LOGD("Set integer field class's field value range: %!+F", fc);
}

enum bt_field_class_integer_preferred_display_base
bt_field_class_integer_get_preferred_display_base(const struct bt_field_class *fc)
{
	const struct bt_field_class_integer *int_fc = (const void *) fc;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_IS_INT(fc, "Field class");
	return int_fc->base;
}

void bt_field_class_integer_set_preferred_display_base(
		struct bt_field_class *fc,
		enum bt_field_class_integer_preferred_display_base base)
{
	struct bt_field_class_integer *int_fc = (void *) fc;

	BT_ASSERT_PRE_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_FC_IS_INT(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HOT(fc, "Field class");
	int_fc->base = base;
	BT_LIB_LOGD("Set integer field class's preferred display base: %!+F", fc);
}

static
void finalize_enumeration_field_class_mapping(
		struct bt_field_class_enumeration_mapping *mapping)
{
	BT_ASSERT(mapping);

	if (mapping->label) {
		g_string_free(mapping->label, TRUE);
		mapping->label = NULL;
	}

	BT_OBJECT_PUT_REF_AND_RESET(mapping->range_set);
}

static
void destroy_enumeration_field_class(struct bt_object *obj)
{
	struct bt_field_class_enumeration *fc = (void *) obj;

	BT_ASSERT(fc);
	BT_LIB_LOGD("Destroying enumeration field class object: %!+F", fc);

	if (fc->mappings) {
		uint64_t i;

		for (i = 0; i < fc->mappings->len; i++) {
			finalize_enumeration_field_class_mapping(
				BT_FIELD_CLASS_ENUM_MAPPING_AT_INDEX(fc, i));
		}

		g_array_free(fc->mappings, TRUE);
		fc->mappings = NULL;
	}

	if (fc->label_buf) {
		g_ptr_array_free(fc->label_buf, TRUE);
		fc->label_buf = NULL;
	}

	g_free(fc);
}

static
struct bt_field_class *create_enumeration_field_class(
		bt_trace_class *trace_class, enum bt_field_class_type type)
{
	struct bt_field_class_enumeration *enum_fc = NULL;

	BT_ASSERT_PRE_NON_NULL(trace_class, "Trace class");
	BT_LOGD("Creating default enumeration field class object: type=%s",
		bt_common_field_class_type_string(type));
	enum_fc = g_new0(struct bt_field_class_enumeration, 1);
	if (!enum_fc) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to allocate one enumeration field class.");
		goto error;
	}

	init_integer_field_class((void *) enum_fc, type,
		destroy_enumeration_field_class);
	enum_fc->mappings = g_array_new(FALSE, TRUE,
		sizeof(struct bt_field_class_enumeration_mapping));
	if (!enum_fc->mappings) {
		BT_LIB_LOGE_APPEND_CAUSE("Failed to allocate a GArray.");
		goto error;
	}

	enum_fc->label_buf = g_ptr_array_new();
	if (!enum_fc->label_buf) {
		BT_LIB_LOGE_APPEND_CAUSE("Failed to allocate a GArray.");
		goto error;
	}

	BT_LIB_LOGD("Created enumeration field class object: %!+F", enum_fc);
	goto end;

error:
	BT_OBJECT_PUT_REF_AND_RESET(enum_fc);

end:
	return (void *) enum_fc;
}

struct bt_field_class *bt_field_class_unsigned_enumeration_create(
		bt_trace_class *trace_class)
{
	return create_enumeration_field_class(trace_class,
		BT_FIELD_CLASS_TYPE_UNSIGNED_ENUMERATION);
}

struct bt_field_class *bt_field_class_signed_enumeration_create(
		bt_trace_class *trace_class)
{
	return create_enumeration_field_class(trace_class,
		BT_FIELD_CLASS_TYPE_SIGNED_ENUMERATION);
}

uint64_t bt_field_class_enumeration_get_mapping_count(
		const struct bt_field_class *fc)
{
	const struct bt_field_class_enumeration *enum_fc = (const void *) fc;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_IS_ENUM(fc, "Field class");
	return (uint64_t) enum_fc->mappings->len;
}

const struct bt_field_class_unsigned_enumeration_mapping *
bt_field_class_unsigned_enumeration_borrow_mapping_by_index_const(
		const struct bt_field_class *fc, uint64_t index)
{
	const struct bt_field_class_enumeration *enum_fc = (const void *) fc;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_VALID_INDEX(index, enum_fc->mappings->len);
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_UNSIGNED_ENUMERATION,
		"Field class");
	return (const void *) BT_FIELD_CLASS_ENUM_MAPPING_AT_INDEX(fc, index);
}

const struct bt_field_class_signed_enumeration_mapping *
bt_field_class_signed_enumeration_borrow_mapping_by_index_const(
		const struct bt_field_class *fc, uint64_t index)
{
	const struct bt_field_class_enumeration *enum_fc = (const void *) fc;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_VALID_INDEX(index, enum_fc->mappings->len);
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_SIGNED_ENUMERATION,
		"Field class");
	return (const void *) BT_FIELD_CLASS_ENUM_MAPPING_AT_INDEX(fc, index);
}

static
const struct bt_field_class_enumeration_mapping *
borrow_enumeration_field_class_mapping_by_label(
		const struct bt_field_class_enumeration *fc, const char *label)
{
	struct bt_field_class_enumeration_mapping *mapping = NULL;
	uint64_t i;

	BT_ASSERT(fc);
	BT_ASSERT_PRE_DEV_NON_NULL(label, "Label");

	for (i = 0; i < fc->mappings->len; i++) {
		struct bt_field_class_enumeration_mapping *this_mapping =
			BT_FIELD_CLASS_ENUM_MAPPING_AT_INDEX(fc, i);

		if (strcmp(this_mapping->label->str, label) == 0) {
			mapping = this_mapping;
			goto end;
		}
	}

end:
	return mapping;
}

const struct bt_field_class_signed_enumeration_mapping *
bt_field_class_signed_enumeration_borrow_mapping_by_label_const(
		const struct bt_field_class *fc, const char *label)
{
	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_SIGNED_ENUMERATION,
		"Field class");
	return (const void *) borrow_enumeration_field_class_mapping_by_label(
		(const void *) fc, label);
}

const struct bt_field_class_unsigned_enumeration_mapping *
bt_field_class_unsigned_enumeration_borrow_mapping_by_label_const(
		const struct bt_field_class *fc, const char *label)
{
	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc,
		BT_FIELD_CLASS_TYPE_UNSIGNED_ENUMERATION, "Field class");
	return (const void *) borrow_enumeration_field_class_mapping_by_label(
		(const void *) fc, label);
}

const char *bt_field_class_enumeration_mapping_get_label(
		const struct bt_field_class_enumeration_mapping *mapping)
{
	BT_ASSERT_PRE_DEV_NON_NULL(mapping, "Enumeration field class mapping");
	return mapping->label->str;
}

const struct bt_integer_range_set_unsigned *
bt_field_class_unsigned_enumeration_mapping_borrow_ranges_const(
		const struct bt_field_class_unsigned_enumeration_mapping *u_mapping)
{
	const struct bt_field_class_enumeration_mapping *mapping =
		(const void *) u_mapping;

	BT_ASSERT_PRE_DEV_NON_NULL(mapping, "Enumeration field class mapping");
	return (const void *) mapping->range_set;
}

const struct bt_integer_range_set_signed *
bt_field_class_signed_enumeration_mapping_borrow_ranges_const(
		const struct bt_field_class_signed_enumeration_mapping *s_mapping)
{
	const struct bt_field_class_enumeration_mapping *mapping =
		(const void *) s_mapping;

	BT_ASSERT_PRE_DEV_NON_NULL(mapping, "Enumeration field class mapping");
	return (const void *) mapping->range_set;
}

enum bt_field_class_enumeration_get_mapping_labels_for_value_status
bt_field_class_unsigned_enumeration_get_mapping_labels_for_value(
		const struct bt_field_class *fc, uint64_t value,
		bt_field_class_enumeration_mapping_label_array *label_array,
		uint64_t *count)
{
	const struct bt_field_class_enumeration *enum_fc = (const void *) fc;
	uint64_t i;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_NON_NULL(label_array, "Label array (output)");
	BT_ASSERT_PRE_DEV_NON_NULL(count, "Count (output)");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_UNSIGNED_ENUMERATION,
		"Field class");
	g_ptr_array_set_size(enum_fc->label_buf, 0);

	for (i = 0; i < enum_fc->mappings->len; i++) {
		uint64_t j;
		const struct bt_field_class_enumeration_mapping *mapping =
			BT_FIELD_CLASS_ENUM_MAPPING_AT_INDEX(enum_fc, i);

		for (j = 0; j < mapping->range_set->ranges->len; j++) {
			const struct bt_integer_range *range = (const void *)
				BT_INTEGER_RANGE_SET_RANGE_AT_INDEX(
					mapping->range_set, j);

			if (value >= range->lower.u &&
					value <= range->upper.u) {
				g_ptr_array_add(enum_fc->label_buf,
					mapping->label->str);
				break;
			}
		}
	}

	*label_array = (void *) enum_fc->label_buf->pdata;
	*count = (uint64_t) enum_fc->label_buf->len;
	return BT_FUNC_STATUS_OK;
}

enum bt_field_class_enumeration_get_mapping_labels_for_value_status
bt_field_class_signed_enumeration_get_mapping_labels_for_value(
		const struct bt_field_class *fc, int64_t value,
		bt_field_class_enumeration_mapping_label_array *label_array,
		uint64_t *count)
{
	const struct bt_field_class_enumeration *enum_fc = (const void *) fc;
	uint64_t i;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_NON_NULL(label_array, "Label array (output)");
	BT_ASSERT_PRE_DEV_NON_NULL(count, "Count (output)");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_SIGNED_ENUMERATION,
		"Field class");
	g_ptr_array_set_size(enum_fc->label_buf, 0);

	for (i = 0; i < enum_fc->mappings->len; i++) {
		uint64_t j;
		const struct bt_field_class_enumeration_mapping *mapping =
			BT_FIELD_CLASS_ENUM_MAPPING_AT_INDEX(enum_fc, i);

		for (j = 0; j < mapping->range_set->ranges->len; j++) {
			const struct bt_integer_range *range = (const void *)
				BT_INTEGER_RANGE_SET_RANGE_AT_INDEX(
					mapping->range_set, j);

			if (value >= range->lower.i &&
					value <= range->upper.i) {
				g_ptr_array_add(enum_fc->label_buf,
					mapping->label->str);
				break;
			}
		}
	}

	*label_array = (void *) enum_fc->label_buf->pdata;
	*count = (uint64_t) enum_fc->label_buf->len;
	return BT_FUNC_STATUS_OK;
}

static
bool enumeration_field_class_has_mapping_with_label(
		const struct bt_field_class_enumeration *enum_fc,
		const char *label)
{
	uint64_t i;
	bool exists = false;

	BT_ASSERT(enum_fc);
	BT_ASSERT(label);

	for (i = 0; i < enum_fc->mappings->len; i++) {
		struct bt_field_class_enumeration_mapping *mapping_candidate =
			BT_FIELD_CLASS_ENUM_MAPPING_AT_INDEX(enum_fc, i);

		if (strcmp(mapping_candidate->label->str, label) == 0) {
			exists = true;
			goto end;
		}
	}

end:
	return exists;
}

static inline
enum bt_field_class_enumeration_add_mapping_status
add_mapping_to_enumeration_field_class(struct bt_field_class *fc,
		const char *label, const struct bt_integer_range_set *range_set)
{
	enum bt_field_class_enumeration_add_mapping_status status =
		BT_FUNC_STATUS_OK;
	struct bt_field_class_enumeration *enum_fc = (void *) fc;
	struct bt_field_class_enumeration_mapping mapping = { 0	};

	BT_ASSERT(fc);
	BT_ASSERT_PRE_NON_NULL(label, "Label");
	BT_ASSERT_PRE_NON_NULL(range_set, "Range set");
	BT_ASSERT_PRE(!enumeration_field_class_has_mapping_with_label(
		enum_fc, label),
		"Duplicate mapping name in enumeration field class: "
		"%![enum-fc-]+F, label=\"%s\"", fc, label);
	mapping.range_set = range_set;
	bt_object_get_ref(mapping.range_set);
	mapping.label = g_string_new(label);
	if (!mapping.label) {
		finalize_enumeration_field_class_mapping(&mapping);
		status = BT_FUNC_STATUS_MEMORY_ERROR;
		goto end;
	}

	g_array_append_val(enum_fc->mappings, mapping);
	BT_LIB_LOGD("Added mapping to enumeration field class: "
		"%![fc-]+F, label=\"%s\"", fc, label);

end:
	return status;
}

enum bt_field_class_enumeration_add_mapping_status
bt_field_class_unsigned_enumeration_add_mapping(
		struct bt_field_class *fc, const char *label,
		const struct bt_integer_range_set_unsigned *range_set)
{
	BT_ASSERT_PRE_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_UNSIGNED_ENUMERATION,
		"Field class");
	return add_mapping_to_enumeration_field_class(fc, label,
		(const void *) range_set);
}

enum bt_field_class_enumeration_add_mapping_status
bt_field_class_signed_enumeration_add_mapping(
		struct bt_field_class *fc, const char *label,
		const struct bt_integer_range_set_signed *range_set)
{
	BT_ASSERT_PRE_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_SIGNED_ENUMERATION,
		"Field class");
	return add_mapping_to_enumeration_field_class(fc, label,
		(const void *) range_set);
}

static
void destroy_real_field_class(struct bt_object *obj)
{
	BT_ASSERT(obj);
	BT_LIB_LOGD("Destroying real field class object: %!+F", obj);
	g_free(obj);
}

struct bt_field_class *bt_field_class_real_create(bt_trace_class *trace_class)
{
	struct bt_field_class_real *real_fc = NULL;

	BT_ASSERT_PRE_NON_NULL(trace_class, "Trace class");
	BT_LOGD_STR("Creating default real field class object.");
	real_fc = g_new0(struct bt_field_class_real, 1);
	if (!real_fc) {
		BT_LIB_LOGE_APPEND_CAUSE("Failed to allocate one real field class.");
		goto error;
	}

	init_field_class((void *) real_fc, BT_FIELD_CLASS_TYPE_REAL,
		destroy_real_field_class);
	BT_LIB_LOGD("Created real field class object: %!+F", real_fc);
	goto end;

error:
	BT_OBJECT_PUT_REF_AND_RESET(real_fc);

end:
	return (void *) real_fc;
}

bt_bool bt_field_class_real_is_single_precision(const struct bt_field_class *fc)
{
	const struct bt_field_class_real *real_fc = (const void *) fc;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_REAL, "Field class");
	return real_fc->is_single_precision;
}

void bt_field_class_real_set_is_single_precision(struct bt_field_class *fc,
		bt_bool is_single_precision)
{
	struct bt_field_class_real *real_fc = (void *) fc;

	BT_ASSERT_PRE_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_REAL, "Field class");
	BT_ASSERT_PRE_DEV_FC_HOT(fc, "Field class");
	real_fc->is_single_precision = (bool) is_single_precision;
	BT_LIB_LOGD("Set real field class's \"is single precision\" property: "
		"%!+F", fc);
}

static
int init_named_field_classes_container(
		struct bt_field_class_named_field_class_container *fc,
		enum bt_field_class_type type,
		bt_object_release_func fc_release_func,
		GDestroyNotify named_fc_destroy_func)
{
	int ret = 0;

	init_field_class((void *) fc, type, fc_release_func);
	fc->named_fcs = g_ptr_array_new_with_free_func(named_fc_destroy_func);
	if (!fc->named_fcs) {
		BT_LIB_LOGE_APPEND_CAUSE("Failed to allocate a GPtrArray.");
		ret = -1;
		goto end;
	}

	fc->name_to_index = g_hash_table_new(g_str_hash, g_str_equal);
	if (!fc->name_to_index) {
		BT_LIB_LOGE_APPEND_CAUSE("Failed to allocate a GHashTable.");
		ret = -1;
		goto end;
	}

end:
	return ret;
}

static
void finalize_named_field_class(struct bt_named_field_class *named_fc)
{
	BT_ASSERT(named_fc);
	BT_LIB_LOGD("Finalizing named field class: "
		"addr=%p, name=\"%s\", %![fc-]+F",
		named_fc, named_fc->name ? named_fc->name->str : NULL,
		named_fc->fc);

	if (named_fc->name) {
		g_string_free(named_fc->name, TRUE);
		named_fc->name = NULL;
	}

	BT_LOGD_STR("Putting named field class's field class.");
	BT_OBJECT_PUT_REF_AND_RESET(named_fc->fc);
}

static
void destroy_named_field_class(gpointer ptr)
{
	if (ptr) {
		finalize_named_field_class(ptr);
		g_free(ptr);
	}
}

static
void destroy_variant_with_selector_option(gpointer ptr)
{
	struct bt_field_class_variant_with_selector_option *opt = ptr;

	if (ptr) {
		finalize_named_field_class(&opt->common);
		BT_OBJECT_PUT_REF_AND_RESET(opt->range_set);
		g_free(ptr);
	}
}

static
void finalize_named_field_classes_container(
		struct bt_field_class_named_field_class_container *fc)
{
	BT_ASSERT(fc);

	if (fc->named_fcs) {
		g_ptr_array_free(fc->named_fcs, TRUE);
		fc->named_fcs = NULL;

	}

	if (fc->name_to_index) {
		g_hash_table_destroy(fc->name_to_index);
		fc->name_to_index = NULL;
	}
}

static
void destroy_structure_field_class(struct bt_object *obj)
{
	BT_ASSERT(obj);
	BT_LIB_LOGD("Destroying structure field class object: %!+F", obj);
	finalize_named_field_classes_container((void *) obj);
	g_free(obj);
}

struct bt_field_class *bt_field_class_structure_create(
		bt_trace_class *trace_class)
{
	int ret;
	struct bt_field_class_structure *struct_fc = NULL;

	BT_ASSERT_PRE_NON_NULL(trace_class, "Trace class");
	BT_LOGD_STR("Creating default structure field class object.");
	struct_fc = g_new0(struct bt_field_class_structure, 1);
	if (!struct_fc) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to allocate one structure field class.");
		goto error;
	}

	ret = init_named_field_classes_container((void *) struct_fc,
		BT_FIELD_CLASS_TYPE_STRUCTURE, destroy_structure_field_class,
		destroy_named_field_class);
	if (ret) {
		/* init_named_field_classes_container() logs errors */
		goto error;
	}

	BT_LIB_LOGD("Created structure field class object: %!+F", struct_fc);
	goto end;

error:
	BT_OBJECT_PUT_REF_AND_RESET(struct_fc);

end:
	return (void *) struct_fc;
}

static
int init_named_field_class(struct bt_named_field_class *named_fc,
		const char *name, struct bt_field_class *fc)
{
	int status = BT_FUNC_STATUS_OK;

	BT_ASSERT(named_fc);
	BT_ASSERT(name);
	BT_ASSERT(fc);
	named_fc->name = g_string_new(name);
	if (!named_fc->name) {
		BT_LIB_LOGE_APPEND_CAUSE("Failed to allocate a GString.");
		status = BT_FUNC_STATUS_MEMORY_ERROR;
		goto end;
	}

	named_fc->fc = fc;
	bt_object_get_no_null_check(named_fc->fc);
	bt_named_field_class_freeze(named_fc);

end:
	return status;
}

static
struct bt_named_field_class *create_named_field_class(const char *name,
		struct bt_field_class *fc)
{
	struct bt_named_field_class *named_fc = g_new0(
		struct bt_named_field_class, 1);

	if (!named_fc) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to allocate a named field class.");
		goto error;
	}

	if (init_named_field_class(named_fc, name, fc)) {
		/* init_named_field_class() logs errors */
		goto error;
	}

	goto end;

error:
	destroy_named_field_class(named_fc);
	named_fc = NULL;

end:
	return named_fc;
}

static
struct bt_field_class_variant_with_selector_option *
create_variant_with_selector_option(
		const char *name, struct bt_field_class *fc,
		const struct bt_integer_range_set *range_set)
{
	struct bt_field_class_variant_with_selector_option *opt = g_new0(
		struct bt_field_class_variant_with_selector_option, 1);

	BT_ASSERT(range_set);

	if (!opt) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to allocate a named field class.");
		goto error;
	}

	if (init_named_field_class(&opt->common, name, fc)) {
		goto error;
	}

	opt->range_set = range_set;
	bt_object_get_no_null_check(opt->range_set);
	bt_integer_range_set_freeze(range_set);
	goto end;

error:
	destroy_variant_with_selector_option(opt);
	opt = NULL;

end:
	return opt;
}

static
int append_named_field_class_to_container_field_class(
		struct bt_field_class_named_field_class_container *container_fc,
		struct bt_named_field_class *named_fc)
{
	BT_ASSERT(container_fc);
	BT_ASSERT(named_fc);
	BT_ASSERT_PRE_DEV_FC_HOT(container_fc, "Field class");
	BT_ASSERT_PRE(!bt_g_hash_table_contains(container_fc->name_to_index,
		named_fc->name->str),
		"Duplicate member/option name in structure/variant field class: "
		"%![container-fc-]+F, name=\"%s\"", container_fc,
		named_fc->name->str);
	g_ptr_array_add(container_fc->named_fcs, named_fc);
	g_hash_table_insert(container_fc->name_to_index, named_fc->name->str,
		GUINT_TO_POINTER(container_fc->named_fcs->len - 1));
	return BT_FUNC_STATUS_OK;
}

enum bt_field_class_structure_append_member_status
bt_field_class_structure_append_member(
		struct bt_field_class *fc, const char *name,
		struct bt_field_class *member_fc)
{
	enum bt_field_class_structure_append_member_status status;
	struct bt_named_field_class *named_fc = NULL;

	BT_ASSERT_PRE_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_STRUCTURE,
		"Field class");
	named_fc = create_named_field_class(name, member_fc);
	if (!named_fc) {
		/* create_named_field_class() logs errors */
		status = BT_FUNC_STATUS_MEMORY_ERROR;
		goto end;
	}

	status = append_named_field_class_to_container_field_class((void *) fc,
		named_fc);
	if (status == BT_FUNC_STATUS_OK) {
		/* Moved to the container */
		named_fc = NULL;
	}

end:
	return status;
}

uint64_t bt_field_class_structure_get_member_count(
		const struct bt_field_class *fc)
{
	struct bt_field_class_structure *struct_fc = (void *) fc;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_STRUCTURE,
		"Field class");
	return (uint64_t) struct_fc->common.named_fcs->len;
}

static
struct bt_named_field_class *
borrow_named_field_class_from_container_field_class_at_index(
		struct bt_field_class_named_field_class_container *fc,
		uint64_t index)
{
	BT_ASSERT(fc);
	BT_ASSERT_PRE_DEV_VALID_INDEX(index, fc->named_fcs->len);
	return fc->named_fcs->pdata[index];
}

const struct bt_field_class_structure_member *
bt_field_class_structure_borrow_member_by_index_const(
		const struct bt_field_class *fc, uint64_t index)
{
	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_STRUCTURE,
		"Field class");
	return (const void *)
		borrow_named_field_class_from_container_field_class_at_index(
			(void *) fc, index);
}

struct bt_field_class_structure_member *
bt_field_class_structure_borrow_member_by_index(
		struct bt_field_class *fc, uint64_t index)
{
	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_STRUCTURE,
		"Field class");
	return (void *)
		borrow_named_field_class_from_container_field_class_at_index(
			(void *) fc, index);
}

static
struct bt_named_field_class *
borrow_named_field_class_from_container_field_class_by_name(
		struct bt_field_class_named_field_class_container *fc,
		const char *name)
{
	struct bt_named_field_class *named_fc = NULL;
	gpointer orig_key;
	gpointer value;

	BT_ASSERT(fc);
	BT_ASSERT_PRE_DEV_NON_NULL(name, "Name");
	if (!g_hash_table_lookup_extended(fc->name_to_index, name, &orig_key,
			&value)) {
		goto end;
	}

	named_fc = fc->named_fcs->pdata[GPOINTER_TO_UINT(value)];

end:
	return named_fc;
}

const struct bt_field_class_structure_member *
bt_field_class_structure_borrow_member_by_name_const(
		const struct bt_field_class *fc, const char *name)
{
	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_STRUCTURE,
		"Field class");
	return (const void *)
		borrow_named_field_class_from_container_field_class_by_name(
			(void *) fc, name);
}

struct bt_field_class_structure_member *
bt_field_class_structure_borrow_member_by_name(
		struct bt_field_class *fc, const char *name)
{
	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_STRUCTURE,
		"Field class");
	return (void *)
		borrow_named_field_class_from_container_field_class_by_name(
			(void *) fc, name);
}

const char *bt_field_class_structure_member_get_name(
		const struct bt_field_class_structure_member *member)
{
	const struct bt_named_field_class *named_fc = (const void *) member;

	BT_ASSERT_PRE_DEV_NON_NULL(member, "Structure field class member");
	return named_fc->name->str;
}

const struct bt_field_class *
bt_field_class_structure_member_borrow_field_class_const(
		const struct bt_field_class_structure_member *member)
{
	const struct bt_named_field_class *named_fc = (const void *) member;

	BT_ASSERT_PRE_DEV_NON_NULL(member, "Structure field class member");
	return named_fc->fc;
}

static
void finalize_variant_field_class(struct bt_field_class_variant *var_fc)
{
	BT_ASSERT(var_fc);
	BT_LIB_LOGD("Finalizing variant field class object: %!+F", var_fc);
	finalize_named_field_classes_container((void *) var_fc);
}

static
void destroy_variant_field_class(struct bt_object *obj)
{
	struct bt_field_class_variant *fc = (void *) obj;

	BT_ASSERT(fc);
	finalize_variant_field_class(fc);
	g_free(fc);
}

static
void destroy_variant_with_selector_field_class(struct bt_object *obj)
{
	struct bt_field_class_variant_with_selector *fc = (void *) obj;

	BT_ASSERT(fc);
	finalize_variant_field_class(&fc->common);
	BT_LOGD_STR("Putting selector field path.");
	BT_OBJECT_PUT_REF_AND_RESET(fc->selector_field_path);
	BT_LOGD_STR("Putting selector field class.");
	BT_OBJECT_PUT_REF_AND_RESET(fc->selector_fc);
	g_free(fc);
}

struct bt_field_class *bt_field_class_variant_create(
		bt_trace_class *trace_class, bt_field_class *selector_fc)
{
	int ret;
	struct bt_field_class_variant *var_fc = NULL;
	struct bt_field_class_variant_with_selector *var_with_sel_fc = NULL;
	enum bt_field_class_type fc_type;

	BT_ASSERT_PRE_NON_NULL(trace_class, "Trace class");

	if (selector_fc) {
		BT_ASSERT_PRE_FC_IS_INT(selector_fc, "Selector field class");
	}

	BT_LIB_LOGD("Creating default variant field class: %![sel-fc-]+F",
		selector_fc);

	if (selector_fc) {
		var_with_sel_fc = g_new0(
			struct bt_field_class_variant_with_selector, 1);
		if (!var_with_sel_fc) {
			BT_LIB_LOGE_APPEND_CAUSE(
				"Failed to allocate one variant field class with selector.");
			goto error;
		}

		if (selector_fc->type == BT_FIELD_CLASS_TYPE_UNSIGNED_INTEGER ||
				selector_fc->type == BT_FIELD_CLASS_TYPE_UNSIGNED_ENUMERATION) {
			fc_type = BT_FIELD_CLASS_TYPE_VARIANT_WITH_UNSIGNED_SELECTOR;
		} else {
			fc_type = BT_FIELD_CLASS_TYPE_VARIANT_WITH_SIGNED_SELECTOR;
		}

		ret = init_named_field_classes_container(
			(void *) var_with_sel_fc, fc_type,
			destroy_variant_with_selector_field_class,
			destroy_variant_with_selector_option);
		if (ret) {
			/* init_named_field_classes_container() logs errors */
			goto error;
		}

		var_with_sel_fc->selector_fc = selector_fc;
		bt_object_get_no_null_check(var_with_sel_fc->selector_fc);
		bt_field_class_freeze(selector_fc);
		var_fc = (void *) var_with_sel_fc;
	} else {
		var_fc = g_new0(struct bt_field_class_variant, 1);
		if (!var_fc) {
			BT_LIB_LOGE_APPEND_CAUSE(
				"Failed to allocate one variant field class without selector.");
			goto error;
		}

		ret = init_named_field_classes_container((void *) var_fc,
			BT_FIELD_CLASS_TYPE_VARIANT_WITHOUT_SELECTOR,
			destroy_variant_field_class, destroy_named_field_class);
		if (ret) {
			/* init_named_field_classes_container() logs errors */
			goto error;
		}
	}

	BT_ASSERT(var_fc);
	BT_LIB_LOGD("Created default variant field class with selector object: "
		"%![var-fc-]+F, %![sel-fc-]+F", var_fc, selector_fc);
	goto end;

error:
	BT_OBJECT_PUT_REF_AND_RESET(var_fc);

end:
	return (void *) var_fc;
}

enum bt_field_class_variant_without_selector_append_option_status
bt_field_class_variant_without_selector_append_option(struct bt_field_class *fc,
		const char *name, struct bt_field_class *option_fc)
{
	enum bt_field_class_variant_without_selector_append_option_status status;
	struct bt_named_field_class *named_fc = NULL;

	BT_ASSERT_PRE_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_NON_NULL(name, "Name");
	BT_ASSERT_PRE_NON_NULL(option_fc, "Option field class");
	BT_ASSERT_PRE_FC_HAS_ID(fc,
		BT_FIELD_CLASS_TYPE_VARIANT_WITHOUT_SELECTOR, "Field class");
	named_fc = create_named_field_class(name, option_fc);
	if (!named_fc) {
		/* create_named_field_class() logs errors */
		status = BT_FUNC_STATUS_MEMORY_ERROR;
		goto end;
	}

	status = append_named_field_class_to_container_field_class((void *) fc,
		named_fc);
	if (status == BT_FUNC_STATUS_OK) {
		/* Moved to the container */
		named_fc = NULL;
	}

end:
	if (named_fc) {
		destroy_named_field_class(named_fc);
	}

	return status;
}

static
int ranges_overlap(GPtrArray *var_fc_opts, const struct bt_integer_range_set *range_set,
		bool is_signed, bool *has_overlap)
{
	int status = BT_FUNC_STATUS_OK;
	struct bt_integer_range_set *full_range_set;
	uint64_t i;

	*has_overlap = false;

	/*
	 * Build a single range set with all the ranges and test for
	 * overlaps.
	 */
	if (is_signed) {
		full_range_set = (void *) bt_integer_range_set_signed_create();
	} else {
		full_range_set = (void *) bt_integer_range_set_unsigned_create();
	}

	if (!full_range_set) {
		BT_LOGE_STR("Failed to create a range set.");
		status = BT_FUNC_STATUS_MEMORY_ERROR;
		goto end;
	}

	/* Add existing option ranges */
	for (i = 0; i < var_fc_opts->len; i++) {
		struct bt_field_class_variant_with_selector_option *opt =
			var_fc_opts->pdata[i];
		uint64_t j;

		for (j = 0; j < opt->range_set->ranges->len; j++) {
			struct bt_integer_range *range = BT_INTEGER_RANGE_SET_RANGE_AT_INDEX(
				opt->range_set, j);

			if (is_signed) {
				status = bt_integer_range_set_signed_add_range(
					(void *) full_range_set, range->lower.i,
					range->upper.i);
			} else {
				status = bt_integer_range_set_unsigned_add_range(
					(void *) full_range_set, range->lower.u,
					range->upper.u);
			}

			if (status) {
				goto end;
			}
		}
	}

	/* Add new ranges */
	for (i = 0; i < range_set->ranges->len; i++) {
		struct bt_integer_range *range = BT_INTEGER_RANGE_SET_RANGE_AT_INDEX(
			range_set, i);

		if (is_signed) {
			status = bt_integer_range_set_signed_add_range(
				(void *) full_range_set, range->lower.i,
				range->upper.i);
		} else {
			status = bt_integer_range_set_unsigned_add_range(
				(void *) full_range_set, range->lower.u,
				range->upper.u);
		}

		if (status) {
			goto end;
		}
	}

	/* Check overlaps */
	if (is_signed) {
		*has_overlap = bt_integer_range_set_signed_has_overlaps(full_range_set);
	} else {
		*has_overlap = bt_integer_range_set_unsigned_has_overlaps(
			full_range_set);
	}

end:
	bt_object_put_ref(full_range_set);
	return status;
}

static
int append_option_to_variant_with_selector_field_class(
		struct bt_field_class *fc, const char *name,
		struct bt_field_class *option_fc,
		const struct bt_integer_range_set *range_set,
		enum bt_field_class_type expected_type)
{
	int status;
	struct bt_field_class_variant_with_selector *var_fc = (void *) fc;
	struct bt_field_class_variant_with_selector_option *opt = NULL;
	bool has_overlap;

	BT_ASSERT_PRE_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_NON_NULL(name, "Name");
	BT_ASSERT_PRE_NON_NULL(option_fc, "Option field class");
	BT_ASSERT_PRE_NON_NULL(range_set, "Range set");
	BT_ASSERT_PRE_FC_HAS_ID(fc, expected_type, "Field class");
	BT_ASSERT_PRE(range_set->ranges->len > 0,
		"Range set is empty: addr=%p", range_set);
	status = ranges_overlap(var_fc->common.common.named_fcs, range_set,
		expected_type == BT_FIELD_CLASS_TYPE_VARIANT_WITH_SIGNED_SELECTOR,
		&has_overlap);
	if (status) {
		/* ranges_overlap() logs errors */
		goto end;
	}

	BT_ASSERT_PRE(!has_overlap,
		"Range set's ranges and existing ranges have an overlap: "
		"addr=%p", range_set);
	opt = create_variant_with_selector_option(name, option_fc, range_set);
	if (!opt) {
		/* create_variant_with_selector_option() logs errors */
		status = BT_FUNC_STATUS_MEMORY_ERROR;
		goto end;
	}

	status = append_named_field_class_to_container_field_class((void *) fc,
		&opt->common);
	if (status == BT_FUNC_STATUS_OK) {
		/* Moved to the container */
		opt = NULL;
	}

end:
	if (opt) {
		destroy_variant_with_selector_option(opt);
	}

	return status;
}

enum bt_field_class_variant_with_selector_append_option_status
bt_field_class_variant_with_unsigned_selector_append_option(
		struct bt_field_class *fc, const char *name,
		struct bt_field_class *option_fc,
		const struct bt_integer_range_set_unsigned *range_set)
{
	return append_option_to_variant_with_selector_field_class(fc,
		name, option_fc, (const void *) range_set,
		BT_FIELD_CLASS_TYPE_VARIANT_WITH_UNSIGNED_SELECTOR);
}

enum bt_field_class_variant_with_selector_append_option_status
bt_field_class_variant_with_signed_selector_append_option(
		struct bt_field_class *fc, const char *name,
		struct bt_field_class *option_fc,
		const struct bt_integer_range_set_signed *range_set)
{
	return append_option_to_variant_with_selector_field_class(fc,
		name, option_fc, (const void *) range_set,
		BT_FIELD_CLASS_TYPE_VARIANT_WITH_SIGNED_SELECTOR);
}

uint64_t bt_field_class_variant_get_option_count(const struct bt_field_class *fc)
{
	const struct bt_field_class_variant *var_fc = (const void *) fc;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_IS_VARIANT(fc, "Field class");
	return (uint64_t) var_fc->common.named_fcs->len;
}

const struct bt_field_class_variant_option *
bt_field_class_variant_borrow_option_by_name_const(
		const struct bt_field_class *fc, const char *name)
{
	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_IS_VARIANT(fc, "Field class");
	return (const void *)
		borrow_named_field_class_from_container_field_class_by_name(
			(void *) fc, name);
}

const struct bt_field_class_variant_option *
bt_field_class_variant_borrow_option_by_index_const(
		const struct bt_field_class *fc, uint64_t index)
{
	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_IS_VARIANT(fc, "Field class");
	return (const void *)
		borrow_named_field_class_from_container_field_class_at_index(
			(void *) fc, index);
}

const struct bt_field_class_variant_with_unsigned_selector_option *
bt_field_class_variant_with_unsigned_selector_borrow_option_by_name_const(
		const struct bt_field_class *fc, const char *name)
{
	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc,
		BT_FIELD_CLASS_TYPE_VARIANT_WITH_UNSIGNED_SELECTOR,
		"Field class");
	return (const void *)
		borrow_named_field_class_from_container_field_class_by_name(
			(void *) fc, name);
}

const struct bt_field_class_variant_with_unsigned_selector_option *
bt_field_class_variant_with_unsigned_selector_borrow_option_by_index_const(
		const struct bt_field_class *fc, uint64_t index)
{
	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc,
		BT_FIELD_CLASS_TYPE_VARIANT_WITH_UNSIGNED_SELECTOR,
		"Field class");
	return (const void *)
		borrow_named_field_class_from_container_field_class_at_index(
			(void *) fc, index);
}

const struct bt_field_class_variant_with_signed_selector_option *
bt_field_class_variant_with_signed_selector_borrow_option_by_name_const(
		const struct bt_field_class *fc, const char *name)
{
	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc,
		BT_FIELD_CLASS_TYPE_VARIANT_WITH_SIGNED_SELECTOR,
		"Field class");
	return (const void *)
		borrow_named_field_class_from_container_field_class_by_name(
			(void *) fc, name);
}

const struct bt_field_class_variant_with_signed_selector_option *
bt_field_class_variant_with_signed_selector_borrow_option_by_index_const(
		const struct bt_field_class *fc, uint64_t index)
{
	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc,
		BT_FIELD_CLASS_TYPE_VARIANT_WITH_SIGNED_SELECTOR,
		"Field class");
	return (const void *)
		borrow_named_field_class_from_container_field_class_at_index(
			(void *) fc, index);
}

const char *bt_field_class_variant_option_get_name(
		const struct bt_field_class_variant_option *option)
{
	const struct bt_named_field_class *named_fc = (const void *) option;

	BT_ASSERT_PRE_DEV_NON_NULL(option, "Variant field class option");
	return named_fc->name->str;
}

const struct bt_field_class *
bt_field_class_variant_option_borrow_field_class_const(
		const struct bt_field_class_variant_option *option)
{
	const struct bt_named_field_class *named_fc = (const void *) option;

	BT_ASSERT_PRE_DEV_NON_NULL(option, "Variant field class option");
	return named_fc->fc;
}

const struct bt_integer_range_set_unsigned *
bt_field_class_variant_with_unsigned_selector_option_borrow_ranges_const(
		const struct bt_field_class_variant_with_unsigned_selector_option *option)
{
	const struct bt_field_class_variant_with_selector_option *opt =
		(const void *) option;

	BT_ASSERT_PRE_DEV_NON_NULL(option, "Variant field class option");
	return (const void *) opt->range_set;
}

const struct bt_integer_range_set_signed *
bt_field_class_variant_with_signed_selector_option_borrow_ranges_const(
		const struct bt_field_class_variant_with_signed_selector_option *option)
{
	const struct bt_field_class_variant_with_selector_option *opt =
		(const void *) option;

	BT_ASSERT_PRE_DEV_NON_NULL(option, "Variant field class option");
	return (const void *) opt->range_set;
}

const struct bt_field_path *
bt_field_class_variant_with_selector_borrow_selector_field_path_const(
		const struct bt_field_class *fc)
{
	const struct bt_field_class_variant_with_selector *var_fc =
		(const void *) fc;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_IS_VARIANT_WITH_SEL(fc, "Field class");
	return var_fc->selector_field_path;
}

static
void init_array_field_class(struct bt_field_class_array *fc,
		enum bt_field_class_type type, bt_object_release_func release_func,
		struct bt_field_class *element_fc)
{
	BT_ASSERT(element_fc);
	init_field_class((void *) fc, type, release_func);
	fc->element_fc = element_fc;
	bt_object_get_no_null_check(fc->element_fc);
	bt_field_class_freeze(element_fc);
}

static
void finalize_array_field_class(struct bt_field_class_array *array_fc)
{
	BT_ASSERT(array_fc);
	BT_LOGD_STR("Putting element field class.");
	BT_OBJECT_PUT_REF_AND_RESET(array_fc->element_fc);
}

static
void destroy_static_array_field_class(struct bt_object *obj)
{
	BT_ASSERT(obj);
	BT_LIB_LOGD("Destroying static array field class object: %!+F", obj);
	finalize_array_field_class((void *) obj);
	g_free(obj);
}

struct bt_field_class *
bt_field_class_static_array_create(bt_trace_class *trace_class,
		struct bt_field_class *element_fc, uint64_t length)
{
	struct bt_field_class_static_array *array_fc = NULL;

	BT_ASSERT_PRE_NON_NULL(trace_class, "Trace class");
	BT_ASSERT_PRE_NON_NULL(element_fc, "Element field class");
	BT_LOGD_STR("Creating default static array field class object.");
	array_fc = g_new0(struct bt_field_class_static_array, 1);
	if (!array_fc) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to allocate one static array field class.");
		goto error;
	}

	init_array_field_class((void *) array_fc, BT_FIELD_CLASS_TYPE_STATIC_ARRAY,
		destroy_static_array_field_class, element_fc);
	array_fc->length = length;
	BT_LIB_LOGD("Created static array field class object: %!+F", array_fc);
	goto end;

error:
	BT_OBJECT_PUT_REF_AND_RESET(array_fc);

end:
	return (void *) array_fc;
}

const struct bt_field_class *
bt_field_class_array_borrow_element_field_class_const(
		const struct bt_field_class *fc)
{
	const struct bt_field_class_array *array_fc = (const void *) fc;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_IS_ARRAY(fc, "Field class");
	return array_fc->element_fc;
}

struct bt_field_class *
bt_field_class_array_borrow_element_field_class(struct bt_field_class *fc)
{
	struct bt_field_class_array *array_fc = (void *) fc;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_IS_ARRAY(fc, "Field class");
	return array_fc->element_fc;
}

uint64_t bt_field_class_static_array_get_length(const struct bt_field_class *fc)
{
	const struct bt_field_class_static_array *array_fc = (const void *) fc;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_STATIC_ARRAY,
		"Field class");
	return (uint64_t) array_fc->length;
}

static
void destroy_dynamic_array_field_class(struct bt_object *obj)
{
	struct bt_field_class_dynamic_array *fc = (void *) obj;

	BT_ASSERT(fc);
	BT_LIB_LOGD("Destroying dynamic array field class object: %!+F", fc);
	finalize_array_field_class((void *) fc);
	BT_LOGD_STR("Putting length field path.");
	BT_OBJECT_PUT_REF_AND_RESET(fc->length_field_path);
	BT_LOGD_STR("Putting length field class.");
	BT_OBJECT_PUT_REF_AND_RESET(fc->length_fc);
	g_free(fc);
}

struct bt_field_class *bt_field_class_dynamic_array_create(
		struct bt_trace_class *trace_class,
		struct bt_field_class *element_fc,
		struct bt_field_class *length_fc)
{
	struct bt_field_class_dynamic_array *array_fc = NULL;

	BT_ASSERT_PRE_NON_NULL(trace_class, "Trace class");
	BT_ASSERT_PRE_NON_NULL(element_fc, "Element field class");
	BT_LOGD_STR("Creating default dynamic array field class object.");
	array_fc = g_new0(struct bt_field_class_dynamic_array, 1);
	if (!array_fc) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to allocate one dynamic array field class.");
		goto error;
	}

	init_array_field_class((void *) array_fc,
		BT_FIELD_CLASS_TYPE_DYNAMIC_ARRAY,
		destroy_dynamic_array_field_class, element_fc);

	if (length_fc) {
		BT_ASSERT_PRE_FC_IS_UNSIGNED_INT(length_fc,
			"Length field class");
		array_fc->length_fc = length_fc;
		bt_object_get_no_null_check(array_fc->length_fc);
		bt_field_class_freeze(length_fc);
	}

	BT_LIB_LOGD("Created dynamic array field class object: %!+F", array_fc);
	goto end;

error:
	BT_OBJECT_PUT_REF_AND_RESET(array_fc);

end:
	return (void *) array_fc;
}

const struct bt_field_path *
bt_field_class_dynamic_array_borrow_length_field_path_const(
		const struct bt_field_class *fc)
{
	const struct bt_field_class_dynamic_array *seq_fc = (const void *) fc;

	BT_ASSERT_PRE_DEV_NON_NULL(fc, "Field class");
	BT_ASSERT_PRE_DEV_FC_HAS_ID(fc, BT_FIELD_CLASS_TYPE_DYNAMIC_ARRAY,
		"Field class");
	return seq_fc->length_field_path;
}

static
void destroy_string_field_class(struct bt_object *obj)
{
	BT_ASSERT(obj);
	BT_LIB_LOGD("Destroying string field class object: %!+F", obj);
	g_free(obj);
}

struct bt_field_class *bt_field_class_string_create(bt_trace_class *trace_class)
{
	struct bt_field_class_string *string_fc = NULL;

	BT_ASSERT_PRE_NON_NULL(trace_class, "Trace class");
	BT_LOGD_STR("Creating default string field class object.");
	string_fc = g_new0(struct bt_field_class_string, 1);
	if (!string_fc) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to allocate one string field class.");
		goto error;
	}

	init_field_class((void *) string_fc, BT_FIELD_CLASS_TYPE_STRING,
		destroy_string_field_class);
	BT_LIB_LOGD("Created string field class object: %!+F", string_fc);
	goto end;

error:
	BT_OBJECT_PUT_REF_AND_RESET(string_fc);

end:
	return (void *) string_fc;
}

BT_HIDDEN
void _bt_field_class_freeze(const struct bt_field_class *c_fc)
{
	struct bt_field_class *fc = (void *) c_fc;

	/*
	 * Element/member/option field classes are frozen when added to
	 * their owner.
	 */
	BT_ASSERT(fc);
	fc->frozen = true;

	switch (fc->type) {
	case BT_FIELD_CLASS_TYPE_STRUCTURE:
	case BT_FIELD_CLASS_TYPE_VARIANT_WITHOUT_SELECTOR:
	case BT_FIELD_CLASS_TYPE_VARIANT_WITH_UNSIGNED_SELECTOR:
	case BT_FIELD_CLASS_TYPE_VARIANT_WITH_SIGNED_SELECTOR:
	{
		struct bt_field_class_named_field_class_container *container_fc =
			(void *) fc;
		uint64_t i;

		for (i = 0; i < container_fc->named_fcs->len; i++) {
			bt_named_field_class_freeze(
				container_fc->named_fcs->pdata[i]);
		}

		break;
	}
	default:
		break;
	}
}

BT_HIDDEN
void _bt_named_field_class_freeze(const struct bt_named_field_class *named_fc)
{
	BT_ASSERT(named_fc);
	((struct bt_named_field_class *) named_fc)->frozen = true;
	bt_field_class_freeze(named_fc->fc);
}

BT_HIDDEN
void bt_field_class_make_part_of_trace_class(const struct bt_field_class *c_fc)
{
	struct bt_field_class *fc = (void *) c_fc;

	BT_ASSERT(fc);
	BT_ASSERT_PRE(!fc->part_of_trace_class,
		"Field class is already part of a trace: %!+F", fc);
	fc->part_of_trace_class = true;

	switch (fc->type) {
	case BT_FIELD_CLASS_TYPE_STRUCTURE:
	case BT_FIELD_CLASS_TYPE_VARIANT_WITHOUT_SELECTOR:
	case BT_FIELD_CLASS_TYPE_VARIANT_WITH_UNSIGNED_SELECTOR:
	case BT_FIELD_CLASS_TYPE_VARIANT_WITH_SIGNED_SELECTOR:
	{
		struct bt_field_class_named_field_class_container *container_fc =
			(void *) fc;
		uint64_t i;

		for (i = 0; i < container_fc->named_fcs->len; i++) {
			struct bt_named_field_class *named_fc =
				container_fc->named_fcs->pdata[i];

			bt_field_class_make_part_of_trace_class(named_fc->fc);
		}

		break;
	}
	case BT_FIELD_CLASS_TYPE_STATIC_ARRAY:
	case BT_FIELD_CLASS_TYPE_DYNAMIC_ARRAY:
	{
		struct bt_field_class_array *array_fc = (void *) fc;

		bt_field_class_make_part_of_trace_class(array_fc->element_fc);
		break;
	}
	default:
		break;
	}
}

void bt_field_class_get_ref(const struct bt_field_class *field_class)
{
	bt_object_get_ref(field_class);
}

void bt_field_class_put_ref(const struct bt_field_class *field_class)
{
	bt_object_put_ref(field_class);
}
