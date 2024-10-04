#include "etc/include.h"

std::unordered_set<str> forbidden = {
    "CON", "PRN", "AUX", "NUL",
    "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
    "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
};
std::unordered_set<str> supportedExtensions = { // no .lst
    "asm", "s", "hla", "inc", "palx", "mid"
};

const str VERSION = "1.0"; // it's better to store this as a string, rather than a double
str filename;

int main() {
// init
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hConsole, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, dwMode);
#endif
    std::cout << "Enter assembly file/dir (e.g. Test.asm or /path/to/Test.asm): ";
    std::getline(std::cin, filename);
    if (filename.empty() || filename.find_first_not_of(" \t\r\f\v") == str::npos /*broad, but okay...*/)
        dbg::Misc::fexit("You can't do that :3");

    str filename2 = filename; // store the old filename (it'll change) a line below
    std::transform(filename.begin(), filename.end(), filename.begin(), ::toupper);

    size_t pos = 0;
    while ((pos = filename.find('/')) != str::npos) {
        str part = filename.substr(0, pos);
        if (forbidden.contains(part))
            dbg::Misc::fexit("You can't do that :3");
        filename.erase(0, pos + 1);
    }

    // remove the file extension & check whether file type is supported or not
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != str::npos) {
        str extension = filename.substr(dotPos + 1);
        for (char& c : extension) c = tolower(c);
        if (!supportedExtensions.count(extension))
            dbg::Misc::fexit("You can't do that :3");
        filename.erase(dotPos);
    }

    str filename3 = filename;

    // check the remaining part of the filename
    if (forbidden.contains(filename))
        dbg::Misc::fexit("You can't do that :3");

    filename = filename2;
    auto q = std::chrono::high_resolution_clock::now();

    std::ifstream originalFile(filename);
    if (!originalFile)
        dbg::Misc::fexit("You can't do that :3");

    str nfilename = filename3 + "_commented" + filename.substr(dotPos);
    std::ofstream newFile(nfilename);
    if (!newFile)
        dbg::Misc::fexit("You can't do that :3");

    str architecture = getArchitecture(filename);

    newFile << "; INFORMATION:" << std::endl;
    newFile << "; \tAssembly Analyzer Version: " << VERSION << std::endl;
    newFile << "; \tAnalyzed on: " << __DATE__ << " " << __TIME__ << std::endl;
    newFile << "; \tInstruction Set Architecture: " << architecture << std::endl;

    str line;
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

bool isDirective(const str& opcode) {
    return !opcode.empty() && opcode[0] == '.';
}

bool isMemoryAddressingMode(const str& operand) {
    return !operand.empty() && operand[0] == '[' && operand.back() == ']' && operand.find('%') != str::npos;
}

str getOperand(const str& line) {
    size_t wPos = line.find(' ');
    if (wPos == str::npos) return "";

    str operand = line.substr(wPos + 1);
    size_t trailingWPos = operand.find_last_not_of(' ');
    if (trailingWPos != str::npos)
        operand = operand.substr(0, trailingWPos + 1);

    return operand;
}

str analyzeLine(const str& line) {
    if (line.find_first_not_of(" \t\r\n") == str::npos) return "";

    str trimmedLine = trim(line);

    if (!trimmedLine.empty() && trimmedLine[trimmedLine.size() - 1] == ':')
        return "Label: " + trimmedLine.substr(0, trimmedLine.size() - 1);

    size_t spacePos = trimmedLine.find(' ');
    str opcode = trimmedLine.substr(0, spacePos);

    if (isInstruction(opcode)) {
        str operands = trimmedLine.substr(spacePos + 1);

        str operandComment;
        if (opcode == "int") {
            size_t spacePos = operands.find(' ');
            str operand = operands.substr(0, spacePos);

            if (operand.find("0x") == 0)
                return str("Instruction: int ") + str("| Interrupt: ") + operand;
        } else if (opcode == "push") {
            str operand = getOperand(line);
            return "push instruction: pushed " + operand + " into stack";
        } else if (opcode == "pop") {
            str operand = getOperand(line);
            return "pop instruction: popped " + operand + " from stack";
        } else if (opcode == "mov" || opcode == "movq" || opcode == "add" || opcode == "addq" || opcode == "sub" || opcode == "subq") {
            size_t spacePos = operands.find(' ');
            str destOperand = operands.substr(0, spacePos);
            str srcOperand = operands.substr(spacePos + 1);

            str dest = analyzeOperand(destOperand, true);
            str src = analyzeOperand(srcOperand, true);

            return "Instruction: " + opcode + " | Destination: " + dest + " | Source: " + src;
        } else {
            operandComment = analyzeOperands(operands);
        }

        return "Instruction: " + opcode + " " + operandComment;
    } else if (opcode == "section") {
        str sectionName = getOperand(line);
        return "Section " + sectionName + " declared";
    }

    if (isDirective(opcode)) {
        if (opcode == ".string") {
            str lineValue = trim(getOperand(line)).substr(8);
            return "string constant " + lineValue + " declared";
        }
    }

    return "Unknown instruction";
}

str analyzeOperands(str& operands) {
    str operandComment;
    size_t pos = 0;
    while ((pos = operands.find(' ')) != str::npos) {
        str operand = operands.substr(0, pos);
        operandComment += analyzeOperand(operand) + " ";
        operands.erase(0, pos + 1);
    }

    operandComment += analyzeOperand(operands);
    return operandComment;
}

str analyzeOperand(const str& operand, const bool appendType) {
    if (operand.starts_with("reg")) {
        return appendType ? operand + " (Register)" : "Register: " + operand;
    }

    if (operand[0] == '$') {
        str immediate = operand.substr(1);
        return appendType ? immediate + " (Immediate)" : "Immediate: " + immediate;
    }

    if (isMemoryAddressingMode(operand)) {
        str labelName = operand.substr(1, operand.size() - 2);
        return appendType ? labelName + " (Memory Address)" : "Memory Address: " + labelName;
    }

    if (operand[0] == '%') {
        str registerName = operand.substr(1);
        return appendType ? registerName + " (Register)" : "Register: " + registerName;
    }

    return "Unknown operand: " + operand;
}

str trim(const str& str) {
    size_t first = str.find_first_not_of(' ');
    if (str::npos == first) return str;

    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

bool isInstruction(const str& opcode) {
    ///                                          yeah these are only that are supported lol vvvvvvvvvvvvvv
    std::vector<str> instructions = { "int", "push", "pop", "mov", "movq", "add", "addq", "sub", "subq" };
    return find(instructions.begin(), instructions.end(), opcode) != instructions.end();
}

str getArchitecture(const str& filename) {
    std::ifstream file(filename);
    if (!file) {
        _ERROR("Error opening/reading \"" + filename + "\" file (is \"" + filename + "\" closed?)");
        return "";
    }

    str architecture = "Unknown"; // default value
    str line;

    while (getline(file, line)) {
        line = trim(line);
        if (line.find(".code64") != str::npos || line.find(".x64") != str::npos || line.find(".quad") != str::npos || line.find("BITS 64") != str::npos)
            architecture = "x86-64";
        else if (line.find(".code32") != str::npos || line.find(".x86") != str::npos || line.find("BITS 32") != str::npos)
            architecture = "x86";
        else if (line.find(".arm") != str::npos || line.find(".thumb") != str::npos)
            architecture = "ARM";
        else if (line.find(".mips") != str::npos || line.find(".mips64") != str::npos)
            architecture = "MIPS";
        else architecture = "Unknown";

        if (architecture != "Unknown") break;
    }

    file.close();
    return architecture;
}
