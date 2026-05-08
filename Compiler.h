#ifndef COMPILER_H
#define COMPILER_H

#include "MacrosForTuring.h"
#include "MemoryManager.h"
#include "TuringMachine.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// Контекст блока управляющей конструкции (while/for/if/else).
struct BlockScope {
    enum Type { WHILE, FOR, IF, ELSE } type;

    std::string                startState;
    std::string                endState;
    std::string                nextBlockState;
    std::vector<std::string>   stepTokens;
};

// Хук для перехвата print-вызовов во время исполнения.
struct PrintHook {
    std::string displayName;
    std::string internalName;
    std::string type;
};

// Компилятор высокоуровневого кода в правила машины Тьюринга.
class Compiler {
public:
    Compiler(TuringMachine & tm, MemoryManager & mem)
        : tm_(tm), mem_(mem)
    {}

    // Компиляция исходного кода в правила МТ.
    void Compile(const std::vector<std::string> & sourceCode) {
        AllocateBuiltins();
        DeclareVariables(sourceCode);
        CompileBody(sourceCode);
    }

    // Пошаговое исполнение с перехватом print.
    void Execute() {
        while (tm_.GetCurrentState() != "halt") {
            std::string curr = tm_.GetCurrentState();

            if (printHooks_.count(curr)) {
                HandlePrint(printHooks_[curr]);
            }

            if (!tm_.Step()) {
                std::cerr << "[Hardware Fault] Unexpected processor halt.\n";
                break;
            }
        }
    }

private:
    TuringMachine &                                    tm_;
    MemoryManager &                                    mem_;
    int                                                stateCounter_ = 0;
    std::vector<BlockScope>                            scopes_;
    std::unordered_map<std::string, PrintHook>         printHooks_;
    std::unordered_map<std::string, std::string>       varTypes_;

    std::string NextState() {
        return "q_auto_" + std::to_string(++stateCounter_);
    }


    // ---- Лексический анализ ----

    std::vector<std::string> Tokenize(const std::string & line) {
        std::vector<std::string> tokens;
        std::string token;
        bool inStr = false;

        for (size_t i = 0; i < line.length(); ++i) {
            if (line[i] == '"' || line[i] == '\'') {
                token += line[i];

                if (inStr) {
                    tokens.push_back(token);
                    token.clear();
                    inStr = false;
                }
                else {
                    inStr = true;
                }
            }
            else if (inStr) {
                token += line[i];
            }
            else if (std::isspace(line[i])) {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
            }
            else if (IsTwoCharOperator(line, i)) {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
                tokens.push_back(line.substr(i, 2));
                ++i;
            }
            else if (std::string("=+-*/%(){};><").find(line[i]) != std::string::npos) {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
                tokens.push_back(std::string(1, line[i]));
            }
            else {
                token += line[i];
            }
        }

        if (!token.empty()) {
            tokens.push_back(token);
        }

        return tokens;
    }

    bool IsTwoCharOperator(const std::string & line, size_t i) {
        if (i + 1 >= line.length()) {
            return false;
        }

        std::string op = line.substr(i, 2);

        return op == "++" || op == "--" || op == "+=" || op == "-=" ||
               op == "*=" || op == "/=" || op == "%=" || op == "==" ||
               op == "!=" || op == "<=" || op == ">=";
    }


    // ---- Вспомогательные функции парсинга ----

    bool IsNumber(const std::string & s) {
        if (s.empty()) {
            return false;
        }

        bool hasDot = false;

        for (char c : s) {
            if (c == '.') {
                if (hasDot) {
                    return false;
                }
                hasDot = true;
            }
            else if (!isdigit(c)) {
                return false;
            }
        }

        return true;
    }

    bool IsCharLiteral(const std::string & s) {
        return s.length() == 3 && s.front() == '\'' && s.back() == '\'';
    }

