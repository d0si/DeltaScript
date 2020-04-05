#include <DeltaScript/DeltaScript.h>
#include <sstream>

namespace DeltaScript {
    Context::Context() {
        lex_ = nullptr;
        root_ = (new Variable("", Variable::VariableFlags::OBJECT))->inc_ref();
    }

    Context::~Context() {
        scopes_.clear();
        root_->unref();
    }

    void Context::execute(const std::string& script) {
        Lexer* old_lex = lex_;
        std::vector<Variable*> old_scopes = scopes_;

        lex_ = new Lexer(script);
        scopes_.clear();
        scopes_.push_back(root_);

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
        scopes_ = old_scopes;
    }

    void Context::add_native_function(const std::string& function_definition, NativeCallback callback, void* data) {
        Lexer* old_lex = lex_;
        lex_ = new Lexer(function_definition);
        Variable* function_base = root_;

        lex_->expect_and_get_next(TokenKind::FUNCTION_K);
        std::string function_name = lex_->get_token_value();
        lex_->expect_and_get_next(TokenKind::IDENTIFIER);

        while (lex_->c_token_kind == TokenKind::PERIOD_P) {
            lex_->parse_next_token();

            VariableReference* ref = function_base->find_child(function_name);

            if (!ref)
                ref = function_base->add_child(function_name, new Variable("", Variable::VariableFlags::OBJECT));

            function_base = ref->var;
            function_name = lex_->get_token_value();

            lex_->expect_and_get_next(TokenKind::IDENTIFIER);
        }

        Variable* function_var = new Variable("", Variable::VariableFlags::FUNCTION | Variable::VariableFlags::NATIVE);
        function_var->set_native_callback(callback, data);
        parse_function_arguments(function_var);

        delete lex_;
        lex_ = old_lex;

        function_base->add_child(function_name, function_var);
    }

    VariableReference* Context::process_function_call(bool& can_execute, VariableReference* function, Variable* parent) {
        if (can_execute) {
            if (!function->var->is_function()) {
                std::stringstream msg;
                msg << "Expecting '" << function->name << "' to be a function";

                throw DeltaScriptException(msg.str());
            }

            lex_->expect_and_get_next(TokenKind::LPAREN_P);

            Variable* function_root = new Variable("", Variable::VariableFlags::FUNCTION);

            if (parent)
                function_root->add_child("this", parent);

            VariableReference* v = function->var->first_child_;

            while (v) {
                VariableReference* value = process_base(can_execute);

                if (can_execute) {
                    if (value->var->is_basic()) {
                        function_root->add_child(v->name, value->var->deep_copy());
                    }
                    else {
                        function_root->add_child(v->name, value->var);
                    }
                }

                CLEAN_VAR_REFERENCE(value);

                if (lex_->c_token_kind != TokenKind::RPAREN_P)
                    lex_->expect_and_get_next(TokenKind::COMMA_P);

                v = v->next_sibling;
            }

            lex_->expect_and_get_next(TokenKind::RPAREN_P);

            VariableReference* return_var = nullptr;
            VariableReference* return_var_ref = function_root->add_child("return");

            scopes_.push_back(function_root);

            if (function->var->is_native()) {
                if (function->var->native_callback_ == nullptr)
                    throw DeltaScriptException("Tried to execute native function without callback handle");

                function->var->native_callback_(function_root, function->var->native_callback_data_);
                function->var->increase_execution_count();
            }
            else {
                Lexer* old_lex = lex_;
                Lexer* new_lex = new Lexer(function->var->get_string());

                lex_ = new_lex;

                try {
                    process_block(can_execute);

                    can_execute = true;

                    function->var->increase_execution_count();
                }
                catch (DeltaScriptException & e) {
                    delete new_lex;
                    lex_ = old_lex;

                    throw e;
                }

                delete new_lex;
                lex_ = old_lex;
            }

            scopes_.pop_back();

            return_var = new VariableReference(return_var_ref->var);
            function_root->remove_reference(return_var_ref);
            delete function_root;

            if (return_var) {
                return return_var;
            }
            else {
                return new VariableReference(new Variable());
            }
        }
        else {
            lex_->expect_and_get_next(TokenKind::LPAREN_P);

            while (lex_->c_token_kind != TokenKind::RPAREN_P) {
                CLEAN_VAR_REFERENCE(process_base(can_execute));

                if (lex_->c_token_kind != TokenKind::RPAREN_P)
                    lex_->expect_and_get_next(TokenKind::COMMA_P);
            }

            lex_->expect_and_get_next(TokenKind::RPAREN_P);

            if (lex_->c_token_kind == TokenKind::LBRACE_P) {
                process_block(can_execute);
            }

            return function;
        }
    }

