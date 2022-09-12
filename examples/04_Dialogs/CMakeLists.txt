
set(EXAMPLE_NAME "04_Dialogs")
project(${EXAMPLE_NAME} CXX C)



add_executable("${EXAMPLE_NAME}" "${EXAMPLE_NAME}.cpp")
target_link_libraries(${EXAMPLE_NAME} libmzgl)


set_target_properties( ${EXAMPLE_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY    ${PROJECT_SOURCE_DIR}/../bin
)

