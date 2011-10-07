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
    int error;
    int nUnused;
    char unused[BZ_MAX_UNUSED];
};

int readFromBz2Stream( void* pointer, char* buffer, int len ) {
    void *unusedTmpVoid=NULL;
    char *unusedTmp=NULL;
    BZ2Context* context = (BZ2Context*) pointer;
    int read = 0;
    while(0 == read && !(BZ_STREAM_END == context->error && 0 == context->nUnused && feof(context->file))) {
        read = BZ2_bzRead(&context->error, context->bz2, buffer, len);
        if(BZ_OK == context->error) {
            return read;
        } else if(BZ_STREAM_END == context->error) {
            BZ2_bzReadGetUnused(&context->error, context->bz2, &unusedTmpVoid, &context->nUnused);
            if(BZ_OK != context->error) { cerr << "Could not BZ2_bzReadGetUnused" << endl; exit(-1);};
            unusedTmp = (char*)unusedTmpVoid;
            for(int i=0;i<context->nUnused;i++) {
                context->unused[i] = unusedTmp[i];
            }
            BZ2_bzReadClose(&context->error, context->bz2);
            if(BZ_OK != context->error) { cerr << "Could not BZ2_bzReadClose" << endl; exit(-1);};
            context->error = BZ_STREAM_END; // set to the stream end for next call to this function
            if(0 == context->nUnused && feof(context->file)) {
                return read;
            } else {
                context->bz2 = BZ2_bzReadOpen(&context->error, context->file, 0, 0, context->unused, context->nUnused);
                if(NULL == context->bz2){ cerr << "Could not open file" << endl; exit(-1);};
            }
        } else { cerr << "Could not read bz2 file" << endl; exit(-1); }
    }
    return read;
}

int closeBz2Stream( void *pointer )
{
    BZ2Context* context = (BZ2Context*) pointer;
    fclose( context->file );
    delete context;
    return 0;
}

xmlTextReaderPtr inputReaderFactory( const char* name )
{
    std::string inputName(name);

    if(inputName.find(".osm.bz2")!=string::npos)
    {
        BZ2Context* context = new BZ2Context();
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
