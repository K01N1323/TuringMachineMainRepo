#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include "TuringMachine.h"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// Описание блока памяти на ленте.
struct Variable {
    std::string  name;            // Имя переменной (например, "counter").
    char         address;         // Однобуквенный адрес на ленте ('a'..'z').
    int          initialValue;    // Начальное значение (количество единиц).
    size_t       padding = 10000; // Зарезервированное пространство для роста.
};

// Менеджер памяти: маппинг имён переменных на физические адреса ленты.
class MemoryManager {
public:
    // Регистрация переменной в памяти.
    void Allocate(
        const std::string & name,
        int                 initialValue = 0,
        size_t              padding      = kDefaultPadding
    );

    // Получение физического адреса по имени.
    char GetAddress(const std::string & name) const;

    // Запись переменных на ленту.
    void Deploy(TuringMachine & tm) const;

    // Декодирование унарного значения с ленты в int.
    int GetDecimalValue(const TuringMachine & tm, const std::string & name) const;

private:
    static constexpr size_t kDefaultPadding = 15;

    std::vector<Variable>                    variables_;
    std::unordered_map<std::string, char>    symbolTable_;
    char                                     nextAddress_ = 'a';
};

#endif  // MEMORY_MANAGER_H
