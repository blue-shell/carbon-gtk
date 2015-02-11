/*
* this file is part of the carbon gtk engine
* Copyright (c) 2010 Hugo Pereira Da Costa <hugo.pereira@free.fr>
* Copyright (c) 2010 Ruslan Kabatsayev <b7.10110111@gmail.com>
*
* based on the Null Theme Engine for Gtk+.
* Copyright (c) 2008 Robert Staudinger <robert.staudinger@gmail.com>
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

#include "carbontheme.h"

#include "config.h"
#include "carbonrcstyle.h"
#include "carbonstyle.h"
#include "carbonstylewrapper.h"
#include "carbonwindecooptions.h"
#include "carbonwindowshadow.h"
#include "carbontimelineserver.h"

#include <gmodule.h>
#include <gtk/gtk.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>


//_________________________________________________
int theme_exit_callback( gpointer )
{
    theme_exit();
    return false;
}

//_________________________________________________
void theme_init( GTypeModule* module )
{

    #if CARBON_DEBUG
    std::cerr << "Carbon::theme_init" << std::endl;
    #endif

    Carbon::RCStyle::registerType( module );
    Carbon::StyleWrapper::registerType( module );
    Carbon::StyleWrapper::registerVersionType();

    // style initialization
    Carbon::Style::instance().initialize();

//     // add quit function to make sure theme is de-allocated properly
//     gtk_quit_add( gtk_main_level(), theme_exit_callback, 0x0 );

}

//_________________________________________________
void theme_exit( void )
{

    #if CARBON_DEBUG
    std::cerr << "Carbon::theme_exit" << std::endl;
    #endif

    // delete style instance
    delete &Carbon::Style::instance();
    delete &Carbon::TimeLineServer::instance();

}

//_________________________________________________
GtkRcStyle* theme_create_rc_style( void )
{ return GTK_RC_STYLE( g_object_new( Carbon::RCStyle::type(), NULL ) ); }

//_________________________________________________
const gchar* g_module_check_init( GModule *module )
{
    return gtk_check_version(
        GTK_MAJOR_VERSION,
        GTK_MINOR_VERSION,
        GTK_MICRO_VERSION - GTK_INTERFACE_AGE );
}

//_________________________________________________
void drawWindowDecoration(cairo_t* context, unsigned long options, gint x,gint y,gint w,gint h, const gchar** ws, gint til, gint tir)
{
    Carbon::Style::instance().drawWindowDecoration( context, (Carbon::WinDeco::Options) options, x, y, w, h, ws, til, tir);
}

//_________________________________________________
void drawWindecoButton(cairo_t* context, unsigned long buttonType,unsigned long buttonState, unsigned long windowState, gint x,gint y,gint w,gint h)
{
    Carbon::Style::instance().drawWindecoButton(
        context, (Carbon::WinDeco::ButtonType)buttonType,
        (Carbon::WinDeco::ButtonStatus)buttonState, (Carbon::WinDeco::Options) windowState, x, y, w, h);
}

//_________________________________________________
void drawWindecoShapeMask(cairo_t* context, unsigned long options, gint x,gint y,gint w,gint h)
{
    Carbon::Style::instance().drawWindecoShapeMask( context, (Carbon::WinDeco::Options) options, x, y, w, h);
}

//_________________________________________________
void drawWindowShadow(cairo_t* context, unsigned long options, gint x, gint y, gint w, gint h)
{
    Carbon::Style::instance().drawWindowShadow(context,(Carbon::WinDeco::Options) options, x, y, w, h);
}

//_________________________________________________
gint getWindecoMetric(unsigned long wm)
{
    return Carbon::WinDeco::getMetric((Carbon::WinDeco::Metric)wm);
}

//_________________________________________________
gint getWindecoButtonSize(unsigned long buttonType)
{
    return Carbon::WinDeco::getButtonSize();
}

//_________________________________________________
unsigned long getWindecoABIVersion(void)
{ return 0x3; }
