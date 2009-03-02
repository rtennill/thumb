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

#ifndef APP_SERIAL_HPP
#define APP_SERIAL_HPP

#include <string>
#include <map>

#include <mxml.h>

//-----------------------------------------------------------------------------

namespace app
{
    typedef mxml_node_t *node;

    class serial
    {
    private:

        std::string file;
        node        head;

        void load();
        void save();

    public:

        serial(const std::string&);
       ~serial();

        node get_head() { return head; }
    };

    // Serialization attribute mutators

    void        set_attr_d(node, const std::string&, int = 0);
    int         get_attr_d(node, const std::string&, int = 0);

    void        set_attr_f(node, const std::string&, double = 0);
    double      get_attr_f(node, const std::string&, double = 0);

    void        set_attr_s(node, const std::string&, const std::string& = "");
    std::string get_attr_s(node, const std::string&, const std::string& = "");

    // Element iteration

    node find(node, const std::string& = "",
                    const std::string& = "",
                    const std::string& = "");
    node next(node,
              node, const std::string& = "",
                    const std::string& = "",
                    const std::string& = "");

    // Mutators

    node create(const std::string&);
    void insert(node, node, node);
    void remove(node);
}

//-----------------------------------------------------------------------------

#endif
