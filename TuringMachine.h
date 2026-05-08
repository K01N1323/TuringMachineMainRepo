#ifndef TURING_MACHINE_H
#define TURING_MACHINE_H

#include <iostream>
#include <map>
#include <string>

// Направление движения каретки по ленте.
enum class Direction { Left = -1, Stay = 0, Right = 1 };

// Реакция машины: следующее состояние, записываемый символ, сдвиг.
struct Action {
  std::string nextState;
  char writeSymbol;
  Direction move;
};

class TuringMachine {
public:
  explicit TuringMachine(const std::string &startState = "q0")
      : head_(0), currentState_(startState) {}

  // Добавление правила перехода.
  void AddRule(const std::string &state, char readSym,
               const std::string &nextState, char writeSym, Direction dir) {
    rules_[{state, readSym}] = {nextState, writeSym, dir};
  }

  // Запись символа на ленту по позиции.
  void SetTapeContent(int position, char symbol) { tape_[position] = symbol; }

  // Доступ к ленте только для чтения.
  const std::map<int, char> &GetTape() const { return tape_; }

  // Один такт работы машины. Возвращает false при остановке.
  bool Step() {
    if (currentState_ == "halt") {
      return false;
    }

    char currentSymbol = ReadTape();
    auto key = std::make_pair(currentState_, currentSymbol);
    auto it = rules_.find(key);

    if (it == rules_.end()) {
      std::cerr << "Runtime error: no rule for state '" << currentState_
                << "' and symbol '" << currentSymbol << "'\n";
      return false;
    }

    Action action = it->second;

    tape_[head_] = action.writeSymbol;
    head_ += static_cast<int>(action.move);
    currentState_ = action.nextState;

    return true;
  }

  // Текущее состояние автомата.
  std::string GetCurrentState() const { return currentState_; }

  // Запуск до остановки.
  void Run() {
    while (Step()) {
    }
  }

  // Вывод ленты в консоль (отладка).
  void PrintTape() {
    std::cout << "State: " << currentState_ << " | Head: " << head_
              << "\nTape: ";

    for (const auto &[pos, symbol] : tape_) {
      std::cout << "[" << pos << "]=" << symbol << " ";
    }

    std::cout << "\n\n";
  }

private:
  // Чтение символа под кареткой. Пустая ячейка => '_'.
  char ReadTape() {
    if (tape_.find(head_) == tape_.end()) {
      return '_';
    }
    return tape_[head_];
  }

  std::map<int, char> tape_;
  int head_;
  std::string currentState_;
  std::map<std::pair<std::string, char>, Action> rules_;
};

#endif // TURING_MACHINE_H