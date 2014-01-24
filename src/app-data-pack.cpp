//  Copyright (C) 2014 Robert Kooima
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

#include <app-data-pack.hpp>
#include <app-conf.hpp>

//-----------------------------------------------------------------------------

#pragma pack(push,2)

struct local_file_header
{
    uint32_t signature;
    uint16_t reader_version;
    uint16_t flags;
    uint16_t compression;
    uint16_t modified_time;
    uint16_t modified_date;
    uint32_t crc;
    uint32_t sizeof_compressed;
    uint32_t sizeof_uncompressed;
    uint16_t sizeof_name;
    uint16_t sizeof_extra;
};

struct file_header
{
    uint32_t signature;
    uint16_t writer_version;
    uint16_t reader_version;
    uint16_t flags;
    uint16_t compression;
    uint16_t modified_time;
    uint16_t modified_date;
    uint32_t crc;
    uint32_t sizeof_compressed;
    uint32_t sizeof_uncompressed;
    uint16_t sizeof_name;
    uint16_t sizeof_extra;
    uint16_t sizeof_comment;
    uint16_t disk_number_start;
    uint16_t internal_attr;
    uint32_t external_attr;
    uint32_t offset;
};

struct eocd
{
    uint32_t signature;
    uint16_t disk_number;
    uint16_t disk_number_start;
    uint16_t file_count;
    uint16_t file_count_total;
    uint32_t directory_size;
    uint32_t directory_offset;
    uint16_t sizeof_comment;
};

#pragma pack(pop)

//-----------------------------------------------------------------------------

// Return the number of file headers in the central directory.

int app::pack_archive::get_file_count() const
{
    const eocd *d = (const eocd *) ((const char *) ptr + len) - 1;

    if (d->signature == 0x06054B50)
        return d->file_count;
    else
        return 0;
}

// Return a pointer to the first file header in the central directory.

const void *app::pack_archive::get_file_first() const
{
    const eocd *d = (const eocd *) ((const char *) ptr + len) - 1;

    if (d->signature == 0x06054B50)
        return (const char *) ptr + d->directory_offset;
    else
        return 0;
}

// Return a pointer to the file header following the given file header.

const void *app::pack_archive::get_file_next(const void *p) const
{
    const file_header *f = (const file_header *) p;

    return (const char *) (f + 1) + f->sizeof_name
                                  + f->sizeof_extra
                                  + f->sizeof_comment;
}

// Return the name of the file at the given file header.

std::string app::pack_archive::get_file_name(const void *p) const
{
    const file_header *f = (const file_header *) p;

    return std::string((const char *) (f + 1), f->sizeof_name);
}

//-----------------------------------------------------------------------------

// Determine whether the named file exists within this archive.

bool app::pack_archive::find(std::string name) const
{
    return false;
}

// Return a buffer containing the named data file.

app::buffer_p app::pack_archive::load(std::string name) const
{
    return 0;
}

// Save the given buffer to the ZIP archive. This will always fail.

bool app::pack_archive::save(std::string name,
                             const void *ptr, size_t *len) const
{
    return false;
}

// Populate lists of all directories and regular files at the given path.

void app::pack_archive::list(std::string dirname, str_set& dirs,
                                                  str_set& regs) const
{
    const std::string path = dirname.empty() ? dirname : dirname + "/";

    int n = get_file_count();

    for (const void *p = get_file_first(); p && n; p = get_file_next(p), n--)
    {
        // If the path matches, add the name to the file or directory list.

        const std::string pathname = get_file_name(p);

        if (pathname.compare(0, path.size(), path) == 0)
        {
            std::string name(pathname, path.size());
            std::string::size_type n = name.find('/');

            if (n == std::string::npos)
                regs.insert(name);
            else
                dirs.insert(std::string(name, 0, n));
        }
    }
}

//-----------------------------------------------------------------------------
