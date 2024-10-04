#pragma once

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

class Color {
public:
    /**
     * Enumerates the available text colors.
     */
    enum Code {
        /**
         * Red text color.
         */
        RED = 31,
        /**
         * Green text color.
         */
        GREEN = 32,
        /**
        * Yellow text color.
        */
        YELLOW = 33,
        /**
         * Blue text color.
         */
        BLUE = 34,
        /**
         * Bright Magenta text color.
         */
        BRIGHT_MAGENTA = 95, // sync with the `first` repo color hint++ file
        /**
         * Default text color (reset to console default).
         */
        DEFAULT = 39,
    };

    /**
     * Colorizes a given text string with the specified color code.
     *
     * @param text The text string to colorize.
     * @param code The color code to use (from the Code enum).
     * @param bold Whether to make the text bold (optional, default is false).
     * @return The colorized text string.
     */
    static std::string colorize(const std::string& text, Code code, bool bold = false) {
        std::string ansiCode = "\x1B[";
        if (bold)
            ansiCode += "1;";
        ansiCode += std::to_string(code) + "m";
        return ansiCode + text + "\x1B[0m";
    }
};