cmake_minimum_required(VERSION 3.16)

project(dashboard VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

# ── Build or import transport_lib ────────────────────────────────────────────
if(NOT TARGET transport_lib)
    add_subdirectory(
        ${CMAKE_CURRENT_SOURCE_DIR}/../transport
        ${CMAKE_BINARY_DIR}/transport
    )
endif()

set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        include/mainwindow.h
        mainwindow.ui
        src/agentitemwidget.cpp
        include/agentitemwidget.h
        src/linechartwidget.cpp
        include/linechartwidget.h
        src/processtablewidget.cpp
        include/processtablewidget.h
        src/metriccardswidget.cpp
        include/metriccardswidget.h
        src/uiutils.cpp
        include/uiutils.h
        include/theme.h
        src/serverclient.cpp
        include/serverclient.h
        src/dashboardsession.cpp
        include/dashboardsession.h
        src/serieshistory.cpp
        include/serieshistory.h
        
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(dashboard
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET dashboard APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(dashboard SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(dashboard
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(dashboard PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    transport_lib
    )

# target_include_directories(dashboard PRIVATE include ../transport/include)
# Include only your own headers
target_include_directories(dashboard PRIVATE include)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.dashboard)
endif()
set_target_properties(dashboard PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS dashboard
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(dashboard)
endif()
