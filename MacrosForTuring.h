#ifndef MACROS_FOR_TURING_H
#define MACROS_FOR_TURING_H

#include "TuringMachine.h"
#include <string>

// Вспомогательные функции для генерации состояний
inline void MoveHead(TuringMachine& tm, const std::string& state, const std::string& nextState,
                     Direction d0, Direction d1, Direction d2) {
    tm.AddRule(state, '?', '?', '?', nextState, '?', '?', '?', d0, d1, d2);
}

// Позиционирование каретки на ленте 0 (память)
inline void GenerateMoveTape0To(TuringMachine& tm, const std::string& startState, int currentPos, int targetPos, const std::string& nextState) {
    std::string curr = startState;
    if (currentPos < targetPos) {
        for (int i = currentPos; i < targetPos; ++i) {
            std::string nxt = (i == targetPos - 1) ? nextState : curr + "_R";
            MoveHead(tm, curr, nxt, Direction::Right, Direction::Stay, Direction::Stay);
            curr = nxt;
        }
    } else if (currentPos > targetPos) {
        for (int i = currentPos; i > targetPos; --i) {
            std::string nxt = (i == targetPos + 1) ? nextState : curr + "_L";
            MoveHead(tm, curr, nxt, Direction::Left, Direction::Stay, Direction::Stay);
            curr = nxt;
        }
    } else {
        MoveHead(tm, curr, nextState, Direction::Stay, Direction::Stay, Direction::Stay);
    }
}

// Перемотка лент АЛУ к началу (до маркера '^')
inline void GenerateRewindTape(TuringMachine& tm, const std::string& startState, int tapeIndex, const std::string& nextState) {
    std::string loopState = startState + "_rw";
    MoveHead(tm, startState, loopState, Direction::Stay, Direction::Stay, Direction::Stay);

    Direction d0 = (tapeIndex == 0) ? Direction::Left : Direction::Stay;
    Direction d1 = (tapeIndex == 1) ? Direction::Left : Direction::Stay;
    Direction d2 = (tapeIndex == 2) ? Direction::Left : Direction::Stay;

    if (tapeIndex == 1) {
        tm.AddRule(loopState, '?', '0', '?', loopState, '?', '?', '?', d0, d1, d2);
        tm.AddRule(loopState, '?', '1', '?', loopState, '?', '?', '?', d0, d1, d2);
        tm.AddRule(loopState, '?', '_', '?', loopState, '?', '?', '?', d0, d1, d2);
        tm.AddRule(loopState, '?', '^', '?', nextState, '?', '?', '?', Direction::Stay, Direction::Stay, Direction::Stay);
    } else if (tapeIndex == 2) {
        tm.AddRule(loopState, '?', '?', '0', loopState, '?', '?', '?', d0, d1, d2);
        tm.AddRule(loopState, '?', '?', '1', loopState, '?', '?', '?', d0, d1, d2);
        tm.AddRule(loopState, '?', '?', '_', loopState, '?', '?', '?', d0, d1, d2);
        tm.AddRule(loopState, '?', '?', '^', nextState, '?', '?', '?', Direction::Stay, Direction::Stay, Direction::Stay);
    }
}

// Очистка ленты 1 и 2 и установка маркера
inline void GenerateInitALU(TuringMachine& tm, const std::string& startState, const std::string& nextState) {
    tm.AddRule(startState, '?', '?', '?', nextState, '?', '^', '^', Direction::Stay, Direction::Right, Direction::Right);
}

