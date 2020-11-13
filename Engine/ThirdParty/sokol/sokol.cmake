if(APPLE)
	set(APPLE_FRAMEWORKS
		Cocoa
		Metal
		Metalkit
		QuartzCore)

	set(SOKOL_DEPENDENCY_LIBS)
	
	foreach(FW ${APPLE_FRAMEWORKS})
		unset(LIB CACHE)
		find_library(LIB ${FW})
		list(APPEND SOKOL_DEPENDENCY_LIBS "${LIB}")
	endforeach()
endif()

define_module(sokol
	THIRD_PARTY sokol
	INCLUDES_MODULE_BASE
	LINK_LIBS ${SOKOL_DEPENDENCY_LIBS})