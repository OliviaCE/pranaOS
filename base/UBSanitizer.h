/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
*/

#pragma once

// includes
#include <base/Types.h>

namespace Base::UBSanitizer {

extern bool g_ubsan_is_deadly;

typedef void* ValueHandle;

class SourceLocation {
public:
    const char* filename() const { return m_filename; }
    u32 line() const { return m_line; }
    u32 column() const { return m_column; }

private:
    const char* m_filename;
    u32 m_line;
    u32 m_column;
};

enum TypeKind : u16 {
    Integer = 0,
    Float = 1,
    Unknown = 0xffff,
};

class TypeDescriptor {
public:
    const char* name() const { return m_name; }
    TypeKind kind() const { return (TypeKind)m_kind; }
    bool is_integer() const { return kind() == TypeKind::Integer; }
    bool is_signed() const { return m_info & 1; }
    bool is_unsigned() const { return !is_signed(); }
    size_t bit_width() const { return 1 << (m_info >> 1); }

private:
    u16 m_kind;
    u16 m_info;
    char m_name[1];
};

struct InvalidValueData {
    SourceLocation location;
    const TypeDescriptor& type;
};

struct NonnullArgData {
    SourceLocation location;
    SourceLocation attribute_location;
    int argument_index;
};

struct NonnullReturnData {
    SourceLocation attribute_location;
};

struct OverflowData {
    SourceLocation location;
    const TypeDescriptor& type;
};

struct VLABoundData {
    SourceLocation location;
    const TypeDescriptor& type;
};

struct ShiftOutOfBoundsData {
    SourceLocation location;
    const TypeDescriptor& lhs_type;
    const TypeDescriptor& rhs_type;
};

}