// Копирование из памяти в регистр (Tape 1 или Tape 2)
// Конвертирует '+' -> '0', '-' -> '1'. Голова Tape0 должна стоять на знаке!
inline void GenerateLoadToALU(TuringMachine& tm, const std::string& startState, int targetTape, const std::string& nextState) {
    std::string readSign = startState + "_sign";
    std::string readBits = startState + "_bits";

    MoveHead(tm, startState, readSign, Direction::Stay, Direction::Stay, Direction::Stay);

    if (targetTape == 1) {
        tm.AddRule(readSign, '+', '?', '?', readBits, '?', '0', '?', Direction::Right, Direction::Right, Direction::Stay);
        tm.AddRule(readSign, '-', '?', '?', readBits, '?', '1', '?', Direction::Right, Direction::Right, Direction::Stay);
    } else {
        tm.AddRule(readSign, '+', '?', '?', readBits, '?', '?', '0', Direction::Right, Direction::Stay, Direction::Right);
        tm.AddRule(readSign, '-', '?', '?', readBits, '?', '?', '1', Direction::Right, Direction::Stay, Direction::Right);
    }

    std::string curr = readBits;
    for (int i = 0; i < 32; ++i) {
        std::string nxt = (i == 31) ? nextState : readBits + "_" + std::to_string(i);
        if (targetTape == 1) {
            tm.AddRule(curr, '0', '?', '?', nxt, '?', '0', '?', Direction::Right, Direction::Right, Direction::Stay);
            tm.AddRule(curr, '1', '?', '?', nxt, '?', '1', '?', Direction::Right, Direction::Right, Direction::Stay);
        } else {
            tm.AddRule(curr, '0', '?', '?', nxt, '?', '?', '0', Direction::Right, Direction::Stay, Direction::Right);
            tm.AddRule(curr, '1', '?', '?', nxt, '?', '?', '1', Direction::Right, Direction::Stay, Direction::Right);
        }
        curr = nxt;
    }
}

// Сохранение из регистра (Tape 2) в память
inline void GenerateStoreFromALU(TuringMachine& tm, const std::string& startState, const std::string& nextState) {
    std::string rewind = startState + "_rw";
    std::string moveRight = startState + "_mr";
    std::string readSign = startState + "_sign";
    std::string readBits = startState + "_bits";

    // Возвращаем Tape 2 к началу
    GenerateRewindTape(tm, startState, 2, moveRight);
    
    // Сдвигаем вправо на 1 позицию, чтобы встать на знак
    MoveHead(tm, moveRight, readSign, Direction::Stay, Direction::Stay, Direction::Right);

    MoveHead(tm, readSign, readSign + "_wait", Direction::Stay, Direction::Stay, Direction::Stay);

    tm.AddRule(readSign + "_wait", '?', '?', '0', readBits, '+', '?', '?', Direction::Right, Direction::Stay, Direction::Right);
    tm.AddRule(readSign + "_wait", '?', '?', '1', readBits, '-', '?', '?', Direction::Right, Direction::Stay, Direction::Right);

    std::string curr = readBits;
    for (int i = 0; i < 32; ++i) {
        std::string nxt = (i == 31) ? nextState : readBits + "_" + std::to_string(i);
        tm.AddRule(curr, '?', '?', '0', nxt, '0', '?', '?', Direction::Right, Direction::Stay, Direction::Right);
        tm.AddRule(curr, '?', '?', '1', nxt, '1', '?', '?', Direction::Right, Direction::Stay, Direction::Right);
        curr = nxt;
    }
}

