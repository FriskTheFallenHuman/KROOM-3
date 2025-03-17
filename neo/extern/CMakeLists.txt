project(external)

set(src_external)
set(src_external_includes)

file(GLOB src_external_miniz miniz/*.c miniz/*.h)
source_group("Miniz" FILES ${src_external_miniz})
list(APPEND src_external ${src_external_miniz})
list(APPEND src_external_includes ${CMAKE_CURRENT_SOURCE_DIR}/miniz)
#always_optimize_sourcefile(miniz/miniz.c)

file(GLOB src_external_minizip minizip/*.cpp minizip/*.c minizip/*.h)
source_group("Minizip" FILES ${src_external_minizip})
list(APPEND src_external ${src_external_minizip})
list(APPEND src_external_includes ${CMAKE_CURRENT_SOURCE_DIR}/minizip)
#always_optimize_sourcefile(minizip/ioapi.c)
#always_optimize_sourcefile(minizip/unzip.cpp)

if(BINKDEC)
file(GLOB src_external_binkdec libbinkdec/src/*.cpp libbinkdec/src/*.c libbinkdec/include/*.h)
source_group("BinkDec" FILES ${src_external_binkdec})
list(APPEND src_external ${src_external_binkdec})
list(APPEND src_external_includes ${CMAKE_CURRENT_SOURCE_DIR}/libbinkdec/include)
endif()

file(GLOB src_external_mikktspace mikktspace/*.c mikktspace/*.h)
source_group("MikktSpace" FILES ${src_external_mikktspace})
list(APPEND src_external ${src_external_mikktspace})

file(GLOB src_external_mesa mesa/*.h)
source_group("Mesa" FILES ${src_external_mesa})
list(APPEND src_external ${src_external_mesa})

file(GLOB src_external_imgui imgui/*.cpp imgui/*.h)
source_group("ImGUI" FILES ${src_external_imgui})
list(APPEND src_external ${src_external_imgui})

set(src_external_jpeg
    jpeg-6/jchuff.h
    jpeg-6/jconfig.h
    jpeg-6/jdct.h
    jpeg-6/jdhuff.h
    jpeg-6/jerror.h
    jpeg-6/jinclude.h
    jpeg-6/jmemsys.h
    jpeg-6/jmorecfg.h
    jpeg-6/jpegint.h
    jpeg-6/jpeglib.h
    jpeg-6/jversion.h)

set(src_external_jpeg
    ${src_external_jpeg}
    jpeg-6/jcapimin.cpp
    jpeg-6/jcapistd.cpp
    jpeg-6/jccoefct.cpp
    jpeg-6/jccolor.cpp
    jpeg-6/jcdctmgr.cpp
    jpeg-6/jchuff.cpp
    jpeg-6/jcinit.cpp
    jpeg-6/jcmainct.cpp
    jpeg-6/jcmarker.cpp
    jpeg-6/jcmaster.cpp
    jpeg-6/jcomapi.cpp
    jpeg-6/jcparam.cpp
    jpeg-6/jcphuff.cpp
    jpeg-6/jcprepct.cpp
    jpeg-6/jcsample.cpp
    jpeg-6/jctrans.cpp
    jpeg-6/jdapimin.cpp
    jpeg-6/jdapistd.cpp
    jpeg-6/jdatadst.cpp
    jpeg-6/jdatasrc.cpp
    jpeg-6/jdcoefct.cpp
    jpeg-6/jdcolor.cpp
    jpeg-6/jddctmgr.cpp
    jpeg-6/jdhuff.cpp
    jpeg-6/jdinput.cpp
    jpeg-6/jdmainct.cpp
    jpeg-6/jdmarker.cpp
    jpeg-6/jdmaster.cpp
    jpeg-6/jdmerge.cpp
    jpeg-6/jdphuff.cpp
    jpeg-6/jdpostct.cpp
    jpeg-6/jdsample.cpp
    jpeg-6/jdtrans.cpp
    jpeg-6/jerror.cpp
    jpeg-6/jfdctflt.cpp
    jpeg-6/jfdctfst.cpp
    jpeg-6/jfdctint.cpp
    jpeg-6/jidctflt.cpp
    jpeg-6/jidctfst.cpp
    jpeg-6/jidctint.cpp
    jpeg-6/jidctred.cpp
    #jpeg-6/jload.cpp
    jpeg-6/jmemmgr.cpp
    jpeg-6/jmemnobs.cpp
    #jpeg-6/jpegtran.cpp
    jpeg-6/jquant1.cpp
    jpeg-6/jquant2.cpp
    jpeg-6/jutils.cpp)
source_group("Jpeg-6" FILES ${src_external_jpeg})
list(APPEND src_external ${src_external_jpeg})
list(APPEND src_external_includes ${CMAKE_CURRENT_SOURCE_DIR}/jpeg-6)

file(GLOB src_external_png png/*.c png/*.h)
source_group("PNG" FILES ${src_external_png})
list(APPEND src_external ${src_external_png})
list(APPEND src_external_includes ${CMAKE_CURRENT_SOURCE_DIR}/png)

if(NOT USE_VULKAN)
file(GLOB src_external_glew glew/src/*.c glew/include/GL/*.h)
list(REMOVE_ITEM src_external_glew ${CMAKE_CURRENT_SOURCE_DIR}/glew/src/visualinfo.c)
if(WIN32)
list(REMOVE_ITEM src_external_glew ${CMAKE_CURRENT_SOURCE_DIR}/glew/include/GL/glxew.h)
else()
list(REMOVE_ITEM src_external_glew ${CMAKE_CURRENT_SOURCE_DIR}/glew/include/GL/wglew.h)
endif()
source_group("Glew" FILES ${src_external_glew})
list(APPEND src_external ${src_external_glew})
list(APPEND src_external_includes ${CMAKE_CURRENT_SOURCE_DIR}/glew/include)
endif()

file(GLOB src_external_stb stb/*.h)
source_group("Stb" FILES ${src_external_stb})
list(APPEND src_external ${src_external_stb})

file(GLOB src_external_tinyexr tinyexr/*.h)
source_group("TinyEXR" FILES ${src_external_tinyexr})
list(APPEND src_external ${src_external_tinyexr})

add_library(${PROJECT_NAME} STATIC ${src_external})
target_compile_definitions(${PROJECT_NAME} PRIVATE GLEW_STATIC)
target_include_directories(${PROJECT_NAME} PRIVATE ${src_external_includes})