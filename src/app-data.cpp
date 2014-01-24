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

#include <stdexcept>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <app-default.hpp>
#include <app-data.hpp>
#include <app-data-pack.hpp>
#include <app-data-file.hpp>
#include <app-conf.hpp>
#include <etc-dir.hpp>

//-----------------------------------------------------------------------------

app::buffer::buffer() : ptr(0), len(0)
{
}

app::buffer::~buffer()
{
    if (ptr) delete [] ptr;
}

const void *app::buffer::get(size_t *size) const
{
    if (size)
       *size = len;

    return ptr;
}

//-----------------------------------------------------------------------------

// Try to map the named file onto a configured alternative.

std::string app::data::translate(const std::string& filename) const
{
    if (::conf && file.get_root())
    {
        // Locate the list of options for the named file.

        if (app::node n = file.get_root().find("file", "name", filename))
        {
            // Enumerate the options.

            for (app::node c = n.find("option"); c; c = n.next(c, "option"))
            {
                // If the option matches the config setting, return the target.

                const std::string name  = c.get_s("name");
                const std::string value = c.get_s("value");

                if (::conf->get_s(name) == value)
                    return c.get_s();
            }
        }
    }

    // No configured option was found.  Return the original string.

    return filename;
}

//-----------------------------------------------------------------------------

#ifdef _WIN32
#define PATH_LIST_SEP ';'
#else
#define PATH_LIST_SEP ':'
#endif

#define USR_SHARE "/usr/share/thumb/data"
#define LOC_SHARE "/usr/local/share/thumb/data"

#include "../data.hpp"

// Identify a valid data directory as one containing the default data file.

static bool is_data_dir(std::string dir)
{
    std::string file(dir + "/" + DEFAULT_DATA_FILE);

    if (is_dir(dir) && is_reg(file))
        return true;
    else
        return false;
}

// Locate all read-only data hierarchies.

static void find_ro_data(app::archive_l& archives)
{
    archives.push_back(new app::pack_archive(data_zip, data_zip_len));

    // Iterate the read-only path environment variable.

    if (char *val = getenv("THUMB_RO_PATH"))
    {
        std::stringstream list(val);
        std::string       path;

        while (std::getline(list, path, PATH_LIST_SEP))
            archives.push_back(new app::file_archive(path, false));
    }

    // Check the system share directory.

    if (is_data_dir(USR_SHARE))
        archives.push_back(new app::file_archive(USR_SHARE, false));

    // Check the system local share directory.

    if (is_data_dir(LOC_SHARE))
        archives.push_back(new app::file_archive(LOC_SHARE, false));
}

// Locate a writable data hierarchy.

static void find_rw_data(app::archive_l& archives)
{
    // Iterate the read-write path environment variable.

    if (char *val = getenv("THUMB_RW_PATH"))
    {
        std::stringstream list(val);
        std::string       path;

        while (std::getline(list, path, ':'))
            archives.push_back(new app::file_archive(path, true));
    }

    // Check for a home directory.

    else if (char *home = getenv("HOME"))
    {
        const std::string path = std::string(home) + "/.thumb";
        archives.push_back(new app::file_archive(path, true));
    }

    // As a last resort, use the current working directory.

    else
    {
        archives.push_back(new app::file_archive(".", true));
    }
}

//-----------------------------------------------------------------------------

app::data::data(const std::string& filename) : filename(filename), file("")
{
    find_ro_data(archives);
    find_rw_data(archives);
}

app::data::~data()
{
    std::list<archive_p>::iterator i;

    for (i = archives.begin(); i != archives.end(); ++i)
        delete *i;
}

// The database is a chicken and its configuration is an egg.

void app::data::init()
{
    if (file.get_root() == 0)
        file = app::file(filename);
}

// Return a buffer containing the named data file.

const void *app::data::load(const std::string& name, size_t *len)
{
    // If the named buffer has not yet been loaded, load it.

    if (buffers.find(name) == buffers.end())
    {
        // Search the list of archives for the first one with the named buffer.

        for (archive_c i = archives.begin(); i != archives.end(); ++i)
        {
            const std::string rename = translate(name);

            if ((*i)->find(rename))
            {
                buffers[name] = (*i)->load(rename);
                break;
            }
        }
    }

    // Return the named buffer.

    if (buffers.find(name) != buffers.end())
        return buffers[name]->get(len);
    else
    {
        throw find_error(name);
        return 0;
    }
}

// Scan the archives for the first one containing the named buffer.

bool app::data::find(const std::string& name)
{
    for (archive_c i = archives.begin(); i != archives.end(); ++i)
    {
        const std::string rename = translate(name);

        if ((*i)->find(rename))
            return true;
    }
    return false;
}

// Scan the archives for the first one that can save this buffer. Save it.

bool app::data::save(const std::string& name, const void *ptr, size_t *len)
{
    for (archive_c i = archives.begin(); i != archives.end(); ++i)
        if ((*i)->save(name, ptr, len))
            return true;

    return false;
}

// Merge the regular file and directory lists of all archives.

void app::data::list(const std::string& name, str_set& dirs,
                                              str_set& regs) const
{
    for (archive_c i = archives.begin(); i != archives.end(); ++i)
        (*i)->list(name, dirs, regs);
}

// If the named buffer has been loaded, free it.

void app::data::free(const std::string& name)
{
    if (buffers.find(name) != buffers.end())
    {
        delete buffers[name];
        buffers.erase(name);
    }
}

//-----------------------------------------------------------------------------
