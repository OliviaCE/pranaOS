/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// includes
#include <base/Checked.h>
#include <base/Function.h>
#include <base/StringBuilder.h>
#include <base/Utf16View.h>
#include <libjs/heap/Heap.h>
#include <libjs/runtime/AbstractOperations.h>
#include <libjs/runtime/Array.h>
#include <libjs/runtime/Error.h>
#include <libjs/runtime/GlobalObject.h>
#include <libjs/runtime/PrimitiveString.h>
#include <libjs/runtime/RegExpObject.h>
#include <libjs/runtime/StringIterator.h>
#include <libjs/runtime/StringObject.h>
#include <libjs/runtime/StringPrototype.h>
#include <libjs/runtime/Utf16String.h>
#include <libjs/runtime/Value.h>
#include <libunicode/CharacterTypes.h>
#include <string.h>

namespace JS {

static Optional<String> ak_string_from(VM& vm, GlobalObject& global_object)
{
    auto this_value = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};
    return this_value.to_string(global_object);
}

static Utf16String utf16_string_from(VM& vm, GlobalObject& global_object)
{
    auto this_value = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};
    return this_value.to_utf16_string(global_object);
}

static Optional<size_t> split_match(Utf16View const& haystack, size_t start, Utf16View const& needle)
{
    auto r = needle.length_in_code_units();
    auto s = haystack.length_in_code_units();
    if (start + r > s)
        return {};
    for (size_t i = 0; i < r; ++i) {
        if (haystack.code_unit_at(start + i) != needle.code_unit_at(i))
            return {};
    }
    return start + r;
}

CodePoint code_point_at(Utf16View const& string, size_t position)
{
    VERIFY(position < string.length_in_code_units());

    auto first = string.code_unit_at(position);
    auto code_point = static_cast<u32>(first);

    if (!Utf16View::is_high_surrogate(first) && !Utf16View::is_low_surrogate(first))
        return { code_point, 1, false };

    if (Utf16View::is_low_surrogate(first) || (position + 1 == string.length_in_code_units()))
        return { code_point, 1, true };

    auto second = string.code_unit_at(position + 1);

    if (!Utf16View::is_low_surrogate(second))
        return { code_point, 1, true };

    code_point = Utf16View::decode_surrogate_pair(first, second);
    return { code_point, 2, false };
}

static Optional<size_t> string_index_of(Utf16View const& string, Utf16View const& search_value, size_t from_index)
{
    size_t string_length = string.length_in_code_units();
    size_t search_length = search_value.length_in_code_units();

    if ((search_length == 0) && (from_index <= string_length))
        return from_index;

    if (search_length > string_length)
        return {};

    for (size_t i = from_index; i <= string_length - search_length; ++i) {
        auto candidate = string.substring_view(i, search_length);
        if (candidate == search_value)
            return i;
    }

    return {};
}

StringPrototype::StringPrototype(GlobalObject& global_object)
    : StringObject(*js_string(global_object.heap(), String::empty()), *global_object.object_prototype())
{
}