// Прямой код -> Дополнительный код (Two's Complement)
inline void GenerateToTwosComplement(TuringMachine& tm, const std::string& startState, int tapeIndex, const std::string& nextState) {
    std::string checkSign = startState + "_check";
    GenerateRewindTape(tm, startState, tapeIndex, checkSign);
    
    std::string goEnd = startState + "_goEnd";
    std::string invertAdd1 = startState + "_invAdd1_0";
    std::string finish = startState + "_fin";

    Direction d1 = (tapeIndex == 1) ? Direction::Right : Direction::Stay;
    Direction d2 = (tapeIndex == 2) ? Direction::Right : Direction::Stay;
    MoveHead(tm, checkSign, checkSign + "_2", Direction::Stay, d1, d2);

    if (tapeIndex == 1) {
        tm.AddRule(checkSign + "_2", '?', '0', '?', finish, '?', '?', '?', Direction::Stay, Direction::Right, Direction::Stay);
        tm.AddRule(checkSign + "_2", '?', '1', '?', goEnd, '?', '?', '?', Direction::Stay, Direction::Right, Direction::Stay);
    } else {
        tm.AddRule(checkSign + "_2", '?', '?', '0', finish, '?', '?', '?', Direction::Stay, Direction::Stay, Direction::Right);
        tm.AddRule(checkSign + "_2", '?', '?', '1', goEnd, '?', '?', '?', Direction::Stay, Direction::Stay, Direction::Right);
    }
    
    std::string currFin = finish;
    for (int i = 0; i < 32; ++i) {
        std::string nxt = finish + "_" + std::to_string(i);
        MoveHead(tm, currFin, nxt, Direction::Stay, d1, d2);
        currFin = nxt;
    }
    MoveHead(tm, currFin, nextState, Direction::Stay, Direction::Stay, Direction::Stay);

    std::string currGo = goEnd;
    for (int i = 0; i < 32; ++i) {
        std::string nxt = goEnd + "_" + std::to_string(i);
        MoveHead(tm, currGo, nxt, Direction::Stay, d1, d2);
        currGo = nxt;
    }
    
    Direction dl1 = (tapeIndex == 1) ? Direction::Left : Direction::Stay;
    Direction dl2 = (tapeIndex == 2) ? Direction::Left : Direction::Stay;
    MoveHead(tm, currGo, invertAdd1, Direction::Stay, dl1, dl2);

    std::string c1 = invertAdd1;
    std::string c0 = startState + "_invAdd1_1";
    
    for (int i = 0; i < 32; ++i) {
        std::string nxt1 = (i == 31) ? nextState : c1 + "_" + std::to_string(i);
        std::string nxt0 = (i == 31) ? nextState : c0 + "_" + std::to_string(i);
        
        if (tapeIndex == 1) {
            tm.AddRule(c1, '?', '0', '?', nxt1, '?', '0', '?', Direction::Stay, Direction::Left, Direction::Stay);
            tm.AddRule(c1, '?', '1', '?', nxt0, '?', '1', '?', Direction::Stay, Direction::Left, Direction::Stay);
            tm.AddRule(c0, '?', '0', '?', nxt0, '?', '1', '?', Direction::Stay, Direction::Left, Direction::Stay);
            tm.AddRule(c0, '?', '1', '?', nxt0, '?', '0', '?', Direction::Stay, Direction::Left, Direction::Stay);
        } else {
            tm.AddRule(c1, '?', '?', '0', nxt1, '?', '?', '0', Direction::Stay, Direction::Stay, Direction::Left);
            tm.AddRule(c1, '?', '?', '1', nxt0, '?', '?', '1', Direction::Stay, Direction::Stay, Direction::Left);
            tm.AddRule(c0, '?', '?', '0', nxt0, '?', '?', '1', Direction::Stay, Direction::Stay, Direction::Left);
            tm.AddRule(c0, '?', '?', '1', nxt0, '?', '?', '0', Direction::Stay, Direction::Stay, Direction::Left);
        }
        c1 = nxt1;
        c0 = nxt0;
    }
    
    std::string resetEnd = startState + "_resetEnd";
    MoveHead(tm, c1, resetEnd, Direction::Stay, Direction::Stay, Direction::Stay);
    MoveHead(tm, c0, resetEnd, Direction::Stay, Direction::Stay, Direction::Stay);
    
    std::string currR = resetEnd;
    for (int i = 0; i < 33; ++i) {
        std::string nxt = (i == 32) ? nextState : resetEnd + "_" + std::to_string(i);
        MoveHead(tm, currR, nxt, Direction::Stay, d1, d2);
        currR = nxt;
    }
}

