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
        if (lex_->c_token_kind == TokenKind::IDENTIFIER || lex_->c_token_kind == TokenKind::INTEGER_L
            || lex_->c_token_kind == TokenKind::FLOAT_L || lex_->c_token_kind == TokenKind::STRING_L
            || lex_->c_token_kind == TokenKind::MINUS_P) {

            CLEAN_VAR_REFERENCE(process_base_command(can_execute));

            lex_->expect_and_get_next(TokenKind::SEMICOLON_P);
        }
        else if (lex_->c_token_kind == TokenKind::LBRACE_P) {
            process_block(can_execute);
        }
        else if (lex_->c_token_kind == TokenKind::SEMICOLON_P) {
            lex_->parse_next_token();
        }
        else if (lex_->c_token_kind == TokenKind::VAR_K) {
            lex_->parse_next_token();

            while (lex_->c_token_kind != TokenKind::SEMICOLON_P) {
                VariableReference* v = nullptr;

                if (can_execute)
                    v = root_->find_child_or_create(lex_->get_token_value());

                lex_->expect_and_get_next(TokenKind::IDENTIFIER);

                while (lex_->get_current_token() == TokenKind::PERIOD_P) {
                    lex_->parse_next_token();

                    if (can_execute) {
                        VariableReference* last_v = v;
                        v = last_v->var->find_child_or_create(lex_->get_token_value());
                    }

                    lex_->expect_and_get_next(TokenKind::IDENTIFIER);
                }

                if (lex_->get_current_token() == TokenKind::ASSIGN_P) {
                    lex_->parse_next_token();

                    VariableReference* v_2 = process_base_command(can_execute);

                    if (can_execute)
                        v->replace_with(v_2);

                    CLEAN_VAR_REFERENCE(v_2);
                }

                if (lex_->get_current_token() != TokenKind::SEMICOLON_P) {
                    lex_->expect_and_get_next(TokenKind::COMMA_P);
                }
            }

            lex_->expect_and_get_next(TokenKind::SEMICOLON_P);
        }
        else if (lex_->c_token_kind == TokenKind::IF_K) {
            lex_->parse_next_token();

            lex_->expect_and_get_next(TokenKind::LPAREN_P);
            VariableReference* ref = process_base_command(can_execute);
            lex_->expect_and_get_next(TokenKind::RPAREN_P);

            bool condition_met = can_execute && ref->var->get_bool();
            CLEAN_VAR_REFERENCE(ref);

            bool no_execute = false;

            process_statement(condition_met ? can_execute : no_execute);

            if (lex_->c_token_kind == TokenKind::ELSE_K) {
                lex_->parse_next_token();

                process_statement(condition_met ? no_execute : can_execute);
            }
        }
        else if (lex_->c_token_kind == TokenKind::WHILE_K) {
            lex_->parse_next_token();

            lex_->expect_and_get_next(TokenKind::LPAREN_P);
            int while_condition_start = lex_->c_token_start;

            bool no_execute = false;
            VariableReference* condition = process_base_command(can_execute);
            bool loop_condition = can_execute && condition->var->get_bool();
            CLEAN_VAR_REFERENCE(condition);

            Lexer* while_cond_lex = lex_->get_sub_lex(while_condition_start);
            lex_->expect_and_get_next(TokenKind::RPAREN_P);
            int while_body_start = lex_->c_token_start;

            process_statement(loop_condition ? can_execute : no_execute);
            Lexer* while_body_lex = lex_->get_sub_lex(while_body_start);
            Lexer* old_lex = lex_;

            while (loop_condition) {
                while_cond_lex->reset();
                lex_ = while_cond_lex;

                condition = process_base_command(can_execute);

                loop_condition = can_execute && condition->var->get_bool();
                CLEAN_VAR_REFERENCE(condition);

                if (loop_condition) {
                    while_body_lex->reset();
                    lex_ = while_body_lex;

                    process_statement(can_execute);
                }
            }

            lex_ = old_lex;
            delete while_cond_lex;
            delete while_body_lex;
        }
        else if (lex_->c_token_kind == TokenKind::FOR_K) {
            lex_->parse_next_token();
            lex_->expect_and_get_next(TokenKind::LPAREN_P);

            process_statement(can_execute);

            int for_condition_start = lex_->c_token_start;
            bool no_execute = false;

            VariableReference* condition = process_base_command(can_execute);
            bool loop_condition = can_execute && condition->var->get_bool();
            CLEAN_VAR_REFERENCE(condition);

            Lexer* for_condition_lex = lex_->get_sub_lex(for_condition_start);
            lex_->expect_and_get_next(TokenKind::SEMICOLON_P);

            int for_iterator_start = lex_->c_token_start;
            CLEAN_VAR_REFERENCE(process_base_command(no_execute));

            Lexer* for_iterator_lex = lex_->get_sub_lex(for_iterator_start);
            lex_->expect_and_get_next(TokenKind::RPAREN_P);

            int for_body_start = lex_->c_token_start;

            process_statement(loop_condition ? can_execute : no_execute);

            Lexer* for_body_lex = lex_->get_sub_lex(for_body_start);
            Lexer* old_lex = lex_;

            if (loop_condition) {
                for_iterator_lex->reset();
                lex_ = for_iterator_lex;

                CLEAN_VAR_REFERENCE(process_base_command(can_execute));
            }

            while (can_execute && loop_condition) {
                for_condition_lex->reset();
                lex_ = for_condition_lex;

                condition = process_base_command(can_execute);
                loop_condition = condition->var->get_bool();
                CLEAN_VAR_REFERENCE(condition);

                if (can_execute && loop_condition) {
                    for_body_lex->reset();
                    lex_ = for_body_lex;

                    process_statement(can_execute);
                }

                if (can_execute && loop_condition) {
                    for_iterator_lex->reset();
                    lex_ = for_iterator_lex;

                    CLEAN_VAR_REFERENCE(process_base_command(can_execute));
                }
            }

            lex_ = old_lex;
            delete for_condition_lex;
            delete for_iterator_lex;
            delete for_body_lex;
        }
        else if (lex_->c_token_kind == TokenKind::RETURN_K) {
            lex_->parse_next_token();
            VariableReference* result = nullptr;

            if (lex_->c_token_kind != TokenKind::SEMICOLON_P)
                result = process_base_command(can_execute);

            if (can_execute) {
                VariableReference* result_var = root_->find_child("return"); // TODO: Scoping
                if (result_var) {
                    result_var->replace_with(result);
                }
                else {
                    throw DeltaScriptException("Return statement is not inside function scope");
                }

                can_execute = false;
            }

            CLEAN_VAR_REFERENCE(result);

            lex_->expect_and_get_next(TokenKind::SEMICOLON_P);
        }
        else if (lex_->c_token_kind == TokenKind::FUNCTION_K) {
            VariableReference* function_var = parse_function_definition();

            if (can_execute) {
                if (function_var->name == "") {
                    throw DeltaScriptException("Functions defined at statement-level are meant to have a name");
                }
                else {
                    scopes.back()->add_child_no_dup(function_var->name, function_var->var);
                }

                CLEAN_VAR_REFERENCE(function_var);
            }
        }
        else { // TODO: Other reserved words
            lex_->expect_and_get_next(TokenKind::EOS);
        }
    }
}  // namespace DeltaScript