    int ParseNumStr(const std::string & s) {
        if (s.find('.') == std::string::npos) {
            return std::stoi(s);
        }

        size_t dot = s.find('.');
        int ip     = std::stoi(s.substr(0, dot));
        std::string fp = s.substr(dot + 1);

        while (fp.length() < 2) {
            fp += "0";
        }

        return ip * 100 + std::stoi(fp.substr(0, 2));
    }

    std::string GetConstantName(const std::string & token) {
        std::string clean = token;
        std::replace(clean.begin(), clean.end(), '.', '_');
        return "_c" + clean;
    }

    char GetAddressFor(const std::string & token) {
        if (IsNumber(token)) {
            return mem_.GetAddress(GetConstantName(token));
        }

        if (IsCharLiteral(token)) {
            return mem_.GetAddress("_c" + std::to_string(static_cast<int>(token[1])));
        }

        return mem_.GetAddress(token);
    }


    // ---- Аллокация служебных переменных ----

    void AllocateBuiltins() {
        TryAllocate("_temp", 0, 10000);
        TryAllocate("_rem",  0, 10000);
        TryAllocate("_rem2", 0, 10000);
        TryAllocate("_c0",   0, 0);
        TryAllocate("_c100", 100, 0);
    }

    void TryAllocate(const std::string & name, int value, size_t pad) {
        try {
            mem_.Allocate(name, value, pad);
        }
        catch (...) {
        }
    }


    // ---- Объявление переменных (первый проход) ----

    void DeclareVariables(const std::vector<std::string> & sourceCode) {
        for (const auto & line : sourceCode) {
            auto tokens = Tokenize(line);

            for (size_t i = 0; i < tokens.size(); ++i) {
                DeclareToken(tokens, i);
            }
        }
    }

    void DeclareToken(const std::vector<std::string> & tokens, size_t i) {
        const std::string & t = tokens[i];

        if (t == "float") {
            varTypes_[tokens[i + 1]] = "float";

            if (i + 1 < tokens.size()) {
                TryAllocate(tokens[i + 1], 0, 10000);
            }
        }
        else if (t == "int_complex") {
            varTypes_[tokens[i + 1]] = "int_complex";

            if (i + 1 < tokens.size()) {
                TryAllocate(tokens[i + 1] + "_r", 0, 10000);
                TryAllocate(tokens[i + 1] + "_i", 0, 10000);
            }
        }
        else if (t == "char") {
            varTypes_[tokens[i + 1]] = "char";

            if (i + 1 < tokens.size()) {
                TryAllocate(tokens[i + 1], 0, 255);
            }
        }
        else if (t == "int" || t == "byte" || t == "bool") {
            varTypes_[tokens[i + 1]] = t;
            int pad = (t == "bool") ? 1 : (t == "byte" ? 255 : 10000);

            if (i + 1 < tokens.size()) {
                TryAllocate(tokens[i + 1], 0, pad);
            }
        }
        else if (IsCharLiteral(t)) {
            int ascii = static_cast<int>(t[1]);
            TryAllocate("_c" + std::to_string(ascii), ascii, 0);
        }
        else if (IsNumber(t)) {
            TryAllocate(GetConstantName(t), ParseNumStr(t), 0);
        }
    }


    // ---- Компиляция тела программы (второй проход) ----

    void CompileBody(const std::vector<std::string> & sourceCode) {
        std::string currentState = "start";

        for (const auto & line : sourceCode) {
            auto tokens = Tokenize(line);

            if (tokens.empty()) {
                continue;
            }

            StripTypeKeywords(tokens);

            if (tokens.empty()) {
                continue;
            }

            currentState = CompileLine(tokens, currentState);
        }

        tm_.AddRule(currentState, '^', "halt", '^', Direction::Stay);
    }

    void StripTypeKeywords(std::vector<std::string> & tokens) {
        auto it = std::remove_if(
            tokens.begin(), tokens.end(),
            [](const std::string & t) {
                return t == "bool" || t == "byte" || t == "int" ||
                       t == "float" || t == "int_complex" || t == "char";
            }
        );

        tokens.erase(it, tokens.end());
    }

