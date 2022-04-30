idf_component_register(SRCS "potentiometer.c" 
                    INCLUDE_DIRS "include" "${CMAKE_SOURCE_DIR}/main"
                    REQUIRES "ble")