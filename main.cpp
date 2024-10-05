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
    std::cout << "Enter assembly file/directory (e.g. Test.asm or /path/to/Test.asm): ";
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
    auto q = std::chrono::high_resolution_clock::now();

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
    newFile << "; \tAnalyzed on: " << __DATE__ << " " << __TIME__ << std::endl;
    newFile << "; \tInstruction Set Architecture: " << architecture << std::endl;

    std::string line;
    while (getline(originalFile, line)) {
        std::string comment = analyzeLine(line);
        newFile << line << (comment.empty() ? "\n" : "\t\t; " + comment + "\n");
    }

    originalFile.close();
    newFile.close();

    auto delta = std::chrono::high_resolution_clock::now() - q;
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
        if (opcode == "int") {
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
            size_t spacePos = operands.find(' ');
            std::string destOperand = operands.substr(0, spacePos);
            std::string srcOperand = operands.substr(spacePos + 1);

            std::string dest = analyzeOperand(destOperand, true);
            std::string src = analyzeOperand(srcOperand, true);

            return "Instruction: " + opcode + " | Destination: " + dest + " | Source: " + src;
        } else {
            operandComment = analyzeOperands(operands);
        }

        return "Instruction: " + opcode + " " + operandComment;
    } else if (opcode == "section") {
        std::string sectionName = getOperand(line);
        return "Section " + sectionName + " declared";
    }

    if (isDirective(opcode)) {
        if (opcode == ".string") {
            std::string lineValue = trim(getOperand(line)).substr(8);
            return "string constant " + lineValue + " declared";
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
    if (operand.starts_with("reg")) {
        return appendType ? operand + " (Register)" : "Register: " + operand;
    }

    if (operand[0] == '$') {
        std::string immediate = operand.substr(1);
        return appendType ? immediate + " (Immediate)" : "Immediate: " + immediate;
    }

    if (isMemoryAddressingMode(operand)) {
        std::string labelName = operand.substr(1, operand.size() - 2);
        return appendType ? labelName + " (Memory Address)" : "Memory Address: " + labelName;
    }

    if (operand[0] == '%') {
        std::string registerName = operand.substr(1);
        return appendType ? registerName + " (Register)" : "Register: " + registerName;
    }

    return "Unknown operand: " + operand;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first) return str;

    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

bool isInstruction(const std::string& opcode) {
    ///                                          yeah these are only that are supported lol vvvvvvvvvvvvvv
    std::vector<std::string> instructions = { "int", "push", "pop", "mov", "movq", "add", "addq", "sub", "subq" };
    return find(instructions.begin(), instructions.end(), opcode) != instructions.end();
}

std::string getArchitecture(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        _ERROR("Error opening/reading \"" + filename + "\" file (is \"" + filename + "\" closed?)");
        return "";
    }

    std::string architecture = "Unknown"; // default value
    std::string line;

    while (getline(file, line)) {
        line = trim(line);
        if (line.find(".code64") != std::string::npos || line.find(".x64") != std::string::npos || line.find(".quad") != std::string::npos || line.find("BITS 64") != std::string::npos)
            architecture = "x86-64";
        else if (line.find(".code32") != std::string::npos || line.find(".x86") != std::string::npos || line.find("BITS 32") != std::string::npos)
            architecture = "x86";
        else if (line.find(".arm") != std::string::npos || line.find(".thumb") != std::string::npos)
            architecture = "ARM";
        else if (line.find(".mips") != std::string::npos || line.find(".mips64") != std::string::npos)
            architecture = "MIPS";
        else architecture = "Unknown";

        if (architecture != "Unknown") break;
    }

    file.close();
    return architecture;
}
