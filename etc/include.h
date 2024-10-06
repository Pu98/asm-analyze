// Include libraries and define functions to use them in main.cpp

#pragma once

#include "dbg.hpp"

#include <unordered_set>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <cctype>
#include <ctime>

// function prototypes (sorted)
auto isInstruction(std::string& opcode) -> bool;
auto trim(const std::string& str) -> std::string;
auto isDirective(const std::string& opcode) -> bool;
auto getOperand(const std::string& line) -> std::string;
auto analyzeLine(const std::string& line) -> std::string;
auto analyzeOperands(std::string& operands) -> std::string;
auto isMemoryAddressingMode(const std::string& operand) -> bool;
auto getArchitecture(const std::string& filename) -> std::string;
auto analyzeOperand(const std::string& operand, bool appendType = false) -> std::string;
auto analyzeDirective(const std::string& opcode, const std::string& operand) -> std::string;
auto analyzeInstruction(const std::string& opcode, const std::string& operands, const std::string& line) -> std::string;