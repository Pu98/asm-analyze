#include "etc/include.h"

std::unordered_set<std::string> forbidden = {
    "CON", "PRN", "AUX", "NUL",
    "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
    "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
};
std::unordered_set<std::string> supportedExtensions = { // no .lst
    "asm", "s", "hla", "inc", "palx", "mid"
};

const std::string VERSION = "0.1.0"; // it's better to store this as a string, rather than a double
std::string filename;

int main() {
    // init
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hConsole, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, dwMode);
#endif
    std::cout << "Enter assembly file/directory (e.g. file.asm or /path/to/file.asm): ";
    std::getline(std::cin, filename);
    if (filename.empty() || filename.find_first_not_of(" \t\r\f\v") == std::string::npos /*broad, but okay...*/)
        dbg::Misc::fexit("You can't do that :3");

    std::string filename2 = filename; // store the old filename (it'll change) a line below
    std::transform(filename.begin(), filename.end(), filename.begin(), ::toupper);

    size_t pos = 0;
    while ((pos = filename.find('/')) != std::string::npos) {
        std::string part = filename.substr(0, pos);
        if (forbidden.contains(part))
            dbg::Misc::fexit("You can't do that :3");
        filename.erase(0, pos + 1);
    }

    // remove the file extension & check whether file type is supported or not
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string extension = filename.substr(dotPos + 1);
        for (char& c : extension) c = tolower(c);
        if (!supportedExtensions.count(extension))
            dbg::Misc::fexit("You can't do that :3");
        filename.erase(dotPos);
    }

    std::string filename3 = filename;

    // check the remaining part of the file name
    if (forbidden.contains(filename))
        dbg::Misc::fexit("You can't do that :3");

    filename = filename2;
    auto begin = std::chrono::high_resolution_clock::now();

    std::ifstream originalFile(filename);
    if (!originalFile)
        dbg::Misc::fexit("File not found!");

    std::string nfilename = filename3 + "_commented" + filename.substr(dotPos);
    std::ofstream newFile(nfilename);
    if (!newFile)
        dbg::Misc::fexit("Cannot open " + filename3 + "_commented" + filename.substr(dotPos) + ", exiting.");

    std::string architecture = getArchitecture(filename);

    newFile << "; INFORMATION:" << std::endl;
    newFile << "; \tAssembly Analyzer Version: " << VERSION << std::endl;
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    // main.cpp(74,53): warning C4996: 'localtime': This function or variable may be unsafe. Consider using localtime_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
    newFile << "; \tAnalyzed on: " << std::put_time(localtime(&currentTime), "%Y-%m-%d %H:%M:%S") << std::endl;
    newFile << "; \tInstruction Set Architecture: " << architecture << std::endl << std::endl;

    std::string line;
    while (getline(originalFile, line)) {
        std::string comment = analyzeLine(line);
        newFile << line << (comment.empty() ? "\n" : "\t\t; " + comment + "\n");
    }

    originalFile.close();
    newFile.close();

    auto delta = std::chrono::high_resolution_clock::now() - begin;
    _INFO("Successfully analyzed " + filename + " in " + std::to_string(std::chrono::duration<double>(delta).count()) + "s");
    dbg::Misc::prefexit();

    return 0;
}

bool isDirective(const std::string& opcode) {
    return !opcode.empty() && opcode[0] == '.';
}

bool isMemoryAddressingMode(const std::string& operand) {
    return !operand.empty() && operand[0] == '[' && operand.back() == ']' && operand.find('%') != std::string::npos;
}

std::string getOperand(const std::string& line) {
    size_t wPos = line.find(' ');
    if (wPos == std::string::npos) return "";

    std::string operand = line.substr(wPos + 1);
    size_t trailingWPos = operand.find_last_not_of(' ');
    if (trailingWPos != std::string::npos)
        operand = operand.substr(0, trailingWPos + 1);

    return operand;
}

