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

#include <bzlib.h>
#include <libxml/xmlreader.h>

struct BZ2Context {
    FILE* file;
    BZFILE* bz2;
    bool error;
    int nUnused;
    char unused[BZ_MAX_UNUSED];
};

int readFromBz2Stream( void* pointer, char* buffer, int len )
{
    void *unusedTmpVoid=NULL;
    char *unusedTmp=NULL;
    BZ2Context* context = (BZ2Context*) pointer;
    if ( len == 0 || context->error || feof(context->file) )
        return -1;
    int error = 0;
    int read = BZ2_bzRead( &error, context->bz2, buffer, len );
    if ( error == BZ_OK )
        return read;

    if ( error == BZ_STREAM_END )
    {
        BZ2_bzReadGetUnused(&error, context->bz2, &unusedTmpVoid, &context->nUnused);
        unusedTmp = (char*)unusedTmpVoid;
        for(int i=0;i<context->nUnused;i++) {
            context->unused[i] = unusedTmp[i];
        }
        BZ2_bzReadClose(&error, context->bz2);
        if(BZ_OK != error) { context->error = true; return 0;};
        context->bz2 = BZ2_bzReadOpen(&error, context->file, 0, 0, context->unused, context->nUnused);
        if(NULL == context->bz2) { context->error = true; return 0;};
        return read;
    }
    context->error = true;
    return 0;
}

int closeBz2Stream( void *pointer )
{
    BZ2Context* context = (BZ2Context*) pointer;
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
        BZ2Context* context = new BZ2Context;
        context->error = false;
        context->file = fopen( name, "r" );
        int error;
        context->bz2 = BZ2_bzReadOpen( &error, context->file, 0, 0, context->unused, context->nUnused );
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
