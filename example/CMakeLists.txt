#============================================================================
# Name        : CMakeLists.txt
# Author      : Juan Manuel Fernández Muñoz
# Date        : July, 2017
# Description : CMake configuration file for ComInterface examples
#============================================================================

# ComInterface example
add_executable(example example-cominterface.cpp)

# Linker libraries
target_link_libraries(example ${PROJECT_NAME})

# Installation
install(TARGETS example
		RUNTIME DESTINATION bin)