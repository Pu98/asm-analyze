// Include libraries and define functions to use them in main.cpp

#pragma once

#include "dbg.hpp"
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <cctype>

typedef std::string str;

// function prototypes
str analyzeLine(const str& line);
str trim(const str& str);
str getOperand(const str& line);
str analyzeOperands(str& operands);
str analyzeOperand(const str& operand, const bool appendType = false);
str getArchitecture(const str& filename);
bool isDirective(const str& opcode);
bool isMemoryAddressingMode(const str& operand);
bool isInstruction(const str& opcode);