void StringPrototype::initialize(GlobalObject& global_object)
{
    auto& vm = this->vm();
    StringObject::initialize(global_object);
    u8 attr = Attribute::Writable | Attribute::Configurable;

    define_native_function(vm.names.charAt, char_at, 1, attr);
    define_native_function(vm.names.charCodeAt, char_code_at, 1, attr);
    define_native_function(vm.names.codePointAt, code_point_at, 1, attr);
    define_native_function(vm.names.repeat, repeat, 1, attr);
    define_native_function(vm.names.startsWith, starts_with, 1, attr);
    define_native_function(vm.names.endsWith, ends_with, 1, attr);
    define_native_function(vm.names.indexOf, index_of, 1, attr);
    define_native_function(vm.names.toLocaleLowerCase, to_locale_lowercase, 0, attr);
    define_native_function(vm.names.toLocaleUpperCase, to_locale_uppercase, 0, attr);
    define_native_function(vm.names.toLowerCase, to_lowercase, 0, attr);
    define_native_function(vm.names.toUpperCase, to_uppercase, 0, attr);
    define_native_function(vm.names.toString, to_string, 0, attr);
    define_native_function(vm.names.valueOf, value_of, 0, attr);
    define_native_function(vm.names.padStart, pad_start, 1, attr);
    define_native_function(vm.names.padEnd, pad_end, 1, attr);
    define_native_function(vm.names.trim, trim, 0, attr);
    define_native_function(vm.names.trimStart, trim_start, 0, attr);
    define_direct_property(vm.names.trimLeft, get_without_side_effects(vm.names.trimStart), attr);
    define_native_function(vm.names.trimEnd, trim_end, 0, attr);
    define_direct_property(vm.names.trimRight, get_without_side_effects(vm.names.trimEnd), attr);
    define_native_function(vm.names.concat, concat, 1, attr);
    define_native_function(vm.names.substr, substr, 2, attr);
    define_native_function(vm.names.substring, substring, 2, attr);
    define_native_function(vm.names.includes, includes, 1, attr);
    define_native_function(vm.names.slice, slice, 2, attr);
    define_native_function(vm.names.split, split, 2, attr);
    define_native_function(vm.names.lastIndexOf, last_index_of, 1, attr);
    define_native_function(vm.names.at, at, 1, attr);
    define_native_function(vm.names.match, match, 1, attr);
    define_native_function(vm.names.matchAll, match_all, 1, attr);
    define_native_function(vm.names.replace, replace, 2, attr);
    define_native_function(vm.names.replaceAll, replace_all, 2, attr);
    define_native_function(vm.names.search, search, 1, attr);
    define_native_function(vm.names.anchor, anchor, 1, attr);
    define_native_function(vm.names.big, big, 0, attr);
    define_native_function(vm.names.blink, blink, 0, attr);
    define_native_function(vm.names.bold, bold, 0, attr);
    define_native_function(vm.names.fixed, fixed, 0, attr);
    define_native_function(vm.names.fontcolor, fontcolor, 1, attr);
    define_native_function(vm.names.fontsize, fontsize, 1, attr);
    define_native_function(vm.names.italics, italics, 0, attr);
    define_native_function(vm.names.link, link, 1, attr);
    define_native_function(vm.names.small, small, 0, attr);
    define_native_function(vm.names.strike, strike, 0, attr);
    define_native_function(vm.names.sub, sub, 0, attr);
    define_native_function(vm.names.sup, sup, 0, attr);
    define_native_function(vm.names.localeCompare, locale_compare, 1, attr);
    define_native_function(*vm.well_known_symbol_iterator(), symbol_iterator, 0, attr);
}

StringPrototype::~StringPrototype()
{
}

