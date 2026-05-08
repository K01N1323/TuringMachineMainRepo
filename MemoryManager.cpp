#include "MemoryManager.h"

#include <stdexcept>

void MemoryManager::Allocate(
    const std::string & name,
    int                 initialValue,
    size_t              padding
) {
    if (symbolTable_.count(name)) {
        throw std::runtime_error(
            "MemoryManager: redefinition of variable '" + name + "'"
        );
    }

    if (nextAddress_ > 'z') {
        throw std::runtime_error(
            "MemoryManager: address space exhausted ('a'..'z')"
        );
    }

    char address = nextAddress_++;
    symbolTable_[name] = address;
    variables_.push_back({name, address, initialValue, padding});
}

char MemoryManager::GetAddress(const std::string & name) const {
    auto it = symbolTable_.find(name);

    if (it == symbolTable_.end()) {
        throw std::runtime_error(
            "MemoryManager: undefined variable '" + name + "'"
        );
    }

    return it->second;
}

void MemoryManager::Deploy(TuringMachine & tm) const {
    // Маркер начала ленты.
    tm.SetTapeContent(0, '^');

    int head = 1;

    for (const auto & var : variables_) {
        // Заголовок переменной: #<address>:
        tm.SetTapeContent(head++, '#');
        tm.SetTapeContent(head++, var.address);
        tm.SetTapeContent(head++, ':');

        // Унарное значение.
        for (int i = 0; i < var.initialValue; ++i) {
            tm.SetTapeContent(head++, '1');
        }

        // Закрывающий маркер.
        tm.SetTapeContent(head++, '#');

        // Защитный интервал для роста переменной.
        for (size_t i = 0; i < var.padding; ++i) {
            tm.SetTapeContent(head++, '_');
        }
    }
}

int MemoryManager::GetDecimalValue(
    const TuringMachine & tm,
    const std::string &   name
) const {
    char address       = GetAddress(name);
    const auto & tape  = tm.GetTape();

    bool inBlock = false;
    int  value   = 0;

    for (const auto & [pos, symbol] : tape) {
        if (symbol == address) {
            inBlock = true;
            continue;
        }

        if (inBlock) {
            if (symbol == ':') {
                continue;
            }

            if (symbol == '1') {
                ++value;
            }

            if (symbol == '#') {
                break;
            }
        }
    }

    return value;
}