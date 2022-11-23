set(lib_name phyphox_ble)
zephyr_library_named(${lib_name})


if(NOT DEFINED PHYPHOX_EXP_IN)
  message( SEND_ERROR "Phyphox experiment (.phyphox) path must be defined as PHYPHOX_EXP_IN" )
endif()

if(NOT DEFINED PHYPHOX_EXP_OUT)
  message( SEND_ERROR "Phyphox experiment out path must be defined as PHYPHOX_EXP_OUT" )
endif()


set(PHYPHOX_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/scripts/phyphox_xml.py)
# Generate Phyphox experiment data from xml
find_package(Python3)
if(NOT Python3_FOUND)
	message( SEND_ERROR "Could not find Python 3.x" )
endif()
add_custom_command( OUTPUT ${PHYPHOX_EXP_OUT}/phyphox_autogen.cpp
							${PHYPHOX_EXP_OUT}/phyphox_autogen.hpp
                    PRE_BUILD
                    COMMAND ${Python3_EXECUTABLE} ${PHYPHOX_SCRIPT} ${PHYPHOX_EXP_IN} ${PHYPHOX_EXP_OUT}
                    DEPENDS ${PHYPHOX_EXP_IN}
                            ${PHYPHOX_SCRIPT}
                  )

zephyr_library_sources(src/phyphox_ble.cpp
                        ${PHYPHOX_EXP_OUT}/phyphox_autogen.cpp)
target_include_directories(app PUBLIC include
                                      libs/uuid_utils/include
                            )
zephyr_include_directories(include
                            libs/uuid_utils/include
                            ${PHYPHOX_EXP_OUT})

target_compile_options(${lib_name} PRIVATE -Wall -Wextra -Wfatal-errors)