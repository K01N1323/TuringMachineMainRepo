#include "MemoryManager.h"

#include <stdexcept>
#include <cmath>

std::string MemoryManager::IntToBinary33(int value) {
  std::string bin;
  if (value >= 0) {
    bin += '+';
  } else {
    bin += '-';
    // Для прямого кода берем модуль. Для дополнительного - по-другому.
    // Задание гласит: "Прямой код со знаком"
    value = -value; 
  }

  // 32 бита
  for (int i = 31; i >= 0; --i) {
    if ((value >> i) & 1) {
      bin += '1';
    } else {
      bin += '0';
    }
  }
  return bin;
}

std::vector<std::string> MemoryManager::GetVariableNames() const {
  std::vector<std::string> names;
  for (const auto& var : variables_) {
    names.push_back(var.name);
  }
  return names;
}

void MemoryManager::Allocate(const std::string &name, int initialValue) {
  if (symbolTable_.count(name)) {
    // В компиляторе мы можем позволить переопределение (в реальности лучше не надо, но для temp переменных окей).
    return;
  }

  symbolTable_[name] = nextHead_;
  variables_.push_back({name, nextHead_, initialValue});
  
  // Каждая переменная: '#' (1 символ) + Знак (1) + Биты (32) = 34 ячейки (до следующего '#')
  nextHead_ += 34; 
}

void MemoryManager::Deploy(TuringMachine &tm) const {
  // Маркер начала ленты 0.
  tm.SetTapeContent(0, 0, '^');

  for (const auto &var : variables_) {
    int head = var.startIndex;
    tm.SetTapeContent(0, head++, '#');
    
    std::string bin = IntToBinary33(var.initialValue);
    for (char c : bin) {
        tm.SetTapeContent(0, head++, c);
    }
  }
  // Финальный '#'
  tm.SetTapeContent(0, nextHead_, '#');
}

int MemoryManager::GetIndex(const std::string &name) const {
  auto it = symbolTable_.find(name);
  if (it == symbolTable_.end()) {
    throw std::runtime_error("MemoryManager: undefined variable '" + name + "'");
  }
  return it->second + 1; // + 1 to point to the sign character instead of '#'
}

int MemoryManager::GetDecimalValue(const TuringMachine &tm,
                                   const std::string &name) const {
  auto it = symbolTable_.find(name);
  if (it == symbolTable_.end()) {
    throw std::runtime_error("MemoryManager: undefined variable '" + name + "'");
  }

  int startIndex = it->second;
  const auto &tape = tm.GetTape(0);
  
  //startIndex - это '#'. Следующий символ - знак.
  int head = startIndex + 1;
  char signChar = tape.find(head) != tape.end() ? tape.at(head) : '+';
  int sign = (signChar == '-') ? -1 : 1;
  head++;
  
  long long value = 0; // используем long long, чтобы не переполнилось при сдвигах
  for (int i = 0; i < 32; ++i) {
      char bitChar = tape.find(head) != tape.end() ? tape.at(head) : '0';
      if (bitChar == '1') {
          value = (value << 1) | 1;
      } else {
          value = (value << 1);
      }
      head++;
  }

  return static_cast<int>(value * sign);
}