    std::string CompileLine(
        const std::vector<std::string> & tokens,
        const std::string &              currentState
    ) {
        if (tokens[0] == "while") {
            return CompileWhile(tokens, currentState);
        }

        if (tokens[0] == "for") {
            return CompileFor(tokens, currentState);
        }

        if (tokens[0] == "if") {
            return CompileIf(tokens, currentState);
        }

        if (tokens[0] == "}" && tokens.size() > 1 && tokens[1] == "else") {
            return CompileElse(tokens, currentState);
        }

        if (tokens[0] == "}") {
            return CompileClose(currentState);
        }

        std::string next = NextState();
        CompileStatement(tokens, currentState, next);
        return next;
    }


    // ---- Управляющие конструкции ----

    std::string CompileWhile(
        const std::vector<std::string> & tokens,
        const std::string &              currentState
    ) {
        std::string cond = NextState() + "_c";
        std::string body = NextState() + "_b";
        std::string end  = NextState() + "_e";

        tm_.AddRule(currentState, '^', cond, '^', Direction::Stay);

        GenerateCompare(
            tm_, cond,
            GetAddressFor(tokens[2]), GetAddressFor("0"),
            body, end, end
        );

        scopes_.push_back({BlockScope::WHILE, cond, end, "", {}});

        return body;
    }

    std::string CompileFor(
        const std::vector<std::string> & tokens,
        const std::string &              currentState
    ) {
        auto it1 = std::find(tokens.begin(), tokens.end(), ";");
        auto it2 = std::find(it1 + 1, tokens.end(), ";");
        auto it3 = std::find(it2 + 1, tokens.end(), ")");

        if (it1 == tokens.end() || it2 == tokens.end() || it3 == tokens.end()) {
            return currentState;
        }

        std::vector<std::string> initTokens(tokens.begin() + 2, it1);
        std::vector<std::string> condTokens(it1 + 1, it2);
        std::vector<std::string> stepTokens(it2 + 1, it3);

        std::string afterInit = NextState() + "_for_init";
        CompileStatement(initTokens, currentState, afterInit);

        std::string state = afterInit;
        std::string cond  = NextState() + "_for_cond";
        std::string body  = NextState() + "_for_body";
        std::string end   = NextState() + "_for_end";

        tm_.AddRule(state, '^', cond, '^', Direction::Stay);

        const std::string & leftArg  = condTokens[0];
        const std::string & op       = condTokens[1];
        const std::string & rightArg = condTokens[2];

        if (op == "<") {
            GenerateCompare(
                tm_, cond,
                GetAddressFor(leftArg), GetAddressFor(rightArg),
                end, body, end
            );
        }
        else if (op == ">") {
            GenerateCompare(
                tm_, cond,
                GetAddressFor(leftArg), GetAddressFor(rightArg),
                body, end, end
            );
        }

        scopes_.push_back({BlockScope::FOR, cond, end, "", stepTokens});

        return body;
    }

    std::string CompileIf(
        const std::vector<std::string> & tokens,
        const std::string &              currentState
    ) {
        std::string cond      = NextState() + "_if_c";
        std::string body      = NextState() + "_if_b";
        std::string nextBlock = NextState() + "_if_next";
        std::string endChain  = NextState() + "_if_end";

        tm_.AddRule(currentState, '^', cond, '^', Direction::Stay);

        GenerateCompare(
            tm_, cond,
            GetAddressFor(tokens[2]), GetAddressFor("0"),
            body, nextBlock, nextBlock
        );

        scopes_.push_back({BlockScope::IF, cond, endChain, nextBlock, {}});

        return body;
    }

