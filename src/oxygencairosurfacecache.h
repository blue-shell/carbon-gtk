#ifndef oxygencairosurfacecache_h
#define oxygencairosurfacecache_h

/*
* this file is part of the oxygen gtk engine
* Copyright (c) 2010 Hugo Pereira Da Costa <hugo@oxygen-icons.org>
*
* This  library is free  software; you can  redistribute it and/or
* modify it  under  the terms  of the  GNU Lesser  General  Public
* License  as published  by the Free  Software  Foundation; either
* version 2 of the License, or( at your option ) any later version.
*
* This library is distributed  in the hope that it will be useful,
* but  WITHOUT ANY WARRANTY; without even  the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License  along  with  this library;  if not,  write to  the Free
* Software Foundation, Inc., 51  Franklin St, Fifth Floor, Boston,
* MA 02110-1301, USA.
*/

#include "oxygencache.h"

#include <cairo.h>

namespace Oxygen
{

    template< typename T>
    class CairoSurfaceCache: public Cache<T, cairo_surface_t*>
    {

        public:

        //! constructor
        CairoSurfaceCache( void )
        {}

        //! destructor
        virtual ~CairoSurfaceCache( void )
        {}

        protected:

        //! erase value from map
        virtual void erase( cairo_surface_t*& pixbuf )
        { g_object_unref( pixbuf ); }

        //! default value
        virtual cairo_surface_t* defaultValue( void ) const
        { return 0L; }

    };

}

#endif