static Value this_string_value(GlobalObject& global_object, Value value)
{
    if (value.is_string())
        return value;
    if (value.is_object() && is<StringObject>(value.as_object()))
        return static_cast<StringObject&>(value.as_object()).value_of();
    auto& vm = global_object.vm();
    vm.throw_exception<TypeError>(global_object, ErrorType::NotA, "String");
    return {};
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::char_at)
{
    auto string = utf16_string_from(vm, global_object);
    if (vm.exception())
        return {};
    auto position = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};

    if (position < 0 || position >= string.length_in_code_units())
        return js_string(vm, String::empty());

    return js_string(vm, string.substring_view(position, 1));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::char_code_at)
{
    auto string = utf16_string_from(vm, global_object);
    if (vm.exception())
        return {};
    auto position = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};

    if (position < 0 || position >= string.length_in_code_units())
        return js_nan();

    return Value(string.code_unit_at(position));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::code_point_at)
{
    auto string = utf16_string_from(vm, global_object);
    if (vm.exception())
        return {};
    auto position = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};

    if (position < 0 || position >= string.length_in_code_units())
        return js_undefined();

    auto code_point = JS::code_point_at(string.view(), position);
    return Value(code_point.code_point);
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::repeat)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};

    auto n = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};

    if (n < 0) {
        vm.throw_exception<RangeError>(global_object, ErrorType::StringRepeatCountMustBe, "positive");
        return {};
    }

    if (Value(n).is_positive_infinity()) {
        vm.throw_exception<RangeError>(global_object, ErrorType::StringRepeatCountMustBe, "finite");
        return {};
    }

    if (n == 0)
        return js_string(vm, String::empty());

    // NOTE: This is an optimization, it is not required by the specification but it produces equivalent behaviour
    if (string->is_empty())
        return js_string(vm, String::empty());

    StringBuilder builder;
    for (size_t i = 0; i < n; ++i)
        builder.append(*string);
    return js_string(vm, builder.to_string());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::starts_with)
{
    auto string = utf16_string_from(vm, global_object);
    if (vm.exception())
        return {};

    auto search_string_value = vm.argument(0);

    bool search_is_regexp = search_string_value.is_regexp(global_object);
    if (vm.exception())
        return {};
    if (search_is_regexp) {
        vm.throw_exception<TypeError>(global_object, ErrorType::IsNotA, "searchString", "string, but a regular expression");
        return {};
    }

    auto search_string = search_string_value.to_utf16_string(global_object);
    if (vm.exception())
        return {};

    auto string_length = string.length_in_code_units();
    auto search_length = search_string.length_in_code_units();

    size_t start = 0;
    if (!vm.argument(1).is_undefined()) {
        auto position = vm.argument(1).to_integer_or_infinity(global_object);
        if (vm.exception())
            return {};
        start = clamp(position, static_cast<double>(0), static_cast<double>(string_length));
    }

    if (search_length == 0)
        return Value(true);

    size_t end = start + search_length;
    if (end > string_length)
        return Value(false);

    auto substring_view = string.substring_view(start, end - start);
    return Value(substring_view == search_string.view());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::ends_with)
{
    auto string = utf16_string_from(vm, global_object);
    if (vm.exception())
        return {};

    auto search_string_value = vm.argument(0);

    bool search_is_regexp = search_string_value.is_regexp(global_object);
    if (vm.exception())
        return {};
    if (search_is_regexp) {
        vm.throw_exception<TypeError>(global_object, ErrorType::IsNotA, "searchString", "string, but a regular expression");
        return {};
    }

    auto search_string = search_string_value.to_utf16_string(global_object);
    if (vm.exception())
        return {};

    auto string_length = string.length_in_code_units();
    auto search_length = search_string.length_in_code_units();

    size_t end = string_length;
    if (!vm.argument(1).is_undefined()) {
        auto position = vm.argument(1).to_integer_or_infinity(global_object);
        if (vm.exception())
            return {};
        end = clamp(position, static_cast<double>(0), static_cast<double>(string_length));
    }

    if (search_length == 0)
        return Value(true);
    if (search_length > end)
        return Value(false);

    size_t start = end - search_length;

    auto substring_view = string.substring_view(start, end - start);
    return Value(substring_view == search_string.view());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::index_of)
{
    auto string = utf16_string_from(vm, global_object);
    if (vm.exception())
        return {};

    auto search_string = vm.argument(0).to_utf16_string(global_object);
    if (vm.exception())
        return {};

    auto utf16_string_view = string.view();
    auto utf16_search_view = search_string.view();

    size_t start = 0;
    if (vm.argument_count() > 1) {
        auto position = vm.argument(1).to_integer_or_infinity(global_object);
        if (vm.exception())
            return {};
        start = clamp(position, static_cast<double>(0), static_cast<double>(utf16_string_view.length_in_code_units()));
    }

    auto index = string_index_of(utf16_string_view, utf16_search_view, start);
    return index.has_value() ? Value(*index) : Value(-1);
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::to_locale_lowercase)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};

    auto lowercase = Unicode::to_unicode_lowercase_full(*string);
    return js_string(vm, move(lowercase));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::to_locale_uppercase)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};

    auto uppercase = Unicode::to_unicode_uppercase_full(*string);
    return js_string(vm, move(uppercase));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::to_lowercase)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};

    auto lowercase = Unicode::to_unicode_lowercase_full(*string);
    return js_string(vm, move(lowercase));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::to_uppercase)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};

    auto uppercase = Unicode::to_unicode_uppercase_full(*string);
    return js_string(vm, move(uppercase));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::to_string)
{
    return this_string_value(global_object, vm.this_value(global_object));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::value_of)
{
    return this_string_value(global_object, vm.this_value(global_object));
}

