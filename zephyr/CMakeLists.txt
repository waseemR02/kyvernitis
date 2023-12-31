if(CONFIG_KYVERNITIS)
    set(KYVERNITIS_DIR ${ZEPHYR_CURRENT_MODULE_DIR})
    zephyr_include_directories(${KYVERNITIS_DIR})
    zephyr_library()
    zephyr_library_sources(
        ${KYVERNITIS_DIR}/kyvernitis.c
        )
endif()
