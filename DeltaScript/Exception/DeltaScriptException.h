#ifndef DELTASCRIPT_EXCEPTION_DELTASCRIPTEXCEPTION_H_
#define DELTASCRIPT_EXCEPTION_DELTASCRIPTEXCEPTION_H_

#include <exception>

namespace DeltaScript {
    namespace Exception {
        class DeltaScriptException : public std::exception {
        public:
            DeltaScriptException();
        };
    }
}

#endif  // DELTASCRIPT_EXCEPTION_DELTASCRIPTEXCEPTION_H_
