#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include "TuringMachine.h"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// Описание блока памяти на ленте.
// Переменная занимает 33 ячейки: 1 знак, 32 бита данных.
// Между переменными ставится разделитель '#'.
struct Variable {
  std::string name;       // Имя переменной
  int startIndex;         // Начальный индекс на 1-й ленте (там, где '#')
  int initialValue;       // Начальное десятичное значение
};

// Менеджер памяти: маппинг имён переменных на физические адреса ленты
class MemoryManager {
public:
  void Allocate(const std::string &name, int initialValue = 0);
  void Deploy(TuringMachine &tm) const;
  int GetDecimalValue(const TuringMachine &tm, const std::string &name) const;
  int GetIndex(const std::string &name) const;
  static std::string IntToBinary33(int value);
  std::vector<std::string> GetVariableNames() const;

private:
  std::vector<Variable> variables_;
  std::unordered_map<std::string, int> symbolTable_;
  int nextHead_ = 1; // Индекс на ленте 0 (0 ячейка - маркер начала ленты '^')
};

#endif // MEMORY_MANAGER_H