enum class PadPlacement {
    Start,
    End,
};

static Value pad_string(GlobalObject& global_object, Utf16String string, PadPlacement placement)
{
    auto& vm = global_object.vm();
    auto string_length = string.length_in_code_units();

    auto max_length = vm.argument(0).to_length(global_object);
    if (vm.exception())
        return {};
    if (max_length <= string_length)
        return js_string(vm, move(string));

    Utf16String fill_string(Vector<u16> { 0x20 });
    if (!vm.argument(1).is_undefined()) {
        fill_string = vm.argument(1).to_utf16_string(global_object);
        if (vm.exception())
            return {};
        if (fill_string.is_empty())
            return js_string(vm, move(string));
    }

    auto fill_code_units = fill_string.length_in_code_units();
    auto fill_length = max_length - string_length;

    StringBuilder filler_builder;
    for (size_t i = 0; i < fill_length / fill_code_units; ++i)
        filler_builder.append(fill_string.view());

    filler_builder.append(fill_string.substring_view(0, fill_length % fill_code_units));
    auto filler = filler_builder.build();

    auto formatted = placement == PadPlacement::Start
        ? String::formatted("{}{}", filler, string.view())
        : String::formatted("{}{}", string.view(), filler);
    return js_string(vm, move(formatted));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::pad_start)
{
    auto string = utf16_string_from(vm, global_object);
    if (vm.exception())
        return {};
    return pad_string(global_object, move(string), PadPlacement::Start);
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::pad_end)
{
    auto string = utf16_string_from(vm, global_object);
    if (vm.exception())
        return {};
    return pad_string(global_object, move(string), PadPlacement::End);
}