    VariableReference* Context::process_factor(bool& can_execute) {
        if (lex_->c_token_kind == TokenKind::LPAREN_P) {
            lex_->parse_next_token();

            VariableReference* a = process_base(can_execute);
            lex_->expect_and_get_next(TokenKind::RPAREN_P);

            return a;
        }
        else if (lex_->c_token_kind == TokenKind::TRUE_L) {
            lex_->parse_next_token();

            return new VariableReference(new Variable(1));
        }
        else if (lex_->c_token_kind == TokenKind::FALSE_L) {
            lex_->parse_next_token();

            return new VariableReference(new Variable(0));
        }
        else if (lex_->c_token_kind == TokenKind::NULL_L) {
            lex_->parse_next_token();

            return new VariableReference(new Variable("", Variable::VariableFlags::NULL_));
        }
        else if (lex_->c_token_kind == TokenKind::UNDEFINED_K) {
            lex_->parse_next_token();

            return new VariableReference(new Variable("", Variable::VariableFlags::UNDEFINED));
        }
        else if (lex_->c_token_kind == TokenKind::IDENTIFIER) {
            VariableReference* a = can_execute ? find_var_in_scopes(lex_->get_token_value()) : new VariableReference(new Variable());

            Variable* parent = nullptr;

            if (can_execute && !a)
                a = new VariableReference(new Variable(), lex_->get_token_value());

            int token_start = lex_->c_token_start;

            // TODO: Handle reserved keywords somehow
            lex_->expect_and_get_next(TokenKind::IDENTIFIER);

            while (lex_->c_token_kind == TokenKind::LPAREN_P || lex_->c_token_kind == TokenKind::PERIOD_P || lex_->c_token_kind == TokenKind::LBRACK_P) {
                if (lex_->c_token_kind == TokenKind::LPAREN_P) {
                    a = process_function_call(can_execute, a, parent);
                }
                else if (lex_->c_token_kind == TokenKind::PERIOD_P) {
                    lex_->parse_next_token();

                    if (can_execute) {
                        const std::string& name = lex_->get_token_value();
                        VariableReference* child = a->var->find_child(name);

                        if (!child)
                            child = find_var_in_parent_classes(a->var, name);

                        if (!child)
                            child = a->var->add_child(name);

                        parent = a->var;
                        a = child;
                    }

                    lex_->expect_and_get_next(TokenKind::IDENTIFIER);
                }
                else if (lex_->c_token_kind == TokenKind::LBRACK_P) {
                    lex_->parse_next_token();

                    VariableReference* index = process_base(can_execute);
                    lex_->expect_and_get_next(TokenKind::RBRACK_P);

                    if (can_execute) {
                        VariableReference* child = a->var->find_child_or_create(index->var->get_string());

                        parent = a->var;
                        a = child;
                    }

                    CLEAN_VAR_REFERENCE(index);
                }
            }

            return a;
        }
        else if (lex_->c_token_kind == TokenKind::INTEGER_L || lex_->c_token_kind == TokenKind::FLOAT_L) {
            Variable* a = new Variable(lex_->get_token_value(), lex_->c_token_kind == TokenKind::INTEGER_L ?
                Variable::VariableFlags::INTEGER : Variable::VariableFlags::DOUBLE);
            lex_->parse_next_token();

            return new VariableReference(a);
        }
        else if (lex_->c_token_kind == TokenKind::STRING_L) {
            Variable* a = new Variable(lex_->get_token_value(), Variable::VariableFlags::STRING);
            lex_->parse_next_token();

            return new VariableReference(a);
        }
        else if (lex_->c_token_kind == TokenKind::LBRACE_P) {
            // TODO: Create object
            return new VariableReference(new Variable());
        }
        else if (lex_->c_token_kind == TokenKind::LBRACK_P) {
            // TODO: Create array
            return new VariableReference(new Variable());
        }
        else if (lex_->c_token_kind == TokenKind::FUNCTION_K) {
            VariableReference* func_ref = parse_function_definition();

            if (func_ref->name != "")
                throw DeltaScriptException("Functions not defined at statement-level are not meant to have a name");

            return func_ref;
        }

        lex_->expect_and_get_next(TokenKind::EOS);
        return nullptr;
    }

