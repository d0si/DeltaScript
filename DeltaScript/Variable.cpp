#include <DeltaScript/DeltaScript.h>
#include <sstream>
#include <nlohmann/json.hpp>

#ifndef WIN32
#define sprintf_s snprintf
#endif

namespace DeltaScript {
    Variable::Variable() {
        ref_count_ = 0;
        last_child_ = nullptr;
        first_child_ = nullptr;
        str_data_ = "";
        int_data_ = 0;
        double_data_ = 0;
        native_callback_ = nullptr;
        native_callback_data_ = nullptr;
        execution_count_ = 0;
        flags_ = VariableFlags::UNDEFINED;
    }

    Variable::Variable(const std::string& value) : Variable() {
        set_string(value);
    }

    Variable::Variable(const std::string& data, unsigned int var_flags) : Variable() {
        flags_ = var_flags;

        if (flags_ & VariableFlags::INTEGER) {
            int_data_ = strtol(data.c_str(), 0, 0);
        }
        else if (flags_ & VariableFlags::DOUBLE) {
            double_data_ = strtod(data.c_str(), 0);
        }
        else {
            str_data_ = data;
        }
    }

    Variable::Variable(int value) : Variable() {
        set_int(value);
    }

    Variable::Variable(double value) : Variable() {
        set_double(value);
    }

    std::string Variable::get_string() const {
        if (is_int()) {
            return std::to_string(int_data_);
        }
        else if (is_double()) {
            return std::to_string(double_data_);
        }
        else if (is_null()) {
            return "null";
        }
        else if (is_undefined()) {
            return "undefined";
        }
        else if (is_object() || is_array()) {
            return to_json();
        }

        return str_data_;
    }

    bool Variable::get_bool() const {
        return get_int() != 0;
    }

    int Variable::get_int() const {
        if (is_int())
            return int_data_;

        if (is_null() || is_undefined())
            return 0;

        if (is_double())
            return (int)double_data_;

        return 0;
    }

    double Variable::get_double() const {
        if (is_double())
            return double_data_;

        if (is_int())
            return int_data_;

        if (is_null() || is_undefined())
            return 0;

        return 0;
    }

    void Variable::set_string(const std::string& value) {
        flags_ = (flags_ & ~VariableFlags::VARTYPE) | VariableFlags::STRING;
        str_data_ = value;
        int_data_ = 0;
        double_data_ = 0;
    }

    void Variable::set_int(int value) {
        flags_ = (flags_ & ~VariableFlags::VARTYPE) | VariableFlags::INTEGER;
        int_data_ = value;
        double_data_ = 0;
        str_data_ = "";
    }

    void Variable::set_double(double value) {
        flags_ = (flags_ & ~VariableFlags::VARTYPE) | VariableFlags::DOUBLE;
        double_data_ = value;
        int_data_ = 0;
        str_data_ = "";
    }

    void Variable::set_undefined() {
        flags_ = (flags_ & ~VariableFlags::VARTYPE) | VariableFlags::UNDEFINED;
        int_data_ = 0;
        double_data_ = 0;
        str_data_ = "";
        remove_all_children();
    }

    void Variable::set_as_array() {
        flags_ = (flags_ & ~VariableFlags::VARTYPE) | VariableFlags::ARRAY;
        int_data_ = 0;
        double_data_ = 0;
        str_data_ = "";
        remove_all_children();
    }

    bool Variable::is_int() const {
        return (flags_ & VariableFlags::INTEGER) != 0;
    }

    bool Variable::is_double() const {
        return (flags_ & VariableFlags::DOUBLE) != 0;
    }

    bool Variable::is_string() const {
        return (flags_ & VariableFlags::STRING) != 0;
    }

    bool Variable::is_numeric() const {
        return (flags_ & VariableFlags::NUMERIC) != 0;
    }

    bool Variable::is_function() const {
        return (flags_ & VariableFlags::FUNCTION) != 0;
    }

    bool Variable::is_object() const {
        return (flags_ & VariableFlags::OBJECT) != 0;
    }

    bool Variable::is_array() const {
        return (flags_ & VariableFlags::ARRAY) != 0;
    }

    bool Variable::is_native() const {
        return (flags_ & VariableFlags::NATIVE) != 0;
    }

    bool Variable::is_undefined() const {
        return (flags_ & VariableFlags::VARTYPE) == VariableFlags::UNDEFINED;
    }

    bool Variable::is_null() const {
        return (flags_ & VariableFlags::NULL_) != 0;
    }

    bool Variable::is_basic() const {
        return children_.empty();
    }