    std::string CompileElse(
        const std::vector<std::string> & tokens,
        const std::string &              currentState
    ) {
        BlockScope scope = scopes_.back();
        scopes_.pop_back();

        tm_.AddRule(currentState, '^', scope.endState, '^', Direction::Stay);

        if (tokens.size() > 2 && tokens[2] == "if") {
            std::string body      = NextState() + "_elif_b";
            std::string nextBlock = NextState() + "_elif_next";

            GenerateCompare(
                tm_, scope.nextBlockState,
                GetAddressFor(tokens[4]), GetAddressFor("0"),
                body, nextBlock, nextBlock
            );

            scopes_.push_back({
                BlockScope::IF,
                scope.nextBlockState,
                scope.endState,
                nextBlock,
                {}
            });

            return body;
        }

        scopes_.push_back({
            BlockScope::ELSE,
            scope.nextBlockState,
            scope.endState,
            "",
            {}
        });

        return scope.nextBlockState;
    }

    std::string CompileClose(const std::string & currentState) {
        if (scopes_.empty()) {
            return currentState;
        }

        BlockScope scope = scopes_.back();
        scopes_.pop_back();

        if (scope.type == BlockScope::WHILE) {
            tm_.AddRule(currentState, '^', scope.startState, '^', Direction::Stay);
            return scope.endState;
        }

        if (scope.type == BlockScope::FOR) {
            std::string stepState = NextState() + "_for_step";
            CompileStatement(scope.stepTokens, currentState, stepState);
            tm_.AddRule(stepState, '^', scope.startState, '^', Direction::Stay);
            return scope.endState;
        }

        // IF или ELSE.
        tm_.AddRule(currentState, '^', scope.endState, '^', Direction::Stay);

        if (scope.type == BlockScope::IF) {
            tm_.AddRule(scope.nextBlockState, '^', scope.endState, '^', Direction::Stay);
        }

        return scope.endState;
    }


    // ---- Компиляция одного оператора ----

    void CompileStatement(
        const std::vector<std::string> & tokens,
        const std::string &              currentState,
        const std::string &              nextState
    ) {
        // Операции над комплексными числами.
        if (varTypes_.count(tokens[0]) && varTypes_[tokens[0]] == "int_complex") {
            CompileComplexStatement(tokens, currentState, nextState);
            return;
        }

        if (tokens[0] == "print" && tokens.size() >= 2) {
            CompilePrint(tokens, currentState, nextState);
        }
        else if (tokens.size() == 2 && tokens[1] == "++") {
            GenerateIncrement(tm_, currentState, GetAddressFor(tokens[0]), nextState);
        }
        else if (tokens.size() == 2 && tokens[1] == "--") {
            GenerateDecrement(tm_, currentState, GetAddressFor(tokens[0]), nextState);
        }
        else if (tokens.size() == 3 && IsCompoundAssignment(tokens[1])) {
            std::string dest = tokens[0];
            std::string op   = tokens[1].substr(0, 1);
            CompileStatement({dest, "=", dest, op, tokens[2]}, currentState, nextState);
        }
        else if (tokens.size() == 3 && tokens[1] == "=") {
            CompileSimpleAssign(tokens, currentState, nextState);
        }
        else if (tokens.size() == 5 && tokens[1] == "=") {
            CompileBinaryExpr(tokens, currentState, nextState);
        }
        else {
            tm_.AddRule(currentState, '^', nextState, '^', Direction::Stay);
        }
    }

    bool IsCompoundAssignment(const std::string & op) {
        return op == "+=" || op == "-=" || op == "*=" ||
               op == "/=" || op == "%=";
    }

    void CompileComplexStatement(
        const std::vector<std::string> & tokens,
        const std::string &              currentState,
        const std::string &              nextState
    ) {
        std::string dest = tokens[0];

        if (tokens.size() == 3 && tokens[1] == "=") {
            std::string mid = NextState();
            CompileStatement({dest + "_r", "=", tokens[2] + "_r"}, currentState, mid);
            CompileStatement({dest + "_i", "=", tokens[2] + "_i"}, mid, nextState);
        }
        else if (tokens.size() == 5 && tokens[1] == "=" &&
                 (tokens[3] == "+" || tokens[3] == "-")) {
            std::string mid = NextState();

            CompileStatement(
                {dest + "_r", "=", tokens[2] + "_r", tokens[3], tokens[4] + "_r"},
                currentState, mid
            );

            CompileStatement(
                {dest + "_i", "=", tokens[2] + "_i", tokens[3], tokens[4] + "_i"},
                mid, nextState
            );
        }
    }