std::string analyzeLine(const std::string& line) {
    if (line.find_first_not_of(" \t\r\n") == std::string::npos) return "";

    std::string trimmedLine = trim(line);

    if (!trimmedLine.empty() && trimmedLine[trimmedLine.size() - 1] == ':')
        return "Label: " + trimmedLine.substr(0, trimmedLine.size() - 1);

    size_t spacePos = trimmedLine.find(' ');
    std::string opcode = trimmedLine.substr(0, spacePos);

    if (isInstruction(opcode)) {
        std::string operands = trimmedLine.substr(spacePos + 1);

        std::string operandComment;
        if (opcode == "global") {
            return "Declare global symbol " + operands;
        } else if (opcode == "len") {
            return "Calculate length of " + operands;
        } else if (opcode == "int") {
            size_t spacePos = operands.find(' ');
            std::string operand = operands.substr(0, spacePos);

            if (operand.find("0x") == 0)
                return std::string("Instruction: int ") + std::string("| Interrupt: ") + operand;
        } else if (opcode == "push") {
            std::string operand = getOperand(line);
            return "push instruction: pushed " + operand + " into stack";
        } else if (opcode == "pop") {
            std::string operand = getOperand(line);
            return "pop instruction: popped " + operand + " from stack";
        } else if (opcode == "mov" || opcode == "movq" || opcode == "add" || opcode == "addq" || opcode == "sub" || opcode == "subq") {
            size_t commaPos = operands.find(',');
            if (commaPos != std::string::npos) {
                std::string destination = operands.substr(0, commaPos);
                std::string source = operands.substr(commaPos + 1);
                //                                                                                              there's a space for some reason ALREADY
                return "Instruction: " + opcode + " | Destination: " + analyzeOperand(destination, true) + " | Source:" + analyzeOperand(source, true);
            }
        } else if (opcode == "equ") {
            size_t equPos = trimmedLine.find("equ");
            if (equPos != std::string::npos) {
                std::string label = trimmedLine.substr(0, equPos);
                std::string expression = trimmedLine.substr(equPos + 3);
                
                return "Define constant " + trim(label) + " as " + trim(expression);
            }
        } else if (opcode == "jmp") {
            std::string operand = getOperand(line);
            return "jmp instruction: jumped to " + operand;
        } else if (opcode == "call") {
            std::string operand = getOperand(line);
            return "call instruction: called " + operand;
        } else if (opcode == "ret") {
            return "ret instruction: returned from function";
        } else if (opcode == "cmp") {
            size_t spacePos = operands.find(' ');
            std::string destOperand = operands.substr(0, spacePos);
            std::string srcOperand = operands.substr(spacePos + 1);

            std::string dest = analyzeOperand(destOperand, true);
            std::string src = analyzeOperand(srcOperand, true);

            return "Instruction: cmp | Destination: " + dest + " | Source: " + src;
        } else if (opcode == "je") {
            std::string operand = getOperand(line);
            return "je instruction: jumped to " + operand + " if equal";
        } else if (opcode == "jne") {
            std::string operand = getOperand(line);
            return "jne instruction: jumped to " + operand + " if not equal";
        } else if (opcode == "inc") {
            std::string operand = getOperand(line);
            return "inc instruction: incremented " + operand;
        } else if (opcode == "dec") {
            std::string operand = getOperand(line);
            return "dec instruction: decremented " + operand;
        } else if (opcode == "mul") {
            size_t spacePos = operands.find(' ');
            std::string destOperand = operands.substr(0, spacePos);
            std::string srcOperand = operands.substr(spacePos + 1);

            std::string dest = analyzeOperand(destOperand, true);
            std::string src = analyzeOperand(srcOperand, true);

            return "Instruction: mul | Destination: " + dest + " | Source: " + src;
        } else if (opcode == "div") {
            size_t spacePos = operands.find(' ');
            std::string destOperand = operands.substr(0, spacePos);
            std::string srcOperand = operands.substr(spacePos + 1);

            std::string dest = analyzeOperand(destOperand, true);
            std::string src = analyzeOperand(srcOperand, true);

            return "Instruction: div | Destination: " + dest + " | Source: " + src;
        } else {
            operandComment = analyzeOperands(operands);
        }

        return "Instruction: " + opcode + " " + operandComment;
    } else if (opcode == "section") {
        std::string sectionName = getOperand(line);
        return "Section " + sectionName + " declared";
    }

    if (isDirective(opcode)) {
        std::string operand = getOperand(line);

        if (opcode == ".string") {
            std::string lineValue = trim(operand).substr(8);
            return "string constant " + lineValue + " declared";
////////////////////////////////////////////////////////////////////////////
        } else if (opcode == ".data") {
            return "Data section declared";
        } else if (opcode == ".bss") {
            return "BSS (uninitialized data) section declared";
        } else if (opcode == ".text") {
            return "Text (code) section declared";
        } else if (opcode == ".globl" || opcode == ".global") {
            return "Global symbol " + operand + " declared";
        } else if (opcode == ".align") {
            return "Align to " + operand + " bytes";
        } else if (opcode == ".byte") {
            return "Byte value " + operand + " declared";
        } else if (opcode == ".word") {
            return "Word value " + operand + " declared";
        } else if (opcode == ".dword") {
            return "Double word value " + operand + " declared";
        } else if (opcode == ".quad") {
            return "Quad word (64-bit) value " + operand + " declared";
        } else if (opcode == ".section") {
            return "Section " + operand + " declared";
        } else if (opcode == ".equ" || opcode == ".set") {
            return "Constant " + operand + " defined";
        } else if (opcode == ".org") {
            return "Set origin to address " + operand;
        } else if (opcode == ".reserve" || opcode == ".space") {
            return "Reserve " + operand + " bytes";
        } else if (opcode == ".file") {
            return "File name set to " + operand;
        } else if (opcode == ".comm") {
            return "Common block " + operand + " declared";
        } else if (opcode == ".end") {
            return "End of assembly";
        } else if (opcode == ".incbin") {
            return "Include binary file " + operand;
        } else {
            return "Unknown directive: " + opcode;
        }
    }

    return "Unknown instruction";
}