// Бинарное сложение Tape 2 = Tape 1 + Tape 2
inline void GenerateBinaryAdd(TuringMachine& tm, const std::string& startState, const std::string& nextState) {
    std::string c0 = startState + "_c0";
    std::string c1 = startState + "_c1";
    
    MoveHead(tm, startState, c0, Direction::Stay, Direction::Left, Direction::Left);
    
    for (int i = 0; i < 33; ++i) {
        std::string nxt0 = (i == 32) ? nextState : c0 + "_" + std::to_string(i);
        std::string nxt1 = (i == 32) ? nextState : c1 + "_" + std::to_string(i);
        
        tm.AddRule(c0, '?', '0', '0', nxt0, '?', '0', '0', Direction::Stay, Direction::Left, Direction::Left);
        tm.AddRule(c0, '?', '0', '1', nxt0, '?', '0', '1', Direction::Stay, Direction::Left, Direction::Left);
        tm.AddRule(c0, '?', '1', '0', nxt0, '?', '1', '1', Direction::Stay, Direction::Left, Direction::Left);
        tm.AddRule(c0, '?', '1', '1', nxt1, '?', '1', '0', Direction::Stay, Direction::Left, Direction::Left);
        
        tm.AddRule(c1, '?', '0', '0', nxt0, '?', '0', '1', Direction::Stay, Direction::Left, Direction::Left);
        tm.AddRule(c1, '?', '0', '1', nxt1, '?', '0', '0', Direction::Stay, Direction::Left, Direction::Left);
        tm.AddRule(c1, '?', '1', '0', nxt1, '?', '1', '0', Direction::Stay, Direction::Left, Direction::Left);
        tm.AddRule(c1, '?', '1', '1', nxt1, '?', '1', '1', Direction::Stay, Direction::Left, Direction::Left);
        
        c0 = nxt0;
        c1 = nxt1;
    }
    
    MoveHead(tm, c0, nextState, Direction::Stay, Direction::Right, Direction::Right);
    MoveHead(tm, c1, nextState, Direction::Stay, Direction::Right, Direction::Right);
}

// Изменение знака Tape 2 (инверсия 0 <-> 1)
inline void GenerateInvertSignTape2(TuringMachine& tm, const std::string& startState, const std::string& nextState) {
    std::string rewind = startState + "_rw";
    GenerateRewindTape(tm, startState, 2, rewind);
    
    std::string doInv = startState + "_do";
    MoveHead(tm, rewind, doInv, Direction::Stay, Direction::Stay, Direction::Right);
    
    std::string goEnd = startState + "_goEnd";
    tm.AddRule(doInv, '?', '?', '0', goEnd, '?', '?', '1', Direction::Stay, Direction::Stay, Direction::Right);
    tm.AddRule(doInv, '?', '?', '1', goEnd, '?', '?', '0', Direction::Stay, Direction::Stay, Direction::Right);
    
    std::string curr = goEnd;
    for(int i = 0; i < 32; ++i) {
        std::string nxt = (i == 31) ? nextState : goEnd + "_" + std::to_string(i);
        MoveHead(tm, curr, nxt, Direction::Stay, Direction::Stay, Direction::Right);
        curr = nxt;
    }
}

// Высокоуровневое сложение
inline void GenerateFullAdd(TuringMachine& tm, const std::string& start, const std::string& next) {
    std::string s1 = start + "_1";
    std::string s2 = start + "_2";
    std::string s3 = start + "_3";
    std::string s4 = start + "_4";
    std::string s5 = start + "_5";
    
    GenerateToTwosComplement(tm, start, 1, s1);
    GenerateToTwosComplement(tm, s1, 2, s2);
    GenerateBinaryAdd(tm, s2, s3);
    
    // Результат в Tape 2, Tape 1 возвращаем к значению
    GenerateRewindTape(tm, s3, 2, s4);
    
    // Теперь конвертируем результат из доп. кода в прямой (алгоритм тот же)
    GenerateToTwosComplement(tm, s4, 2, next);
}

// Высокоуровневое вычитание (Tape 1 - Tape 2 -> Tape 2)
inline void GenerateFullSub(TuringMachine& tm, const std::string& start, const std::string& next) {
    std::string s1 = start + "_1";
    GenerateInvertSignTape2(tm, start, s1);
    GenerateFullAdd(tm, s1, next);
}

#endif // MACROS_FOR_TURING_H