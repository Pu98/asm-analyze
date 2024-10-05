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

// please no
//typedef std::string str;

// function prototypes
std::string analyzeLine(const std::string& line);
std::string trim(const std::string& str);
std::string getOperand(const std::string& line);
std::string analyzeOperands(std::string& operands);
std::string analyzeOperand(const std::string& operand, const bool appendType = false);
std::string getArchitecture(const std::string& filename);
bool isDirective(const std::string& opcode);
bool isMemoryAddressingMode(const std::string& operand);
bool isInstruction(const std::string& opcode);