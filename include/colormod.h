#include <ostream>

namespace Color {

    enum Code {

        FG_BLACK = 30, 
        FG_RED = 31, 
        FG_GREEN = 32, 
        FG_YELLOW = 33, 
        FG_BLUE = 34, 
        FG_MAGENTA = 35, 
        FG_CYAN = 36, 
        FG_LIGHT_GRAY = 37,
        FG_DEFAULT = 39,  
        FG_DARK_GRAY = 90, 
        FG_LIGHT_RED = 91, 
        FG_LIGHT_GREEN = 92, 
        FG_LIGHT_YELLOW = 93, 
        FG_LIGHT_BLUE = 94, 
        FG_LIGHT_MAGENTA = 95, 
        FG_LIGHT_CYAN = 96, 
        FG_WHITE = 97, 
        BG_RED      = 41,
        BG_GREEN    = 42,
        BG_BLUE     = 44,
        BG_DEFAULT  = 49,

        RESET = 0, //  (everything back to normal)
        BOLDBRIGHT  = 1, // (often a brighter shade of the same colour)
        UNDERLINE = 4,
        INVERSE = 7, // (swap foreground and background colours)
        BOLDBRIGHTOFF = 21,
        UNDERLINEOFF = 24,
        INVERSEOFF = 27
    };

    class Modifier {

        public:

            Modifier( Code pCode ) : code( pCode ) {}

            friend std::ostream& operator<<( std::ostream& os, const Modifier& mod ) {
                
                return os << "\033[" << mod.code << "m";
            }

        private:

            Code code;
    };

}