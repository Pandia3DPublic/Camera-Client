cmake_minimum_required(VERSION 3.1)
set (CMAKE_CXX_STANDARD 11)

#make list of own source code
project(CameraClient LANGUAGES CXX)
file(GLOB srcfiles 
${PROJECT_SOURCE_DIR}/src/*.h   
${PROJECT_SOURCE_DIR}/src/*.cpp
${PROJECT_SOURCE_DIR}/src/*.cc
${PROJECT_SOURCE_DIR}/src/*/*.h   
${PROJECT_SOURCE_DIR}/src/*/*.cpp
${PROJECT_SOURCE_DIR}/src/*/*.cc
)
#configure headers 
set (datapath ${PROJECT_SOURCE_DIR}/data)
configure_file (
  "${PROJECT_SOURCE_DIR}/src/cmakedefines.h.in"
  "${PROJECT_SOURCE_DIR}/src/cmakedefines.h"
)


#opencv stuff
message("\n \n Finding opencv package:")
set(OpenCV_DIR 3rdparty/opencv)
find_package(OpenCV)

#protobuf stuff
message("\n \n Finding protobuf package:")
set(Protobuf_DIR 3rdparty/protobuf)
include(${Protobuf_DIR}/cmake/protobuf-config.cmake)
include(${Protobuf_DIR}/cmake/protobuf-module.cmake)
include(${Protobuf_DIR}/cmake/protobuf-options.cmake)
include(${Protobuf_DIR}/cmake/protobuf-targets.cmake)
find_package(Protobuf HINTS ${Protobuf_DIR})

#Kinect stuff
message("\n \n Finding Kinect package:")
set(KINECT_LIBRARY_DIR ${PROJECT_SOURCE_DIR}\\3rdparty\\Kinect\\lib)
set(KINECT_DLL_DIR ${PROJECT_SOURCE_DIR}\\3rdparty\\Kinect\\bin)
set(KINECT_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}\\3rdparty\\Kinect\\include)
find_library(KINECT_FOUND k4a HINTS ${KINECT_LIBRARY_DIR} REQUIRED)

#ffmpeg stuff
message("\n \n Finding ffmpeg:")
set(FFMPEG_LIBRARY_DIR ${PROJECT_SOURCE_DIR}/3rdparty/ffmpeg/lib)
set(FFMPEG_DLL_DIR ${PROJECT_SOURCE_DIR}/3rdparty/ffmpeg/bin)
set(FFMPEG_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/3rdparty/ffmpeg/include)

#do target stuff
set(target_names main) #cpp files with same name must be located in main #testexe
message("####################")
message(${target_names})

FOREACH(target_name ${target_names})
	add_executable( ${target_name} src/${target_name}.cpp ${srcfiles})



# Check for opencv, if found print message, include dirs and link libraries.
	if(OPENCV_FOUND)
		message("Opencv Found")
		include_directories(${OpenCV_INCLUDE_DIRS})
		target_link_libraries( ${target_name} ${OpenCV_LIBS})
	elseif(NOT OPENCV_FOUND)
		message("Opencv Not Found")
	endif()
#check for protobuf 
	if (Protobuf_FOUND)
		message("Protobuf found")
		include_directories(${Protobuf_INCLUDE_DIRS})
		target_link_libraries(${target_name} ${Protobuf_LIBRARIES})
		message( STATUS "Protobuf version : ${Protobuf_VERSION}" )
		message( STATUS "Protobuf include path : ${Protobuf_INCLUDE_DIRS}" )
	    message( STATUS "Protobuf libraries : ${Protobuf_LIBRARIES}" )
	endif()
# check for Kinect
	if(KINECT_FOUND)
		message("Kinect Found")
		include_directories(${KINECT_INCLUDE_DIRS})
		target_link_libraries( ${target_name} "${KINECT_LIBRARY_DIR}/k4a.lib")
	elseif(NOT KINECT_FOUND)
		message("Kinect Not Found")
	endif()
# check for ffmpeg
message("ffmpeg: including dirs and linking libraries")
include_directories(${FFMPEG_INCLUDE_DIRS})
target_link_libraries( ${target_name} "${FFMPEG_LIBRARY_DIR}/avcodec.lib" "${FFMPEG_LIBRARY_DIR}/avdevice.lib" "${FFMPEG_LIBRARY_DIR}/avfilter.lib" "${FFMPEG_LIBRARY_DIR}/avformat.lib" "${FFMPEG_LIBRARY_DIR}/avutil.lib" "${FFMPEG_LIBRARY_DIR}/postproc.lib" "${FFMPEG_LIBRARY_DIR}/swresample.lib" "${FFMPEG_LIBRARY_DIR}/swscale.lib")


ENDFOREACH()

list(GET target_names 0 firstname)


#post build event
#copy opencv dll into build folder
add_custom_command(TARGET ${firstname} POST_BUILD        # Adds a post-build event to MyTest
COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
    "${PROJECT_SOURCE_DIR}/3rdparty/opencv/x64/vc15/bin/opencv_world412.dll"      # <--this is in-file
     $<TARGET_FILE_DIR:${firstname}>)                 # <--this is out-file path
#protobuf 
add_custom_command(TARGET ${firstname} POST_BUILD        
COMMAND ${CMAKE_COMMAND} -E copy_if_different             
    "${PROJECT_SOURCE_DIR}/3rdparty/protobuf/bin/protoc.exe"                           
 	 $<TARGET_FILE_DIR:${firstname}>)    
#Kinect 
add_custom_command(TARGET ${firstname} POST_BUILD        
COMMAND ${CMAKE_COMMAND} -E copy_if_different             
    "${KINECT_DLL_DIR}/k4a.dll"                           
 	 $<TARGET_FILE_DIR:${firstname}>)   
add_custom_command(TARGET ${firstname} POST_BUILD        
COMMAND ${CMAKE_COMMAND} -E copy_if_different             
    "${KINECT_DLL_DIR}/depthengine_2_0.dll"                           
 	 $<TARGET_FILE_DIR:${firstname}>)
#ffmpeg 
add_custom_command(TARGET ${firstname} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${FFMPEG_DLL_DIR}/avcodec-58.dll"
	$<TARGET_FILE_DIR:${firstname}>)
add_custom_command(TARGET ${firstname} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${FFMPEG_DLL_DIR}/avdevice-58.dll"
	$<TARGET_FILE_DIR:${firstname}>)
add_custom_command(TARGET ${firstname} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${FFMPEG_DLL_DIR}/avfilter-7.dll"
	$<TARGET_FILE_DIR:${firstname}>)
add_custom_command(TARGET ${firstname} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${FFMPEG_DLL_DIR}/avformat-58.dll"
	$<TARGET_FILE_DIR:${firstname}>)
add_custom_command(TARGET ${firstname} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${FFMPEG_DLL_DIR}/avutil-56.dll"
	$<TARGET_FILE_DIR:${firstname}>)
add_custom_command(TARGET ${firstname} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${FFMPEG_DLL_DIR}/postproc-55.dll"
	$<TARGET_FILE_DIR:${firstname}>)
add_custom_command(TARGET ${firstname} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${FFMPEG_DLL_DIR}/swresample-3.dll"
	$<TARGET_FILE_DIR:${firstname}>)
add_custom_command(TARGET ${firstname} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${FFMPEG_DLL_DIR}/swscale-5.dll"
	$<TARGET_FILE_DIR:${firstname}>)