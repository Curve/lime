function(lime_mingw_generate_proxy TARGET DEFINITIONS)
    message(STATUS "[lime] Generating Proxy for \"${TARGET}\"")

    file(STRINGS ${DEFINITIONS} DEFS)
    set(MAP_EXPORTS "")

    set(DEF_FILE "${CMAKE_BINARY_DIR}/lime-exports.def")
    set(SRC_FILE "${CMAKE_BINARY_DIR}/lime-exports.cpp")

    file(REMOVE ${DEF_FILE})
    file(REMOVE ${SRC_FILE})

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

        file(APPEND ${DEF_FILE} "\t${entry} = LIME_PROXY_EXPORT_${CURRENT_INDEX} @${CURRENT_ORDINAL}\n")
        string(APPEND MAP_EXPORTS " \\\n\tTRANSFORMER(${CURRENT_INDEX}, \"${entry}\")")
    endforeach()

    list(LENGTH DEFS EXPORT_COUNT)
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/proxy.cpp.in" ${SRC_FILE})

    target_sources(${TARGET} PUBLIC ${SRC_FILE} ${DEF_FILE})
endfunction()
