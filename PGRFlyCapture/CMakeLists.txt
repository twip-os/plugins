set(target_name PGRFlyCapture)


cmake_minimum_required(VERSION 3.1...3.15)

option(BUILD_TARGET64 "Build for 64 bit target if set to ON or 32 bit if set to OFF." ON)
set(ITOM_SDK_DIR NOTFOUND CACHE PATH "path of SDK subfolder of itom root (build) directory")

#this is to automatically detect the SDK subfolder of the itom build directory.
find_path(ITOM_SDK_DIR "cmake/itom_sdk.cmake"
    HINTS "C:/itom/build/itom/SDK"
          "${CMAKE_CURRENT_BINARY_DIR}/../../itom/SDK"
    DOC "path of SDK subfolder of itom root (build) directory")

if(NOT ITOM_SDK_DIR)
    message(SEND_ERROR "ITOM_SDK_DIR is invalid. Provide itom SDK directory path first")
endif()


set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${ITOM_SDK_DIR}/cmake)

include(ItomBuildMacros)
itom_init_cmake_policy(3.12)
itom_init_plugin_library(${target_name}) #Start the project, init compiler settings and set default configurations for plugins

if(LINUX)
    add_definitions( "--std=c++0x --std=c++11" )
endif(LINUX)


include(ItomBuildMacros)
itom_init_cmake_policy(3.12)


find_package(ITOM_SDK COMPONENTS dataobject itomCommonLib itomCommonQtLib itomWidgets REQUIRED)
itom_find_package_qt(ON Core Widgets Xml LinguistTools)

if(WIN32)
    find_path( FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR FlyCapture2.h DOC "Include directory of Point Gray Research FlyCapture API")
else(WIN32)
    find_path( FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR FlyCapture2.h PATHS /usr/include/flycapture DOC "Include directory of Point Gray Research FlyCapture API")
endif(WIN32)

