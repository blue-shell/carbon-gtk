#ifndef oxygentreeviewengine_h
#define oxygentreeviewengine_h
/*
* this file is part of the oxygen gtk engine
* Copyright (c) 2010 Hugo Pereira Da Costa <hugo@oxygen-icons.org>
* Copyright (c) 2010 Ruslan Kabatsayev <b7.10110111@gmail.com>
*
* This  library is free  software; you can  redistribute it and/or
* modify it  under  the terms  of the  GNU Lesser  General  Public
* License  as published  by the Free  Software  Foundation; either
* version 2 of the License, or(at your option ) any later version.
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

#include "oxygengenericengine.h"
#include "oxygendatamap.h"
#include "oxygentreeviewdata.h"

#include <gtk/gtk.h>

namespace Oxygen
{
    //! forward declaration
    class Animations;

    //! stores data associated to editable comboboxes
    /*!
    ensures that the text entry and the button of editable comboboxes
    gets hovered and focus flags at the same time
    */
    class TreeViewEngine: public GenericEngine<TreeViewData>
    {

        public:

        //! constructor
        TreeViewEngine( Animations* parent ):
            GenericEngine<TreeViewData>( parent )
            {}

        //! destructor
        virtual ~TreeViewEngine( void )
        {}

        //! register widget
        virtual bool registerWidget( GtkWidget* widget, bool drawTreeBranchLines )
        {
            if( GenericEngine<TreeViewData>::registerWidget( widget ) && GTK_IS_TREE_VIEW( widget ) )
            {
                gtk_tree_view_set_show_expanders( GTK_TREE_VIEW( widget ), true );
                gtk_tree_view_set_enable_tree_lines( GTK_TREE_VIEW( widget ), drawTreeBranchLines );
            }
            return true;
        }

        //! true if widget is hovered
        bool hovered( GtkWidget* widget )
        { return data().value( widget ).hovered(); }

        //! true if given cell is hovered
        bool isCellHovered( GtkWidget* widget, int x, int y, int w, int h )
        { return data().value( widget ).isCellHovered( x, y, w, h ); }

        bool isCellHovered( GtkWidget* widget, int x, int y, int w, int h, bool fullWidth )
        { return data().value( widget ).isCellHovered( x, y, w, h, fullWidth ); }

    };

}

#endif
