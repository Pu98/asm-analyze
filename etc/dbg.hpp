#pragma once

#include <iostream>
#include <string>
#include "color.hpp"
#include <cstdlib>
#include <limits>

/**
  A namespace for debugging-related functionality.
 */
namespace dbg {
    /**
      A class for logging debug messages with different levels of severity.
     */
    class Debugger {
    public:
        /**
          Enumerates the available log levels.
         */
        enum Level {
            /**
              Informational log level.
             */
            INFO,
            /**
              Warning log level.
             */
            WARN,
            /**
              Error log level.
             */
            ERR,
            /**
              Fatal error log level.
             */
            FATAL
        };

        /**
          Logs a message with the specified level.
         *
          @param message The message to log.
          @param level The log level to use (from the Level enum).
         */
        static void log(const std::string& message, Level level) {
            std::string l{};
            Color::Code cc{ Color::RED }; // init (see line 66-72)

            switch (level) {
            case INFO:
                l = "INFO";
                cc = Color::GREEN;
                break;
            case WARN:
                l = "WARN";
                cc = Color::YELLOW;
                break;
            case ERR:
                l = "ERROR";
                break;
            case FATAL:
                l = "FATAL";
                break;
            }

            /**
             * In file included from /home/runner/work/asm-analyze/asm-analyze/main.cpp:2:
             * /home/runner/work/asm-analyze/asm-analyze/include/dbg.hpp: In static member function �static void dbg::Debugger::log(const string&, dbg::Debugger::Level)�:
             * /home/runner/work/asm-analyze/asm-analyze/include/dbg.hpp:41:56: warning: �cc� may be used uninitialized in this function [-Wmaybe-uninitialized]
             *     41 |             std::string colored = Color::colorize(l, cc);
             *        |                                                        ^
             */

            std::string colored = Color::colorize(l, cc);

            std::cout << "[" << colored << "] ";
            std::cout << message << std::endl;
        }

        /**
          Logs an informational message.
         *
          @param message The message to log.
         */
        static void info(const std::string& message) {
            log(message, INFO);
        }

        /**
          Logs a warning message.
         *
          @param message The message to log.
         */
        static void warn(const std::string& message) {
            log(message, WARN);
        }

        /**
          Logs an error message.
         *
          @param message The message to log.
         */
        static void error(const std::string& message) {
            log(message, ERR);
        }

        /**
          Logs a fatal error message.
         *
          @param message The message to log.
         */
        static void fatal(const std::string& message) {
            log(message, FATAL);
        }
    };

    /**
     A class for miscellaneous helper functions that didn't fit in any other one.
    */
    class Misc {
    public:
        /**
          Logs a fatal error message and exists the program.
         *
          @param message The message to log.
          @param code Exit code (default is 1).
         */
        static void fexit(const std::string& message, const int& code = 1) {
            dbg::Debugger::fatal(message);
            prefexit();
            std::exit(code); // i don't know where to put this :(
        }

        static void prefexit() {
            std::cout << "Press Enter to exit...";
            while (std::cin.get() != '\n') {}
        }
    };

    /**
      A namespace for debugging macros.
     */
    namespace Macros {
        /**
          Logs an informational message using the Debugger class.
         *
          @param message The message to log.
         */
#define _INFO(message) dbg::Debugger::info(message)

         /**
           Logs a warning message using the Debugger class.
          *
           @param message The message to log.
          */
#define _WARN(message) dbg::Debugger::warn(message)

          /**
            Logs an error message using the Debugger class.
           *
            @param message The message to log.
           */
#define _ERROR(message) dbg::Debugger::error(message)

           /**
             Logs a fatal error message using the Debugger class.
            *
             @param message The message to log.
            */
#define _FATAL(message) dbg::Debugger::fatal(message)
    }
}