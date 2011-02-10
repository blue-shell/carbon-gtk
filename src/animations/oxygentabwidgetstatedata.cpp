/*
* this file is part of the oxygen gtk engine
* Copyright (c) 2010 Hugo Pereira Da Costa <hugo@oxygen-icons.org>
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

#include "oxygentabwidgetstatedata.h"
#include "../oxygengtkutils.h"
#include "../config.h"

#include <iostream>

namespace Oxygen
{

    //_____________________________________________
    const int TabWidgetStateData::IndexInvalid = -1;

    //_____________________________________________
    void TabWidgetStateData::connect( GtkWidget* widget )
    {

        #if OXYGEN_DEBUG
        std::cerr << "Oxygen::TabWidgetStateData::connect - " << widget << " (" << G_OBJECT_TYPE_NAME( widget ) << ")" << std::endl;
        #endif

        _target = widget;

        // connect timeLines
        _current._timeLine.connect( (GSourceFunc)delayedUpdate, this );
        _previous._timeLine.connect( (GSourceFunc)delayedUpdate, this );

        // set directions
        _current._timeLine.setDirection( TimeLine::Forward );
        _previous._timeLine.setDirection( TimeLine::Backward );

    }

    //_____________________________________________
    void TabWidgetStateData::disconnect( GtkWidget* widget )
    {
        #if OXYGEN_DEBUG
        std::cerr << "Oxygen::TabWidgetStateData::disconnect - " << widget << " (" << G_OBJECT_TYPE_NAME( widget ) << ")" << std::endl;
        #endif

        _current._timeLine.disconnect();
        _current._index = IndexInvalid;

        _previous._timeLine.disconnect();
        _previous._index = IndexInvalid;

        _target = 0L;

    }

    //_____________________________________________
    bool TabWidgetStateData::updateState( int index, bool state )
    {
        if( state && index != _current._index )
        {

            // stop current animation if running
            if( _current._timeLine.isRunning() ) _current._timeLine.stop();

            // stop previous animation if running
            if( _current._index != IndexInvalid )
            {
                if( _previous._timeLine.isRunning() ) _previous._timeLine.stop();

                // move current tab index to previous
                _previous._index = _current._index;
                _previous._timeLine.start();
            }

            // assign new index to current and start animation
            _current._index = index;
            if( _current._index != IndexInvalid ) _current._timeLine.start();

            return true;

        } else if( (!state) && index == _current._index ) {

            // stop current animation if running
            if( _current._timeLine.isRunning() ) _current._timeLine.stop();

            // stop previous animation if running
            if( _previous._timeLine.isRunning() ) _previous._timeLine.stop();

            // move current tab index to previous
            _previous._index = _current._index;
            if( _previous._index != IndexInvalid ) _previous._timeLine.start();

            // assign invalid index to current
            _current._index = IndexInvalid;

            return true;

        } else return false;

    }

    //_____________________________________________
    gboolean TabWidgetStateData::delayedUpdate( gpointer pointer )
    {

        TabWidgetStateData& data( *static_cast<TabWidgetStateData*>( pointer ) );

        if( !data._target ) return FALSE;

        // FIXME: it might be enough to union the rects of the current and previous tab
        if( GTK_IS_NOTEBOOK( data._target ) )
        {

            GdkRectangle rect( Gtk::gdk_rectangle() );
            Gtk::gtk_notebook_get_tabbar_rect( GTK_NOTEBOOK( data._target ), &rect );
            Gtk::gtk_widget_queue_draw( data._target, &rect );

        } else {

            gtk_widget_queue_draw( data._target );

        }

        return FALSE;

    }

}
