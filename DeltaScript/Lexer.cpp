#include <DeltaScript/DeltaScript.h>
#include <sstream>

namespace DeltaScript {
    LexerException::LexerException(const std::string& message) : message(message) {

    }

    Lexer::Lexer(const std::string& source) {
        source_ = new char[source.size() + 1];
        std::copy(source.begin(), source.end(), source_);
        source_[source.size()] = '\0';

        source_owner = true;
        source_start_ = 0;
        source_end_ = source.length();

        reset();
    }

    Lexer::Lexer(Lexer* parent, int source_start, int source_end) {
        source_ = parent->source_;

        source_owner = false;
        source_start_ = source_start;
        source_end_ = source_end;

        reset();
    }

    Lexer::~Lexer() {
        if (source_owner)
            delete[] source_;
    }

    void Lexer::reset() {
        c_source_position_ = source_start_;

        c_token_start = 0;
        c_token_end = 0;
        token_last_end = 0;

        c_token_kind = TokenKind::EOS;
        c_token_value = "";

        get_next_char();
        get_next_char();

        parse_next_token();
    }

    void Lexer::expect_and_get_next(TokenKind expected_kind) {
        if (c_token_kind != expected_kind) {
            std::ostringstream msg;
            msg << "Expected " << Token::get_token_kind_as_string(expected_kind)
                << ", got " << Token::get_token_kind_as_string(c_token_kind)
                << " at " << Token::get_position_info(source_, source_end_, c_token_start);

            throw LexerException(msg.str());
        }

        parse_next_token();
    }

    void Lexer::get_next_char() {
        c_char = n_char;

        if (c_source_position_ < source_end_) {
            n_char = source_[c_source_position_];
        }
        else {
            n_char = 0;
        }

        ++c_source_position_;
    }

    void Lexer::parse_next_token() {
        c_token_kind = TokenKind::EOS;
        c_token_value = "";

        while (c_char && Util::is_white_space(c_char))
            get_next_char();

        if (c_char == '/' && n_char == '/') {
            process_inline_comment();

            return;
        }

        if (c_char == '/' && n_char == '*') {
            process_multiline_comment();

            return;
        }

        c_token_start = c_source_position_ - 2;

        if (Util::is_alpha(c_char)) {
            process_identifier();
        }
        else if (Util::is_digit(c_char)) {
            process_number();
        }
        else if (c_char == '"') {
            process_double_quote_string_literal();
        }
        else if (c_char == '\'') {
            process_single_quote_string_literal();
        }
        else {
            process_punctuators();
        }
    }

    void Lexer::process_inline_comment() {
        while (c_char && c_char != '\n')
            get_next_char();

        get_next_char();

        parse_next_token();
    }

    void Lexer::process_multiline_comment() {
        while (c_char && !(c_char == '*' && n_char == '/'))
            get_next_char();

        get_next_char();
        get_next_char();

        parse_next_token();
    }

    void Lexer::process_identifier() {
        while (Util::is_alpha(c_char) || Util::is_digit(c_char)) {
            c_token_value += c_char;
            get_next_char();
        }

        c_token_kind = TokenKind::IDENTIFIER;

        check_for_reserved_keywords();
    }

    void Lexer::process_number() {
        bool is_hex = false;

        if (c_char == '0') {
            c_token_value += c_char;
            get_next_char();
        }
        if (c_char == 'x') {
            is_hex = true;
            c_token_value += c_char;
            get_next_char();
        }

        c_token_kind = TokenKind::INTEGER_L;

        while (Util::is_digit(c_char) || (is_hex && Util::is_hex(c_char))) {
            c_token_value += c_char;
            get_next_char();
        }

        if (!is_hex && c_char == '.') {
            c_token_kind = TokenKind::FLOAT_L;
            c_token_value += c_char;
            get_next_char();

            while (Util::is_digit(c_char)) {
                c_token_value += c_char;
                get_next_char();
            }
        }

        if (!is_hex && (c_char == 'e' || c_char == 'E')) {
            c_token_kind = TokenKind::FLOAT_L;
            c_token_value += c_char;
            get_next_char();

            while (Util::is_digit(c_char)) {
                c_token_value += c_char;
                get_next_char();
            }
        }
    }

