cmake_minimum_required(VERSION 3.20)
project(WebRtc_Cpp_Linux)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CROSSCOMPILING TRUE)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
include_directories(
        #################################################################
        #                   >      Project Files
        "src"
        "picojson"
        #################################################################
        #                   >    WebRtc Project Files

        "$ENV{HOME}/webrtc_linux/src"
        "$ENV{HOME}/webrtc_linux/src/test"
        "$ENV{HOME}/webrtc_linux/src/build/out/Debug/obj"
        "$ENV{HOME}/webrtc_linux/src/third_party/abseil-cpp"
        "$ENV{HOME}/webrtc_linux/src/third_party/googletest/src/googletest/include"
        "$ENV{HOME}/webrtc_linux/src/third_party/libyuv/include"
        "$ENV{HOME}/webrtc_linux/src/third_party/libyuv/source"
        "$ENV{HOME}/webrtc_linux/src/third_party/jsoncpp/source/include"
        "$ENV{HOME}/webrtc_linux/src/base"
        "$ENV{HOME}/webrtc_linux/src/media/base"
        "$ENV{HOME}/webrtc_linux/src/media/engine"
        "$ENV{HOME}/webrtc_linux/src/pc"
        "$ENV{HOME}/Downloads/curl-7.84.0/include"

        #################################################################
        #                   >    External Modules

        "/usr/include/websocketpp"
        "/usr/include/nlohmann"
        "/usr/include/boost"
)

add_definitions(
        -DWEBRTC_POSIX
        -D_ITERATOR_DEBUG_LEVEL=0
        -DUSE_AURA=1
        -D_HAS_EXCEPTIONS=0
        -D__STD_C
        -D_CRT_RAND_S
        -D_CRT_SECURE_NO_DEPRECATE
        -D_SCL_SECURE_NO_DEPRECATE
        -D_ATL_NO_OPENGL
        -DCERT_CHAIN_PARA_HAS_EXTRA_FIELDS
        -DPSAPI_VERSION=2
        -D_SECURE_ATL
        -D__WRL_NO_DEFAULT_LIB__
        -DNOMINMAX
        -D_UNICODE
        -DUNICODE
        -DNTDDI_VERSION=NTDDI_WIN10_RS2
        -DNDEBUG
        -DNVALGRIND
        -DDYNAMIC_ANNOTATIONS_ENABLED=0
        -DWEBRTC_ENABLE_PROTOBUF=0
        -DWEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE
        -DRTC_ENABLE_VP9
        -DHAVE_SCTP
        -DWEBRTC_LIBRARY_IMPL
        -DWEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=0
        -DABSL_ALLOCATOR_NOTHROW=1
        -DHAVE_SCTP
        -DWEBRTC_USE_BUILTIN_OPUS=1
)

add_executable(WebRtc_Cpp_Linux
        main.cpp
        src/rtc_connection.h
        src/rtc_wrapper.h
        src/rtc_ice.h
        src/http.h
        src/file_process.h
        /home/lyx/webrtc_linux/src/test/vcm_capturer.cc
        /home/lyx/webrtc_linux/src/test/test_video_capturer.cc
        /home/lyx/webrtc_linux/src/media/base/video_adapter.cc
        src/file_process.h)

# openssl
find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIRS})
    message(STATUS "OpenSSL Found!")
endif()

# curl
find_package(CURL REQUIRED)

if(CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIR})
    message(STATUS "Curl Found: ${CURL_VERSION_STRING} ${CURL_LIBRARIES} ${CURL_LINK_LIBRARIES}!")
else(CURL_FOUND)
    message(FATAL_ERROR "Could not find the CURL library and development files.")
endif()


target_link_libraries(WebRtc_Cpp_Linux PUBLIC ${CURL_LIBRARIES})
target_link_libraries(WebRtc_Cpp_Linux PUBLIC OpenSSL::Crypto OpenSSL::SSL)
target_link_libraries(WebRtc_Cpp_Linux

        PUBLIC "$ENV{HOME}/webrtc_linux/libwebrtc.a"
        -lpthread
        -lboost_system
        -ldl
        )