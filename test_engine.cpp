#include "Compiler.h"
#include "MemoryManager.h"
#include "TuringMachine.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main() {
    TuringMachine tm("start");
    MemoryManager mem;
    Compiler compiler(tm, mem);
    std::vector<std::string> code = {"var x = 5", "var y = -3", "var z = 2*x + 4*y - 10"};
    compiler.Compile(code);
    mem.Deploy(tm);
    tm.Run();
    std::cout << "Result z: " << mem.GetDecimalValue(tm, "z") << std::endl;
    return 0;
}