    void Lexer::check_for_reserved_keywords() {
        if (c_token_value == "await") {
            c_token_kind = TokenKind::AWAIT_K;
        }
        else if (c_token_value == "break") {
            c_token_kind = TokenKind::BREAK_K;
        }
        else if (c_token_value == "case") {
            c_token_kind = TokenKind::CASE_K;
        }
        else if (c_token_value == "catch") {
            c_token_kind = TokenKind::CATCH_K;
        }
        else if (c_token_value == "class") {
            c_token_kind = TokenKind::CLASS_K;
        }
        else if (c_token_value == "const") {
            c_token_kind = TokenKind::CONST_K;
        }
        else if (c_token_value == "continue") {
            c_token_kind = TokenKind::CONTINUE_K;
        }
        else if (c_token_value == "debugger") {
            c_token_kind = TokenKind::DEBUGGER_K;
        }
        else if (c_token_value == "default") {
            c_token_kind = TokenKind::DEFAULT_K;
        }
        else if (c_token_value == "delete") {
            c_token_kind = TokenKind::DELETE_K;
        }
        else if (c_token_value == "do") {
            c_token_kind = TokenKind::DO_K;
        }
        else if (c_token_value == "else") {
            c_token_kind = TokenKind::ELSE_K;
        }
        else if (c_token_value == "export") {
            c_token_kind = TokenKind::EXPORT_K;
        }
        else if (c_token_value == "extends") {
            c_token_kind = TokenKind::EXTENDS_K;
        }
        else if (c_token_value == "finally") {
            c_token_kind = TokenKind::FINALLY_K;
        }
        else if (c_token_value == "for") {
            c_token_kind = TokenKind::FOR_K;
        }
        else if (c_token_value == "function") {
            c_token_kind = TokenKind::FUNCTION_K;
        }
        else if (c_token_value == "if") {
            c_token_kind = TokenKind::IF_K;
        }
        else if (c_token_value == "import") {
            c_token_kind = TokenKind::IMPORT_K;
        }
        else if (c_token_value == "in") {
            c_token_kind = TokenKind::IN_K;
        }
        else if (c_token_value == "instanceof") {
            c_token_kind = TokenKind::INSTANCEOF_K;
        }
        else if (c_token_value == "let") {
            c_token_kind = TokenKind::LET_K;
        }
        else if (c_token_value == "new") {
            c_token_kind = TokenKind::NEW_K;
        }
        else if (c_token_value == "return") {
            c_token_kind = TokenKind::RETURN_K;
        }
        else if (c_token_value == "static") {
            c_token_kind = TokenKind::STATIC_K;
        }
        else if (c_token_value == "super") {
            c_token_kind = TokenKind::SUPER_K;
        }
        else if (c_token_value == "switch") {
            c_token_kind = TokenKind::SWITCH_K;
        }
        else if (c_token_value == "this") {
            c_token_kind = TokenKind::THIS_K;
        }
        else if (c_token_value == "throw") {
            c_token_kind = TokenKind::THROW_K;
        }
        else if (c_token_value == "try") {
            c_token_kind = TokenKind::TRY_K;
        }
        else if (c_token_value == "typeof") {
            c_token_kind = TokenKind::TYPEOF_K;
        }
        else if (c_token_value == "var") {
            c_token_kind = TokenKind::VAR_K;
        }
        else if (c_token_value == "void") {
            c_token_kind = TokenKind::VOID_K;
        }
        else if (c_token_value == "while") {
            c_token_kind = TokenKind::WHILE_K;
        }
        else if (c_token_value == "with") {
            c_token_kind = TokenKind::WITH_K;
        }
        else if (c_token_value == "yield") {
            c_token_kind = TokenKind::YIELD_K;
        }
    }

