#include "etc/include.h"

const static std::unordered_set<std::string> forbidden = {
    "CON", "PRN", "AUX", "NUL",
    "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
    "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
};

const static std::unordered_set<std::string> supportedExtensions = { // no .lst
    "asm", "s", "hla", "inc", "palx", "mid"
};

const static std::string VERSION = "0.1.0"; // it's better to store this as a string, rather than a double
const static std::string filename;
constexpr static int EIGHT = 8;

auto main() -> int {
#ifdef _WIN32
    // prepare console
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hConsole, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, dwMode);
#endif
    std::cout << "Enter assembly file/directory (e.g. file.asm or /path/to/file.asm): ";
    std::string filename;
    std::getline(std::cin, filename);
    if (filename.empty() || filename.find_first_not_of(" \t\r\f\v") == std::string::npos) {
        dbg::Misc::fexit("You can't do that :3");
    }

    // Convert to uppercase for path checking
    std::string ufilename = filename;
    std::transform(ufilename.begin(), ufilename.end(), ufilename.begin(), ::toupper);

    size_t pos = 0;
    while ((pos = ufilename.find('/')) != std::string::npos) {
        std::string part = ufilename.substr(0, pos);
        if (forbidden.count(part) > 0) {
            dbg::Misc::fexit("You can't do that :3");
        }
        ufilename.erase(0, pos + 1);
    }

    // Remove the file extension & check whether file type is supported or not
    size_t dotPos = filename.find_last_of('.');
    std::string extension;
    if (dotPos != std::string::npos) {
        extension = filename.substr(dotPos + 1);
        for (char& c : extension) {
            c = static_cast<char>(tolower(c));
        }
        if (supportedExtensions.count(extension) == 0U) {
            dbg::Misc::fexit("You can't do that :3");
        }
        ufilename.erase(dotPos);
    }

    // Check the remaining part of the file name
    if (forbidden.count(ufilename) > 0) {
        dbg::Misc::fexit("You can't do that :3");
    }

    auto begin = std::chrono::high_resolution_clock::now();

    std::ifstream originalFile(filename);
    if (!originalFile) {
        dbg::Misc::fexit("File not found!");
    }

    std::string nfilename = filename;
    if (dotPos != std::string::npos) {
        nfilename.insert(dotPos, "_analyzed");
    }
    else {
        nfilename += "_analyzed";
    }
    std::ofstream newFile(nfilename);
    if (!newFile) {
        dbg::Misc::fexit("Cannot open " + nfilename + ", exiting.");
    }

    std::string architecture = getArchitecture(filename);

    newFile << "; INFORMATION:" << '\n';
    newFile << "; \tAssembly Analyzer Version: " << VERSION << '\n';
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    struct tm localTime {};
#ifdef _WIN32
    localtime_s(&localTime, &currentTime);
#else
    localtime_r(&currentTime, &localTime);
#endif
    newFile << "; \tAnalyzed on: " << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << '\n';
    newFile << "; \tInstruction Set Architecture: " << architecture << '\n' << '\n';

    std::string line;
    while (getline(originalFile, line)) {
        std::string comment = analyzeLine(line);
        newFile << line << (comment.empty() ? "\n" : "\t\t; " + comment + "\n");
    }

    originalFile.close();
    newFile.close();

    auto delta = std::chrono::high_resolution_clock::now() - begin;
    dbg::Macros::info("Successfully analyzed " + filename + " in " + std::to_string(std::chrono::duration<double>(delta).count()) + "s");
    dbg::Misc::prefexit();

    return 0;
}

auto isDirective(const std::string& opcode) -> bool {
    return !opcode.empty() && opcode[0] == '.';
}

auto isMemoryAddressingMode(const std::string& operand) -> bool {
    return !operand.empty() && operand[0] == '[' && operand.back() == ']' && operand.find('%') != std::string::npos;
}

auto getOperand(const std::string& line) -> std::string {
    size_t wPos = line.find(' ');
    if (wPos == std::string::npos) {
        return "";
    }

    std::string operand = line.substr(wPos + 1);
    size_t trailingWPos = operand.find_last_not_of(' ');
    if (trailingWPos != std::string::npos) {
        operand = operand.substr(0, trailingWPos + 1);
    }

    return operand;
}

