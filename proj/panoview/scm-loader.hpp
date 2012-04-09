//  Copyright (C) 2005-2011 Robert Kooima
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

#ifndef PANOVIEW_SCM_LOADER_HPP
#define PANOVIEW_SCM_LOADER_HPP

#include <gui-gui.hpp>

//------------------------------------------------------------------------------

class scm_viewer;

class scm_loader : public gui::dialog
{
	gui::string *status;

public:

    scm_loader(scm_viewer *, int, int);

    void set_status(int c);
};

//------------------------------------------------------------------------------

#endif