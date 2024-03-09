#include <chrono>
#include <iostream>
#include <thread>
#include <unistd.h>
 

using namespace std::chrono_literals;


void print(int x) {

    printf("x = %d\n", x);
}


void process(int& x) {
    
    ++x;
    print(x);
}


int main()
{

    std::cout << "The process started with ID = " << getpid() << std::endl;

    bool shouldStop = false;
    int x = 0;

    while ( !shouldStop ) {

        process( x ) ;

        if ( x == 1800 ) {

            shouldStop = true;
        }

        std::this_thread::sleep_for( 2000ms );
    }

    return 0;
}