    VariableReference* Variable::find_child(const std::string& child_name) const {
        static int i = 0;
        ++i;

        auto c = children_.find(child_name);

        if (c != children_.end())
            return c->second;

        if (child_name == "length") {
            if (is_array()) {
                return new VariableReference(new Variable(get_array_size()));
            }
            else if (is_string()) {
                return new VariableReference(new Variable((int)get_string().size()));
            }
        }

        return nullptr;
    }

    VariableReference* Variable::find_child_or_create(const std::string& child_name, unsigned int var_flags) {
        VariableReference* ref = find_child(child_name);

        if (ref)
            return ref;

        return add_child(child_name, new Variable("", var_flags));
    }

    VariableReference* Variable::find_child_or_create_by_path(const std::string& path) {
        size_t p = path.find('.');
        if (p == std::string::npos)
            return find_child_or_create(path);

        return find_child_or_create(path.substr(0, p), VariableFlags::OBJECT)
            ->var->find_child_or_create_by_path(path.substr(p + 1));
    }

    VariableReference* Variable::add_child(const std::string& child_name, Variable* child) {
        if (is_undefined())
            flags_ = VariableFlags::OBJECT;

        if (!child)
            child = new Variable();

        VariableReference* ref = new VariableReference(child, child_name);
        ref->owner = true;

        if (!children_.empty()) {
            VariableReference* old_child = find_child(child_name);

            if (old_child) {
                old_child->replace_with(ref);
                delete ref;

                return old_child;
            }
            else {
                last_child_->next_sibling = ref;
                ref->prev_sibling = last_child_;
                last_child_ = ref;

                return children_[child_name] = ref;
            }
        }
        else {
            last_child_ = first_child_ = ref;

            return children_[child_name] = ref;
        }
    }

    void Variable::remove_child(const std::string& child_name, Variable* child, bool throw_if_not_found) {
        VariableReference* ref = find_child(child_name);

        if (!ref && throw_if_not_found) {
            throw VariableReferenceException("Removing non-existent child");
        }
        else if (ref) {
            remove_reference(ref);
        }
    }

    void Variable::remove_reference(VariableReference* ref) {
        if (!ref)
            return;

        if (!children_.erase(ref->name))
            throw VariableReferenceException("Cannot remove reference that does not exist in that variable");

        if (ref->next_sibling)
            ref->next_sibling->prev_sibling = ref->prev_sibling;

        if (ref->prev_sibling)
            ref->prev_sibling->next_sibling = ref->next_sibling;

        if (first_child_ == ref)
            first_child_ = ref->next_sibling;

        if (last_child_ == ref)
            last_child_ = ref->prev_sibling;

        delete ref;
    }

    void Variable::remove_all_children() {
        auto it = children_.begin();
        while (it != children_.end()) {
            VariableReference* temp = it->second;
            it = children_.erase(it);

            delete temp;
        }

        first_child_ = nullptr;
        last_child_ = nullptr;
    }

    Variable* Variable::get_array_val_at_index(int index) const {
        std::string str_index = std::to_string(index);

        VariableReference* ref = find_child(str_index);

        if (ref) {
            return ref->var;
        }
        else {
            return new Variable("", VariableFlags::NULL_);
        }
    }

    void Variable::set_array_val_at_index(int index, Variable* value) {
        char str_index[64];
        sprintf_s(str_index, sizeof(str_index), "%d", index);

        VariableReference* ref = find_child(str_index);

        if (ref) {
            if (value->is_undefined()) {
                remove_reference(ref);
            }
            else {
                ref->replace_with(value);
            }
        }
        else {
            if (!value->is_undefined()) {
                add_child(str_index, value);
            }
        }
    }

    int Variable::get_array_size() const {
        if (!is_array())
            return 0;

        int highest = -1;

        for (auto& child : children_) {
            VariableReference* ref = child.second;

            if (Util::is_number(ref->name)) {
                int val = atoi(ref->name.c_str());

                if (val > highest)
                    highest = val;
            }
        }

        return highest + 1;
    }

    int Variable::get_children_count() const {
        return children_.size();
    }
    
    std::unordered_map<std::string, VariableReference*> Variable::get_children() const {
        return children_;
    }

