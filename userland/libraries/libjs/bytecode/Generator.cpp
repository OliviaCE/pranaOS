/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// includes
#include <libjs/AST.h>
#include <libjs/bytecode/BasicBlock.h>
#include <libjs/bytecode/Generator.h>
#include <libjs/bytecode/Instruction.h>
#include <libjs/bytecode/Op.h>
#include <libjs/bytecode/Register.h>

namespace JS::Bytecode {

Generator::Generator()
    : m_string_table(make<StringTable>())
{
}

Generator::~Generator()
{
}

Executable Generator::generate(ASTNode const& node, bool is_in_generator_function)
{
    Generator generator;
    generator.switch_to_basic_block(generator.make_block());
    if (is_in_generator_function) {
        generator.enter_generator_context();

        auto& start_block = generator.make_block();
        generator.emit<Bytecode::Op::Yield>(Label { start_block });
        generator.switch_to_basic_block(start_block);
    }
    node.generate_bytecode(generator);
    if (is_in_generator_function) {

        for (auto& block : generator.m_root_basic_blocks) {
            if (block.is_terminated())
                continue;
            generator.switch_to_basic_block(block);
            generator.emit<Bytecode::Op::LoadImmediate>(js_undefined());
            generator.emit<Bytecode::Op::Yield>(nullptr);
        }
    }
    return { move(generator.m_root_basic_blocks), move(generator.m_string_table), generator.m_next_register };
}

void Generator::grow(size_t additional_size)
{
    VERIFY(m_current_basic_block);
    m_current_basic_block->grow(additional_size);
}

void* Generator::next_slot()
{
    VERIFY(m_current_basic_block);
    return m_current_basic_block->next_slot();
}

Register Generator::allocate_register()
{
    VERIFY(m_next_register != NumericLimits<u32>::max());
    return Register { m_next_register++ };
}

Label Generator::nearest_continuable_scope() const
{
    return m_continuable_scopes.last();
}

void Generator::begin_continuable_scope(Label continue_target)
{
    m_continuable_scopes.append(continue_target);
}

void Generator::end_continuable_scope()
{
    m_continuable_scopes.take_last();
}
Label Generator::nearest_breakable_scope() const
{
    return m_breakable_scopes.last();
}
void Generator::begin_breakable_scope(Label breakable_target)
{
    m_breakable_scopes.append(breakable_target);
}

void Generator::end_breakable_scope()
{
    m_breakable_scopes.take_last();
}
}