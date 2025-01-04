cmake_minimum_required(VERSION 3.20)
project(Photino)

# --------------------------------------------------------------------------------------------------
# Variables
# --------------------------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD          "11")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(NUGET_EXECUTABLE            "nuget.exe")
set(PACKAGE_WEBVIEW2_NAME       "Microsoft.Web.WebView2")
set(PACKAGE_WEBVIEW2_VERSION    "1.0.1462.37")
set(PACKAGE_WIL_NAME            "Microsoft.Windows.ImplementationLibrary")
set(PACKAGE_WIL_VERSION         "1.0.220914.1")
set(PACKAGES_DIRECTORY          "${CMAKE_BINARY_DIR}/Photino.Native/packages")
set(PACKAGES_FILE               "${CMAKE_SOURCE_DIR}/Photino.Native/packages.config")

if (APPLE)
    set(CMAKE_CXX_FLAGS         "-ObjC++")
endif()

# --------------------------------------------------------------------------------------------------
# Definitions
# --------------------------------------------------------------------------------------------------

add_definitions(-DUNICODE -D_UNICODE)

# --------------------------------------------------------------------------------------------------
# Utilities
# --------------------------------------------------------------------------------------------------

function(nuget_package
    PACKAGE_NAME
    PACKAGE_VERSION
    PACKAGE_INCLUDE
    PACKAGE_TARGETS_PATH)
    # Each package gets its own target which doesn't actually produce any outputs.
    add_library(${PACKAGE_NAME} INTERFACE IMPORTED GLOBAL)

    # NuGet packages always follow the same naming scheme when restored from `packages.config`.
    set(PACKAGE_DIRECTORY "${PACKAGES_DIRECTORY}/${PACKAGE_NAME}.${PACKAGE_VERSION}")

    # We create the include directory because we don't want warnings about non-existent files from
    # CMake when generating the initial projects files. These files will be populated once the
    # packages are restored.
    file(MAKE_DIRECTORY "${PACKAGE_DIRECTORY}/${PACKAGE_INCLUDE}")

    # These include directories will be passed onto any targets depending on this target.
    set_property(
        TARGET ${PACKAGE_NAME}
        PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${PACKAGE_DIRECTORY}/${PACKAGE_INCLUDE}")

    if (CMAKE_GENERATOR MATCHES "Visual Studio")
        # This only works for `.vcxproj` files. We depend directly on the `.targets` file that is
        # included with the NuGet packages.
        set_property(
            TARGET ${PACKAGE_NAME}
            PROPERTY INTERFACE_LINK_LIBRARIES "${PACKAGE_DIRECTORY}/${PACKAGE_TARGETS_PATH}")
    endif()
endfunction()

# --------------------------------------------------------------------------------------------------
# NuGet Packages
# --------------------------------------------------------------------------------------------------

if (WIN32)
    # TODO: Move this to `nuget_package` so that we don't need to maintain a separate
    # `packages.config` file.
    add_custom_command(
        OUTPUT  ${PACKAGES_DIRECTORY}/_restored.timestamp
        COMMAND ${NUGET_EXECUTABLE} restore ${PACKAGES_FILE} -OutputDirectory ${PACKAGES_DIRECTORY}
        COMMAND ${CMAKE_COMMAND} -E touch ${PACKAGES_DIRECTORY}/_restored.timestamp
        DEPENDS ${PACKAGES_FILE}
        COMMENT "Restoring NuGet packages..."
        VERBATIM)

    add_custom_target(RestorePackages ALL
        DEPENDS ${PACKAGES_DIRECTORY}/_restored.timestamp)

    nuget_package(
        "${PACKAGE_WEBVIEW2_NAME}"
        "${PACKAGE_WEBVIEW2_VERSION}"
        "build/native/include"
        "build/native/Microsoft.Web.WebView2.targets")

    nuget_package(
        "${PACKAGE_WIL_NAME}"
        "${PACKAGE_WIL_VERSION}"
        "include"
        "build/native/Microsoft.Windows.ImplementationLibrary.targets")

    add_dependencies("${PACKAGE_WEBVIEW2_NAME}" RestorePackages)
    add_dependencies("${PACKAGE_WIL_NAME}" RestorePackages)
endif()

# --------------------------------------------------------------------------------------------------
# Photino.Native
# --------------------------------------------------------------------------------------------------

add_library(Photino.Native SHARED # <-- The thing we actually care about. :)
    "${CMAKE_SOURCE_DIR}/Photino.Native/Exports.cpp"
    "${CMAKE_SOURCE_DIR}/Photino.Native/Photino.Dialog.h"
    "${CMAKE_SOURCE_DIR}/Photino.Native/Photino.Errors.cpp"
    "${CMAKE_SOURCE_DIR}/Photino.Native/Photino.Errors.h"
    "${CMAKE_SOURCE_DIR}/Photino.Native/Photino.h"
    "${CMAKE_SOURCE_DIR}/Photino.Native/Photino.Menu.h")

if (APPLE)
    file(GLOB PHOTINO_NATIVE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/Photino.Native/Photino.Mac*.mm"
        "${CMAKE_SOURCE_DIR}/Photino.Native/Photino.Mac*.h")

    target_sources(Photino.Native PRIVATE ${PHOTINO_NATIVE_SOURCES})

    target_link_libraries(Photino.Native PRIVATE
        "-framework AppKit"
        "-framework Cocoa"
        "-framework Foundation"
        "-framework Security"
        "-framework UserNotifications"
        "-framework WebKit")
elseif (LINUX)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(LIBNOTIFY REQUIRED libnotify)
    pkg_check_modules(WEBKIT2GTK REQUIRED webkit2gtk-4.1)
    pkg_check_modules(X11 REQUIRED x11)

    file(GLOB PHOTINO_NATIVE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/Photino.Native/Photino.Linux*.cpp"
        "${CMAKE_SOURCE_DIR}/Photino.Native/Photino.Linux*.h")

    target_sources(Photino.Native PRIVATE ${PHOTINO_NATIVE_SOURCES})

    target_link_libraries(Photino.Native PRIVATE
        ${GTK3_LIBRARIES}
        ${LIBNOTIFY_LIBRARIES}
        ${WEBKIT2GTK_LIBRARIES}
        ${X11_LIBRARIES})
    
    target_include_directories(Photino.Native PRIVATE
        ${GTK3_INCLUDE_DIRS}
        ${LIBNOTIFY_INCLUDE_DIRS}
        ${WEBKIT2GTK_INCLUDE_DIRS}
        ${X11_INCLUDE_DIRS})
elseif (WIN32)
    # We want to be able to generate NMake project files so that we can get `compile_commands.json`,
    # which does not work with the Visual Studio generator. So, we must do the equivalent of
    # including the `.targets` files from the NuGet packages.
    if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
        if (CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
            set(PACKAGE_ARCHITECTURE_PATH "x64")
        elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
            set(PACKAGE_ARCHITECTURE_PATH "arm64")
        elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "X86")
            set(PACKAGE_ARCHITECTURE_PATH "x86")
        endif()

        string(CONCAT PACKAGE_WEBVIEW2_STATIC_LIBRARY
            "${PACKAGES_DIRECTORY}/${PACKAGE_WEBVIEW2_NAME}.${PACKAGE_WEBVIEW2_VERSION}"
            "/build/native/${PACKAGE_ARCHITECTURE_PATH}/WebView2LoaderStatic.lib")

        target_link_libraries(Photino.Native PRIVATE "${PACKAGE_WEBVIEW2_STATIC_LIBRARY}")
    endif()

    file(GLOB PHOTINO_NATIVE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/Photino.Native/Dependencies/wintoastlib.cpp"
        "${CMAKE_SOURCE_DIR}/Photino.Native/Photino.Windows*.cpp"
        "${CMAKE_SOURCE_DIR}/Photino.Native/Photino.Windows*.h")

    target_sources(Photino.Native PRIVATE ${PHOTINO_NATIVE_SOURCES})

    target_link_libraries(Photino.Native PRIVATE
        Microsoft.Web.WebView2
        Microsoft.Windows.ImplementationLibrary
        Shlwapi)

    add_dependencies(Photino.Native Microsoft.Web.WebView2)
    add_dependencies(Photino.Native Microsoft.Windows.ImplementationLibrary)
endif()