    void CompilePrint(
        const std::vector<std::string> & tokens,
        const std::string &              currentState,
        const std::string &              nextState
    ) {
        std::string printArg   = tokens[1];
        std::string printState = NextState() + "_print";
        std::string internalName;
        std::string type = "int";

        if (printArg.front() == '"') {
            type         = "string_literal";
            internalName = printArg;
        }
        else if (printArg.front() == '\'') {
            type         = "char_literal";
            internalName = printArg;
        }
        else {
            internalName = IsNumber(printArg) ? GetConstantName(printArg) : printArg;
            type         = varTypes_.count(printArg) ? varTypes_[printArg] : "int";
        }

        printHooks_[printState] = {printArg, internalName, type};

        tm_.AddRule(currentState, '^', printState, '^', Direction::Stay);
        tm_.AddRule(printState,   '^', nextState,  '^', Direction::Stay);
    }

    void CompileSimpleAssign(
        const std::vector<std::string> & tokens,
        const std::string &              currentState,
        const std::string &              nextState
    ) {
        if (tokens[0] != tokens[2]) {
            GenerateAssign(
                tm_, currentState,
                GetAddressFor(tokens[0]), GetAddressFor(tokens[2]),
                nextState
            );
        }
        else {
            tm_.AddRule(currentState, '^', nextState, '^', Direction::Stay);
        }
    }

    void CompileBinaryExpr(
        const std::vector<std::string> & tokens,
        const std::string &              currentState,
        const std::string &              nextState
    ) {
        std::string dest = tokens[0];
        std::string arg1 = tokens[2];
        std::string op   = tokens[3];
        std::string arg2 = tokens[4];

        if (op == "+") {
            CompileAdd(dest, arg1, arg2, currentState, nextState);
        }
        else if (op == "-") {
            CompileSub(dest, arg1, arg2, currentState, nextState);
        }
        else if (op == "*") {
            CompileMul(dest, arg1, arg2, currentState, nextState);
        }
        else if (op == "/") {
            CompileDiv(dest, arg1, arg2, currentState, nextState);
        }
        else if (op == "%") {
            CompileMod(dest, arg1, arg2, currentState, nextState);
        }
    }

    void CompileAdd(
        const std::string & dest,
        const std::string & arg1,
        const std::string & arg2,
        const std::string & cur,
        const std::string & next
    ) {
        if (dest == arg1) {
            GenerateAdd(tm_, cur, GetAddressFor(dest), GetAddressFor(arg2), next);
        }
        else if (dest == arg2) {
            GenerateAdd(tm_, cur, GetAddressFor(dest), GetAddressFor(arg1), next);
        }
        else {
            std::string mid = cur + "_m";
            GenerateAssign(tm_, cur, GetAddressFor(dest), GetAddressFor(arg1), mid);
            GenerateAdd(tm_, mid, GetAddressFor(dest), GetAddressFor(arg2), next);
        }
    }

    void CompileSub(
        const std::string & dest,
        const std::string & arg1,
        const std::string & arg2,
        const std::string & cur,
        const std::string & next
    ) {
        if (dest == arg1) {
            GenerateSubtract(tm_, cur, GetAddressFor(dest), GetAddressFor(arg2), next);
        }
        else {
            std::string mid = cur + "_m";
            GenerateAssign(tm_, cur, GetAddressFor(dest), GetAddressFor(arg1), mid);
            GenerateSubtract(tm_, mid, GetAddressFor(dest), GetAddressFor(arg2), next);
        }
    }

    void CompileMul(
        const std::string & dest,
        const std::string & arg1,
        const std::string & arg2,
        const std::string & cur,
        const std::string & next
    ) {
        std::string t1 = cur + "_t1";
        std::string t2 = cur + "_t2";

        GenerateClear(tm_, cur, mem_.GetAddress("_temp"), t1);

        GenerateMultiply(
            tm_, t1,
            mem_.GetAddress("_temp"), GetAddressFor(arg1), GetAddressFor(arg2),
            t2
        );

        GenerateAssign(tm_, t2, GetAddressFor(dest), mem_.GetAddress("_temp"), next);
    }

