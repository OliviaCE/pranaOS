/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// includes
#include <base/Function.h>
#include <base/StringBuilder.h>
#include <libjs/Interpreter.h>
#include <libjs/runtime/AbstractOperations.h>
#include <libjs/runtime/BoundFunction.h>
#include <libjs/runtime/Error.h>
#include <libjs/runtime/FunctionObject.h>
#include <libjs/runtime/FunctionPrototype.h>
#include <libjs/runtime/GlobalObject.h>
#include <libjs/runtime/MarkedValueList.h>
#include <libjs/runtime/NativeFunction.h>
#include <libjs/runtime/OrdinaryFunctionObject.h>

namespace JS {

FunctionPrototype::FunctionPrototype(GlobalObject& global_object)
    : Object(*global_object.object_prototype())
{
}

void FunctionPrototype::initialize(GlobalObject& global_object)
{
    auto& vm = this->vm();
    Object::initialize(global_object);
    u8 attr = Attribute::Writable | Attribute::Configurable;
    define_native_function(vm.names.apply, apply, 2, attr);
    define_native_function(vm.names.bind, bind, 1, attr);
    define_native_function(vm.names.call, call, 1, attr);
    define_native_function(vm.names.toString, to_string, 0, attr);
    define_native_function(*vm.well_known_symbol_has_instance(), symbol_has_instance, 1, 0);
    define_direct_property(vm.names.length, Value(0), Attribute::Configurable);
    define_direct_property(vm.names.name, js_string(heap(), ""), Attribute::Configurable);
}

FunctionPrototype::~FunctionPrototype()
{
}

JS_DEFINE_NATIVE_FUNCTION(FunctionPrototype::apply)
{
    auto* this_object = vm.this_value(global_object).to_object(global_object);
    if (!this_object)
        return {};
    if (!this_object->is_function()) {
        vm.throw_exception<TypeError>(global_object, ErrorType::NotA, "Function");
        return {};
    }
    auto& function = static_cast<FunctionObject&>(*this_object);
    auto this_arg = vm.argument(0);
    auto arg_array = vm.argument(1);
    if (arg_array.is_nullish())
        return vm.call(function, this_arg);
    auto arguments = create_list_from_array_like(global_object, arg_array);
    if (vm.exception())
        return {};
    return vm.call(function, this_arg, move(arguments));
}

JS_DEFINE_NATIVE_FUNCTION(FunctionPrototype::bind)
{
    auto* this_object = vm.this_value(global_object).to_object(global_object);
    if (!this_object)
        return {};
    if (!this_object->is_function()) {
        vm.throw_exception<TypeError>(global_object, ErrorType::NotA, "Function");
        return {};
    }
    auto& this_function = static_cast<FunctionObject&>(*this_object);
    auto bound_this_arg = vm.argument(0);

    Vector<Value> arguments;
    if (vm.argument_count() > 1) {
        arguments = vm.running_execution_context().arguments;
        arguments.remove(0);
    }

    return this_function.bind(bound_this_arg, move(arguments));
}

JS_DEFINE_NATIVE_FUNCTION(FunctionPrototype::call)
{
    auto* this_object = vm.this_value(global_object).to_object(global_object);
    if (!this_object)
        return {};
    if (!this_object->is_function()) {
        vm.throw_exception<TypeError>(global_object, ErrorType::NotA, "Function");
        return {};
    }
    auto& function = static_cast<FunctionObject&>(*this_object);
    auto this_arg = vm.argument(0);
    MarkedValueList arguments(vm.heap());
    if (vm.argument_count() > 1) {
        for (size_t i = 1; i < vm.argument_count(); ++i)
            arguments.append(vm.argument(i));
    }
    return vm.call(function, this_arg, move(arguments));
}

JS_DEFINE_NATIVE_FUNCTION(FunctionPrototype::to_string)
{
    auto* this_object = vm.this_value(global_object).to_object(global_object);
    if (!this_object)
        return {};
    if (!this_object->is_function()) {
        vm.throw_exception<TypeError>(global_object, ErrorType::NotA, "Function");
        return {};
    }
    String function_name;
    String function_parameters;
    String function_body;

    if (is<OrdinaryFunctionObject>(this_object)) {
        auto& ordinary_function = static_cast<OrdinaryFunctionObject&>(*this_object);
        StringBuilder parameters_builder;
        auto first = true;
        for (auto& parameter : ordinary_function.parameters()) {

            if (auto* name_ptr = parameter.binding.get_pointer<FlyString>()) {
                if (!first)
                    parameters_builder.append(", ");
                first = false;
                parameters_builder.append(*name_ptr);
                if (parameter.default_value) {

                    parameters_builder.append(" = TODO");
                }
            }
        }
        function_name = ordinary_function.name();
        function_parameters = parameters_builder.build();
        function_body = "  ???";
    } else {
    
        if (is<NativeFunction>(this_object))
            function_name = static_cast<NativeFunction&>(*this_object).name();
        function_body = "  [native code]";
    }

    auto function_source = String::formatted(
        "function {}({}) {{\n{}\n}}",
        function_name.is_null() ? "" : function_name,
        function_parameters.is_null() ? "" : function_parameters,
        function_body);
    return js_string(vm, function_source);
}

JS_DEFINE_NATIVE_FUNCTION(FunctionPrototype::symbol_has_instance)
{
    return ordinary_has_instance(global_object, vm.argument(0), vm.this_value(global_object));
}

}