if(FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR)
    include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${ITOM_SDK_INCLUDE_DIRS}
        ${FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR}
    )

    if(BUILD_TARGET64)
        if(MSVC_VERSION GREATER 1899)
            set(PGR_LIBDIR_HINT "${FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR}/../lib64/vs2015")
            set(PGR_BINDIR_HINT "${FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR}/../bin64/vs2015")
        elseif(MSVC_VERSION EQUAL 1800)
            set(PGR_LIBDIR_HINT "${FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR}/../lib64/vs2013")
            set(PGR_BINDIR_HINT "${FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR}/../bin64/vs2013")
        else()
            set(PGR_LIBDIR_HINT "${FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR}/../lib64")
            set(PGR_BINDIR_HINT "${FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR}/../bin64")
        endif()
    else()
        if(MSVC_VERSION GREATER 1899)
            set(PGR_LIBDIR_HINT "${FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR}/../lib/vs2015")
            set(PGR_BINDIR_HINT "${FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR}/../bin/vs2015")
        elseif(MSVC_VERSION EQUAL 1800)
            set(PGR_LIBDIR_HINT "${FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR}/../lib/vs2013")
            set(PGR_BINDIR_HINT "${FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR}/../bin/vs2013")
        else()    
            set(PGR_LIBDIR_HINT "${FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR}/../lib")
            set(PGR_BINDIR_HINT "${FLYCAPTURE_PGRFLYCAP_INCLUDE_DIR}/../bin")
        endif()
    endif()

    set(PLUGIN_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/dialogPGRFlyCapture.h
        ${CMAKE_CURRENT_SOURCE_DIR}/dockWidgetPGRFlyCapture.h
        ${CMAKE_CURRENT_SOURCE_DIR}/PGRFlyCapture.h
        ${CMAKE_CURRENT_SOURCE_DIR}/pluginVersion.h
        ${CMAKE_CURRENT_BINARY_DIR}/gitVersion.h
    )
    
    set(PLUGIN_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/dialogPGRFlyCapture.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/dockWidgetPGRFlyCapture.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PGRFlyCapture.cpp
    )

    set(PLUGIN_UI
        ${CMAKE_CURRENT_SOURCE_DIR}/dialogPGRFlyCapture.ui
        ${CMAKE_CURRENT_SOURCE_DIR}/dockWidgetPGRFlyCapture.ui
    )
    
    set(PLUGIN_RCC
    )

    #Add version information to the plugIn-dll unter MSVC
    if(MSVC)
        list(APPEND PLUGIN_SOURCES ${ITOM_SDK_INCLUDE_DIR}/../pluginLibraryVersion.rc)
    endif()
    
    if(UNIX)
        FIND_LIBRARY(FLYCAPTURE_LIBRARY NAMES libflycapture flycapture PATHS /usr/lib NO_DEFAULT_PATH)
    else()
        if(MSVC_VERSION GREATER 1899) #>= MSVC 15.0
            find_library(FLYCAPTURE_LIBRARY NAMES "FlyCapture2_v140"  PATHS ${PGR_LIBDIR_HINT} NO_DEFAULT_PATH) #requires Microsoft C++ Redistributible 2015
        elseif(MSVC_VERSION EQUAL 1800)
            find_library(FLYCAPTURE_LIBRARY NAMES "FlyCapture2_v120"  PATHS ${PGR_LIBDIR_HINT} NO_DEFAULT_PATH) #requires Microsoft C++ Redistributible 2010
        elseif(MSVC_VERSION EQUAL 1700)
            find_library(FLYCAPTURE_LIBRARY NAMES "FlyCapture2_v110"  PATHS ${PGR_LIBDIR_HINT} NO_DEFAULT_PATH) #requires Microsoft C++ Redistributible 2012            
        elseif(MSVC_VERSION EQUAL 1600)
            find_library(FLYCAPTURE_LIBRARY NAMES "FlyCapture2_v100"  PATHS ${PGR_LIBDIR_HINT} NO_DEFAULT_PATH) #requires Microsoft C++ Redistributible 2010
        elseif(MSVC_VERSION EQUAL 1500)
            find_library(FLYCAPTURE_LIBRARY NAMES "FlyCapture2_v90"  PATHS ${PGR_LIBDIR_HINT} NO_DEFAULT_PATH) #requires Microsoft C++ Redistributible 2009
        elseif(MSVC_VERSION EQUAL 1400)
            find_library(FLYCAPTURE_LIBRARY NAMES "FlyCapture2"  PATHS ${PGR_LIBDIR_HINT} NO_DEFAULT_PATH) #requires Microsoft C++ Redistributible 2008
        else()
            find_library(FLYCAPTURE_LIBRARY NAMES "FlyCapture2_v110"  PATHS ${PGR_LIBDIR_HINT} NO_DEFAULT_PATH) #requires Microsoft C++ Redistributible 2012
        endif()
    endif()
    
    if(FLYCAPTURE_LIBRARY)
        message(STATUS "FlyCapture library found at ${FLYCAPTURE_LIBRARY}")
    else()
        message(SEND_FATAL "FlyCapture library not found")
    endif()
    
    if(WIN32)
        if(MSVC_VERSION GREATER 1899) #>= MSVC 15.0
            find_file(FLYCAPTURE_BINARY "FlyCapture2_v140.dll"  PATHS ${PGR_BINDIR_HINT} PATH_SUFFIXES vs2015 NO_DEFAULT_PATH) #requires Microsoft C++ Redistributible 2015    
        elseif(MSVC_VERSION EQUAL 1800)
            find_file(FLYCAPTURE_BINARY "FlyCapture2_v120.dll"  PATHS ${PGR_BINDIR_HINT} NO_DEFAULT_PATH) #requires Microsoft C++ Redistributible 2010    
        elseif(MSVC_VERSION EQUAL 1700)
            find_file(FLYCAPTURE_BINARY "FlyCapture2_v110.dll"  PATHS ${PGR_BINDIR_HINT} NO_DEFAULT_PATH) #requires Microsoft C++ Redistributible 2010
        elseif(MSVC_VERSION EQUAL 1600)
            find_file(FLYCAPTURE_BINARY "FlyCapture2_v100.dll"  PATHS ${PGR_BINDIR_HINT} NO_DEFAULT_PATH) #requires Microsoft C++ Redistributible 2012            
        elseif(MSVC_VERSION EQUAL 1500)
            find_file(FLYCAPTURE_BINARY "FlyCapture2_v90.dll"  PATHS ${PGR_BINDIR_HINT} NO_DEFAULT_PATH) #requires Microsoft C++ Redistributible 2009
        elseif(MSVC_VERSION EQUAL 1400)
            find_file(FLYCAPTURE_BINARY "FlyCapture2.dll"  PATHS ${PGR_BINDIR_HINT} NO_DEFAULT_PATH) #requires Microsoft C++ Redistributible 2008
        else()
            find_file(FLYCAPTURE_BINARY "FlyCapture2_v110.dll"  PATHS ${PGR_BINDIR_HINT} NO_DEFAULT_PATH) #requires Microsoft C++ Redistributible 2012
        endif()
    
        find_file(FLYCAPTURE_LIBIOMP5MD "libiomp5md.dll"  PATHS ${PGR_BINDIR_HINT} NO_DEFAULT_PATH)
		find_file(FLYCAPTURE_AVCODEC "avcodec-57.dll"  PATHS ${PGR_BINDIR_HINT} NO_DEFAULT_PATH)
		find_file(FLYCAPTURE_AVDEVICE "avdevice-57.dll"  PATHS ${PGR_BINDIR_HINT} NO_DEFAULT_PATH)
		find_file(FLYCAPTURE_AVFORMAT "avformat-57.dll"  PATHS ${PGR_BINDIR_HINT} NO_DEFAULT_PATH)
		find_file(FLYCAPTURE_AVUTIL "avutil-55.dll"  PATHS ${PGR_BINDIR_HINT} NO_DEFAULT_PATH)
		find_file(FLYCAPTURE_VIDEOENCODER "PtGreyVideoEncoder_v140.dll"  PATHS ${PGR_BINDIR_HINT} NO_DEFAULT_PATH)
		find_file(FLYCAPTURE_SWRESAMPLE "swresample-2.dll"  PATHS ${PGR_BINDIR_HINT} NO_DEFAULT_PATH)
		find_file(FLYCAPTURE_SWSCALE "swscale-4.dll"  PATHS ${PGR_BINDIR_HINT} NO_DEFAULT_PATH)
    
        if(FLYCAPTURE_BINARY)
            message(STATUS "FlyCapture binary found at ${FLYCAPTURE_BINARY}")
        else(FLYCAPTURE_BINARY)
            message(SEND_FATAL "FlyCapture binary not found")
        endif(FLYCAPTURE_BINARY)

        if(FLYCAPTURE_LIBIOMP5MD)
            message(STATUS "libiomp5md.dll found at ${FLYCAPTURE_LIBIOMP5MD}")
        else(FLYCAPTURE_LIBIOMP5MD)
            message(SEND_FATAL "libiomp5md.dll not found")
        endif(FLYCAPTURE_LIBIOMP5MD)
		
		if(FLYCAPTURE_AVCODEC)
            message(STATUS "avcodec-57.dll found at ${FLYCAPTURE_AVCODEC}")
        else(FLYCAPTURE_AVCODEC)
            message(SEND_FATAL "avcodec-57.dll not found")
        endif(FLYCAPTURE_AVCODEC)
		
		if(FLYCAPTURE_AVDEVICE)
            message(STATUS "avdevice-57.dll found at ${FLYCAPTURE_AVDEVICE}")
        else(FLYCAPTURE_AVDEVICE)
            message(SEND_FATAL "avdevice-57.dll not found")
        endif(FLYCAPTURE_AVDEVICE)
		
		if(FLYCAPTURE_AVFORMAT)
            message(STATUS "avformat-57.dll found at ${FLYCAPTURE_AVFORMAT}")
        else(FLYCAPTURE_AVFORMAT)
            message(SEND_FATAL "avformat-57.dll not found")
        endif(FLYCAPTURE_AVFORMAT)

        if(FLYCAPTURE_AVUTIL)
            message(STATUS "avutil-55.dll found at ${FLYCAPTURE_AVUTIL}")
        else(FLYCAPTURE_AVUTIL)
            message(SEND_FATAL "avutil-55.dll not found")
        endif(FLYCAPTURE_AVUTIL)
		
		if(FLYCAPTURE_VIDEOENCODER)
            message(STATUS "PtGreyVideoEncoder_v140.dll found at ${FLYCAPTURE_VIDEOENCODER}")
        else(FLYCAPTURE_VIDEOENCODER)
            message(SEND_FATAL "PtGreyVideoEncoder_v140.dll not found")
        endif(FLYCAPTURE_VIDEOENCODER)
		
		if(FLYCAPTURE_SWRESAMPLE)
            message(STATUS "swresample-2.dll found at ${FLYCAPTURE_SWRESAMPLE}")
        else(FLYCAPTURE_SWRESAMPLE)
            message(SEND_FATAL "swresample-2.dll not found")
        endif(FLYCAPTURE_SWRESAMPLE)
		
		if(FLYCAPTURE_SWSCALE)
            message(STATUS "swscale-4.dll found at ${FLYCAPTURE_SWSCALE}")
        else(FLYCAPTURE_SWSCALE)
            message(SEND_FATAL "swscale-4.dll not found")
        endif(FLYCAPTURE_SWSCALE)
    endif()
    
    add_library(${target_name} SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS} ${PLUGIN_UI} ${PLUGIN_RCC})
    
    # Qt: enable all automoc, autouic and autorcc.
    set_target_properties(${target_name} PROPERTIES AUTOMOC ON AUTORCC ON AUTOUIC ON)

    target_link_libraries(${target_name} ${ITOM_SDK_LIBRARIES} ${FLYCAPTURE_LIBRARY} ${QT5_LIBRARIES} ${VISUALLEAKDETECTOR_LIBRARIES}) 


    
    itom_configure_plugin_documentation(${target_name} pgrFlyCapture)
    
    #translation
    set(FILES_TO_TRANSLATE ${PLUGIN_SOURCES} ${PLUGIN_HEADERS} ${PLUGIN_UI})
    itom_library_translation(QM_FILES TARGET ${target_name} FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE})

    # COPY SECTION
    set(COPY_SOURCES "")
    set(COPY_DESTINATIONS "")
    itom_add_pluginlibrary_to_copy_list(${target_name} COPY_SOURCES COPY_DESTINATIONS)
    itom_add_plugin_qm_files_to_copy_list(${target_name} QM_FILES COPY_SOURCES COPY_DESTINATIONS)
    itom_post_build_copy_files(${target_name} COPY_SOURCES COPY_DESTINATIONS)
    
    if(WIN32)
        itom_post_build_copy_files_to_lib_folder(${target_name} FLYCAPTURE_BINARY)
        itom_post_build_copy_files_to_lib_folder(${target_name} FLYCAPTURE_LIBIOMP5MD)
		itom_post_build_copy_files_to_lib_folder(${target_name} FLYCAPTURE_AVCODEC)
		itom_post_build_copy_files_to_lib_folder(${target_name} FLYCAPTURE_AVDEVICE)
        itom_post_build_copy_files_to_lib_folder(${target_name} FLYCAPTURE_AVFORMAT)
		itom_post_build_copy_files_to_lib_folder(${target_name} FLYCAPTURE_AVUTIL)
		itom_post_build_copy_files_to_lib_folder(${target_name} FLYCAPTURE_VIDEOENCODER)
        itom_post_build_copy_files_to_lib_folder(${target_name} FLYCAPTURE_SWRESAMPLE)
		itom_post_build_copy_files_to_lib_folder(${target_name} FLYCAPTURE_SWSCALE)
    endif()
    
else()
    message(WARNING "${target_name}-API directory could not be found. ${target_name} will not be build")    
endif()