std::string analyzeOperands(std::string& operands) {
    std::string operandComment;
    size_t pos = 0;
    while ((pos = operands.find(' ')) != std::string::npos) {
        std::string operand = operands.substr(0, pos);
        operandComment += analyzeOperand(operand) + " ";
        operands.erase(0, pos + 1);
    }

    operandComment += analyzeOperand(operands);
    return operandComment;
}

std::string analyzeOperand(const std::string& operand, const bool appendType) {
    if (operand.empty()) {
        return "Empty operand";
    }

    // Recognized registers
    static const std::unordered_set<std::string> registers = {
        "eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp",
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rsp", "rbp",
        "ah", "bh", "ch", "dh", "al", "bl", "cl", "dl",
        "spl", "bpl", "sil", "dil",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
        "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
        "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",
        "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"
    };

    // Check if the operand is a register
    if (registers.count(operand)) {
        return appendType ? operand + " (Register)" : "Register: " + operand;
    }

    // Check for immediate values (numeric literals)
    if (operand[0] == '$' || std::isdigit(operand[0]) || operand == "0") {
        std::string immediate = operand[0] == '$' ? operand.substr(1) : operand;
        return appendType ? immediate + " (Immediate)" : "Immediate: " + immediate;
    }

    // Check for memory addressing mode
    if (isMemoryAddressingMode(operand)) {
        std::string labelName = operand.substr(1, operand.size() - 2);
        return appendType ? labelName + " (Memory Address)" : "Memory Address: " + labelName;
    }

    // Assume other operands are labels or identifiers
    return appendType ? operand + " (Label/Identifier)" : "Label/Identifier: " + operand;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first) return str;

    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

bool isInstruction(const std::string& opcode) {
    std::vector<std::string> instructions = { "int", "push", "pop", "mov", "movq", "add", "addq", "sub", "subq",
                                              "jmp", "call", "ret", "cmp", "je", "jne", "inc", "dec", "mul", "div",
                                              "global", "len"
    };
    return find(instructions.begin(), instructions.end(), opcode) != instructions.end();
}

std::string getArchitecture(const std::string& filename) {
    std::ifstream file(filename);
    if (!file)
        _ERROR("Error opening/reading " + filename + " file (is " + filename + " closed?)");
//        return "";

    std::string architecture = "Unknown"; // default value is Unknown
    std::string line;

    while (getline(file, line)) {
        line = trim(line);

        // x86-64
        if (line.find(".code64") != std::string::npos || line.find(".x64") != std::string::npos ||
            line.find(".quad") != std::string::npos || line.find("BITS 64") != std::string::npos ||
            line.find("__x86_64__") != std::string::npos || line.find("__amd64__") != std::string::npos) {
            architecture = "x86-64";
        }
        // x86
        else if (line.find(".code32") != std::string::npos || line.find(".x86") != std::string::npos ||
            line.find("BITS 32") != std::string::npos || line.find("__i386__") != std::string::npos) {
            architecture = "x86";
        }
        // ARM
        else if (line.find(".arm") != std::string::npos || line.find(".thumb") != std::string::npos ||
            line.find("__ARM_ARCH") != std::string::npos) {
            architecture = "ARM";
        }
        // MIPS
        else if (line.find(".mips") != std::string::npos || line.find(".mips64") != std::string::npos ||
            line.find("__mips__") != std::string::npos) {
            architecture = "MIPS";
        }
        // PowerPC
        else if (line.find(".ppc") != std::string::npos || line.find("__powerpc__") != std::string::npos ||
            line.find("__ppc__") != std::string::npos) {
            architecture = "PowerPC";
        }
        // RISC-V
        else if (line.find(".riscv") != std::string::npos || line.find("__riscv") != std::string::npos) {
            architecture = "RISC-V";
        }
        // SPARC
        else if (line.find(".sparc") != std::string::npos || line.find("__sparc__") != std::string::npos) {
            architecture = "SPARC";
        }

        if (architecture != "Unknown") break;
    }

    file.close();
    return architecture;
}