    Variable* Variable::execute_math_operation(Variable* second, TokenKind operation) {
        Variable* first = this;

        if (operation == TokenKind::STRICT_EQUAL_P || operation == TokenKind::STRICT_NEQUAL_P) {
            bool equal = (first->flags_ & VariableFlags::VARTYPE) == (second->flags_ & VariableFlags::VARTYPE);

            if (equal) {
                Variable* contents = first->execute_math_operation(second, TokenKind::EQUAL_P);

                if (!contents->get_bool())
                    equal = false;

                if (!contents->ref_count_)
                    delete contents;
            }

            if (operation == TokenKind::STRICT_EQUAL_P) {
                return new Variable(equal);
            }
            else {
                return new Variable(!equal);
            }
        }

        if (first->is_undefined() && second->is_undefined()) {
            switch (operation) {
            case TokenKind::EQUAL_P:
                return new Variable(true);
            case TokenKind::NEQUAL_P:
                return new Variable(false);
            default:
                return new Variable();
            }
        }
        else if ((first->is_numeric() || first->is_undefined())
            && (second->is_numeric() || second->is_undefined())) {
            if (!first->is_double() && !second->is_double()) {
                int first_i = first->get_int();
                int second_i = second->get_int();

                switch (operation) {
                case TokenKind::PLUS_P:
                    return new Variable(first_i + second_i);
                case TokenKind::MINUS_P:
                    return new Variable(first_i - second_i);
                case TokenKind::MUL_P:
                    return new Variable(first_i * second_i);
                case TokenKind::DIV_P:
                    return new Variable(first_i / second_i);
                case TokenKind::BIT_AND_P:
                    return new Variable(first_i & second_i);
                case TokenKind::BIT_OR_P:
                    return new Variable(first_i | second_i);
                case TokenKind::BIT_XOR_P:
                    return new Variable(first_i ^ second_i);
                case TokenKind::MOD_P:
                    return new Variable(first_i % second_i);
                case TokenKind::EQUAL_P:
                    return new Variable(first_i == second_i);
                case TokenKind::NEQUAL_P:
                    return new Variable(first_i != second_i);
                case TokenKind::LT_P:
                    return new Variable(first_i < second_i);
                case TokenKind::LTE_P:
                    return new Variable(first_i <= second_i);
                case TokenKind::GT_P:
                    return new Variable(first_i > second_i);
                case TokenKind::GTE_P:
                    return new Variable(first_i >= second_i);
                default:
                    throw DeltaScriptException("Operation " + Token::get_token_kind_as_string(operation) + " is not on the Int type");
                }
            }
            else {
                double first_d = first->get_double();
                double second_d = second->get_double();

                switch (operation) {
                case TokenKind::PLUS_P:
                    return new Variable(first_d + second_d);
                case TokenKind::MINUS_P:
                    return new Variable(first_d - second_d);
                case TokenKind::MUL_P:
                    return new Variable(first_d * second_d);
                case TokenKind::DIV_P:
                    return new Variable(first_d / second_d);
                case TokenKind::EQUAL_P:
                    return new Variable(first_d == second_d);
                case TokenKind::NEQUAL_P:
                    return new Variable(first_d != second_d);
                case TokenKind::LT_P:
                    return new Variable(first_d < second_d);
                case TokenKind::LTE_P:
                    return new Variable(first_d <= second_d);
                case TokenKind::GT_P:
                    return new Variable(first_d > second_d);
                case TokenKind::GTE_P:
                    return new Variable(first_d >= second_d);
                default:
                    throw DeltaScriptException("Operation " + Token::get_token_kind_as_string(operation) + " is not on the Int type");
                }
            }
        }
        else if (first->is_array()) {
            switch (operation) {
            case TokenKind::EQUAL_P:
                return new Variable(first == second);
            case TokenKind::NEQUAL_P:
                return new Variable(first != second);
            default:
                throw DeltaScriptException("Operation " + Token::get_token_kind_as_string(operation) + " is not on the Array type");
            }
        }
        else if (first->is_object()) {
            switch (operation) {
            case TokenKind::EQUAL_P:
                return new Variable(first == second);
            case TokenKind::NEQUAL_P:
                return new Variable(first != second);
            default:
                throw DeltaScriptException("Operation " + Token::get_token_kind_as_string(operation) + " is not on the Object type");
            }
        }
        else {
            std::string first_s = first->get_string();
            std::string second_s = second->get_string();

            switch (operation) {
            case TokenKind::PLUS_P:
                return new Variable(first_s + second_s);
            case TokenKind::EQUAL_P:
                return new Variable(first_s == second_s);
            case TokenKind::NEQUAL_P:
                return new Variable(first_s != second_s);
            case TokenKind::LT_P:
                return new Variable(first_s == second_s);
            case TokenKind::LTE_P:
                return new Variable(first_s == second_s);
            case TokenKind::GT_P:
                return new Variable(first_s == second_s);
            case TokenKind::GTE_P:
                return new Variable(first_s == second_s);
            default:
                throw DeltaScriptException("Operation " + Token::get_token_kind_as_string(operation) + " is not on the String type");
            }
        }

        throw DeltaScriptException("Mathematical operation error. Could not apply any operation to requested values");
        return nullptr;
    }

