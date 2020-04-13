#ifndef DELTASCRIPT_DELTASCRIPT_H_
#define DELTASCRIPT_DELTASCRIPT_H_

#include <string>
#include <vector>
#include <unordered_map>

#define CLEAN_VAR_REFERENCE(x) { VariableReference* v = x; if (v && !v->owner) delete v; }
#define CREATE_REFERENCE(ref, var) { if (!ref || ref->owner) ref = new VariableReference(var); else ref->replace_with(var); }

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
        UNDEFINED_K, // undefined
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
        PLUS_P,     // +
        MINUS_P,    // -
        MUL_P,      // *
        DIV_P,      // /
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
        PLUS_EQ_P,  // +=
        MINUS_EQ_P, // -=
        MUL_EQ_P,   // *=
        MOD_EQ_P,   // %=
        EXP_EQ_P,   // **=
        SHFT_L_EQ_P, // <<=
        SHFT_R_EQ_P, // >>=
        SHFT_RR_EQ_P, // >>>=
        BIT_AND_EQ_P, // &=
        BIT_OR_EQ_P, // |=
        BIT_XOR_EQ_P, // ^=
        ARROW_P,    // =>

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

    class DeltaScriptException {
    public:
        DeltaScriptException(const std::string& message) : message(message) {}

        std::string message;
    };

    class LexerException : public DeltaScriptException {
    public:
        LexerException(const std::string& message);
    };

    class VariableReferenceException : public DeltaScriptException {
    public:
        VariableReferenceException(const std::string& message);
    };

    class Lexer {
    private:
        char* source_;
        int source_start_;
        size_t source_end_;
        int c_source_position_;
        bool source_owner;

        char c_char = 0, n_char = 0;
    public:
        TokenKind c_token_kind = TokenKind::EOS;
        int c_token_start = 0;
    private:
        int c_token_end = 0;
        int p_token_end = 0;
        std::string c_token_value;

    public:
        Lexer(const std::string& source);
        Lexer(Lexer* parent, int source_start, int source_end);
        ~Lexer();

        TokenKind get_current_token() const;
        std::string get_token_value() const;
        
        void reset();
        void expect_and_get_next(TokenKind expected_kind);
        void get_next_char();
        void get_previous_char();
        void parse_next_token();

        Lexer* get_sub_lex(int start_position);

        std::string get_sub_string(int start_position);

    private:
        void process_inline_comment();
        void process_multiline_comment();
        void process_identifier();
        void process_number();
        void process_double_quote_string_literal();
        void process_single_quote_string_literal();
        void process_punctuators();

        void check_for_reserved_keywords();
    };

    class VariableReference;
    class Variable;
    typedef void (*NativeCallback) (Variable* var, void* data);

    class Variable {
    public:
        enum VariableFlags : unsigned int {
            UNDEFINED = 0,
            FUNCTION = 1,
            OBJECT = 2,
            ARRAY = 4,
            DOUBLE = 8,
            INTEGER = 16,
            STRING = 32,
            NULL_ = 64,
            NATIVE = 128,
            NUMERIC = NULL_ | DOUBLE | INTEGER,
            VARTYPE = DOUBLE | INTEGER | STRING | FUNCTION | OBJECT | ARRAY | NULL_,
        };

    protected:
        std::string str_data_;
        long int_data_;
        double double_data_;
        unsigned int flags_;
        NativeCallback native_callback_;
        void* native_callback_data_;
    private:
        std::unordered_map<std::string, VariableReference*> children_;
        VariableReference* first_child_;
        VariableReference* last_child_;
        int ref_count_;
        int execution_count_;

    public:
        Variable();
        Variable(const std::string& value);
        Variable(const std::string& data, unsigned int var_flags);
        Variable(int value);
        Variable(double value);

        std::string get_string();
        bool get_bool();
        int get_int();
        double get_double();
        void set_string(const std::string& value);
        void set_int(int value);
        void set_double(double value);
        void set_undefined();
        void set_as_array();

        bool is_int() const;
        bool is_double() const;
        bool is_string() const;
        bool is_numeric() const;
        bool is_function() const;
        bool is_object() const;
        bool is_array() const;
        bool is_native() const;
        bool is_undefined() const;
        bool is_null() const;
        bool is_basic() const;

        VariableReference* find_child(const std::string& child_name);
        VariableReference* find_child_or_create(const std::string& child_name, unsigned int var_flags = VariableFlags::UNDEFINED);
        VariableReference* find_child_or_create_by_path(const std::string& path);
        VariableReference* add_child(const std::string& child_name, Variable* child = nullptr);
        void remove_child(const std::string& child_name, Variable* child, bool throw_if_not_found = false);
        void remove_reference(VariableReference* ref);
        void remove_all_children();
        Variable* get_array_val_at_index(int index);
        void set_array_val_at_index(int index, Variable* value);
        int get_array_size();
        int get_children_count();

        Variable* execute_math_operation(Variable* second, TokenKind operation);

        void copy_from(Variable* value);
        void copy_simple_data_from(Variable* value);
        Variable* deep_copy();

        Variable* inc_ref();
        void unref();
        int get_ref_count();

        void increase_execution_count();
        int get_execution_count();
        void set_native_callback(NativeCallback callback, void* data);

        friend class Context;
    };

    class VariableReference {
    public:
        VariableReference();
        VariableReference(Variable* var, const std::string& name = "");
        VariableReference(const VariableReference& value);
        ~VariableReference();

        VariableReference* next_sibling;
        VariableReference* prev_sibling;
        Variable* var;
        std::string name;
        bool owner;

        VariableReference* replace_with(Variable* new_value);
        VariableReference* replace_with(VariableReference* new_value);
    private:
        void unreference(Variable* value);
    };

    class Context {
    private:
        Lexer* lex_;
        std::vector<Variable*> scopes_;
        Variable* root_;

    public:
        Context();
        ~Context();

        void execute(const std::string& script);
        // VariableReference* evaluate(const std::string& script);
        // std::string evaluate_as_string(const std::string& script);

        void add_native_function(const std::string& function_definition, NativeCallback callback, void* data);

    private:
        VariableReference* process_function_call(bool& can_execute, VariableReference* function, Variable* parent);
        VariableReference* process_factor(bool& can_execute);
        VariableReference* process_unary(bool& can_execute);
        VariableReference* process_term(bool& can_execute);
        VariableReference* process_expression(bool& can_execute);
        VariableReference* process_shift(bool& can_execute);
        VariableReference* process_condition(bool& can_execute);
        VariableReference* process_logic(bool& can_execute);
        VariableReference* process_ternary(bool& can_execute);
        VariableReference* process_base(bool& can_execute);

        void process_block(bool& can_execute);
        void process_statement(bool& can_execute);

        VariableReference* parse_function_definition();
        void parse_function_arguments(Variable* function_variable);

        VariableReference* find_var_in_scopes(const std::string& child_name);
        VariableReference* find_var_in_parent_classes(Variable* object, const std::string& name);
    };

    namespace Util {
        bool is_white_space(char value);
        bool is_line_terminator(char value);
        bool is_line_terminator_crlf(char value, char next_value);

        bool is_alpha(char value);
        bool is_digit(char value);
        bool is_number(const std::string& value);
        bool is_hex(char value);
    }
}  // namespace DeltaScript

#endif  // DELTASCRIPT_DELTASCRIPT_H_
