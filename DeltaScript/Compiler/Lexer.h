#ifndef DELTASCRIPT_COMPILER_LEXER_H_
#define DELTASCRIPT_COMPILER_LEXER_H_

namespace DeltaScript {
    namespace Compiler {
        class Lexer {
        public:

            static bool is_white_space(char value);
            static bool is_line_terminator(char value);
            static bool is_line_terminator_sequence(char value, char next_value);
        };
    }
}

#endif  // DELTASCRIPT_COMPILER_LEXER_H_
