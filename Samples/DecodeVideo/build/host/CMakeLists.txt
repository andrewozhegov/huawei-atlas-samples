project(host)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

if(NOT DEFINED ENV{FFMPEG_PATH})
    message(FATAL_ERROR "please define environment variable:FFMPEG_PATH")
else()
    # message("FFMPEG_PATH $ENV{FFMPEG_PATH}")
    set(FFMPEG_PATH $ENV{FFMPEG_PATH})
endif()
find_package(FFMPEG REQUIRED)

#aux_source_directory(${PROJECT_SRC_ROOT}/SrcEngine SrcEngine_SRC)
aux_source_directory(${PROJECT_SRC_ROOT}/StreamPuller SrcEngine_SRC)

aux_source_directory(${PROJECT_SRC_ROOT}/DstEngine DstEngine_SRC)
aux_source_directory(${PROJECT_SRC_ROOT} main_src)

add_executable(main ${main_src} ${SrcEngine_SRC} ${DstEngine_SRC})
target_include_directories(main PUBLIC ${DDK_PROTOBUF_INCLUDE_DIRS} ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(main ${DDK_HOST_LIBRARIES} ${DDK_PROTOBUF_LIBRARYS} ${FFMPEG_LIBRARIES})
