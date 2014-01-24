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

#ifndef APP_DATA_PACK_HPP
#define APP_DATA_PACK_HPP

#include <app-data.hpp>

//-----------------------------------------------------------------------------

namespace app
{
    // Packaged data archive

    class pack_archive : public archive
    {
    public:

        pack_archive(const void *ptr, size_t len) : ptr(ptr), len(len) { }

        virtual bool     find(std::string)                         const;
        virtual buffer_p load(std::string)                         const;
        virtual bool     save(std::string, const void *, size_t *) const;
        virtual void     list(std::string, str_set&, str_set&)     const;

    private:

        const void *ptr;
        size_t      len;

        int         get_file_count()            const;
        const void *get_file_first()            const;
        const void *get_file_next(const void *) const;
        std::string get_file_name(const void *) const;

    };
}

//-----------------------------------------------------------------------------

#endif