    VariableReference* Context::process_unary(bool& can_execute) {
        VariableReference* a;

        if (lex_->c_token_kind == TokenKind::NOT_P) {
            lex_->parse_next_token();

            a = process_factor(can_execute);

            if (can_execute) {
                Variable zero(0);
                Variable* result = a->var->execute_math_operation(&zero, TokenKind::EQUAL_P);

                CREATE_REFERENCE(a, result);
            }
        }
        else {
            a = process_factor(can_execute);
        }

        return a;
    }

    VariableReference* Context::process_term(bool& can_execute) {
        VariableReference* a = process_unary(can_execute);

        while (lex_->c_token_kind == TokenKind::MUL_P || lex_->c_token_kind == TokenKind::DIV_P
            || lex_->c_token_kind == TokenKind::MOD_P) {
            TokenKind operation = lex_->c_token_kind;
            lex_->parse_next_token();

            VariableReference* b = process_unary(can_execute);

            if (can_execute) {
                Variable* result = a->var->execute_math_operation(b->var, operation);
                CREATE_REFERENCE(a, result);
            }

            CLEAN_VAR_REFERENCE(b);
        }

        return a;
    }

    VariableReference* Context::process_expression(bool& can_execute) {
        bool negate = false;

        if (lex_->c_token_kind == TokenKind::MINUS_P) {
            lex_->parse_next_token();
            negate = true;
        }

        VariableReference* a = process_term(can_execute);

        if (negate) {
            Variable zero(0);
            Variable* result = zero.execute_math_operation(a->var, TokenKind::MINUS_P);

            CREATE_REFERENCE(a, result);
        }

        while (lex_->c_token_kind == TokenKind::PLUS_P || lex_->c_token_kind == TokenKind::MINUS_P
            || lex_->c_token_kind == TokenKind::INCR_P || lex_->c_token_kind == TokenKind::DECR_P) {
            TokenKind operation = lex_->c_token_kind;
            lex_->parse_next_token();

            if (operation == TokenKind::INCR_P || operation == TokenKind::DECR_P) {
                if (can_execute) {
                    Variable one(1);
                    Variable* result = a->var->execute_math_operation(&one, (operation == TokenKind::INCR_P) ? TokenKind::PLUS_P : TokenKind::MINUS_P);
                    VariableReference* old_value = new VariableReference(a->var);

                    a->replace_with(result);
                    CLEAN_VAR_REFERENCE(a);
                    a = old_value;
                }
            }
            else {
                VariableReference* b = process_term(can_execute);

                if (can_execute) {
                    Variable* result = a->var->execute_math_operation(b->var, operation);
                    CREATE_REFERENCE(a, result);
                }

                CLEAN_VAR_REFERENCE(b);
            }
        }

        return a;
    }

