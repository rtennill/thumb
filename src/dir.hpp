//  Copyright (C) 2007 Robert Kooima
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

#ifndef DIR_HPP
#define DIR_HPP

#include <string>
#include <set>

//-----------------------------------------------------------------------------

void dir(std::string, std::set<std::string>&,
                      std::set<std::string>&);

bool mkpath(std::string, bool=true);

bool find_ro_data(std::string&);
bool find_rw_data(std::string&);

//-----------------------------------------------------------------------------

#endif
