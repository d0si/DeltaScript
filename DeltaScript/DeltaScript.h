#ifndef DELTASCRIPT_DELTASCRIPT_H_
#define DELTASCRIPT_DELTASCRIPT_H_

namespace DeltaScript {
    class Token {
        enum TokenKind : unsigned int {
            EOS = 0, // End of stream/source

            // Punctuators, see https://www.ecma-international.org/ecma-262/10.0/index.html#sec-punctuators
            LBRACK,     // [
            LPAREN,     // (
            LBRACE,     // {

            RBRACK,     // ]
            RPAREN,     // )
            RBRACE,     // }

            DOT,        // .
            ELLIPSIS,   // ...
            SEMICOLON,  // ;
            COMMA,      // ,

            LT,         // <
            GT,         // >
            LTE,        // <=
            GTE,        // >=

            EQUAL,      // ==
            NEQUAL,     // !=
            STRICT_EQUAL,  // ===
            STRICT_NEQUAL, // !==
        };
    };



    class Lexer {
    public:

    };

    namespace Util {
        bool is_white_space(char value);
        bool is_line_terminator(char value);
        bool is_line_terminator_sequence(char value, char next_value);
    }
}

#endif  // DELTASCRIPT_DELTASCRIPT_H_
