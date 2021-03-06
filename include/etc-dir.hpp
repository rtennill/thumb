//  Copyright (C) 2007-2011 Robert Kooima
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

#ifndef ETC_DIR_HPP
#define ETC_DIR_HPP

#include <string>
#include <set>

//-----------------------------------------------------------------------------

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#define PATH_LIST_SEPARATOR ';'
#else
#define PATH_SEPARATOR '/'
#define PATH_LIST_SEPARATOR ':'
#endif

std::string fixpath(std::string path);

std::string pathname(std::string, std::string);

void dir(std::string, std::set<std::string>&,
                      std::set<std::string>&);

bool mkpath(std::string, bool=true);

bool is_dir(const std::string&);
bool is_reg(const std::string&);

//-----------------------------------------------------------------------------

bool get_app_res_path(std::string& path);

//-----------------------------------------------------------------------------

#endif