static const Utf8View whitespace_characters = Utf8View("\x09\x0A\x0B\x0C\x0D\x20\xC2\xA0\xE1\x9A\x80\xE2\x80\x80\xE2\x80\x81\xE2\x80\x82\xE2\x80\x83\xE2\x80\x84\xE2\x80\x85\xE2\x80\x86\xE2\x80\x87\xE2\x80\x88\xE2\x80\x89\xE2\x80\x8A\xE2\x80\xAF\xE2\x81\x9F\xE3\x80\x80\xE2\x80\xA8\xE2\x80\xA9\xEF\xBB\xBF");

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::trim)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    return js_string(vm, Utf8View(*string).trim(whitespace_characters, TrimMode::Both).as_string());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::trim_start)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    return js_string(vm, Utf8View(*string).trim(whitespace_characters, TrimMode::Left).as_string());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::trim_end)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    return js_string(vm, Utf8View(*string).trim(whitespace_characters, TrimMode::Right).as_string());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::concat)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    StringBuilder builder;
    builder.append(*string);
    for (size_t i = 0; i < vm.argument_count(); ++i) {
        auto string_argument = vm.argument(i).to_string(global_object);
        if (vm.exception())
            return {};
        builder.append(string_argument);
    }
    return js_string(vm, builder.to_string());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::substring)
{
    auto string = utf16_string_from(vm, global_object);
    if (vm.exception())
        return {};
    auto string_length = static_cast<double>(string.length_in_code_units());

    auto start = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};

    auto end = string_length;
    if (!vm.argument(1).is_undefined()) {
        end = vm.argument(1).to_integer_or_infinity(global_object);
        if (vm.exception())
            return {};
    }

    size_t final_start = clamp(start, static_cast<double>(0), string_length);
    size_t final_end = clamp(end, static_cast<double>(0), string_length);

    size_t from = min(final_start, final_end);
    size_t to = max(final_start, final_end);

    return js_string(vm, string.substring_view(from, to - from));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::substr)
{
    auto string = utf16_string_from(vm, global_object);
    if (vm.exception())
        return {};
    auto size = string.length_in_code_units();

    auto int_start = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};
    if (Value(int_start).is_negative_infinity())
        int_start = 0;
    if (int_start < 0)
        int_start = max(size + (i32)int_start, 0);

    auto length = vm.argument(1);

    auto int_length = length.is_undefined() ? size : length.to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};

    if (Value(int_start).is_positive_infinity() || (int_length <= 0) || Value(int_length).is_positive_infinity())
        return js_string(vm, String::empty());

    auto int_end = min((i32)(int_start + int_length), size);

    if (int_start >= int_end)
        return js_string(vm, String::empty());

    return js_string(vm, string.substring_view(int_start, int_end - int_start));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::includes)
{
    auto string = utf16_string_from(vm, global_object);
    if (vm.exception())
        return {};

    auto search_string_value = vm.argument(0);

    bool search_is_regexp = search_string_value.is_regexp(global_object);
    if (vm.exception())
        return {};
    if (search_is_regexp) {
        vm.throw_exception<TypeError>(global_object, ErrorType::IsNotA, "searchString", "string, but a regular expression");
        return {};
    }

    auto search_string = search_string_value.to_utf16_string(global_object);
    if (vm.exception())
        return {};

    size_t start = 0;
    if (!vm.argument(1).is_undefined()) {
        auto position = vm.argument(1).to_integer_or_infinity(global_object);
        if (vm.exception())
            return {};
        start = clamp(position, static_cast<double>(0), static_cast<double>(string.length_in_code_units()));
    }

    auto index = string_index_of(string.view(), search_string.view(), start);
    return Value(index.has_value());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::slice)
{
    auto string = utf16_string_from(vm, global_object);
    if (vm.exception())
        return {};
    auto string_length = static_cast<double>(string.length_in_code_units());

    auto int_start = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};
    if (Value(int_start).is_negative_infinity())
        int_start = 0;
    else if (int_start < 0)
        int_start = max(string_length + int_start, 0);
    else
        int_start = min(int_start, string_length);

    auto int_end = string_length;
    if (!vm.argument(1).is_undefined()) {
        int_end = vm.argument(1).to_integer_or_infinity(global_object);
        if (vm.exception())
            return {};
        if (Value(int_end).is_negative_infinity())
            int_end = 0;
        else if (int_end < 0)
            int_end = max(string_length + int_end, 0);
        else
            int_end = min(int_end, string_length);
    }

    if (int_start >= int_end)
        return js_string(vm, String::empty());

    return js_string(vm, string.substring_view(int_start, int_end - int_start));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::split)
{
    auto object = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};

    auto separator_argument = vm.argument(0);
    auto limit_argument = vm.argument(1);

    if (!separator_argument.is_nullish()) {
        auto splitter = separator_argument.get_method(global_object, *vm.well_known_symbol_split());
        if (vm.exception())
            return {};
        if (splitter)
            return vm.call(*splitter, separator_argument, object, limit_argument);
    }

    auto string = object.to_utf16_string(global_object);
    if (vm.exception())
        return {};

    auto* array = Array::create(global_object, 0);
    size_t array_length = 0;

    auto limit = NumericLimits<u32>::max();
    if (!limit_argument.is_undefined()) {
        limit = limit_argument.to_u32(global_object);
        if (vm.exception())
            return {};
    }

    auto separator = separator_argument.to_utf16_string(global_object);
    if (vm.exception())
        return {};

    if (limit == 0)
        return array;

    auto string_length = string.length_in_code_units();
    auto separator_length = separator.length_in_code_units();

    if (separator_argument.is_undefined()) {
        array->create_data_property_or_throw(0, js_string(vm, move(string)));
        return array;
    }

    if (string_length == 0) {
        if (separator_length > 0)
            array->create_data_property_or_throw(0, js_string(vm, move(string)));
        return array;
    }

    size_t start = 0;      // 'p' in the spec.
    auto position = start; // 'q' in the spec.
    while (position != string_length) {
        auto match = split_match(string.view(), position, separator.view()); // 'e' in the spec.
        if (!match.has_value() || match.value() == start) {
            ++position;
            continue;
        }

        auto segment = string.substring_view(start, position - start);
        array->create_data_property_or_throw(array_length, js_string(vm, segment));
        ++array_length;
        if (array_length == limit)
            return array;
        start = match.value();
        position = start;
    }

    auto rest = string.substring_view(start);
    array->create_data_property_or_throw(array_length, js_string(vm, rest));

    return array;
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::last_index_of)
{
    auto string = utf16_string_from(vm, global_object);
    if (vm.exception())
        return {};

    auto search_string = vm.argument(0).to_utf16_string(global_object);
    if (vm.exception())
        return {};

    auto string_length = string.length_in_code_units();
    auto search_length = search_string.length_in_code_units();

    auto position = vm.argument(1).to_number(global_object);
    if (vm.exception())
        return {};
    double pos = position.is_nan() ? static_cast<double>(INFINITY) : position.to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};

    size_t start = clamp(pos, static_cast<double>(0), static_cast<double>(string_length));
    Optional<size_t> last_index;

    for (size_t k = 0; (k <= start) && (k + search_length <= string_length); ++k) {
        bool is_match = true;

        for (size_t j = 0; j < search_length; ++j) {
            if (string.code_unit_at(k + j) != search_string.code_unit_at(j)) {
                is_match = false;
                break;
            }
        }

        if (is_match)
            last_index = k;
    }

    return last_index.has_value() ? Value(*last_index) : Value(-1);
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::at)
{
    auto string = utf16_string_from(vm, global_object);
    if (vm.exception())
        return {};
    auto length = string.length_in_code_units();

    auto relative_index = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};
    if (Value(relative_index).is_infinity())
        return js_undefined();

    Checked<size_t> index { 0 };
    if (relative_index >= 0) {
        index += relative_index;
    } else {
        index += length;
        index -= -relative_index;
    }
    if (index.has_overflow() || index.value() >= length)
        return js_undefined();

    return js_string(vm, string.substring_view(index.value(), 1));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::symbol_iterator)
{
    auto this_object = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};
    auto string = this_object.to_string(global_object);
    if (vm.exception())
        return {};
    return StringIterator::create(global_object, string);
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::match)
{
    auto this_object = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};
    auto regexp = vm.argument(0);
    if (!regexp.is_nullish()) {
        if (auto* matcher = regexp.get_method(global_object, *vm.well_known_symbol_match()))
            return vm.call(*matcher, regexp, this_object);
        if (vm.exception())
            return {};
    }

    auto string = this_object.to_utf16_string(global_object);
    if (vm.exception())
        return {};

    auto rx = regexp_create(global_object, regexp, js_undefined());
    if (!rx)
        return {};
    return Value(rx).invoke(global_object, *vm.well_known_symbol_match(), js_string(vm, move(string)));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::match_all)
{
    auto this_object = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};
    auto regexp = vm.argument(0);
    if (!regexp.is_nullish()) {
        auto is_regexp = regexp.is_regexp(global_object);
        if (vm.exception())
            return {};
        if (is_regexp) {
            auto flags = regexp.as_object().get("flags");
            if (vm.exception())
                return {};
            auto flags_object = require_object_coercible(global_object, flags);
            if (vm.exception())
                return {};
            auto flags_string = flags_object.to_string(global_object);
            if (vm.exception())
                return {};
            if (!flags_string.contains("g")) {
                vm.throw_exception<TypeError>(global_object, ErrorType::StringNonGlobalRegExp);
                return {};
            }
        }
        if (auto* matcher = regexp.get_method(global_object, *vm.well_known_symbol_match_all()))
            return vm.call(*matcher, regexp, this_object);
        if (vm.exception())
            return {};
    }

    auto string = this_object.to_utf16_string(global_object);
    if (vm.exception())
        return {};

    auto rx = regexp_create(global_object, regexp, js_string(vm, "g"));
    if (!rx)
        return {};
    return Value(rx).invoke(global_object, *vm.well_known_symbol_match_all(), js_string(vm, move(string)));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::replace)
{
    auto this_object = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};
    auto search_value = vm.argument(0);
    auto replace_value = vm.argument(1);

    if (!search_value.is_nullish()) {
        if (auto* replacer = search_value.get_method(global_object, *vm.well_known_symbol_replace()))
            return vm.call(*replacer, search_value, this_object, replace_value);
        if (vm.exception())
            return {};
    }

    auto string = this_object.to_utf16_string(global_object);
    if (vm.exception())
        return {};
    auto search_string = search_value.to_utf16_string(global_object);
    if (vm.exception())
        return {};

    if (!replace_value.is_function()) {
        auto replace_string = replace_value.to_utf16_string(global_object);
        if (vm.exception())
            return {};
        replace_value = js_string(vm, move(replace_string));
        if (vm.exception())
            return {};
    }

    Optional<size_t> position = string_index_of(string.view(), search_string.view(), 0);
    if (!position.has_value())
        return js_string(vm, move(string));

    auto preserved = string.substring_view(0, position.value());
    String replacement;

    if (replace_value.is_function()) {
        auto result = vm.call(replace_value.as_function(), js_undefined(), js_string(vm, search_string), Value(position.value()), js_string(vm, string));
        if (vm.exception())
            return {};

        replacement = result.to_string(global_object);
        if (vm.exception())
            return {};
    } else {
        replacement = get_substitution(global_object, search_string.view(), string.view(), *position, {}, js_undefined(), replace_value);
        if (vm.exception())
            return {};
    }

    StringBuilder builder;
    builder.append(preserved);
    builder.append(replacement);
    builder.append(string.substring_view(*position + search_string.length_in_code_units()));

    return js_string(vm, builder.build());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::replace_all)
{
    auto this_object = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};
    auto search_value = vm.argument(0);
    auto replace_value = vm.argument(1);

    if (!search_value.is_nullish()) {
        bool is_regexp = search_value.is_regexp(global_object);
        if (vm.exception())
            return {};

        if (is_regexp) {
            auto flags = search_value.as_object().get(vm.names.flags);
            if (vm.exception())
                return {};
            auto flags_object = require_object_coercible(global_object, flags);
            if (vm.exception())
                return {};
            auto flags_string = flags_object.to_string(global_object);
            if (vm.exception())
                return {};
            if (!flags_string.contains("g")) {
                vm.throw_exception<TypeError>(global_object, ErrorType::StringNonGlobalRegExp);
                return {};
            }
        }

        auto* replacer = search_value.get_method(global_object, *vm.well_known_symbol_replace());
        if (vm.exception())
            return {};
        if (replacer) {
            auto result = vm.call(*replacer, search_value, this_object, replace_value);
            if (vm.exception())
                return {};
            return result;
        }
    }

    auto string = this_object.to_utf16_string(global_object);
    if (vm.exception())
        return {};
    auto search_string = search_value.to_utf16_string(global_object);
    if (vm.exception())
        return {};

    if (!replace_value.is_function()) {
        auto replace_string = replace_value.to_utf16_string(global_object);
        if (vm.exception())
            return {};
        replace_value = js_string(vm, move(replace_string));
        if (vm.exception())
            return {};
    }

    auto string_length = string.length_in_code_units();
    auto search_length = search_string.length_in_code_units();

    Vector<size_t> match_positions;
    size_t advance_by = max(1u, search_length);
    auto position = string_index_of(string.view(), search_string.view(), 0);

    while (position.has_value()) {
        match_positions.append(*position);
        position = string_index_of(string.view(), search_string.view(), *position + advance_by);
    }

    size_t end_of_last_match = 0;
    StringBuilder result;

    for (auto position : match_positions) {
        auto preserved = string.substring_view(end_of_last_match, position - end_of_last_match);
        String replacement;

        if (replace_value.is_function()) {
            auto result = vm.call(replace_value.as_function(), js_undefined(), js_string(vm, search_string), Value(position), js_string(vm, string));
            if (vm.exception())
                return {};

            replacement = result.to_string(global_object);
            if (vm.exception())
                return {};
        } else {
            replacement = get_substitution(global_object, search_string.view(), string.view(), position, {}, js_undefined(), replace_value);
            if (vm.exception())
                return {};
        }

        result.append(preserved);
        result.append(replacement);

        end_of_last_match = position + search_length;
    }

    if (end_of_last_match < string_length)
        result.append(string.substring_view(end_of_last_match));

    return js_string(vm, result.build());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::search)
{
    auto this_object = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};
    auto regexp = vm.argument(0);
    if (!regexp.is_nullish()) {
        if (auto* searcher = regexp.get_method(global_object, *vm.well_known_symbol_search()))
            return vm.call(*searcher, regexp, this_object);
        if (vm.exception())
            return {};
    }

    auto string = this_object.to_utf16_string(global_object);
    if (vm.exception())
        return {};

    auto rx = regexp_create(global_object, regexp, js_undefined());
    if (!rx)
        return {};
    return Value(rx).invoke(global_object, *vm.well_known_symbol_search(), js_string(vm, move(string)));
}