    VariableReference* Context::process_shift(bool& can_execute) {
        VariableReference* a = process_expression(can_execute);

        if (lex_->c_token_kind == TokenKind::SHFT_L_P || lex_->c_token_kind == TokenKind::SHFT_R_P
            || lex_->c_token_kind == TokenKind::SHFT_RR_P) {
            TokenKind operation = lex_->c_token_kind;
            lex_->parse_next_token();

            VariableReference* b = process_base(can_execute);
            int shift = can_execute ? b->var->get_int() : 0;
            CLEAN_VAR_REFERENCE(b);

            if (can_execute) {
                switch (operation) {
                case DeltaScript::TokenKind::SHFT_L_P:
                    a->var->set_int(a->var->get_int() << shift);
                    break;
                case DeltaScript::TokenKind::SHFT_R_P:
                    a->var->set_int(a->var->get_int() >> shift);
                    break;
                case DeltaScript::TokenKind::SHFT_RR_P:
                    a->var->set_int(((unsigned int)a->var->get_int()) >> shift);
                    break;
                }
            }
        }

        return a;
    }

    VariableReference* Context::process_condition(bool& can_execute) {
        VariableReference* a = process_shift(can_execute);
        VariableReference* b;

        while (lex_->c_token_kind == TokenKind::EQUAL_P || lex_->c_token_kind == TokenKind::NEQUAL_P
            || lex_->c_token_kind == TokenKind::STRICT_EQUAL_P || lex_->c_token_kind == TokenKind::STRICT_NEQUAL_P
            || lex_->c_token_kind == TokenKind::LTE_P || lex_->c_token_kind == TokenKind::GTE_P
            || lex_->c_token_kind == TokenKind::LT_P || lex_->c_token_kind == TokenKind::GT_P) {
            TokenKind operation = lex_->c_token_kind;
            lex_->parse_next_token();

            b = process_shift(can_execute);

            if (can_execute) {
                Variable* result = a->var->execute_math_operation(b->var, operation);

                CREATE_REFERENCE(a, result);
            }

            CLEAN_VAR_REFERENCE(b);
        }

        return a;
    }

    VariableReference* Context::process_logic(bool& can_execute) {
        VariableReference* a = process_condition(can_execute);
        VariableReference* b;

        while (lex_->c_token_kind == TokenKind::BIT_AND_P || lex_->c_token_kind == TokenKind::BIT_OR_P
            || lex_->c_token_kind == TokenKind::AND_P || lex_->c_token_kind == TokenKind::OR_P) {
            bool no_execute = false;
            TokenKind operation = lex_->c_token_kind;
            lex_->parse_next_token();

            bool short_circuit_operation = false;
            bool boolean = false;

            if (operation == TokenKind::AND_P) {
                operation = TokenKind::BIT_AND_P;
                short_circuit_operation = !a->var->get_bool();
                boolean = true;
            }
            else if (operation == TokenKind::OR_P) {
                operation = TokenKind::BIT_OR_P;
                short_circuit_operation = a->var->get_bool();
                boolean = true;
            }

            b = process_condition(short_circuit_operation ? no_execute : can_execute);
            if (can_execute && !short_circuit_operation) {
                if (boolean) {
                    Variable* new_a = new Variable(a->var->get_bool());
                    Variable* new_b = new Variable(b->var->get_bool());

                    CREATE_REFERENCE(a, new_a);
                    CREATE_REFERENCE(b, new_b);
                }

                Variable* result = a->var->execute_math_operation(b->var, operation);
                CREATE_REFERENCE(a, result);
            }

            CLEAN_VAR_REFERENCE(b);
        }

        return a;
    }

    VariableReference* Context::process_ternary(bool& can_execute) {
        VariableReference* lhs = process_logic(can_execute);
        bool no_execute = false;

        if (lex_->c_token_kind == TokenKind::CONDITIONAL_P) {
            lex_->parse_next_token();

            if (!can_execute) {
                CLEAN_VAR_REFERENCE(lhs);
                CLEAN_VAR_REFERENCE(process_base(no_execute));

                lex_->expect_and_get_next(TokenKind::COLON_P);
                CLEAN_VAR_REFERENCE(process_base(no_execute));
            }
            else {
                bool first = lhs->var->get_bool();
                CLEAN_VAR_REFERENCE(lhs);

                if (first) {
                    lhs = process_base(can_execute);

                    lex_->expect_and_get_next(TokenKind::COLON_P);
                    CLEAN_VAR_REFERENCE(process_base(no_execute));
                }
                else {
                    CLEAN_VAR_REFERENCE(process_base(no_execute));

                    lex_->expect_and_get_next(TokenKind::COLON_P);
                    lhs = process_base(can_execute);
                }
            }
        }

        return lhs;
    }