auto analyzeDirective(const std::string& opcode, const std::string& operand) -> std::string {
    if (opcode == ".string") {
        std::string lineValue = trim(operand).substr(EIGHT);
        return "string constant " + lineValue + " declared";
    } if (opcode == ".data") {
        return "Data section declared";
    } if (opcode == ".bss") {
        return "BSS (uninitialized data) section declared";
    } if (opcode == ".text") {
        return "Text (code) section declared";
    } if (opcode == ".globl" || opcode == ".global") {
        return "Global symbol " + operand + " declared";
    } if (opcode == ".align") {
        return "Align to " + operand + " bytes";
    } if (opcode == ".byte") {
        return "Byte value " + operand + " declared";
    } if (opcode == ".word") {
        return "Word value " + operand + " declared";
    } if (opcode == ".dword") {
        return "Double word value " + operand + " declared";
    } if (opcode == ".quad") {
        return "Quad word (64-bit) value " + operand + " declared";
    } if (opcode == ".section") {
        return "Section " + operand + " declared";
    } if (opcode == ".equ" || opcode == ".set") {
        return "Constant " + operand + " defined";
    } if (opcode == ".org") {
        return "Set origin to address " + operand;
    } if (opcode == ".reserve" || opcode == ".space") {
        return "Reserve " + operand + " bytes";
    } if (opcode == ".file") {
        return "File name set to " + operand;
    } if (opcode == ".comm") {
        return "Common block " + operand + " declared";
    } if (opcode == ".end") {
        return "End of assembly";
    } if (opcode == ".incbin") {
        return "Include binary file " + operand;
    }

    return "Unknown directive: " + opcode;
}

auto analyzeLine(const std::string& line) -> std::string {
    if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
        return "";
    }

    std::string trimmedLine = trim(line);

    if (!trimmedLine.empty() && trimmedLine[trimmedLine.size() - 1] == ':') {
        return "Label: " + trimmedLine.substr(0, trimmedLine.size() - 1);
    }

    size_t spacePos = trimmedLine.find(' ');
    std::string opcode = trimmedLine.substr(0, spacePos);

    if (isInstruction(opcode)) {
        std::string operands = trimmedLine.substr(spacePos + 1);
        return analyzeInstruction(opcode, operands, line);
    }

    if (isDirective(opcode)) {
        std::string operand = getOperand(line);
        return analyzeDirective(opcode, operand);
    }

    return "Unknown instruction";
}

