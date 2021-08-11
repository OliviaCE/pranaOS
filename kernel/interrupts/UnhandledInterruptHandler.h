/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
*/

#pragma once

// includes
#include <base/String.h>
#include <base/Types.h>
#include <kernel/interrupts/GenericInterruptHandler.h>

namespace Kernel {
class UnhandledInterruptHandler final : public GenericInterruptHandler {
public:
    explicit UnhandledInterruptHandler(u8 interrupt_vector);
    virtual ~UnhandledInterruptHandler();

    virtual bool handle_interrupt(const RegisterState&) override;

    [[noreturn]] virtual bool eoi() override;

    virtual HandlerType type() const override { return HandlerType::UnhandledInterruptHandler; }
    virtual StringView purpose() const override { return "Unhandled Interrupt Handler"; }
    virtual StringView controller() const override { VERIFY_NOT_REACHED(); }

    virtual size_t sharing_devices_count() const override { return 0; }
    virtual bool is_shared_handler() const override { return false; }
    virtual bool is_sharing_with_others() const override { return false; }

private:
};
}