project(ffmpeg)

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

if(ENABLE_CCACHE AND CCACHE_PROGRAM)
  set(ffmpeg_conf "--cc=${CCACHE_PROGRAM} ${CMAKE_C_COMPILER}" "--cxx=${CCACHE_PROGRAM} ${CMAKE_CXX_COMPILER}")
else()
  set(ffmpeg_conf --cc=${CMAKE_C_COMPILER} --cxx=${CMAKE_CXX_COMPILER})
endif()

if(CROSSCOMPILING)
  set(pkgconf "PKG_CONFIG_LIBDIR=${DEPENDS_PATH}/lib/pkgconfig")
  list(APPEND ffmpeg_conf --pkg-config=${PKG_CONFIG_EXECUTABLE} --pkg-config-flags=--static)
  list(APPEND ffmpeg_conf --enable-cross-compile --cpu=${CPU} --arch=${CPU} --target-os=${OS})
  list(APPEND ffmpeg_conf --ar=${CMAKE_AR} --strip=${CMAKE_STRIP})
  message(STATUS "CROSS: ${ffmpeg_conf}")
endif()

if(CMAKE_C_FLAGS)
  list(APPEND ffmpeg_conf --extra-cflags=${CMAKE_C_FLAGS})
endif()

if(CMAKE_CXX_FLAGS)
  list(APPEND ffmpeg_conf --extra-cxxflags=${CMAKE_CXX_FLAGS})
endif()

if(CMAKE_EXE_LINKER_FLAGS)
  list(APPEND ffmpeg_conf --extra-ldflags=${CMAKE_EXE_LINKER_FLAGS})
endif()

if(ENABLE_NEON)
  list(APPEND ffmpeg_conf --enable-neon)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release)
  list(APPEND ffmpeg_conf --disable-debug)
endif()

if(CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd)
  list(APPEND ffmpeg_conf --enable-pic)
  if(ENABLE_VAAPI)
    list(APPEND ffmpeg_conf --enable-vaapi)
  else()
    list(APPEND ffmpeg_conf --disable-vaapi)
  endif()
  if(ENABLE_VDPAU)
    list(APPEND ffmpeg_conf --enable-vdpau)
  else()
    list(APPEND ffmpeg_conf --disable-vdpau)
  endif()
elseif(CORE_SYSTEM_NAME STREQUAL android)
  if(CPU MATCHES arm64)
    list(APPEND ffmpeg_conf --cpu=cortex-a53 --arch=aarch64)
  elseif(CPU MATCHES arm)
    list(APPEND ffmpeg_conf --cpu=cortex-a9)
  elseif(CPU MATCHES x86_64)
    list(APPEND ffmpeg_conf --cpu=x86_64 --enable-pic)
  else()
    list(APPEND ffmpeg_conf --cpu=i686 --disable-mmx)
  endif()
  list(APPEND ffmpeg_conf --target-os=linux --extra-libs=-liconv --disable-linux-perf)
elseif(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
  list(APPEND ffmpeg_conf --disable-crystalhd --enable-videotoolbox 
                          --target-os=darwin)
elseif(CORE_SYSTEM_NAME STREQUAL osx)
  list(APPEND ffmpeg_conf --disable-crystalhd --enable-videotoolbox
                          --target-os=darwin
                          --disable-securetransport)
endif()

if(CPU MATCHES arm)
  list(APPEND ffmpeg_conf --enable-pic --disable-armv5te --disable-armv6t2)
elseif(CPU MATCHES mips)
  list(APPEND ffmpeg_conf --disable-mips32r2 --disable-mipsdsp --disable-mipsdspr2)
endif()

find_package(GnuTls)
if(GNUTLS_FOUND)
  list(APPEND ffmpeg_conf --enable-gnutls)
endif()

if(ENABLE_DAV1D)
  list(APPEND ffmpeg_conf --enable-libdav1d)
  set(pkgconf_path "PKG_CONFIG_PATH=${PKG_CONFIG_PATH}")
else()
  list(APPEND ffmpeg_conf --disable-libdav1d)
endif()

if(EXTRA_FLAGS)
  list(APPEND ffmpeg_conf ${EXTRA_FLAGS})
endif()

message(STATUS "FFMPEG_CONF: ${ffmpeg_conf}")

set(MAKE_COMMAND $(MAKE))
if(CMAKE_GENERATOR STREQUAL Ninja)
  set(MAKE_COMMAND make)
  include(ProcessorCount)
  ProcessorCount(N)
  if(NOT N EQUAL 0)
    set(MAKE_COMMAND make -j${N})
  endif()
endif()

include(ExternalProject)
externalproject_add(ffmpeg
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND ${pkgconf} ${pkgconf_path} <SOURCE_DIR>/configure
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      --extra-version="kodi-${FFMPEG_VER}"
                      --disable-devices
                      --disable-doc
                      --disable-programs
                      --enable-gpl
                      --enable-runtime-cpudetect
                      --enable-postproc
                      --enable-pthreads
                      ${ffmpeg_conf}
                    BUILD_COMMAND ${MAKE_COMMAND})

install(CODE "Message(Done)")

# Quell warnings
set(BUILD_SHARED_LIBS)
set(XBMC_BUILD_DIR)
set(KODI_BUILD_DIR)