static Value create_html(GlobalObject& global_object, Value string, const String& tag, const String& attribute, Value value)
{
    auto& vm = global_object.vm();
    require_object_coercible(global_object, string);
    if (vm.exception())
        return {};
    auto str = string.to_string(global_object);
    if (vm.exception())
        return {};
    StringBuilder builder;
    builder.append('<');
    builder.append(tag);
    if (!attribute.is_empty()) {
        auto value_string = value.to_string(global_object);
        if (vm.exception())
            return {};
        value_string.replace("\"", "&quot;", true);
        builder.append(' ');
        builder.append(attribute);
        builder.append("=\"");
        builder.append(value_string);
        builder.append('"');
    }
    builder.append('>');
    builder.append(str);
    builder.append("</");
    builder.append(tag);
    builder.append('>');
    return js_string(vm, builder.build());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::anchor)
{
    return create_html(global_object, vm.this_value(global_object), "a", "name", vm.argument(0));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::big)
{
    return create_html(global_object, vm.this_value(global_object), "big", String::empty(), Value());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::blink)
{
    return create_html(global_object, vm.this_value(global_object), "blink", String::empty(), Value());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::bold)
{
    return create_html(global_object, vm.this_value(global_object), "b", String::empty(), Value());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::fixed)
{
    return create_html(global_object, vm.this_value(global_object), "tt", String::empty(), Value());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::fontcolor)
{
    return create_html(global_object, vm.this_value(global_object), "font", "color", vm.argument(0));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::fontsize)
{
    return create_html(global_object, vm.this_value(global_object), "font", "size", vm.argument(0));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::italics)
{
    return create_html(global_object, vm.this_value(global_object), "i", String::empty(), Value());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::link)
{
    return create_html(global_object, vm.this_value(global_object), "a", "href", vm.argument(0));
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::small)
{
    return create_html(global_object, vm.this_value(global_object), "small", String::empty(), Value());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::strike)
{
    return create_html(global_object, vm.this_value(global_object), "strike", String::empty(), Value());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::sub)
{
    return create_html(global_object, vm.this_value(global_object), "sub", String::empty(), Value());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::sup)
{
    return create_html(global_object, vm.this_value(global_object), "sup", String::empty(), Value());
}

JS_DEFINE_NATIVE_FUNCTION(StringPrototype::locale_compare)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};

    auto that_string = vm.argument(0).to_string(global_object);
    if (vm.exception())
        return {};

    if (string == that_string)
        return Value(0);
    if (string.value() < that_string)
        return Value(-1);

    return Value(1);
}

}