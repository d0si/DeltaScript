#include <DeltaScript/DeltaScript.h>

namespace DeltaScript {
    namespace Util {
        bool is_white_space(char value) {
            return value == 0x0009 /*<TAB>*/ || value == 0x000B /*<VT>*/ || value == 0x000C /*<FF>*/
                || value == 0x0020 /*<SP>*/ || value == 0x00A0 /*<NBSP>*/;
            // Missing are <ZWNBSP> (0xFEFF) and other Unicode characters
        }

        bool is_line_terminator(char value) {
            return value == 0x000A /*<LF>*/ || value == 0x000D /*<CR>*/ || value == 0x2028 /*<LS>*/
                || value == 0x2029 /*<PS>*/;
        }

        bool is_line_terminator_sequence(char value, char next_value) {
            return value == 0x000A /*<LF>*/ || value == 0x000D /*<CR>*/ || value == 0x2028 /*<LS>*/
                || value == 0x2029 /*<PS>*/ || (value == 0x000D && next_value == 0x000A);
        }
    }  // namespace Util
}  // namespace DeltaScript
