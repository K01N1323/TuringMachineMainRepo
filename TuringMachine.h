#ifndef TURING_MACHINE_H
#define TURING_MACHINE_H

#include <iostream>
#include <map>
#include <string>
#include <array>
#include <vector>
#include <tuple>

// Направление движения каретки по ленте.
enum class Direction { Left = -1, Stay = 0, Right = 1 };

// Реакция машины: следующее состояние, записываемые символы, сдвиги
struct Action {
    std::string nextState;
    std::array<char, 3> writeSymbols;
    std::array<Direction, 3> moves;
};

// Правило перехода (условие)
struct RuleCondition {
    std::string state;
    std::array<char, 3> readSymbols;
    
    // Проверка соответствия с учетом wildcard '?'
    bool Matches(const std::string& currentState, const std::array<char, 3>& currentSymbols) const {
        if (state != currentState) return false;
        for (int i = 0; i < 3; ++i) {
            if (readSymbols[i] != '?' && readSymbols[i] != currentSymbols[i]) {
                return false;
            }
        }
        return true;
    }
};

class TuringMachine {
public:
    explicit TuringMachine(const std::string &startState = "q0")
        : currentState_(startState) {
        head_.fill(0);
    }

    // Добавление правила перехода (для 3 лент).
    void AddRule(const std::string &state, 
                 char read1, char read2, char read3,
                 const std::string &nextState, 
                 char write1, char write2, char write3, 
                 Direction dir1, Direction dir2, Direction dir3) {
        RuleCondition cond = {state, {read1, read2, read3}};
        Action action = {nextState, {write1, write2, write3}, {dir1, dir2, dir3}};
        rules_.push_back({cond, action});
    }

    // Запись символа на ленту по позиции.
    void SetTapeContent(int tapeIndex, int position, char symbol) {
        if (tapeIndex >= 0 && tapeIndex < 3) {
            tapes_[tapeIndex][position] = symbol;
        }
    }

    // Доступ к ленте только для чтения.
    const std::map<int, char>& GetTape(int tapeIndex) const {
        return tapes_[tapeIndex];
    }
    
    // Позиция каретки
    int GetHead(int tapeIndex) const {
        return head_[tapeIndex];
    }

    // Один такт работы машины. Возвращает false при остановке
    bool Step() {
        if (currentState_ == "halt") {
            return false;
        }

        std::array<char, 3> currentSymbols = {ReadTape(0), ReadTape(1), ReadTape(2)};
        
        bool ruleFound = false;
        Action action;
        
        for (const auto& rule : rules_) {
            if (rule.first.Matches(currentState_, currentSymbols)) {
                action = rule.second;
                ruleFound = true;
                break; // Берем первое совпавшее правило
            }
        }

        if (!ruleFound) {
            std::cerr << "Runtime error: no rule for state '" << currentState_
                      << "' and symbols '" << currentSymbols[0] << "', '" 
                      << currentSymbols[1] << "', '" << currentSymbols[2] << "'\n";
            return false;
        }

        static int steps = 0;
        if (++steps % 1000 == 0) {
            std::cout << "Step: " << steps << " State: " << currentState_ << " Heads: " << head_[0] << "," << head_[1] << "," << head_[2] << std::endl;
        }

        for (int i = 0; i < 3; ++i) {
            // Если пишем '?', значит оставляем символ без изменений
            if (action.writeSymbols[i] != '?') {
                tapes_[i][head_[i]] = action.writeSymbols[i];
            }
            head_[i] += static_cast<int>(action.moves[i]);
        }
        
        currentState_ = action.nextState;

        return true;
    }

    // Текущее состояние автомата.
    std::string GetCurrentState() const { return currentState_; }

    // Запуск до остановки (используется только для тестов без GUI).
    void Run() {
        while (Step()) {
        }
    }

private:
    // Чтение символа под кареткой. Пустая ячейка => '_'.
    char ReadTape(int tapeIndex) {
        if (tapes_[tapeIndex].find(head_[tapeIndex]) == tapes_[tapeIndex].end()) {
            return '_';
        }
        return tapes_[tapeIndex][head_[tapeIndex]];
    }

    std::array<std::map<int, char>, 3> tapes_;
    std::array<int, 3> head_;
    std::string currentState_;
    
    // Храним правила в векторе, так как с wildcard '?' поиск по ключу в map сложнее
    std::vector<std::pair<RuleCondition, Action>> rules_;
};

#endif // TURING_MACHINE_H