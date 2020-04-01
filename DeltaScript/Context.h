#ifndef DELTASCRIPT_CONTEXT_H_
#define DELTASCRIPT_CONTEXT_H_

#include <memory>
#include <string>

namespace DeltaScript {
    class Context {
    private:

    public:
        Context();

        template<class T>
        void register_class();

        template<class T>
        void add_global(const std::string variable_name, std::shared_ptr<T> value);

        void eval_string_no_result(const char* script);
        std::shared_ptr<void> eval_string(const std::string script);
    };
}

#endif  // DELTASCRIPT_CONTEXT_H_
