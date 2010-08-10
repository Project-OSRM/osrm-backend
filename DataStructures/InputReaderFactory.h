/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef INPUTREADERFACTORY_H
#define INPUTREADERFACTORY_H

#include <stdio.h>
#include <string.h>
#include <libxml/xmlreader.h>

#include <bzlib.h>

struct Context {
    FILE* file;
    BZFILE* bz2;
    bool error;
};

int readFromBz2Stream( void* pointer, char* buffer, int len )
{
    Context* context = (Context*) pointer;
    if ( len == 0 || context->error )
        return 0;

    int error = 0;
    int read = BZ2_bzRead( &error, context->bz2, buffer, len );
    if ( error == BZ_OK )
        return read;

    context->error = true;
    if ( error == BZ_STREAM_END )
        return read;
    return 0;
}

int closeBz2Stream( void *pointer )
{
    Context* context = (Context*) pointer;
    BZ2_bzclose( context->bz2 );
    fclose( context->file );
    delete context;
    return 0;
}

xmlTextReaderPtr inputReaderFactory( const char* name )
{
    std::string inputName(name);

    if(inputName.find(".osm.bz2")!=string::npos)
    {
        Context* context = new Context;
        context->error = false;
        context->file = fopen( name, "r" );
        int error;
        context->bz2 = BZ2_bzReadOpen( &error, context->file, 0, 0, NULL, 0 );
        if ( context->bz2 == NULL || context->file == NULL ) {
            delete context;
            return NULL;
        }
        return xmlReaderForIO( readFromBz2Stream, closeBz2Stream, (void*) context, NULL, NULL, 0 );
    } else {
        return xmlNewTextReaderFilename(name);
    }
}

#endif // INPUTREADERFACTORY_H
