//  Copyright (C) 2011-2014 Robert Kooima
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

#ifndef ETC_LOG_HPP
#define ETC_LOG_HPP

#include <string>

//------------------------------------------------------------------------------

namespace etc {
    void log(const char *, ...);
    void log(const std::string&);
}

//------------------------------------------------------------------------------

#endif