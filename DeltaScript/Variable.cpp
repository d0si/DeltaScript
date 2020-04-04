#include <DeltaScript/DeltaScript.h>

namespace DeltaScript {
    Variable::Variable() {
        ref_count_ = 0;
        last_child_ = nullptr;
        first_child_ = nullptr;
        str_data_ = "";
        int_data_ = 0;
        double_data_ = 0;
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

    const std::string& Variable::get_string() {
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

        return str_data_;
    }

    inline bool Variable::get_bool() {
        return get_int() != 0;
    }

    int Variable::get_int() {
        if (is_int())
            return int_data_;

        if (is_null() || is_undefined())
            return 0;

        if (is_double())
            return (int)double_data_;

        return 0;
    }

    double Variable::get_double() {
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

    VariableReference* Variable::find_child(const std::string& child_name) {
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

    Variable* Variable::get_array_val_at_index(int index) {
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

    int Variable::get_array_size() {
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

    int Variable::get_children_count() {
        return children_.size();
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

    int Variable::get_ref_count() {
        return ref_count_;
    }
}  // namespace DeltaScript
