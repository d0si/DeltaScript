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

        bool is_line_terminator_crlf(char value, char next_value) {
            return value == 0x000D /*<CR>*/ && next_value == 0x000A /*<LF>*/;
        }

        bool is_alpha(char value) {
            return (value >= 'a' && value <= 'z')
                || (value >= 'A' && value <= 'Z')
                || value == '_';
        }

        bool is_digit(char value) {
            return (value >= '0' && value <= '9');
        }

        bool is_hex(char value) {
            return (value >= '0' && value <= '9')
                || (value >= 'a' && value <= 'f')
                || (value >= 'A' && value <= 'F');
        }
    }  // namespace Util
}  // namespace DeltaScript
