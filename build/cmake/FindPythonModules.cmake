find_package(PythonInterp 3.6.0)

if(PYTHONINTERP_FOUND)
  if(PythonModules_FIND_COMPONENTS)
    foreach(MODULE ${PythonModules_FIND_COMPONENTS})
      execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import  ${MODULE}"
                      OUTPUT_VARIABLE _TRASH RESULT_VARIABLE MODULE_NOT_FOUND ERROR_QUIET)
      if(MODULE_NOT_FOUND)
        set(PythonModules_${MODULE}_FOUND "NOTFOUND")
        if(PythonModules_FIND_REQUIRED)
          message(FATAL_ERROR "Python interpreter cannot find required module '${MODULE}'")
        endif()
      else()
        set(PythonModules_${MODULE}_FOUND "FOUND")
      endif()
    endforeach()
  endif()
else()
  message(FATAL_ERROR "Could not find a suitable python3 interpreter. Ensure that variable PYTHON_EXECUTABLE is set to a python3 interpreter.")
endif()
