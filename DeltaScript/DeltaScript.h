#ifndef DELTASCRIPT_DELTASCRIPT_H_
#define DELTASCRIPT_DELTASCRIPT_H_

#include <string>

namespace DeltaScript {
    enum class TokenKind : unsigned int {
        EOS = 0, // End of stream/source

        // Names and keywords, see https://www.ecma-international.org/ecma-262/10.0/index.html#sec-names-and-keywords
        IDENTIFIER,
        // ECMAScript keywords, may not be used as Identifiers (reserved)
        AWAIT_K,    // await
        BREAK_K,    // break
        CASE_K,     // case
        CATCH_K,    // catch
        CLASS_K,    // class
        CONST_K,    // const
        CONTINUE_K, // continue
        DEBUGGER_K, // debugger
        DEFAULT_K,  // default
        DELETE_K,   // delete
        DO_K,       // do
        ELSE_K,     // else
        EXPORT_K,   // export
        EXTENDS_K,  // extends
        FINALLY_K,  // finally
        FOR_K,      // for
        FUNCTION_K, // function
        IF_K,       // if
        IMPORT_K,   // import
        IN_K,       // in
        INSTANCEOF_K, // instanceof
        LET_K,      // let
        NEW_K,      // new
        RETURN_K,   // return
        STATIC_K,   // static
        SUPER_K,    // super
        SWITCH_K,   // switch
        THIS_K,     // this
        THROW_K,    // throw
        TRY_K,      // try
        TYPEOF_K,   // typeof
        VAR_K,      // var
        VOID_K,     // void
        WHILE_K,    // while
        WITH_K,     // with
        YIELD_K,    // yield

        // Punctuators, see https://www.ecma-international.org/ecma-262/10.0/index.html#sec-punctuators
        LBRACK_P,   // [
        LPAREN_P,   // (
        LBRACE_P,   // {
        RBRACK_P,   // ]
        RPAREN_P,   // )
        RBRACE_P,   // }
        PERIOD_P,   // .
        ELLIPSIS_P, // ...
        COLON_P,    // :
        SEMICOLON_P, // ;
        COMMA_P,    // ,
        LT_P,       // <
        GT_P,       // >
        LTE_P,      // <=
        GTE_P,      // >=
        ASSIGN_P,   // =
        EQUAL_P,    // ==
        NEQUAL_P,   // !=
        STRICT_EQUAL_P, // ===
        STRICT_NEQUAL_P, // !==
        ADD_P,      // +
        SUB_P,      // -
        MUL_P,      // *
        EXP_P,      // **
        MOD_P,      // %
        INCR_P,     // ++
        DECR_P,     // --
        SHFT_L_P,   // <<
        SHFT_R_P,   // >>
        SHFT_RR_P,  // >>>
        BIT_AND_P,  // &
        BIT_OR_P,   // |
        BIT_XOR_P,  // ^
        BIT_NOT_P,  // ~
        AND_P,      // &&
        OR_P,       // ||
        NOT_P,      // !
        CONDITIONAL_P, // ?
        // +=
        // -=
        // *=
        // %=
        // **=
        // <<=
        // >>=
        // >>>=
        // &=
        // |=
        // ^=
        // =>

        // Literals, see https://www.ecma-international.org/ecma-262/10.0/index.html#sec-ecmascript-language-lexical-grammar-literals
        NULL_L,     // null
        TRUE_L,     // true
        FALSE_L,    // false

        //NUMERIC_L,  // Numeric literal (0 1 2 3 4 5 6 7 8 9 e E . + -), not supported: 0o 0x a b c d e f A B C D E F
        INTEGER_L,  // Integer
        FLOAT_L,  // Float
        STRING_L,   // String literal, characters between two unescaped ' or "

    };

    class Token {
    public:
        static std::string get_position_info(const char* source, size_t source_length, int position);
        static std::string get_token_kind_as_string(TokenKind kind);
    };

    class LexerException {
    public:
        LexerException(const std::string& message);

        std::string message;
    };

    class Lexer {
    private:
        char* source_;
        int source_start_;
        size_t source_end_;
        int c_source_position_;
        bool source_owner;

        char c_char = 0, n_char = 0;
        TokenKind c_token_kind = TokenKind::EOS;
        int c_token_start = 0;
        int c_token_end = 0;

        int token_last_end;
        std::string c_token_value;

    public:
        Lexer(const std::string& source);
        Lexer(Lexer* parent, int source_start, int source_end);
        ~Lexer();

        void reset();
        void expect_and_get_next(TokenKind expected_kind);
        void get_next_char();
        void parse_next_token();

    private:
        void process_inline_comment();
        void process_multiline_comment();
        void process_identifier();
        void process_number();
        void process_double_quote_string_literal();
        void process_single_quote_string_literal();

        void check_for_reserved_keywords();
    };

    namespace Util {
        bool is_white_space(char value);
        bool is_line_terminator(char value);
        bool is_line_terminator_crlf(char value, char next_value);

        bool is_alpha(char value);
        bool is_digit(char value);
        bool is_hex(char value);
    }
}

#endif  // DELTASCRIPT_DELTASCRIPT_H_