auto analyzeInstruction(const std::string& opcode, const std::string& operands, const std::string& line) -> std::string {
    if (opcode == "global") {
        return "Declare global symbol " + operands;
    } if (opcode == "len") {
        return "Calculate length of " + operands;
    } if (opcode == "int") {
        size_t spacePos = operands.find(' ');
        std::string operand = operands.substr(0, spacePos);

        if (operand.find("0x") == 0) {
            return std::string("Instruction: int ") + std::string("| Interrupt: ") + operand;
        }
    } if (opcode == "push") {
        std::string operand = getOperand(line);
        return "push instruction: pushed " + operand + " into stack";
    } if (opcode == "mov" || opcode == "movq" || opcode == "add" || opcode == "addq" || opcode == "sub" || opcode == "subq") {
        size_t commaPos = operands.find(',');
        if (commaPos != std::string::npos) {
            std::string destination = operands.substr(0, commaPos);
            std::string source = operands.substr(commaPos + 1);
            return "Instruction: " + opcode + " | Destination: " + analyzeOperand(destination, true) + " | Source:" + analyzeOperand(source, true);
        }
    } if (opcode == "jmp") {
        std::string operand = getOperand(line);
        return "jmp instruction: jumped to " + operand;
    } if (opcode == "call") {
        std::string operand = getOperand(line);
        return "call instruction: called " + operand;
    } if (opcode == "ret") {
        return "ret instruction: returned from function";
    } if (opcode == "nop") {
        return "no operation";
    } if (opcode == "cmp") {
        size_t spacePos = operands.find(' ');
        std::string destOperand = operands.substr(0, spacePos);
        std::string srcOperand = operands.substr(spacePos + 1);

        std::string dest = analyzeOperand(destOperand, true);
        std::string src = analyzeOperand(srcOperand, true);

        return "Instruction: cmp | Destination: " + dest + " | Source: " + src;
    } if (opcode == "je") {
        std::string operand = getOperand(line);
        return "je instruction: jumped to " + operand + " if equal";
    } if (opcode == "jne") {
        std::string operand = getOperand(line);
        return "jne instruction: jumped to " + operand + " if not equal";
    } if (opcode == "inc") {
        std::string operand = getOperand(line);
        return "inc instruction: incremented " + operand;
    } if (opcode == "dec") {
        std::string operand = getOperand(line);
        return "dec instruction: decremented " + operand;
    } if (opcode == "mul") {
        size_t spacePos = operands.find(' ');
        std::string destOperand = operands.substr(0, spacePos);
        std::string srcOperand = operands.substr(spacePos + 1);

        std::string dest = analyzeOperand(destOperand, true);
        std::string src = analyzeOperand(srcOperand, true);

        return "Instruction: mul | Destination: " + dest + " | Source: " + src;
    } if (opcode == "div") {
        size_t spacePos = operands.find(' ');
        std::string destOperand = operands.substr(0, spacePos);
        std::string srcOperand = operands.substr(spacePos + 1);

        std::string dest = analyzeOperand(destOperand, true);
        std::string src = analyzeOperand(srcOperand, true);

        return "Instruction: div | Destination: " + dest + " | Source: " + src;
    }
    return "Unknown instruction: " + opcode;
}

auto analyzeOperands(std::string& operands) -> std::string {
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

auto analyzeOperand(const std::string& operand, bool appendType) -> std::string {
    if (operand.empty()) { return "Empty operand"; }

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
    if (registers.count(operand) != 0U) {
        return appendType ? operand + " (Register)" : "Register: " + operand;
    }

    // Check for immediate values (numeric literals)
    if (operand[0] == '$' || (std::isdigit(operand[0]) != 0) || operand == "0") {
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

auto trim(const std::string& str) -> std::string {
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first) { return str; }

    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

auto isInstruction(std::string& opcode) -> bool {
    // Recognized instructions
    const static std::vector<std::string> instructions = {
        "int", "push", "pop", "mov", "movq", "add", "addq", "sub", "subq",
        "jmp", "call", "ret", "cmp", "je", "jne", "inc", "dec", "mul", "div",
        "global", "len", "nop"
    };
    return find(instructions.begin(), instructions.end(), opcode) != instructions.end();
}

auto getArchitecture(const std::string& filename) -> std::string {
    std::ifstream file(filename);
    if (!file) {
        dbg::Macros::fatal("Error opening/reading " + filename + " file (is " + filename + " closed?)");
    }

    std::string architecture = "Unknown"; // default value is Unknown
    std::string line;

    while (getline(file, line)) {
        line = trim(line);

        // x86-64
        if (line.find(".code64") != std::string::npos || line.find(".x64") != std::string::npos ||
            line.find(".quad") != std::string::npos || line.find("BITS 64") != std::string::npos ||
            line.find("__x86_64__") != std::string::npos || line.find("__amd64__") != std::string::npos ||
            line.find("__aarch64__") != std::string::npos) {
            architecture = "x86-64";
        }
        // x86
        else if (line.find(".code32") != std::string::npos || line.find(".x86") != std::string::npos ||
            line.find("BITS 32") != std::string::npos || line.find("__i386__") != std::string::npos) {
            architecture = "x86";
        }
        // ARM
        else if (line.find(".arm") != std::string::npos || line.find(".thumb") != std::string::npos ||
            line.find("__ARM_ARCH") != std::string::npos || line.find("__arm__") != std::string::npos) {
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

        if (architecture != "Unknown") { break; }
    }

    file.close();
    return architecture;
}