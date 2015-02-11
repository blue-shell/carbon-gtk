include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR} )
include_directories( animations debug )

# Sources
# please keep this list alphabetically sorted for clarity
set( carbon_gtk_SOURCES
    animations/carbonanimations.cpp
    animations/carbonarrowstatedata.cpp
    animations/carbonbackgroundhintengine.cpp
    animations/carbonbaseengine.cpp
    animations/carboncomboboxdata.cpp
    animations/carboncomboboxentrydata.cpp
    animations/carbondialogengine.cpp
    animations/carbonflatwidgetengine.cpp
    animations/carbonfollowmousedata.cpp
    animations/carbonhook.cpp
    animations/carbonhoverdata.cpp
    animations/carboninnershadowdata.cpp
    animations/carbonmainwindowdata.cpp
    animations/carbonmenubarstatedata.cpp
    animations/carbonmenustatedata.cpp
    animations/carbonmenuitemengine.cpp
    animations/carbonmenuitemdata.cpp
    animations/carbonpaneddata.cpp
    animations/carbonsignal.cpp
    animations/carbonscrollbardata.cpp
    animations/carbonscrollbarstatedata.cpp
    animations/carbonscrolledwindowdata.cpp
    animations/carbontabwidgetdata.cpp
    animations/carbontabwidgetstatedata.cpp
    animations/carbontimeline.cpp
    animations/carbontimelineserver.cpp
    animations/carbontimer.cpp
    animations/carbontoolbarstatedata.cpp
    animations/carbontreeviewdata.cpp
    animations/carbontreeviewengine.cpp
    animations/carbontreeviewstatedata.cpp
    animations/carbonwidgetsizedata.cpp
    animations/carbonwidgetstatedata.cpp
    animations/carbonwidgetstateengine.cpp
    debug/carbonwidgetexplorer.cpp
    carbonapplicationname.cpp
    carbonargbhelper.cpp
    carboncairocontext.cpp
    carboncairoutils.cpp
    carboncoloreffect.cpp
    carboncolorutils.cpp
    carbonfontinfo.cpp
    carbongtkcellinfo.cpp
    carbongtkicons.cpp
    carbongtkrc.cpp
    carbongtktypenames.cpp
    carbongtkutils.cpp
    carbonloghandler.cpp
    carbonobjectcounter.cpp
    carbonobjectcountermap.cpp
    carbonoption.cpp
    carbonoptionmap.cpp
    carbonpalette.cpp
    carbonpathlist.cpp
    carbonpropertynames.cpp
    carbonqtsettings.cpp
    carbonrcstyle.cpp
    carbonrgba.cpp
    carbonshadowconfiguration.cpp
    carbonshadowhelper.cpp
    carbonstyle.cpp
    carbonstylehelper.cpp
    carbonstyleoptions.cpp
    carbonstylewrapper.cpp
    carbontaboptions.cpp
    carbontheme.cpp
    carbontileset.cpp
    carbonwindecobutton.cpp
    carbonwindecooptions.cpp
    carbonwindowmanager.cpp
    carbonwindowshadow.cpp
)

add_library( carbon-gtk SHARED ${carbon_gtk_SOURCES} )

# Add debug cflags
set( CMAKE_CXX_FLAGS " -Wall -g ${CMAKE_CXX_FLAGS}" )
set( CMAKE_C_FLAGS " -Wall -g ${CMAKE_C_FLAGS}" )

target_link_libraries( carbon-gtk ${GTK_LIBRARIES} ${CAIRO_LIBRARIES} ${X11_X11_LIB} )

install( TARGETS carbon-gtk DESTINATION ${INSTALL_PATH_GTK_ENGINES} )