    void CompileDiv(
        const std::string & dest,
        const std::string & arg1,
        const std::string & arg2,
        const std::string & cur,
        const std::string & next
    ) {
        std::string destType = varTypes_.count(dest) ? varTypes_[dest] : "int";
        std::string arg1Type = ResolveType(arg1);
        std::string arg2Type = ResolveType(arg2);

        std::string t_q = cur + "_q";
        char zeroAddr    = GetAddressFor("0");

        // int / int → float: домножаем делимое на 100.
        if (destType == "float" && arg1Type != "float" && arg2Type != "float") {
            std::string t_mul = cur + "_fmul";

            GenerateClear(tm_, cur, mem_.GetAddress("_temp"), t_mul + "_1");

            GenerateMultiply(
                tm_, t_mul + "_1",
                mem_.GetAddress("_temp"), GetAddressFor(arg1), GetAddressFor("100"),
                t_mul + "_2"
            );

            GenerateDivMod(
                tm_, t_mul + "_2",
                mem_.GetAddress("_temp"), GetAddressFor(arg2),
                mem_.GetAddress("_rem"), mem_.GetAddress("_rem2"),
                zeroAddr, t_q
            );

            GenerateAssign(tm_, t_q, GetAddressFor(dest), mem_.GetAddress("_rem"), next);
        }
        else {
            GenerateDivMod(
                tm_, cur,
                GetAddressFor(arg1), GetAddressFor(arg2),
                mem_.GetAddress("_temp"), mem_.GetAddress("_rem"),
                zeroAddr, t_q
            );

            GenerateAssign(tm_, t_q, GetAddressFor(dest), mem_.GetAddress("_temp"), next);
        }
    }

    void CompileMod(
        const std::string & dest,
        const std::string & arg1,
        const std::string & arg2,
        const std::string & cur,
        const std::string & next
    ) {
        std::string t_r = cur + "_r";
        char zeroAddr    = GetAddressFor("0");

        GenerateDivMod(
            tm_, cur,
            GetAddressFor(arg1), GetAddressFor(arg2),
            mem_.GetAddress("_temp"), mem_.GetAddress("_rem"),
            zeroAddr, t_r
        );

        GenerateAssign(tm_, t_r, GetAddressFor(dest), mem_.GetAddress("_rem"), next);
    }

    std::string ResolveType(const std::string & name) {
        if (varTypes_.count(name)) {
            return varTypes_[name];
        }

        if (IsNumber(name) && name.find('.') != std::string::npos) {
            return "float";
        }

        return "int";
    }


    // ---- Вывод результатов ----

    void HandlePrint(const PrintHook & hook) {
        if (hook.type == "string_literal") {
            std::cout << hook.internalName.substr(1, hook.internalName.length() - 2)
                      << "\n";
        }
        else if (hook.type == "char" || hook.type == "char_literal") {
            int val = mem_.GetDecimalValue(tm_, hook.internalName);
            std::cout << ">> " << hook.displayName
                      << " = '" << static_cast<char>(val) << "'\n";
        }
        else if (hook.type == "int_complex") {
            int r = mem_.GetDecimalValue(tm_, hook.displayName + "_r");
            int i = mem_.GetDecimalValue(tm_, hook.displayName + "_i");
            std::cout << ">> " << hook.displayName
                      << " = " << r << " + " << i << "i\n";
        }
        else {
            int val = mem_.GetDecimalValue(tm_, hook.internalName);

            if (hook.type == "float") {
                std::cout << ">> " << hook.displayName
                          << " = " << (val / 100) << "."
                          << (val % 100 < 10 ? "0" : "") << (val % 100) << "\n";
            }
            else {
                std::cout << ">> " << hook.displayName
                          << " = " << val << "\n";
            }
        }
    }
};

#endif  // COMPILER_H