if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(SOKOL_DEPENDENCY_LIBS
		Cocoa
		Metal
		Metalkit
		QuartzCore)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(SOKOL_DEPENDENCY_LIBS
		X11
		Xi
		Xcursor
		GL
		m
		dl
		asound
		pthread)
endif()

define_module(sokol
	THIRD_PARTY sokol
	INCLUDES_MODULE_BASE
	HEADER_ONLY
	SYSTEM_LIBS ${SOKOL_DEPENDENCY_LIBS})