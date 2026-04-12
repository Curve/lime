function(lime_mingw_generate_proxy)
    cmake_parse_arguments(PARSE_ARGV 0 proxy "" "NAME;DEFINITIONS;TARGET" "")

    if (NOT proxy_NAME)
        set(proxy_NAME "exports")
    endif()

    lime_message(STATUS "Generating Proxy from \"${proxy_DEFINITIONS}\"")

    if (NOT WIN32)
        lime_message(WARNING "Proxy-Generation is only supported on windows!")
        lime_message(WARNING "You might be interested in ld preload for unix")
    endif()

    file(STRINGS ${proxy_DEFINITIONS} DEFS)
    set(MAP_EXPORTS "")

    set(ORIGIN_DIR "${CMAKE_BINARY_DIR}/${proxy_NAME}-proxy")
    set(SOURCE_DIR "${ORIGIN_DIR}/src")
    set(HEADER_DIR "${ORIGIN_DIR}/include")

    file(MAKE_DIRECTORY "${SOURCE_DIR}")
    file(MAKE_DIRECTORY "${HEADER_DIR}/lime")

    set(DEF_FILE "${SOURCE_DIR}/${proxy_NAME}.def")
    set(HPP_FILE "${HEADER_DIR}/lime/${proxy_NAME}.hpp")
    set(CPP_FILE "${SOURCE_DIR}/${proxy_NAME}.cpp")

    file(REMOVE ${DEF_FILE})
    file(REMOVE ${HPP_FILE})
    file(REMOVE ${CPP_FILE})

    file(APPEND ${DEF_FILE} "EXPORTS\n")

    foreach(entry ${DEFS})
        string(REGEX MATCH "=([0-9]+)$" ORDINAL_MATCH ${entry})

        list(FIND DEFS "${entry}" CURRENT_INDEX)
        MATH(EXPR CURRENT_INDEX "${CURRENT_INDEX}+1")

        string(REGEX REPLACE "=([0-9]+)$" "" entry ${entry})

        if (NOT CMAKE_MATCH_1)
            set(CURRENT_ORDINAL ${CURRENT_INDEX})
        else()
            set(CURRENT_ORDINAL ${CMAKE_MATCH_1})
        endif()

        file(APPEND ${DEF_FILE} "\t${entry} = LIME_PROXY_EXPORT_${proxy_NAME}_${CURRENT_INDEX} @${CURRENT_ORDINAL}\n")
        string(APPEND MAP_EXPORTS " \\\n\tTRANSFORMER(${CURRENT_INDEX}, \"${entry}\")")
    endforeach()

    list(LENGTH DEFS EXPORT_COUNT)

    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/proxy.hpp.in" ${HPP_FILE})
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/proxy.cpp.in" ${CPP_FILE})

    target_sources(${proxy_TARGET} PUBLIC ${DEF_FILE} ${CPP_FILE})
    target_include_directories(${proxy_TARGET} PUBLIC ${HEADER_DIR})
endfunction()
