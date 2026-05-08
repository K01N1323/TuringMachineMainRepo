#include "Compiler.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main() {
    TuringMachine tm("start");
    MemoryManager mem;
    Compiler      compiler(tm, mem);

    const std::string filename = "program.txt";
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "[Error] Failed to open file: " << filename << "\n";
        return 1;
    }

    std::vector<std::string> sourceCode;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty()) {
            sourceCode.push_back(line);
        }
    }

    file.close();

    std::cout << "[Compiler] Source loaded. Compiling...\n";
    compiler.Compile(sourceCode);
    mem.Deploy(tm);

    std::cout << "[Processor] Executing program:\n";
    std::cout << "------------------------------------\n";

    compiler.Execute();

    std::cout << "------------------------------------\n";
    std::cout << "[Processor] Program finished.\n";

    return 0;
}