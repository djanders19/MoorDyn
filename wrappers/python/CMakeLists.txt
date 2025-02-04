set(PYSRCS "${CMAKE_CURRENT_SOURCE_DIR}/moordyn/__init__.py"
           "${CMAKE_CURRENT_SOURCE_DIR}/moordyn/moordyn.py"
           "${CMAKE_CURRENT_SOURCE_DIR}/cmoordyn.cpp")

# Prepare the install script, injecting some information coming from cMake
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in"
               "${CMAKE_CURRENT_BINARY_DIR}/tmp/setup.py")
file(
    COPY "${CMAKE_CURRENT_BINARY_DIR}/tmp/setup.py"
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ
                     OWNER_WRITE
                     OWNER_EXECUTE
                     GROUP_READ
                     GROUP_EXECUTE
                     WORLD_READ
                     WORLD_EXECUTE)
set(SETUP_PY "${CMAKE_CURRENT_BINARY_DIR}/setup.py")

# Create the target, depending of course on the main library
set(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/build/moordyn.built")
add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/cmoordyn.cpp" "${CMAKE_CURRENT_BINARY_DIR}/cmoordyn.cpp"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/moordyn" "${CMAKE_CURRENT_BINARY_DIR}/moordyn"
    COMMAND ${Python_EXECUTABLE} ${SETUP_PY} build
    COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
    DEPENDS ${PYSRCS}
)
add_custom_target(python_wrapper ALL DEPENDS ${OUTPUT})
add_dependencies(python_wrapper moordyn)

# Prepare the installation if eventually required
if(${PYTHON_WRAPPER_USERINSTALL})
    set(PY_INSTALL_ARGS "--user")
endif()
install(CODE "
  execute_process(COMMAND ${Python_EXECUTABLE} ${SETUP_PY} install ${PY_INSTALL_ARGS}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
")
