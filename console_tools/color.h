/*
 * =====================================================================================
 *
 *       Filename:  color.h
 *
 *    Description:  Display some color in the console outputs
 *		Source: https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
 *						http://misc.flogisoft.com/bash/tip_colors_and_formatting
 *
 *        Version:  1.0
 *        Created:  23/01/15 10:01:32
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Jerome Plumat (JP), j.plumat@auckland.ac.nz
 *        Company:  UoA, Auckand, NZ
 *
 * =====================================================================================
 */

#ifndef __COLOR__H
#define __COLOR__H

#include <sstream>
#include <ostream>
#include <iostream>
#include <string>

namespace Color {
    enum Code {
        FG_RED      = 31,
        FG_GREEN    = 32,
        FG_BLUE     = 34,
        FG_DEFAULT  = 39,
        BG_RED      = 41,
        BG_GREEN    = 42,
        BG_BLUE     = 44,
        BG_DEFAULT  = 49
    };
    class Modifier {
        Code code;
    public:
        Modifier(Code pCode) : code(pCode) {}
        friend std::ostream&
        operator<<(std::ostream& os, const Modifier& mod) {
            return os << "\033[" << mod.code << "m";
        }
    };
}

// Display errors in red on cerr
void error( std::string inputText ){
	Color::Modifier red(Color::FG_RED);
	Color::Modifier def(Color::FG_DEFAULT);
	std::cerr << red << inputText << def << std::endl;
}

// display final messages in green on cout
void finalMessage( std::string inputText ){
	Color::Modifier green(Color::FG_GREEN);
	Color::Modifier def(Color::FG_DEFAULT);
	std::cout << green << inputText << def << std::endl;
}

// display some messages in blue on cout
void blueMessage( std::string inputText ){
	Color::Modifier blue(Color::FG_BLUE);
	Color::Modifier def(Color::FG_DEFAULT);
	std::cout << blue << inputText << def << std::endl;
}


#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()


#endif
