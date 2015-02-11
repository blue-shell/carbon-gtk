/*
* this file is part of the carbon gtk engine
* Copyright (c) 2010 Hugo Pereira Da Costa <hugo.pereira@free.fr>
* Copyright (c) 2010 Ruslan Kabatsayev <b7.10110111@gmail.com>
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

#include "carbonqtsettings.h"
#include "carboncoloreffect.h"
#include "carboncolorutils.h"
#include "carbonfontinfo.h"
#include "carbongtkicons.h"
#include "carbongtkrc.h"
#include "carbonshadowhelper.h"
#include "carbontimeline.h"
#include "config.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#endif

namespace Carbon
{

    //_________________________________________________________
    const std::string QtSettings::_defaultKdeIconPath = "/usr/share/icons/";

    //_________________________________________________________
    /*
    Note: the default values set in the constructor are actually ignored.
    Real default values are set via Carbon::QtSettings::loadKdeGlobalsOptions,
    from the carbonrc file provided with carbon-gtk
    */
    QtSettings::QtSettings( void ):
        _wmShadowsSupported( false ),
        _kdeIconTheme( "carbon" ),
        _kdeFallbackIconTheme( "gnome" ),
        _inactiveChangeSelectionColor( false ),
        _useIconEffect( true ),
        _useBackgroundGradient( true ),
        _checkBoxStyle( CS_CHECK ),
        _tabStyle( TS_SINGLE ),
        _scrollBarAddLineButtons( 2 ),
        _scrollBarSubLineButtons( 1 ),
        _toolBarDrawItemSeparator( true ),
        _tooltipTransparent( true ),
        _tooltipDrawStyledFrames( true ),
        _viewDrawFocusIndicator( true ),
        _viewDrawTreeBranchLines( true ),
        _viewDrawTriangularExpander( true ),
        _viewTriangularExpanderSize( ArrowSmall ),
        _viewInvertSortIndicator( false ),
        _menuHighlightMode( MM_DARK ),
        _windowDragEnabled( true ),
        _windowDragMode( WD_FULL ),
        _useWMMoveResize( true ),
        _startDragDist( 4 ),
        _startDragTime( 500 ),
        _animationsEnabled( true ),
        _genericAnimationsEnabled( true ),
        _menuBarAnimationType( Fade ),
        _menuAnimationType( Fade ),
        _toolBarAnimationType( Fade ),
        _genericAnimationsDuration( 150 ),
        _menuBarAnimationsDuration( 150 ),
        _menuBarFollowMouseAnimationsDuration( 80 ),
        _menuAnimationsDuration( 150 ),
        _menuFollowMouseAnimationsDuration( 40 ),
        _toolBarAnimationsDuration( 50 ),
        _buttonSize( ButtonDefault ),
        _frameBorder( BorderDefault ),
        _windecoBlendType( FollowStyleHint ),
        _activeShadowConfiguration( Palette::Active ),
        _inactiveShadowConfiguration( Palette::Inactive ),
        _backgroundOpacity( 255 ),
        _argbEnabled( true ),
        _widgetExplorerEnabled( true ),
        _initialized( false ),
        _kdeColorsInitialized( false ),
        _gtkColorsInitialized( false ),
        _KDESession( false )
    {}

    //_________________________________________________________
    bool QtSettings::initialize( unsigned int flags )
    {

        const bool forced( flags&Forced );

        // no attempt at initializing if gtk settings is not yet set
        if( !gtk_settings_get_default() ) return false;

        if( _initialized && !forced ) return false;
        else if( !forced ) _initialized = true;

        if( g_getenv( "KDE_FULL_SESSION" ) )
        { _KDESession = true; }

        // init application name
        if( flags & AppName )
        {
            initUserConfigDir();
            initApplicationName();
            initArgb();
        }

        // keep track of whats changed
        bool changed( false );

        // support for wm shadows
        {
            const bool wmShadowsSupported( isAtomSupported( ShadowHelper::netWMShadowAtomName ) );
            if( wmShadowsSupported != _wmShadowsSupported )
            {
                _wmShadowsSupported = wmShadowsSupported;
                changed |= true;
            }
        }

        // configuration path
        {
            const PathList old( _kdeConfigPathList );
            _kdeConfigPathList = kdeConfigPathList();
            changed |= (old != _kdeConfigPathList );
        }

        // icon path
        {
            const PathList old( _kdeIconPathList );
            _kdeIconPathList = kdeIconPathList();
            changed |= (old != _kdeIconPathList );
        }

        // load kdeglobals and carbon option maps
        const bool kdeGlobalsChanged = loadKdeGlobals();
        const bool carbonChanged = loadCarbon();

        // do nothing if settings not changed
        if( forced && !(changed||kdeGlobalsChanged||carbonChanged) ) return false;

        // dialog button ordering
        /* TODO: in principle this should be needed only once */
        if( flags & Extra )
        {
            GtkSettings* settings( gtk_settings_get_default() );
            gtk_settings_set_long_property( settings, "gtk-alternative-button-order", 1, "carbon-gtk" );
        }

        // clear gtkrc
        _rc.clear();

        // kde globals options
        if( flags & KdeGlobals )
        { loadKdeGlobalsOptions(); }

        // carbon options
        if( flags & Carbon )
        { loadCarbonOptions(); }

        #if !CARBON_FORCE_KDE_ICONS_AND_FONTS
        // TODO: Add support for gtk schemes when not _KDESession
        if( _KDESession )
        #endif
        {

            // reload fonts
            if( flags & Fonts )
            { loadKdeFonts(); }

            // reload icons
            #if CARBON_ICON_HACK
            if( flags & Icons )
            { loadKdeIcons(); }
            #endif

        }

        // color palette
        if( flags & Colors )
        {
            loadKdePalette( forced );
            generateGtkColors();
        }

        // apply extra programatically set metrics metrics
        if( flags & Extra )
        { loadExtraOptions(); }

        // print generated Gtkrc and commit
        #if CARBON_DEBUG
        std::cerr << "Carbon::QtSettings::initialize - Gtkrc: " << std::endl;
        std::cerr << _rc << std::endl;
        #endif

        // pass all resources to gtk and clear
        _rc.commit();

        #if CARBON_DEBUG
        std::cerr << "Carbon::QtSettings::initialize - done. " << std::endl;
        #endif

        return true;

    }

    //_________________________________________________________
    bool QtSettings::runCommand( const std::string& command, char*& result ) const
    { return g_spawn_command_line_sync( command.c_str(), &result, 0L, 0L, 0L ) && result; }

    //_________________________________________________________
    bool QtSettings::loadKdeGlobals( void )
    {

        // save backup
        OptionMap old = _kdeGlobals;

        // clear and reload
        _kdeGlobals.clear();
        for( PathList::const_reverse_iterator iter = _kdeConfigPathList.rbegin(); iter != _kdeConfigPathList.rend(); ++iter )
        {
            const std::string filename( sanitizePath( *iter + "/kdeglobals" ) );
            _kdeGlobals.merge( OptionMap( filename ) );
            monitorFile( filename );
        }

        #if CARBON_DEBUG
        std::cerr << "Carbon::QtSettings::loadKdeGlobals - kdeglobals: " << std::endl;
        std::cerr << _kdeGlobals << std::endl;
        #endif

        // check change
        return old != _kdeGlobals;

    }

    //_______________________________________________________
    bool QtSettings::isAtomSupported( const std::string& atomNameQuery ) const
    {

        // create atom
        #ifdef GDK_WINDOWING_X11

        // get screen and check
        GdkScreen* screen = gdk_screen_get_default();
        if( !screen ) return false;

        // get display and check
        GdkDisplay *gdkDisplay( gdk_screen_get_display( screen ) );
        Display* display( GDK_DISPLAY_XDISPLAY( gdkDisplay ) );

        // create atom
        Atom netSupportedAtom( XInternAtom( display, "_NET_SUPPORTED", False) );
        if( !netSupportedAtom ) return false;

        // root window
        Window root( GDK_WINDOW_XID( gdk_screen_get_root_window( screen ) ) );
        if( !root ) return false;

        Atom type;
        int format;
        unsigned char *data;
        unsigned long count;
        unsigned long after;
        int length = 32768;

        while( true )
        {

            // get atom property on root window
            // length is incremented until after is zero
            if( XGetWindowProperty(
                display, root,
                netSupportedAtom, 0l, length,
                false, XA_ATOM, &type,
                &format, &count, &after, &data) != Success ) return false;

            if( after == 0 ) break;

            // free data, increase length
            XFree( data );
            length *= 2;
            continue;

        }

        Atom* atoms = reinterpret_cast<Atom*>( data );
        bool found( false );
        for( unsigned long i = 0; i<count && !found; i++ )
        {
            char* atomName = XGetAtomName( display, atoms[i]);
            if( strcmp( atomName, atomNameQuery.c_str() ) == 0 ) found = true;
            XFree( atomName );
        }

        return found;

        #else
        return false;
        #endif

    }

    //_________________________________________________________
    bool QtSettings::loadCarbon( void )
    {

        // save backup
        OptionMap old = _carbon;

        // clear and reload
        _carbon.clear();
        for( PathList::const_reverse_iterator iter = _kdeConfigPathList.rbegin(); iter != _kdeConfigPathList.rend(); ++iter )
        {
            const std::string filename( sanitizePath( *iter + "/carbonrc" ) );
            _carbon.merge( filename );
            monitorFile( filename );
        }

        #if CARBON_DEBUG
        std::cerr << "Carbon::QtSettings::loadCarbon - Carbonrc: " << std::endl;
        std::cerr << _carbon << std::endl;
        #endif

        // check change
        return old != _carbon;

    }

    //_________________________________________________________
    PathList QtSettings::kdeConfigPathList( void ) const
    {

        PathList out;

        // load icon install prefix
        gchar* path = 0L;
        if( runCommand( "kde4-config --path config", path ) && path )
        {

            out.split( path );
            g_free( path );

        } else {

            out.push_back( userConfigDir() );

        }

        out.push_back( GTK_THEME_DIR );

        #if CARBON_DEBUG
        std::cerr << "Carbon::QtSettings::kdeConfigPathList - loading configuration from path: " << std::endl;
        std::cerr << out << std::endl;
        #endif

        return out;

    }

    //_________________________________________________________
    PathList QtSettings::kdeIconPathList( void ) const
    {

        // load icon install prefix
        PathList out;
        char* path = 0L;
        if( runCommand( "kde4-config --path icon", path ) && path )
        {
            out.split( path );
            g_free( path );
        }

        // make sure defaultKdeIconPath is included in the list
        if( std::find( out.begin(), out.end(), _defaultKdeIconPath ) == out.end() )
        { out.push_back( _defaultKdeIconPath ); }

        return out;

    }

    //_________________________________________________________
    void QtSettings::initUserConfigDir( void )
    {

        // create directory name
        _userConfigDir = std::string( g_get_user_config_dir() ) + "/carbon-gtk";

        // make sure that corresponding directory does exist
        struct stat st;
        if( stat( _userConfigDir.c_str(), &st ) != 0 )
        {
            #ifdef G_OS_WIN32
            // S_IRWXG and S_IRWXO are undefined on Windows, and g_mkdir()
            // ignores its second parameter on Windows anyway.
            g_mkdir( _userConfigDir.c_str(), 0 );
            #else
            g_mkdir( _userConfigDir.c_str(), S_IRWXU|S_IRWXG|S_IRWXO );
            #endif
        }

        // note: in some cases, the target might exist and not be a directory
        // nothing we can do about it. We won't overwrite the file to prevent dataloss

    }

    //_________________________________________________________
    void QtSettings::initArgb( void )
    {
        // get program name
        const char* appName = g_get_prgname();
        if( !appName ) return;

        // user-defined configuration file
        const std::string userConfig( userConfigDir() + "/argb-apps.conf");

        // make sure user configuration file exists
        std::ofstream newConfig( userConfig.c_str(), std::ios::app );
        if( newConfig )
        {
            // if the file is empty (newly created), put a hint there
            if( !newConfig.tellp() )
            { newConfig << "# argb-apps.conf\n# Put your user-specific ARGB app settings here\n\n"; }
            newConfig.close();

        }

        // check for ARGB hack being disabled
        if(g_getenv("CARBON_DISABLE_ARGB_HACK"))
        {
            std::cerr << "Carbon::QtSettings::initArgb - ARGB hack is disabled; program name: " << appName << std::endl;
            std::cerr << "Carbon::QtSettings::initArgb - if disabling ARGB hack helps, please add this string:\n\ndisable:" << appName << "\n\nto ~/.config/carbon-gtk/argb-apps.conf\nand report it here: https://bugs.kde.org/show_bug.cgi?id=260640" << std::endl;
            _argbEnabled = false;
            return;
        }

        // get debug flag from environement
        const bool CARBON_ARGB_DEBUG = g_getenv("CARBON_ARGB_DEBUG");

        // read blacklist
        // system-wide configuration file
        const std::string configFile( std::string( GTK_THEME_DIR ) + "/argb-apps.conf" );
        std::ifstream systemIn( configFile.c_str() );
        if( !systemIn )
        {

            if( G_UNLIKELY(CARBON_DEBUG||CARBON_ARGB_DEBUG) )
            { std::cerr << "Carbon::QtSettings::initArgb - ARGB config file \"" << configFile << "\" not found" << std::endl; }

        }

        // load options into a string
        std::string contents;
        std::vector<std::string> lines;
        while( std::getline( systemIn, contents, '\n' ) )
        {
            if( contents.empty() || contents[0] == '#' ) continue;
            lines.push_back( contents );
        }

        // user specific blacklist
        std::ifstream userIn( userConfig.c_str() );
        if( !userIn )
        {

            if( G_UNLIKELY(CARBON_DEBUG||CARBON_ARGB_DEBUG) )
            { std::cerr << "Carbon::QtSettings::initArgb - user-defined ARGB config file \"" << userConfig << "\" not found - only system-wide one will be used" << std::endl; }

        }

        // load options into a string
        while( std::getline( userIn, contents, '\n' ) )
        {
            if( contents.empty() || contents[0] == '#' ) continue;
            lines.push_back( contents );
        }

        // true if application was found in one of the lines
        bool found( false );
        for( std::vector<std::string>::const_reverse_iterator iter = lines.rbegin(); iter != lines.rend() && !found; ++iter )
        {

            // store line locally
            std::string contents( *iter );

            // split string using ":" as a delimiter
            std::vector<std::string> appNames;
            size_t position( std::string::npos );
            while( ( position = contents.find( ':' ) ) != std::string::npos )
            {
                std::string appName( contents.substr(0, position ) );
                if( !appName.empty() ) { appNames.push_back( appName ); }
                contents = contents.substr( position+1 );
            }

            if( !contents.empty() ) appNames.push_back( contents );
            if( appNames.empty() ) continue;

            // check line type
            bool enabled( true );
            if( appNames[0] == "enable" ) enabled = true;
            else if( appNames[0] == "disable" ) enabled = false;
            else continue;

            // compare application names to this application
            for( unsigned int i = 1; i < appNames.size(); i++ )
            {
                if( appNames[i] == "all" || appNames[i] == appName )
                {
                    found = true;
                    _argbEnabled = enabled;
                    break;
                }
            }

        }

    }

    //_________________________________________________________
    void QtSettings::addIconTheme( PathList& pathList, const std::string& theme )
    {

        // do nothing if theme have already been included in the loop
        if( _iconThemes.find( theme ) != _iconThemes.end() ) return;
        _iconThemes.insert( theme );

        #if CARBON_DEBUG
        std::cerr << "Carbon::QtSettings::addIconTheme - adding " << theme << std::endl;
        #endif

        // add all possible path (based on _kdeIconPathList) and look for possible parent
        std::string parentTheme;
        for( PathList::const_iterator iter = _kdeIconPathList.begin(); iter != _kdeIconPathList.end(); ++iter )
        {

            // create path and check for existence
            std::string path( sanitizePath( *iter + '/' + theme ) );
            struct stat st;
            if( stat( path.c_str(), &st ) != 0 ) continue;

            // add to path list
            pathList.push_back( path );
            if( parentTheme.empty() )
            {
                const std::string index( sanitizePath( *iter + '/' + theme + "/index.theme" ) );
                OptionMap themeOptions( index );
                parentTheme = themeOptions.getValue( "[Icon Theme]", "Inherits" );
            }

        }

        // add parent if needed
        if( !parentTheme.empty() )
        {
            // split using "," as a separator
            PathList parentThemes( parentTheme, "," );
            for( PathList::const_iterator iter = parentThemes.begin(); iter != parentThemes.end(); ++iter )
            { addIconTheme( pathList, *iter ); }
        }

        return;

    }

    //_________________________________________________________
    void QtSettings::loadKdeIcons( void )
    {

        // update icon search path
        // put existing default path in a set
        PathSet searchPath( defaultIconSearchPath() );

        // add kde's path. Loop is reversed because added path must be prepended.
        for( PathList::const_reverse_iterator iter = _kdeIconPathList.rbegin(); iter != _kdeIconPathList.rend(); ++iter )
        {

            // remove trailing backslash, if any
            std::string path( *iter );
            if( path.empty() ) continue;
            if( path[path.size()-1] == '/' ) path = path.substr( 0, path.size()-1 );

            // check if already present and prepend if not
            if( searchPath.find( path ) == searchPath.end() )
            { gtk_icon_theme_prepend_search_path(gtk_icon_theme_get_default(), path.c_str() ); }
        }

        // load icon theme and path to gtk
        _iconThemes.clear();
        _kdeIconTheme = _kdeGlobals.getValue( "[Icons]", "Theme", "carbon" );

        // store to settings
        GtkSettings* settings( gtk_settings_get_default() );

        gtk_settings_set_string_property( settings, "gtk-icon-theme-name", _kdeIconTheme.c_str(), "carbon-gtk" );
        gtk_settings_set_string_property( settings, "gtk-fallback-icon-theme-name", _kdeFallbackIconTheme.c_str(), "carbon-gtk" );

        // load icon sizes from kde
        // const int desktopIconSize( _kdeGlobals.getOption( "[DesktopIcons]", "Size" ).toInt( 48 ) );
        const int dialogIconSize( _kdeGlobals.getOption( "[DialogIcons]", "Size" ).toInt( 32 ) );
        const int panelIconSize( _kdeGlobals.getOption( "[PanelIcons]", "Size" ).toInt( 32 ) );
        const int mainToolbarIconSize( _kdeGlobals.getOption( "[MainToolbarIcons]", "Size" ).toInt( 22 ) );
        const int smallIconSize( _kdeGlobals.getOption( "[SmallIcons]", "Size" ).toInt( 16 ) );
        const int toolbarIconSize( _kdeGlobals.getOption( "[ToolbarIcons]", "Size" ).toInt( 22 ) );

        // set gtk icon sizes
        _icons.setIconSize( "panel-menu", smallIconSize );
        _icons.setIconSize( "panel", panelIconSize );
        _icons.setIconSize( "gtk-small-toolbar", toolbarIconSize );
        _icons.setIconSize( "gtk-large-toolbar", mainToolbarIconSize );
        _icons.setIconSize( "gtk-dnd", mainToolbarIconSize );
        _icons.setIconSize( "gtk-button", smallIconSize );
        _icons.setIconSize( "gtk-menu", smallIconSize );
        _icons.setIconSize( "gtk-dialog", dialogIconSize );
        _icons.setIconSize( "", smallIconSize );

        // load translation table, generate full translation list, and path to gtk
        _icons.loadTranslations( sanitizePath( std::string( GTK_THEME_DIR ) + "/icons4" ) );

        // generate full path list
        PathList iconThemeList;
        addIconTheme( iconThemeList, _kdeIconTheme );
        addIconTheme( iconThemeList, _kdeFallbackIconTheme );

        _rc.merge( _icons.generate( iconThemeList ) );

    }

    //_________________________________________________________
    PathSet QtSettings::defaultIconSearchPath( void ) const
    {
        PathSet searchPath;

        // load icon theme
        GtkIconTheme* theme( gtk_icon_theme_get_default() );
        if( !GTK_IS_ICON_THEME( theme ) ) return searchPath;

        // get default
        gchar** gtkSearchPath;
        int nElements;

        gtk_icon_theme_get_search_path( theme, &gtkSearchPath, &nElements );
        for( int i=0; i<nElements; i++ )
        { searchPath.insert( gtkSearchPath[i] ); }

        // free
        g_strfreev( gtkSearchPath );

        return searchPath;
    }

    //_________________________________________________________
    void QtSettings::loadKdePalette( bool forced )
    {

        if( _kdeColorsInitialized && !forced ) return;
        _kdeColorsInitialized = true;

        // contrast
        ColorUtils::setContrast( _kdeGlobals.getOption( "[KDE]", "contrast" ).toVariant<double>( 7 ) / 10 );

        // palette
        _palette.clear();
        _palette.setColor( Palette::Active, Palette::Window, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[Colors:Window]", "BackgroundNormal" ) ) );
        _palette.setColor( Palette::Active, Palette::WindowText, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[Colors:Window]", "ForegroundNormal" ) ) );

        _palette.setColor( Palette::Active, Palette::Button, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[Colors:Button]", "BackgroundNormal" ) ) );
        _palette.setColor( Palette::Active, Palette::ButtonText, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[Colors:Button]", "ForegroundNormal" ) ) );

        _palette.setColor( Palette::Active, Palette::Selected, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[Colors:Selection]", "BackgroundNormal" ) ) );
        _palette.setColor( Palette::Active, Palette::SelectedText, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[Colors:Selection]", "ForegroundNormal" ) ) );

        _palette.setColor( Palette::Active, Palette::Tooltip, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[Colors:Tooltip]", "BackgroundNormal" ) ) );
        _palette.setColor( Palette::Active, Palette::TooltipText, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[Colors:Tooltip]", "ForegroundNormal" ) ) );

        _palette.setColor( Palette::Active, Palette::Focus, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[Colors:View]", "DecorationFocus" ) ) );
        _palette.setColor( Palette::Active, Palette::Hover, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[Colors:View]", "DecorationHover" ) ) );

        _palette.setColor( Palette::Active, Palette::Base, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[Colors:View]", "BackgroundNormal" ) ) );
        _palette.setColor( Palette::Active, Palette::BaseAlternate, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[Colors:View]", "BackgroundAlternate" ) ) );
        _palette.setColor( Palette::Active, Palette::Text, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[Colors:View]", "ForegroundNormal" ) ) );
        _palette.setColor( Palette::Active, Palette::NegativeText, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[Colors:View]", "ForegroundNegative" ) ) );

        _palette.setColor( Palette::Active, Palette::ActiveWindowBackground, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[WM]", "activeBackground" ) ) );
        _palette.setColor( Palette::Active, Palette::InactiveWindowBackground, ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( "[WM]", "inactiveBackground" ) ) );

        // load effects
        const ColorUtils::Effect disabledEffect( Palette::Disabled, _kdeGlobals );
        const ColorUtils::Effect inactiveEffect( Palette::Inactive, _kdeGlobals );

        // generate inactive and disabled palette from active, applying effects from kdeglobals
        _inactiveChangeSelectionColor = ( _kdeGlobals.getOption( "[ColorEffects:Inactive]", "ChangeSelectionColor" ).toVariant<std::string>( "false" ) == "true" );
        _palette.generate( Palette::Active, Palette::Inactive, inactiveEffect, _inactiveChangeSelectionColor );
        _palette.generate( Palette::Active, Palette::Disabled, disabledEffect );

        #if CARBON_DEBUG
        std::cerr << "Carbon::QtSettings::loadKdePalette - disabled effect: " << std::endl;
        std::cerr << disabledEffect << std::endl;

        std::cerr << "Carbon::QtSettings::loadKdePalette - inactive effect: " << std::endl;
        std::cerr << inactiveEffect << std::endl;

        std::cerr << "Carbon::QtSettings::loadKdePalette - palette: " << std::endl;
        std::cerr << _palette << std::endl;
        #endif

    }

    //_________________________________________________________
    void QtSettings::generateGtkColors( void )
    {

        #if CARBON_DEBUG
        std::cerr << "Carbon::QtSettings::generateGtkColors" << std::endl;
        #endif

        // customize gtk palette
        _palette.setGroup( Palette::Active );

        // default colors
        _rc.setCurrentSection( Gtk::RC::defaultSection() );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  bg[NORMAL]", _palette.color( Palette::Window ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  bg[PRELIGHT]", _palette.color( Palette::Window ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  bg[ACTIVE]", _palette.color( Palette::Window ) ) );

        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  bg[SELECTED]", _palette.color( Palette::Selected ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  bg[INSENSITIVE]", _palette.color( Palette::Window ) ) );

        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[NORMAL]", _palette.color( Palette::WindowText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[ACTIVE]", _palette.color( Palette::WindowText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[PRELIGHT]", _palette.color( Palette::WindowText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[SELECTED]", _palette.color( Palette::WindowText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[INSENSITIVE]", _palette.color( Palette::Disabled, Palette::WindowText ) ) );

        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  base[NORMAL]", _palette.color( Palette::Base ) ) );

        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  base[ACTIVE]", _palette.color( Palette::Inactive, Palette::Selected ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  base[PRELIGHT]", _palette.color( Palette::Base ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  base[SELECTED]", _palette.color( Palette::Selected ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  base[INSENSITIVE]", _palette.color( Palette::Disabled, Palette::Base ) ) );

        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[NORMAL]", _palette.color( Palette::Text ) ) );

        if( _inactiveChangeSelectionColor ) _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[ACTIVE]", _palette.color( Palette::Text ) ) );
        else _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[ACTIVE]", _palette.color( Palette::SelectedText ) ) );

        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[PRELIGHT]", _palette.color( Palette::Text ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[SELECTED]", _palette.color( Palette::SelectedText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[INSENSITIVE]", _palette.color( Palette::Disabled, Palette::Text ) ) );
        addLinkColors( "[Colors:Window]" );

        // buttons
        _rc.addSection( "carbon-buttons-internal", Gtk::RC::defaultSection() );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  bg[NORMAL]", _palette.color( Palette::Button ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  bg[ACTIVE]", _palette.color( Palette::Button ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  bg[PRELIGHT]", _palette.color( Palette::Button ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  bg[INSENSITIVE]", _palette.color( Palette::Disabled, Palette::Button ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[NORMAL]", _palette.color( Palette::ButtonText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[ACTIVE]", _palette.color( Palette::ButtonText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[PRELIGHT]", _palette.color( Palette::ButtonText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[INSENSITIVE]", _palette.color( Palette::Disabled, Palette::ButtonText ) ) );
        addLinkColors( "[Colors:Button]" );

        _rc.matchClassToSection( "*Button", "carbon-buttons-internal" );
        _rc.matchClassToSection( "GtkOptionMenu", "carbon-buttons-internal" );
        _rc.matchWidgetClassToSection( "*<GtkButton>.<GtkLabel>", "carbon-buttons-internal" );
        _rc.matchWidgetClassToSection( "*<GtkButton>.<GtkAlignment>.<GtkBox>.<GtkLabel>", "carbon-buttons-internal" );

        // combobox
        _rc.addSection( "carbon-combobox-internal", "carbon-buttons-internal" );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[NORMAL]", _palette.color( Palette::ButtonText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[ACTIVE]", _palette.color( Palette::ButtonText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[PRELIGHT]", _palette.color( Palette::ButtonText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[INSENSITIVE]", _palette.color( Palette::Disabled, Palette::ButtonText ) ) );
        _rc.matchWidgetClassToSection( "*<GtkComboBox>.*<GtkCellView>", "carbon-combobox-internal" );

        // checkboxes and radio buttons
        _rc.addSection( "carbon-checkbox-buttons-internal", "carbon-buttons-internal" );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[NORMAL]", _palette.color( Palette::WindowText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[ACTIVE]", _palette.color( Palette::WindowText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[PRELIGHT]", _palette.color( Palette::WindowText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[INSENSITIVE]", _palette.color( Palette::Disabled, Palette::WindowText ) ) );
        _rc.matchWidgetClassToSection( "*<GtkCheckButton>.<GtkLabel>", "carbon-checkbox-buttons-internal" );

        // progressbar labels
        _rc.addSection( "carbon-progressbar-internal", Gtk::RC::defaultSection() );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[NORMAL]", _palette.color( Palette::WindowText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[ACTIVE]", _palette.color( Palette::WindowText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[PRELIGHT]", _palette.color( Palette::WindowText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[INSENSITIVE]", _palette.color( Palette::Disabled, Palette::WindowText ) ) );

        _rc.matchClassToSection( "GtkProgressBar", "carbon-progressbar-internal" );

        // menu items
        _rc.addSection( "carbon-menubar-item-internal", "carbon-menu-font-internal" );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[NORMAL]", _palette.color( Palette::Text ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[NORMAL]", _palette.color( Palette::WindowText ) ) );

        if( _menuHighlightMode == MM_STRONG )
        {

            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[PRELIGHT]", _palette.color( Palette::Text ) ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[SELECTED]", _palette.color( Palette::SelectedText ) ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[ACTIVE]", _palette.color( Palette::SelectedText ) ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[PRELIGHT]", _palette.color( Palette::WindowText ) ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[SELECTED]", _palette.color( Palette::SelectedText ) ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[ACTIVE]", _palette.color( Palette::SelectedText ) ) );

        } else {

            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[PRELIGHT]", _palette.color( Palette::Text ) ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[SELECTED]", _palette.color( Palette::Text ) ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[ACTIVE]", _palette.color( Palette::Text ) ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[PRELIGHT]", _palette.color( Palette::WindowText ) ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[SELECTED]", _palette.color( Palette::WindowText ) ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[ACTIVE]", _palette.color( Palette::WindowText ) ) );

        }
        _rc.matchWidgetClassToSection( "*<GtkMenuItem>.<GtkLabel>", "carbon-menubar-item-internal" );


        if( _menuHighlightMode == MM_STRONG )
        {
            _rc.addSection( "carbon-menu-item-internal", "carbon-menubar-item-internal" );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  text[PRELIGHT]", _palette.color( Palette::SelectedText ) ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[PRELIGHT]", _palette.color( Palette::SelectedText ) ) );
            _rc.matchWidgetClassToSection( "*<GtkMenu>.<GtkMenuItem>.<GtkLabel>", "carbon-menu-item-internal" );
        }

        // text entries
        _rc.addSection( "carbon-entry-internal", Gtk::RC::defaultSection() );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  bg[NORMAL]", _palette.color( Palette::Base ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  bg[INSENSITIVE]", _palette.color( Palette::Disabled, Palette::Base ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  base[INSENSITIVE]", _palette.color( Palette::Disabled, Palette::Base ) ) );
        addLinkColors( "[Colors:View]" );
        _rc.matchClassToSection( "GtkSpinButton", "carbon-entry-internal" );
        _rc.matchClassToSection( "GtkEntry", "carbon-entry-internal" );
        _rc.matchClassToSection( "GtkTextView", "carbon-entry-internal" );
        _rc.matchWidgetClassToSection( "*<GtkComboBoxEntry>.<GtkButton>", "carbon-entry-internal" );
        _rc.matchWidgetClassToSection( "*<GtkCombo>.<GtkButton>", "carbon-entry-internal" );

        // tooltips
        _rc.addSection( "carbon-tooltips-internal", Gtk::RC::defaultSection() );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  bg[NORMAL]", _palette.color( Palette::Tooltip ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  fg[NORMAL]", _palette.color( Palette::TooltipText ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<int>( "  xthickness", 3 ) );
        _rc.addToCurrentSection( Gtk::RCOption<int>( "  ythickness", 3 ) );
        addLinkColors( "[Colors:Tooltip]" );

        _rc.matchWidgetToSection( "gtk-tooltip*", "carbon-tooltips-internal" );

        // special case for google chrome
        /* based on http://code.google.com/p/chromium/wiki/LinuxGtkThemeIntegration */
        _rc.addSection( "carbon-chrome-gtk-frame-internal", Gtk::RC::defaultSection() );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( " ChromeGtkFrame::frame-color", _palette.color( Palette::Window ) ) );
        _rc.addToCurrentSection( Gtk::RCOption<std::string>( " ChromeGtkFrame::inactive-frame-color", _palette.color( Palette::Window ) ) );
        _rc.matchClassToSection( "ChromeGtkFrame", "carbon-chrome-gtk-frame-internal" );

    }


    //_________________________________________________________
    void QtSettings::addLinkColors( const std::string& section )
    {

        {
            // link colors
            // from http://ubuntuforums.org/showthread.php?p=7009302
            const ColorUtils::Rgba linkColor( ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( section, "ForegroundLink" ) ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  GtkWidget::link-color", linkColor ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  GtkHTML::alink_color", linkColor ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  GtkHTML::link_color", linkColor ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  GnomeHref::link-color", linkColor ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  GtkIMHtml::hyperlink-color", linkColor ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  GtkIMHtml::hyperlink-prelight-color", linkColor ) );
        }

        {
            // visited link colors
            // from http://ubuntuforums.org/showthread.php?p=7009302
            const ColorUtils::Rgba visitedLinkColor( ColorUtils::Rgba::fromKdeOption( _kdeGlobals.getValue( section, "ForegroundVisited" ) ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  GtkWidget::visited-link-color", visitedLinkColor ) );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  GtkHTML::vlink_color", visitedLinkColor ) );
        }

    }

    //_________________________________________________________
    void QtSettings::loadKdeFonts( void )
    {

        // try load default font
        FontInfo::Map fonts;
        FontInfo defaultFont;
        if( _kdeGlobals.hasOption( "[General]", "font" ) )
        {

            defaultFont = FontInfo::fromKdeOption( _kdeGlobals.getValue( "[General]", "font", "" ) );

        } else {

            #if CARBON_DEBUG
            std::cerr << "Carbon::QtSettings::loadKdeFonts - cannot load default font" << std::endl;
            #endif

        }

        fonts[FontInfo::Default] = defaultFont;

        // load extra fonts
        typedef std::map<FontInfo::FontType, std::string> FontNameMap;
        FontNameMap optionNames;
        optionNames.insert( std::make_pair( FontInfo::Desktop, "desktopFont" ) );
        optionNames.insert( std::make_pair( FontInfo::Fixed, "fixed" ) );
        optionNames.insert( std::make_pair( FontInfo::Menu, "menuFont" ) );
        optionNames.insert( std::make_pair( FontInfo::ToolBar, "toolBarFont" ) );
        for( FontNameMap::const_iterator iter = optionNames.begin(); iter != optionNames.end(); ++iter )
        {
            FontInfo local;
            if( _kdeGlobals.hasOption( "[General]", iter->second ) )
            {

                local = FontInfo::fromKdeOption( _kdeGlobals.getValue( "[General]", iter->second, "" ) );

            } else {

                #if CARBON_DEBUG
                std::cerr << "Carbon::QtSettings::loadKdeFonts - cannot load font named " << iter->second << std::endl;
                #endif
                local = defaultFont;

            }

            // store in map
            fonts[iter->first] = local;

        }

        // pass fonts to RC
        if( fonts[FontInfo::Default].isValid() )
        {
            // pass to settings
            GtkSettings* settings( gtk_settings_get_default() );
            gtk_settings_set_string_property( settings, "gtk-font-name", fonts[FontInfo::Default].toString( false ).c_str(), "carbon-gtk" );

            _rc.setCurrentSection( Gtk::RC::defaultSection() );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  font_name", fonts[FontInfo::Default].toString() ) );
        }

        if( fonts[FontInfo::Menu].isValid() )
        {
            _rc.addSection( "carbon-menu-font-internal", Gtk::RC::defaultSection() );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  font_name", fonts[FontInfo::Menu].toString() ) );
            _rc.matchWidgetClassToSection( "*<GtkMenuItem>.<GtkLabel>", "carbon-menu-font-internal" );
        }

        if( fonts[FontInfo::ToolBar].isValid() )
        {
            _rc.addSection( "carbon-toolbar-font-internal", Gtk::RC::defaultSection() );
            _rc.addToCurrentSection( Gtk::RCOption<std::string>( "  font_name", fonts[FontInfo::ToolBar].toString() ) );
            _rc.matchWidgetClassToSection( "*<GtkToolbar>.*", "carbon-toolbar-font-internal" );
        }

        // don't check for section and tag presence - use default font if not present
        _WMFont=FontInfo::fromKdeOption( _kdeGlobals.getValue( "[WM]", "activeFont", "Sans Serif,8,-1,5,50,0,0,0,0,0" ) );

    }

    //_________________________________________________________
    void QtSettings::loadKdeGlobalsOptions( void )
    {

        #if CARBON_DEBUG
        std::cerr << "Carbon::QtSettings::loadKdeGlobalsOptions" << std::endl;
        #endif

        // toolbar style
        const std::string toolbarTextPosition( _kdeGlobals.getOption( "[Toolbar style]", "ToolButtonStyle" ).toVariant<std::string>( "TextBelowIcon" ) );
        GtkToolbarStyle toolbarStyle( GTK_TOOLBAR_BOTH );
        if( toolbarTextPosition == "TextOnly" ) toolbarStyle = GTK_TOOLBAR_TEXT;
        else if( toolbarTextPosition == "TextBesideIcon" ) toolbarStyle = GTK_TOOLBAR_BOTH_HORIZ;
        else if( toolbarTextPosition == "NoText" ) toolbarStyle = GTK_TOOLBAR_ICONS;

        GtkSettings* settings( gtk_settings_get_default() );
        gtk_settings_set_long_property( settings, "gtk-toolbar-style", toolbarStyle, "carbon-gtk" );

        // icons on buttons
        if( _kdeGlobals.getValue( "[KDE]", "ShowIconsOnPushButtons", "true" ) == "false" )
        { gtk_settings_set_long_property( settings, "gtk-button-images", 0, "carbon-gtk" ); }

        // active icon effects
        _useIconEffect = _kdeGlobals.getOption( "[MainToolbarIcons]", "ActiveEffect" ).toVariant<std::string>( "gamma" ) != "none";

        // start drag time and distance
        _startDragDist = _kdeGlobals.getOption( "[KDE]", "StartDragDist" ).toVariant<int>( 4 );
        _startDragTime = _kdeGlobals.getOption( "[KDE]", "StartDragTime" ).toVariant<int>( 500 );

    }

    //_________________________________________________________
    void QtSettings::loadCarbonOptions( void )
    {

        // background pixmap
        _backgroundPixmap = _carbon.getValue( "[Common]", "BackgroundPixmap", "" );

        // background gradient
        _useBackgroundGradient = ( _carbon.getValue( "[Common]", "UseBackgroundGradient", "true" ) == "true" );

        // checkbox style
        _checkBoxStyle = (_carbon.getValue( "[Style]", "CheckBoxStyle", "CS_CHECK" ) == "CS_CHECK") ? CS_CHECK:CS_X;

        // checkbox style
        _tabStyle = (_carbon.getValue( "[Style]", "TabStyle", "TS_SINGLE" ) == "TS_SINGLE") ? TS_SINGLE:TS_PLAIN;

        // scrollbar buttons
        _scrollBarAddLineButtons = _carbon.getOption( "[Style]", "ScrollBarAddLineButtons" ).toVariant<int>( 2 );
        _scrollBarSubLineButtons = _carbon.getOption( "[Style]", "ScrollBarSubLineButtons" ).toVariant<int>( 1 );

        // toolbar separators
        _toolBarDrawItemSeparator = _carbon.getOption( "[Style]", "ToolBarDrawItemSeparator" ).toVariant<std::string>("true") == "true";

        // tooltips
        _tooltipTransparent = _carbon.getOption( "[Style]", "ToolTipTransparent" ).toVariant<std::string>("true") == "true";
        _tooltipDrawStyledFrames = _carbon.getOption( "[Style]", "ToolTipDrawStyledFrames" ).toVariant<std::string>("true") == "true";

        // focus indicator in views
        _viewDrawFocusIndicator = _carbon.getOption( "[Style]", "ViewDrawFocusIndicator" ).toVariant<std::string>("true") == "true";

        // tree branch lines
        _viewDrawTreeBranchLines = _carbon.getOption( "[Style]", "ViewDrawTreeBranchLines" ).toVariant<std::string>("true") == "true";

        // triangular expanders
        _viewDrawTriangularExpander = _carbon.getOption( "[Style]", "ViewDrawTriangularExpander" ).toVariant<std::string>("true") == "true";

        // triangular expander (arrow) size
        std::string expanderSize( _carbon.getOption( "[Style]", "ViewTriangularExpanderSize" ).toVariant<std::string>("TE_SMALL") );
        if( expanderSize == "TE_NORMAL" ) _viewTriangularExpanderSize = ArrowNormal;
        else if( expanderSize == "TE_TINY" ) _viewTriangularExpanderSize = ArrowTiny;
        else _viewTriangularExpanderSize = ArrowSmall;

        // invert view sort indicators
        _viewInvertSortIndicator = _carbon.getOption( "[Style]", "ViewInvertSortIndicator" ).toVariant<std::string>("false") == "true";

        // menu highlight mode
        std::string highlightMode( _carbon.getOption( "[Style]", "MenuHighlightMode" ).toVariant<std::string>("MM_DARK") );
        if( highlightMode == "MM_SUBTLE" ) _menuHighlightMode = MM_SUBTLE;
        else if( highlightMode == "MM_STRONG" ) _menuHighlightMode = MM_STRONG;
        else _menuHighlightMode = MM_DARK;

        // window drag mode
        _windowDragEnabled = _carbon.getOption( "[Style]", "WindowDragEnabled" ).toVariant<std::string>("true") == "true";

        std::string windowDragMode( _carbon.getOption( "[Style]", "WindowDragMode" ).toVariant<std::string>("WD_FULL") );
        if( windowDragMode == "WD_MINIMAL" ) _windowDragMode = WD_MINIMAL;
        else _windowDragMode = WD_FULL;

        // use window manager to handle window drag
        _useWMMoveResize = _carbon.getOption( "[Style]", "UseWMMoveResize" ).toVariant<std::string>("true") == "true";

        // animations
        _animationsEnabled = ( _carbon.getOption( "[Style]", "AnimationsEnabled" ).toVariant<std::string>("true") == "true" );
        _genericAnimationsEnabled = ( _carbon.getOption( "[Style]", "GenericAnimationsEnabled" ).toVariant<std::string>("true") == "true" );

        // menubar animation type
        std::string menuBarAnimationType( _carbon.getValue( "[Style]", "MenuBarAnimationType", "MB_FADE") );
        if( menuBarAnimationType == "MB_NONE" ) _menuBarAnimationType = NoAnimation;
        else if( menuBarAnimationType == "MB_FADE" ) _menuBarAnimationType = Fade;
        else if( menuBarAnimationType == "MB_FOLLOW_MOUSE" ) _menuBarAnimationType = FollowMouse;

        // menubar animation type
        std::string menuAnimationType( _carbon.getValue( "[Style]", "MenuAnimationType", "ME_FADE") );
        if( menuAnimationType == "ME_NONE" ) _menuAnimationType = NoAnimation;
        else if( menuAnimationType == "ME_FADE" ) _menuAnimationType = Fade;
        else if( menuAnimationType == "ME_FOLLOW_MOUSE" ) _menuAnimationType = FollowMouse;

        // toolbar animation type
        std::string toolBarAnimationType( _carbon.getValue( "[Style]", "ToolBarAnimationType", "TB_FADE") );
        if( toolBarAnimationType == "TB_NONE" ) _toolBarAnimationType = NoAnimation;
        else if( toolBarAnimationType == "TB_FADE" ) _toolBarAnimationType = Fade;
        else if( toolBarAnimationType == "TB_FOLLOW_MOUSE" ) _toolBarAnimationType = FollowMouse;

        // animations duration
        _genericAnimationsDuration = _carbon.getOption( "[Style]", "GenericAnimationsDuration" ).toVariant<int>(150);
        _menuBarAnimationsDuration = _carbon.getOption( "[Style]", "MenuBarAnimationsDuration" ).toVariant<int>(150);
        _menuBarFollowMouseAnimationsDuration = _carbon.getOption( "[Style]", "MenuBarFollowMouseAnimationsDuration" ).toVariant<int>(80);
        _menuAnimationsDuration = _carbon.getOption( "[Style]", "MenuAnimationsDuration" ).toVariant<int>(150);
        _menuFollowMouseAnimationsDuration = _carbon.getOption( "[Style]", "MenuFollowMouseAnimationsDuration" ).toVariant<int>(40);
        _toolBarAnimationsDuration = _carbon.getOption( "[Style]", "ToolBarAnimationsDuration" ).toVariant<int>(50);

        // animation steps
        TimeLine::setSteps( _carbon.getOption( "[Style]", "AnimationSteps" ).toVariant<int>( 10 ) );

        // widget explorer
        _widgetExplorerEnabled =
            _carbon.getOption( "[Style]", "WidgetExplorerEnabled" ).toVariant<std::string>("false") == "true" ||
            _carbon.getOption( "[Style]", "WidgetExplorerGtkEnabled" ).toVariant<std::string>("false") == "true";

        // window decoration button size
        std::string buttonSize( _carbon.getValue( "[Windeco]", "ButtonSize", "Normal") );
        if( buttonSize == "Small" ) _buttonSize = ButtonSmall;
        else if( buttonSize == "Large" ) _buttonSize = ButtonLarge;
        else if( buttonSize == "Very Large" ) _buttonSize = ButtonVeryLarge;
        else if( buttonSize == "Huge" ) _buttonSize = ButtonHuge;
        else _buttonSize = ButtonDefault;

        // window decoration frame border size
        std::string frameBorder(  _carbon.getValue( "[Windeco]", "FrameBorder", "Normal") );
        if( frameBorder == "No Border" ) _frameBorder = BorderNone;
        else if( frameBorder == "No Side Border" ) _frameBorder = BorderNoSide;
        else if( frameBorder == "Tiny" ) _frameBorder = BorderTiny;
        else if( frameBorder == "Large" ) _frameBorder = BorderLarge;
        else if( frameBorder == "Very Large" ) _frameBorder = BorderVeryLarge;
        else if( frameBorder == "Huge" ) _frameBorder = BorderHuge;
        else if( frameBorder == "Very Huge" ) _frameBorder = BorderVeryHuge;
        else if( frameBorder == "Oversized" ) _frameBorder = BorderOversized;
        else _frameBorder = BorderDefault;

        // window decoration title alignment
        std::string titleAlign( _carbon.getValue( "[Windeco]", "TitleAlignment", "Center" ) );
        if( titleAlign == "Left" ) _titleAlignment = PANGO_ALIGN_LEFT;
        else if( titleAlign == "Center" ) _titleAlignment = PANGO_ALIGN_CENTER;
        else if( titleAlign == "Right" ) _titleAlignment = PANGO_ALIGN_RIGHT;
        else _titleAlignment = PANGO_ALIGN_CENTER;

        // window decoration radial gradient enable option
        std::string wdBlendType( _carbon.getValue( "[Windeco]", "BlendColor", "Follow Style Hint" ) );
        if( wdBlendType == "Follow Style Hint" ) _windecoBlendType=FollowStyleHint;
        else if( wdBlendType == "Radial Gradient" ) _windecoBlendType=RadialGradient;
        else if( wdBlendType == "Solid Color" ) _windecoBlendType=SolidColor;
        else _windecoBlendType=FollowStyleHint;

        // shadow configurations
        _activeShadowConfiguration.initialize( _carbon );
        _inactiveShadowConfiguration.initialize( _carbon );

        if(_kdeGlobals.getOption( "[General]", "widgetStyle" ).toVariant<std::string>("carbon") == "carbon transparent")
        {
            _backgroundOpacity = _carbon.getOption( "[Common]", "BackgroundOpacity" ).toVariant<int>(255);

        } else {

            _backgroundOpacity = 255;

        }

        #if CARBON_DEBUG
        std::cerr << _activeShadowConfiguration << std::endl;
        std::cerr << _inactiveShadowConfiguration << std::endl;
        #endif

        // copy relevant options to to gtk
        // scrollbar width
        _rc.setCurrentSection( Gtk::RC::defaultSection() );
        _rc.addToCurrentSection( Gtk::RCOption<int>(
            "  GtkScrollbar::slider-width",
            _carbon.getOption( "[Style]", "ScrollBarWidth" ).toVariant<int>(15) + 1 ) );

        _rc.addToCurrentSection( Gtk::RCOption<bool>("  GtkScrollbar::has-backward-stepper", _scrollBarSubLineButtons > 0 ) );
        _rc.addToCurrentSection( Gtk::RCOption<bool>("  GtkScrollbar::has-forward-stepper", _scrollBarAddLineButtons > 0 ) );

        // note the inversion for add and sub, due to the fact that kde options refer to the button location, and not its direction
        _rc.addToCurrentSection( Gtk::RCOption<bool>("  GtkScrollbar::has-secondary-backward-stepper", _scrollBarAddLineButtons > 1 ) );
        _rc.addToCurrentSection( Gtk::RCOption<bool>("  GtkScrollbar::has-secondary-forward-stepper", _scrollBarSubLineButtons > 1 ) );

        // mnemonics
        GtkSettings* settings( gtk_settings_get_default() );
        if( _carbon.hasOption( "[Style]", "MnemonicsMode" ) )
        {

            const std::string mnemonicsMode( _carbon.getOption( "[Style]", "MnemonicsMode" ).toVariant<std::string>("MN_ALWAYS") );
            if( mnemonicsMode == "MN_NEVER" )
            {

                gtk_settings_set_long_property( settings, "gtk-enable-mnemonics", false, "carbon-gtk" );
                gtk_settings_set_long_property( settings, "gtk-auto-mnemonics", false, "carbon-gtk" );

            } else if( mnemonicsMode == "MN_AUTO" ) {

                gtk_settings_set_long_property( settings, "gtk-enable-mnemonics", true, "carbon-gtk" );
                gtk_settings_set_long_property( settings, "gtk-auto-mnemonics", true, "carbon-gtk" );

            } else {

                gtk_settings_set_long_property( settings, "gtk-enable-mnemonics", true, "carbon-gtk" );
                gtk_settings_set_long_property( settings, "gtk-auto-mnemonics", false, "carbon-gtk" );

            }

        } else {

            // for backward compatibility
            const bool showMnemonics( _carbon.getOption( "[Style]", "ShowMnemonics" ).toVariant<std::string>("true") == "true" );
            if( showMnemonics )
            {

                gtk_settings_set_long_property( settings, "gtk-enable-mnemonics", true, "carbon-gtk" );
                gtk_settings_set_long_property( settings, "gtk-auto-mnemonics", false, "carbon-gtk" );

            } else {

                gtk_settings_set_long_property( settings, "gtk-enable-mnemonics", false, "carbon-gtk" );
                gtk_settings_set_long_property( settings, "gtk-auto-mnemonics", false, "carbon-gtk" );

            }

        }

    }

    //_________________________________________________________
    void QtSettings::loadExtraOptions( void )
    {

        // deal with pathbar button margins
        // this needs to be done programatically in order to properly account for RTL locales
        _rc.addSection( "carbon-pathbutton-internal", Gtk::RC::defaultSection() );
        _rc.addToCurrentSection( "  GtkButton::inner-border = { 2, 2, 1, 0 }" );

        if( gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL )
        {

            _rc.addToCurrentSection( "  GtkToggleButton::inner-border={ 10, 0, 1, 0 }" );

        } else {

            _rc.addToCurrentSection( "  GtkToggleButton::inner-border={ 0, 10, 1, 0 }" );

        }

        _rc.matchWidgetClassToSection( "*PathBar.GtkToggleButton", "carbon-pathbutton-internal" );

        // entry margins
        _rc.addSection( "carbon-entry-margins-internal", Gtk::RC::defaultSection() );
        _rc.addToCurrentSection( Gtk::RCOption<int>( "  xthickness", 5 ) );
        _rc.addToCurrentSection( Gtk::RCOption<int>( "  ythickness", applicationName().isXul() ? 2:1 ) );
        _rc.matchClassToSection( "GtkEntry", "carbon-entry-margins-internal" );

        // combobox buttons
        _rc.addSection( "carbon-combobox-button-internal", Gtk::RC::defaultSection() );
        _rc.addToCurrentSection( Gtk::RCOption<int>( "  xthickness", 2 ) );
        _rc.addToCurrentSection( Gtk::RCOption<int>( "  ythickness", applicationName().isXul() ? 2:0 ) );
        _rc.matchWidgetClassToSection( "*<GtkComboBox>.<GtkButton>", "carbon-combobox-button-internal" );

    }

    //_________________________________________________________
    std::string QtSettings::sanitizePath( const std::string& path ) const
    {

        std::string out( path );
        size_t position( std::string::npos );
        while( ( position = out.find( "//" ) ) != std::string::npos )
        { out.replace( position, 2, "/" ); }

        return out;
    }

    //_________________________________________________________
    void QtSettings::monitorFile( const std::string& filename )
    {

        // check if file was already added
        if( _monitoredFiles.find( filename ) != _monitoredFiles.end() )
        { return; }

        // check file existence
        if( !std::ifstream( filename.c_str() ) )
        { return; }

        // create FileMonitor
        FileMonitor monitor;
        monitor.file = g_file_new_for_path( filename.c_str() );
        if( ( monitor.monitor = g_file_monitor( monitor.file, G_FILE_MONITOR_NONE, 0L, 0L ) ) )
        {

            // insert in map
            _monitoredFiles.insert( std::make_pair( filename, monitor ) );

        } else {

            // clear file and return
            g_object_unref( monitor.file );
            return;

        }

    }

    //_________________________________________________________
    void QtSettings::clearMonitoredFiles( void )
    {
        for( FileMap::iterator iter = _monitoredFiles.begin(); iter != _monitoredFiles.end(); iter++ )
        {
            iter->second.signal.disconnect();
            g_object_unref( iter->second.file );
            g_object_unref( iter->second.monitor );
        }

        _monitoredFiles.clear();
    }

}
