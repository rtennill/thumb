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

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "app-data.hpp"
#include "app-conf.hpp"
#include "util.hpp"
#include "dir.hpp"

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

//=============================================================================
// File system data archive

app::file_buffer::file_buffer(std::string name)
{
    struct stat info;
    int fd;

    // Open the named file for reading and determine its size.

#ifndef _WIN32
    if ((fd = open(name.c_str(), O_RDONLY)) == -1)
        throw open_error(name);
#else
    if ((fd = open(name.c_str(), O_RDONLY | O_BINARY)) == -1)
        throw open_error(name);
#endif

    if (fstat(fd, &info) != 0)
        throw stat_error(name);

    // Initialize the buffer.

    len = (size_t) info.st_size;
    ptr = new unsigned char[len + 1];

    // Read all data.

    if (read(fd, ptr, len) < (ssize_t) len)
        throw read_error(name);

    // Null-terminate.  (This will be a problem if we mmap.)

    ptr[len] = 0;

    close(fd);
}

bool app::file_archive::find(std::string name) const
{
    std::string curr = path + "/" + name;

    // Determine whether the named file exists within this archive.

    struct stat info;

    if (stat(curr.c_str(), &info) == 0 && (info.st_mode & S_IFMT) == S_IFREG)
        return true;
    else
        return false;
}

app::buffer_p app::file_archive::load(std::string name) const
{
    std::string curr = path + "/" + name;

    return new file_buffer(curr);
}

bool app::file_archive::save(std::string name,
                             const void *ptr, size_t *len) const
{
    std::string curr = path + "/" + name;

    // Ensure the archive is writable and the directory exists.

    if (writable && mkpath(curr))
    {
        size_t count = len ? (*len) : strlen((const char *) ptr);
        int fd;

        // Open the named file for writing.

        if ((fd = open(curr.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0666)) ==-1)
            throw open_error(name);

        // Write all data.

        if (write(fd, ptr, count) < (ssize_t) count)
            throw write_error(name);

        close(fd);

        return true;
    }
    return false;
}

void app::file_archive::list(std::string name, str_set& dirs,
                                               str_set& regs) const
{
    std::string curr = path + "/" + name;

    dir(curr, dirs, regs);
}

//=============================================================================

std::string app::data::translate(const std::string& filename) const
{
    // Try to map the named file onto a configured alternative.

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

app::data::data(const std::string& filename) : filename(filename), file("")
{
    std::string path;

    // Locate a read-write archive.

    if (find_rw_data(path))
        archives.push_back(new file_archive(path, true));

    // Locate a read-only archive.

    if (find_ro_data(path))
        archives.push_back(new file_archive(path, false));
}

app::data::~data()
{
    std::list<archive_p>::iterator i;

    // Delete all archives.

    for (i = archives.begin(); i != archives.end(); ++i)
        delete *i;
}

void app::data::init()
{
    // The database is a chicken and its configuration is an egg.

    if (!file.get_root()) file = app::file(filename);
}

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

bool app::data::save(const std::string& name, const void *ptr, size_t *len)
{
    // Search the list of archives for the first one that can save this buffer.

    for (archive_c i = archives.begin(); i != archives.end(); ++i)
        if ((*i)->save(name, ptr, len))
            return true;

    return false;
}

void app::data::list(const std::string& name, str_set& dirs,
                                              str_set& regs) const
{
    // Merge the file lists of all archives.

    for (archive_c i = archives.begin(); i != archives.end(); ++i)
        (*i)->list(name, dirs, regs);
}

void app::data::free(const std::string& name)
{
    // If the named buffer has been loaded, free it.

    if (buffers.find(name) != buffers.end())
    {
        delete buffers[name];
        buffers.erase(name);
    }
}

//=============================================================================
