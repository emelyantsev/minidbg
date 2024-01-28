
unsigned long inc( unsigned long val ) {

    return val * 2;
};


int main() {

    unsigned long x = 12;

    unsigned long y = inc( x ) ;

    x = inc( y );

    return 0;
}