    void Variable::copy_from(Variable* value) {
        if (value) {
            copy_simple_data_from(value);
            remove_all_children();

            for (auto& it : value->children_) {
                VariableReference* child = it.second;
                Variable* copy;

                if (it.first != "prototype") {
                    copy = child->var->deep_copy();
                }
                else {
                    copy = child->var;
                }

                add_child(it.first, copy);
            }
        }
        else {
            set_undefined();
        }
    }

    void Variable::copy_simple_data_from(Variable* value) {
        str_data_ = value->str_data_;
        int_data_ = value->int_data_;
        double_data_ = value->double_data_;
        flags_ = (flags_ & ~VariableFlags::VARTYPE) | (value->flags_ & VariableFlags::VARTYPE);
    }

    Variable* Variable::deep_copy() {
        Variable* new_var = new Variable();

        new_var->copy_simple_data_from(this);

        for (auto& it : children_) {
            VariableReference* child = it.second;
            Variable* copy;

            if (child->name != "prototype") {
                copy = child->var->deep_copy();
            }
            else {
                copy = child->var;
            }

            new_var->add_child(child->name, copy);
        }

        return new_var;
    }

    Variable* Variable::inc_ref() {
        ++ref_count_;

        return this;
    }

    void Variable::unref() {
        if (ref_count_ <= 0) {
            throw VariableReferenceException("Too many unrefs in variable. Stack may be corrupted.");
        }
        else if ((--ref_count_) == 0) {
            delete this;
        }
    }

    int Variable::get_ref_count() const {
        return ref_count_;
    }

    void Variable::increase_execution_count() {
        ++execution_count_;
    }

    int Variable::get_execution_count() const {
        return execution_count_;
    }

    void Variable::set_native_callback(NativeCallback callback, void* data) {
        native_callback_ = callback;
        native_callback_data_ = data;
    }

    nlohmann::json object_to_json(const Variable* var);
    nlohmann::json array_to_json(const Variable* var);
    nlohmann::json variable_to_json(const Variable* var);

    nlohmann::json object_to_json(const Variable* var) {
        nlohmann::json json_value = nlohmann::json::object();

        for (auto& it : var->get_children()) {
            json_value[it.first] = variable_to_json(it.second->var);
        }

        return json_value;
    }

    nlohmann::json array_to_json(const Variable* var) {
        nlohmann::json json_value = nlohmann::json::array();

        for (auto& it : var->get_children()) {
            json_value[it.first] = variable_to_json(it.second->var);
        }

        return json_value;
    }

    nlohmann::json variable_to_json(const Variable* var) {
        if (var->is_object()) {
            return object_to_json(var);
        }
        else if (var->is_array()) {
            return array_to_json(var);
        }
        else if (var->is_int()) {
            return nlohmann::json(var->get_int());
        }
        else if (var->is_double()) {
            return nlohmann::json(var->get_double());
        }
        else if (var->is_string()) {
            return nlohmann::json(var->get_string());
        }
        else {
            return nlohmann::json(nullptr);
        }
    }

    std::string Variable::to_json() const {
        return variable_to_json(this).dump();
    }

    Variable* object_from_json(nlohmann::json json_value);
    Variable* array_from_json(nlohmann::json json_value);
    Variable* variable_from_json(nlohmann::json json_value);

    Variable* object_from_json(nlohmann::json json_value) {
        Variable* value = new Variable();

        for (auto it = json_value.begin(); it != json_value.end(); ++it) {
            value->add_child(it.key(), variable_from_json(*it));
        }

        return value;
    }

    Variable* array_from_json(nlohmann::json json_value) {
        Variable* value = new Variable();
        value->set_as_array();
        int i = 0;

        for (auto it = json_value.begin(); it != json_value.end(); ++it) {
            value->set_array_val_at_index(i, variable_from_json(*it));

            ++i;
        }

        return value;
    }

    Variable* variable_from_json(nlohmann::json json_value) {
        if (json_value.is_object()) {
            return object_from_json(json_value);
        }
        else if (json_value.is_array()) {
            return array_from_json(json_value);
        }
        else if (json_value.is_number_integer()) {
            return new Variable(json_value.get<int>());
        }
        else if (json_value.is_number()) {
            return new Variable(json_value.get<double>());
        }
        else if (json_value.is_boolean()) {
            return new Variable(json_value.get<bool>());
        }
        else if (json_value.is_string()) {
            return new Variable(json_value.get<std::string>());
        }
        else {
            return new Variable();
        }
    }

    Variable* Variable::from_json(const std::string& string_value) {
        return variable_from_json(nlohmann::json::parse(string_value));
    }
}  // namespace DeltaScript
