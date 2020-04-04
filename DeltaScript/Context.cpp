#include <DeltaScript/DeltaScript.h>
#include <sstream>

namespace DeltaScript {
    void Context::execute(const std::string& script) {
        Lexer* old_lex = lex_;

        lex_ = new Lexer(script);

        try {
            bool can_execute = true;
            while (lex_->get_current_token() != TokenKind::EOS)
                process_statement(can_execute);
        }
        catch (const DeltaScriptException & e) {
            // TODO: Add call stack details

            delete lex_;
            lex_ = old_lex;

            throw e;
        }

        delete lex_;
        lex_ = old_lex;
    }

    VariableReference* Context::process_base_command(bool& can_execute) {
        return nullptr;
    }

    void Context::process_block(bool& can_execute) {

    }

    void Context::process_statement(bool& can_execute) {
        if (lex_->c_token_kind == TokenKind::IDENTIFIER
            || lex_->c_token_kind == TokenKind::INTEGER_L
            || lex_->c_token_kind == TokenKind::FLOAT_L
            || lex_->c_token_kind == TokenKind::STRING_L
            || lex_->c_token_kind == TokenKind::MINUS_P) {
            CLEAN_VAR_REFERENCE(process_base_command(can_execute));
            lex_->expect_and_get_next(TokenKind::SEMICOLON_P);
        }
        else if (lex_->c_token_kind == TokenKind::LBRACE_P) {
            process_block(can_execute);
        }
        else if (lex_->c_token_kind == TokenKind::SEMICOLON_P) {
            lex_->expect_and_get_next(TokenKind::SEMICOLON_P);
        }
        else if (lex_->c_token_kind == TokenKind::VAR_K) {
            lex_->expect_and_get_next(TokenKind::VAR_K);

            while (lex_->c_token_kind != TokenKind::SEMICOLON_P) {
                VariableReference* v = nullptr;

                if (can_execute)
                    v = root_->find_child_or_create(lex_->get_token_value());

                lex_->expect_and_get_next(TokenKind::IDENTIFIER);

                while (lex_->get_current_token() == TokenKind::PERIOD_P) {
                    lex_->expect_and_get_next(TokenKind::PERIOD_P);

                    if (can_execute) {
                        VariableReference* last_v = v;
                        v = last_v->var->find_child_or_create(lex_->get_token_value());
                    }

                    lex_->expect_and_get_next(TokenKind::IDENTIFIER);
                }

                if (lex_->get_current_token() == TokenKind::ASSIGN_P) {
                    lex_->expect_and_get_next(TokenKind::ASSIGN_P);

                    VariableReference* v_2 = process_base_command(can_execute);

                    if (can_execute)
                        v->replace_with(v_2);

                    CLEAN_VAR_REFERENCE(v_2);
                }

                if (lex_->get_current_token() != TokenKind::SEMICOLON_P) {
                    lex_->expect_and_get_next(TokenKind::COMMA_P);
                }
            }
        }
    }
}  // namespace DeltaScript
