if (NOT TARGET panda-lib)
    find_package(unievent REQUIRED)
    find_package(panda-protocol-websocket REQUIRED)
    find_package(panda-encode-base2n REQUIRED)
    include("${CMAKE_CURRENT_LIST_DIR}/unievent-websocket-targets.cmake")
endif()