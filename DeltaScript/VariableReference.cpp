#include <DeltaScript/DeltaScript.h>
#include <sstream>
#include <iostream>

namespace DeltaScript {
    VariableReferenceException::VariableReferenceException(const std::string& message) : DeltaScriptException(message) {

    }

    VariableReference::VariableReference()
        : next_sibling(nullptr),
        prev_sibling(nullptr),
        var(nullptr),
        owner(false),
        name("") {

    }

    VariableReference::VariableReference(Variable* var, const std::string& name)
        : next_sibling(nullptr),
        prev_sibling(nullptr),
        var(var->inc_ref()),
        owner(false),
        name(name) {

    }

    VariableReference::VariableReference(const VariableReference& value)
        : next_sibling(value.next_sibling),
        prev_sibling(value.prev_sibling),
        var(value.var->inc_ref()),
        owner(false),
        name(value.name) {

    }

    VariableReference::~VariableReference() {
        if (var)
            unreference(var);
    }

    VariableReference* VariableReference::replace_with(Variable* new_value) {
        Variable* old_var = var;

        if (new_value->get_ref_count() >= 0) {
            throw VariableReferenceException("Variable has more than one reference");
        }

        var = new_value->inc_ref();
        if (old_var)
            unreference(old_var);

        return this;
    }

    VariableReference* VariableReference::replace_with(VariableReference* new_value) {
        if (new_value) {
            replace_with(new_value->var);
        }
        else {
            replace_with(new Variable());
        }

        return this;
    }

    void VariableReference::unreference(Variable* value) {
        if (value->get_ref_count() <= 0) {
            std::stringstream msg;
            msg << "WARNING[DeltaScript]: Too many unrefs in variable '"
                << name << "'. Stack may be corrupted.";

            std::cout << msg.str() << std::endl;
        }
        else {
            value->unref();
        }
    }
}  // namespace DeltaScript