    VariableReference* Context::process_base(bool& can_execute) {
        VariableReference* lhs = process_ternary(can_execute);

        if (lex_->c_token_kind == TokenKind::ASSIGN_P || lex_->c_token_kind == TokenKind::PLUS_EQ_P
            || lex_->c_token_kind == TokenKind::MINUS_EQ_P) {
            if (can_execute && !lhs->owner) {
                if (lhs->name.length() > 0) {
                    VariableReference* real_lhs = root_->add_child(lhs->name, lhs->var);
                    CLEAN_VAR_REFERENCE(lhs);
                    lhs = real_lhs;
                }
                else {
                    throw DeltaScriptException("Trying to assign to an unnamed type");
                }
            }

            TokenKind op_token = lex_->c_token_kind;
            lex_->parse_next_token();

            VariableReference* rhs = process_base(can_execute);

            if (can_execute) {
                if (op_token == TokenKind::ASSIGN_P) {
                    lhs->replace_with(rhs);
                }
                else if (op_token == TokenKind::PLUS_EQ_P) {
                    Variable* result = lhs->var->execute_math_operation(rhs->var, TokenKind::PLUS_P);
                    lhs->replace_with(result);
                }
                else if (op_token == TokenKind::MINUS_EQ_P) {
                    Variable* result = lhs->var->execute_math_operation(rhs->var, TokenKind::MINUS_P);
                    lhs->replace_with(result);
                }
            }
        }

        return lhs;
    }

    void Context::process_block(bool& can_execute) {
        lex_->expect_and_get_next(TokenKind::LBRACE_P);

        if (can_execute) {
            while (lex_->c_token_kind != TokenKind::EOS && lex_->c_token_kind != TokenKind::RBRACE_P)
                process_statement(can_execute);

            lex_->expect_and_get_next(TokenKind::RBRACE_P);
        }
        else {
            int braces = 1;

            while (lex_->c_token_kind != TokenKind::EOS && braces > 0) {
                if (lex_->c_token_kind == TokenKind::LBRACE_P) {
                    ++braces;
                }
                else if (lex_->c_token_kind == TokenKind::RBRACE_P) {
                    --braces;
                }

                lex_->parse_next_token();
            }
        }
    }

