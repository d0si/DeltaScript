#include <DeltaScript/Compiler/Lexer.h>

namespace DeltaScript {
    namespace Compiler {
        bool Lexer::is_white_space(char value) {
            // <TAB>, <VT>, <FF>, <SP>, <NBSP> //, <BOM>, <USP>
            return value == 0x09 || value == 0x0B || value == 0x0C ||
                value == 0x20 || value == 0xA0;
        }

        bool Lexer::is_line_terminator(char value) {
            // <LF>, <CR>, <LS>, <PS>
            return value == 0x0A || value == 0x0D || value == 0x2028 || value == 0x2029;
        }

        bool Lexer::is_line_terminator_sequence(char value, char next_value) {
            // <LF>, <CR>, <LS>, <PS>
            return value == 0x0A || value == 0x0D || value == 0x2028 || value == 0x2029;
        }



    }
}
