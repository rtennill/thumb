//  Copyright (C) 2005 Robert Kooima
//
//  THUMB is free software; you can redistribute it and/or modify it under
//  the terms of  the GNU General Public License as  published by the Free
//  Software  Foundation;  either version 2  of the  License,  or (at your
//  option) any later version.
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.

#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <vector>
#include <set>

#include <math.h>

// TODO: Put this stuff where it belongs.

//-----------------------------------------------------------------------------

#ifndef CLAMP
#define CLAMP(n, a, b) std::min(std::max(n, a), b)
#endif

//-----------------------------------------------------------------------------

typedef std::vector<std::string> strvec;
typedef std::set   <std::string> strset;

//-----------------------------------------------------------------------------

#define get_bit(b, i) (((b) >> ((i)    )) & 1)
#define get_oct(b, i) (((b) >> ((i) * 3)) & 7)

#define set_bit(b, i, n) ((b) & (~(1 << ((i)    ))) | ((n) << ((i)    )))
#define set_oct(b, i, n) ((b) & (~(7 << ((i) * 3))) | ((n) << ((i) * 3)))

//-----------------------------------------------------------------------------

float cosi(int);
float sini(int);

//-----------------------------------------------------------------------------

#endif
