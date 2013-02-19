CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(shared_memory_sources
    main.cpp
    spells.cpp
)

SET(shared_memory_headers
    spells.h
)

ADD_EXECUTABLE(shared_memory ${shared_memory_sources} ${shared_memory_headers})

TARGET_LINK_LIBRARIES(shared_memory Common debug ${MySQL_LIBRARY_DEBUG} optimized ${MySQL_LIBRARY_RELEASE})

IF(MSVC)
    SET_TARGET_PROPERTIES(shared_memory PROPERTIES LINK_FLAGS_RELEASE "/OPT:REF /OPT:ICF")
    TARGET_LINK_LIBRARIES(shared_memory "Ws2_32.lib")
ENDIF(MSVC)

IF(MINGW)
    TARGET_LINK_LIBRARIES(shared_memory "WS2_32")
ENDIF(MINGW)

IF(UNIX)
    TARGET_LINK_LIBRARIES(shared_memory "dl")
    TARGET_LINK_LIBRARIES(shared_memory "z")
    TARGET_LINK_LIBRARIES(shared_memory "m")
    TARGET_LINK_LIBRARIES(shared_memory "rt")
    TARGET_LINK_LIBRARIES(shared_memory "pthread")
    ADD_DEFINITIONS(-fPIC)
ENDIF(UNIX)

SET(EXECUTABLE_OUTPUT_PATH ../Bin)