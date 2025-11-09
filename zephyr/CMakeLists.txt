if(CONFIG_KYVERNITIS)
    set(KYVERNITIS_DIR ${ZEPHYR_CURRENT_MODULE_DIR})
    zephyr_include_directories(${KYVERNITIS_DIR}/include)
    zephyr_library()
    zephyr_library_sources(
        ${KYVERNITIS_DIR}/lib/kyvernitis.c
        ${KYVERNITIS_DIR}/lib/drive.c
        ${KYVERNITIS_DIR}/lib/joint_control.c
        )
    if(CONFIG_SOC_FAMILY_STM32)
       zephyr_library_sources(
           ${KYVERNITIS_DIR}/drivers/sensor/wheel_encoders_stm32/qdec_distance_stm32.c
      )
    # add_subdirectory(${KYVERNITIS_DIR}/drivers build)
endif()