    void Lexer::process_double_quote_string_literal() {
        get_next_char();

        while (c_char && c_char != '"') {
            if (c_char == '\\') {
                get_next_char();

                switch (c_char) {
                case 'n':
                    c_token_value += '\n';
                    break;
                case '"':
                    c_token_value += '"';
                    break;
                case '\\':
                    c_token_value += '\\';
                    break;
                default:
                    c_token_value += c_char;
                }
            }
            else {
                c_token_value += c_char;
            }

            get_next_char();
        }

        get_next_char();

        c_token_kind = TokenKind::STRING_L;
    }

    void Lexer::process_single_quote_string_literal() {
        get_next_char();

        while (c_char && c_char != '\'') {
            if (c_char == '\\') {
                get_next_char();

                switch (c_char) {
                case 'n':
                    c_token_value += '\n';
                    break;
                case 'a':
                    c_token_value += '\a';
                    break;
                case 'r':
                    c_token_value += '\r';
                    break;
                case 't':
                    c_token_value += '\t';
                    break;
                case '\'':
                    c_token_value += '\'';
                    break;
                case '\\':
                    c_token_value += '\\';
                    break;
                case 'x': {
                    char buf[3] = "??";

                    get_next_char();
                    buf[0] = c_char;
                    get_next_char();
                    buf[1] = c_char;

                    c_token_value += (char)strtol(buf, 0, 16);

                    break;
                }
                default: {
                    if (c_char >= '0' && c_char <= '7') {
                        char buf[4] = "???";

                        buf[0] = c_char;
                        get_next_char();
                        buf[1] = c_char;
                        get_next_char();
                        buf[2] = c_char;

                        c_token_value += (char)strtol(buf, 0, 8);
                    }
                    else {
                        c_token_start += c_char;
                    }
                }
                }
            }
            else {
                c_token_value += c_char;
            }

            get_next_char();
        }

        get_next_char();

        c_token_kind = TokenKind::STRING_L;
    }

    void Lexer::process_punctuators() {
        char p_char = c_char;

        if (c_char)
            get_next_char();

        switch (p_char) {
        case '[':
            c_token_kind = TokenKind::LBRACK_P;
            break;
        case '(':
            c_token_kind = TokenKind::LPAREN_P;
            break;
        case '{':
            c_token_kind = TokenKind::LBRACE_P;
            break;
        case ']':
            c_token_kind = TokenKind::RBRACK_P;
            break;
        case ')':
            c_token_kind = TokenKind::RPAREN_P;
            break;
        case '}':
            c_token_kind = TokenKind::RBRACE_P;
            break;
        case '.':
            c_token_kind = TokenKind::PERIOD_P;
            break; // Check for ellipsis (...)
        case ':':
            c_token_kind = TokenKind::COLON_P;
            break;
        case ';':
            c_token_kind = TokenKind::SEMICOLON_P;
            break;
        case ',':
            c_token_kind = TokenKind::COMMA_P;
            break;
        case '<':
            c_token_kind = TokenKind::LT_P;
            break; // Check for <=, <<, <<=
        case '>':
            c_token_kind = TokenKind::GT_P;
            break; // Check for >=, >>, >>>, >>=, >>>=
        case '=':
            c_token_kind = TokenKind::ASSIGN_P;
            break; // Check for ==, ===, =>
        case '!':
            c_token_kind = TokenKind::NOT_P;
            break; // Check for !=, !==
        case '+':
            c_token_kind = TokenKind::ADD_P;
            break; // Check for ++, +=
        case '-':
            c_token_kind = TokenKind::SUB_P;
            break; // Check for --, -=
        case '*':
            c_token_kind = TokenKind::MUL_P;
            break; // Check for *, *=, **=
        case '%':
            c_token_kind = TokenKind::MOD_P;
            break; // Check for %=
        case '&':
            c_token_kind = TokenKind::BIT_AND_P;
            break; // Check for &&, &=
        case '|':
            c_token_kind = TokenKind::BIT_OR_P;
            break; // Check for ||, |=
        case '^':
            c_token_kind = TokenKind::BIT_XOR_P;
            break; // Check for ^=
        case '~':
            c_token_kind = TokenKind::BIT_NOT_P;
            break;
        case '?':
            c_token_kind = TokenKind::CONDITIONAL_P;
            break;
        }
    }
}
