#include <DeltaScript/DeltaScript.h>
#include <sstream>

namespace DeltaScript {
    std::string Token::get_position_info(const char* source, size_t source_length, int position) {
        int line = 1, column = 1;
        char c;

        for (size_t i = 0; i < position; ++i) {
            if (i < source_length) {
                c = source[i];
            }
            else {
                c = 0;
            }

            ++column;

            if (Util::is_line_terminator(c)) {
                ++line;
                column = 0;

                if ((i + 1) < source_length && Util::is_line_terminator_crlf(c, source[i + 1]))
                    ++i;
            }
        }

        std::ostringstream buf;
        buf << "(line: " << line << ", column: " << column << ")";

        return buf.str();
    }

    std::string Token::get_token_kind_as_string(TokenKind kind) {
        switch (kind) {
        case TokenKind::EOS:
            return "<EOS>";
        case TokenKind::IDENTIFIER:
            return "Identifier";
        case TokenKind::AWAIT_K:
            return "await";
        case TokenKind::BREAK_K:
            return "break";
        case TokenKind::CASE_K:
            return "case";
        case TokenKind::CATCH_K:
            return "catch";
        case TokenKind::CLASS_K:
            return "class";
        case TokenKind::CONST_K:
            return "const";
        case TokenKind::CONTINUE_K:
            return "continue";
        case TokenKind::DEBUGGER_K:
            return "debugger";
        case TokenKind::DEFAULT_K:
            return "default";
        case TokenKind::DELETE_K:
            return "delete";
        case TokenKind::DO_K:
            return "do";
        case TokenKind::ELSE_K:
            return "else";
        case TokenKind::EXPORT_K:
            return "export";
        case TokenKind::EXTENDS_K:
            return "extends";
        case TokenKind::FINALLY_K:
            return "finally";
        case TokenKind::FOR_K:
            return "for";
        case TokenKind::FUNCTION_K:
            return "function";
        case TokenKind::IF_K:
            return "if";
        case TokenKind::IMPORT_K:
            return "import";
        case TokenKind::IN_K:
            return "in";
        case TokenKind::INSTANCEOF_K:
            return "instanceof";
        case TokenKind::LET_K:
            return "let";
        case TokenKind::NEW_K:
            return "new";
        case TokenKind::RETURN_K:
            return "return";
        case TokenKind::STATIC_K:
            return "static";
        case TokenKind::SUPER_K:
            return "super";
        case TokenKind::SWITCH_K:
            return "switch";
        case TokenKind::THIS_K:
            return "this";
        case TokenKind::THROW_K:
            return "throw";
        case TokenKind::TRY_K:
            return "try";
        case TokenKind::TYPEOF_K:
            return "typeof";
        case TokenKind::UNDEFINED_K:
            return "undefined";
        case TokenKind::VAR_K:
            return "var";
        case TokenKind::VOID_K:
            return "void";
        case TokenKind::WHILE_K:
            return "while";
        case TokenKind::WITH_K:
            return "with";
        case TokenKind::YIELD_K:
            return "yield";
        case TokenKind::LBRACK_P:
            return "'['";
        case TokenKind::LPAREN_P:
            return "'('";
        case TokenKind::LBRACE_P:
            return "'{'";
        case TokenKind::RBRACK_P:
            return "']'";
        case TokenKind::RPAREN_P:
            return "')'";
        case TokenKind::RBRACE_P:
            return "'}'";
        case TokenKind::PERIOD_P:
            return "'.'";
        case TokenKind::ELLIPSIS_P:
            return "'...'";
        case TokenKind::COLON_P:
            return "':'";
        case TokenKind::SEMICOLON_P:
            return "';'";
        case TokenKind::COMMA_P:
            return "','";
        case TokenKind::LT_P:
            return "'<'";
        case TokenKind::GT_P:
            return "'>'";
        case TokenKind::LTE_P:
            return "'<='";
        case TokenKind::GTE_P:
            return "'>='";
        case TokenKind::ASSIGN_P:
            return "'='";
        case TokenKind::EQUAL_P:
            return "'=='";
        case TokenKind::NEQUAL_P:
            return "'!='";
        case TokenKind::STRICT_EQUAL_P:
            return "'==='";
        case TokenKind::STRICT_NEQUAL_P:
            return "'!=='";
        case TokenKind::PLUS_P:
            return "'+'";
        case TokenKind::MINUS_P:
            return "'-'";
        case TokenKind::MUL_P:
            return "'*'";
        case TokenKind::DIV_P:
            return "'/'";
        case TokenKind::EXP_P:
            return "'**'";
        case TokenKind::MOD_P:
            return "'%'";
        case TokenKind::INCR_P:
            return "'++'";
        case TokenKind::DECR_P:
            return "'--'";
        case TokenKind::SHFT_L_P:
            return "'<<'";
        case TokenKind::SHFT_R_P:
            return "'>>'";
        case TokenKind::SHFT_RR_P:
            return "'>>>'";
        case TokenKind::BIT_AND_P:
            return "'&'";
        case TokenKind::BIT_OR_P:
            return "'|'";
        case TokenKind::BIT_XOR_P:
            return "'^'";
        case TokenKind::BIT_NOT_P:
            return "'~'";
        case TokenKind::AND_P:
            return "'&&'";
        case TokenKind::OR_P:
            return "'||'";
        case TokenKind::NOT_P:
            return "'!'";
        case TokenKind::CONDITIONAL_P:
            return "'?'";
        case TokenKind::PLUS_EQ_P:
            return "'+='";
        case TokenKind::MINUS_EQ_P:
            return "'-='";
        case TokenKind::MUL_EQ_P:
            return "'*='";
        case TokenKind::MOD_EQ_P:
            return "'%='";
        case TokenKind::EXP_EQ_P:
            return "'**='";
        case TokenKind::SHFT_L_EQ_P:
            return "'<<='";
        case TokenKind::SHFT_R_EQ_P:
            return "'>>='";
        case TokenKind::SHFT_RR_EQ_P:
            return "'>>>='";
        case TokenKind::BIT_AND_EQ_P:
            return "'&='";
        case TokenKind::BIT_OR_EQ_P:
            return "'|='";
        case TokenKind::BIT_XOR_EQ_P:
            return "'^='";
        case TokenKind::ARROW_P:
            return "'=>'";
        case TokenKind::NULL_L:
            return "null";
        case TokenKind::TRUE_L:
            return "true";
        case TokenKind::FALSE_L:
            return "false";
        case TokenKind::INTEGER_L:
            return "integer value";
        case TokenKind::FLOAT_L:
            return "float value";
        case TokenKind::STRING_L:
            return "string value";
        }

        return "<unknown token>";
    }
}  // namespace DeltaScript
