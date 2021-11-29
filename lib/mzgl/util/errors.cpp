//
//  errors.cpp
//  TapeSampler
//
//  Created by Marek Bereza on 23/11/2017.
//
//

#include "errors.h"


void _M_ASSERT(const char* expr_str, bool expr, const char* file, int line, const char* msg)
{
	if (!expr)
	{
		std::cerr << "Assert failed:\t" << msg << "\n"
		<< "Expected:\t" << expr_str << "\n"
		<< "Source:\t\t" << file << ", line " << line << "\n";
		abort();
	}
}


