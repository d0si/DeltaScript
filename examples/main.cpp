#include <DeltaScript/DeltaScript.h>
#include <iostream>

int main() {
    DeltaScript::Context ctx;
    ctx.add_native_function("function print(str)", [](DeltaScript::Variable* var, void* data) {
        std::cout << "[Native print function called]" << std::endl;
        std::cout << var->find_child("str")->var->get_string() << std::endl;
        }, nullptr);

    try {
        ctx.execute("print('something');");
    }
    catch (DeltaScript::DeltaScriptException & e) {
        std::cout << "[Caught DeltaScriptException]:" << std::endl;
        std::cout << e.message << std::endl;
    }

    return 0;
}