    void Context::process_statement(bool& can_execute) {
        if (lex_->c_token_kind == TokenKind::IDENTIFIER || lex_->c_token_kind == TokenKind::INTEGER_L
            || lex_->c_token_kind == TokenKind::FLOAT_L || lex_->c_token_kind == TokenKind::STRING_L
            || lex_->c_token_kind == TokenKind::MINUS_P) {

            CLEAN_VAR_REFERENCE(process_base(can_execute));

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
                VariableReference* ref = nullptr;

                if (can_execute)
                    ref = scopes_.back()->find_child_or_create(lex_->get_token_value());

                lex_->expect_and_get_next(TokenKind::IDENTIFIER);

                while (lex_->get_current_token() == TokenKind::PERIOD_P) {
                    lex_->parse_next_token();

                    if (can_execute) {
                        VariableReference* last_ref = ref;
                        ref = last_ref->var->find_child_or_create(lex_->get_token_value());
                    }

                    lex_->expect_and_get_next(TokenKind::IDENTIFIER);
                }

                if (lex_->get_current_token() == TokenKind::ASSIGN_P) {
                    lex_->parse_next_token();

                    VariableReference* ref_2 = process_base(can_execute);

                    if (can_execute)
                        ref->replace_with(ref_2);

                    CLEAN_VAR_REFERENCE(ref_2);
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
            VariableReference* ref = process_base(can_execute);
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
            VariableReference* condition = process_base(can_execute);
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

                condition = process_base(can_execute);

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

            VariableReference* condition = process_base(can_execute);
            bool loop_condition = can_execute && condition->var->get_bool();
            CLEAN_VAR_REFERENCE(condition);

            Lexer* for_condition_lex = lex_->get_sub_lex(for_condition_start);
            lex_->expect_and_get_next(TokenKind::SEMICOLON_P);

            int for_iterator_start = lex_->c_token_start;
            CLEAN_VAR_REFERENCE(process_base(no_execute));

            Lexer* for_iterator_lex = lex_->get_sub_lex(for_iterator_start);
            lex_->expect_and_get_next(TokenKind::RPAREN_P);

            int for_body_start = lex_->c_token_start;

            process_statement(loop_condition ? can_execute : no_execute);

            Lexer* for_body_lex = lex_->get_sub_lex(for_body_start);
            Lexer* old_lex = lex_;

            if (loop_condition) {
                for_iterator_lex->reset();
                lex_ = for_iterator_lex;

                CLEAN_VAR_REFERENCE(process_base(can_execute));
            }

            while (can_execute && loop_condition) {
                for_condition_lex->reset();
                lex_ = for_condition_lex;

                condition = process_base(can_execute);
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

                    CLEAN_VAR_REFERENCE(process_base(can_execute));
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
                result = process_base(can_execute);

            if (can_execute) {
                VariableReference* result_var = scopes_.back()->find_child("return"); // TODO: Scoping
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
                    scopes_.back()->add_child(function_var->name, function_var->var);
                }

                CLEAN_VAR_REFERENCE(function_var);
            }
        }
        else { // TODO: Other reserved words
            lex_->expect_and_get_next(TokenKind::EOS);
        }
    }

    VariableReference* Context::parse_function_definition() {
        lex_->expect_and_get_next(TokenKind::FUNCTION_K);

        std::string function_name = "";

        if (lex_->c_token_kind == TokenKind::IDENTIFIER) {
            function_name = lex_->get_token_value();
            lex_->parse_next_token();
        }

        VariableReference* function_ref = new VariableReference(new Variable("", Variable::VariableFlags::FUNCTION), function_name);
        parse_function_arguments(function_ref->var);

        int function_begin = lex_->c_token_start;
        bool no_execute = false;

        process_block(no_execute);

        function_ref->var->str_data_ = lex_->get_sub_string(function_begin);

        return function_ref;
    }

    void Context::parse_function_arguments(Variable* function_variable) {
        lex_->expect_and_get_next(TokenKind::LPAREN_P);

        while (lex_->c_token_kind != TokenKind::RPAREN_P) {
            function_variable->add_child(lex_->get_token_value());

            lex_->expect_and_get_next(TokenKind::IDENTIFIER);

            if (lex_->c_token_kind != TokenKind::RPAREN_P)
                lex_->expect_and_get_next(TokenKind::COMMA_P);
        }

        lex_->expect_and_get_next(TokenKind::RPAREN_P);
    }

    VariableReference* Context::find_var_in_scopes(const std::string& child_name) {
        for (int i = scopes_.size() - 1; i >= 0; --i) {
            VariableReference* ref = scopes_[i]->find_child(child_name);

            if (ref)
                return ref;
        }

        return nullptr;
    }

    VariableReference* Context::find_var_in_parent_classes(Variable* object, const std::string& name) {
        VariableReference* parent_class = object->find_child("prototype");

        while (parent_class) {
            VariableReference* implementation = parent_class->var->find_child(name);

            if (implementation)
                return implementation;

            parent_class = parent_class->var->find_child("prototype");
        }

        // TODO: Add expansions for natively supported types (string, array, object)

        return nullptr;
    }
}  // namespace DeltaScript
