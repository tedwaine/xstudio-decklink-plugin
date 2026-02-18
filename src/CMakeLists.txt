project(blackmagic_decklink VERSION 0.1.0 LANGUAGES CXX)

find_package(nlohmann_json REQUIRED)
find_package(CAF CONFIG COMPONENTS core REQUIRED)
find_package(OpenEXR REQUIRED)
find_package(Imath REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(Qt6 COMPONENTS Core REQUIRED)

# Try to find xStudio via installed CMake config (works on Linux)
find_package(xStudio QUIET)

if(NOT xStudio_FOUND)
	# On macOS (or when xStudio is not installed system-wide), set up imported targets manually.
	# XSTUDIO_ROOT: path to xstudio source tree (for headers)
	# XSTUDIO_APP: path to xSTUDIO .app bundle (for dylibs)
	if(NOT XSTUDIO_ROOT)
		message(FATAL_ERROR "xStudio not found. Set XSTUDIO_ROOT to the xstudio source tree and XSTUDIO_APP to the .app bundle path.")
	endif()
	if(NOT XSTUDIO_APP)
		message(FATAL_ERROR "XSTUDIO_APP not set. Point it to the xSTUDIO .app bundle (e.g. /path/to/xSTUDIO.app).")
	endif()

	set(XSTUDIO_FRAMEWORKS_DIR "${XSTUDIO_APP}/Contents/Frameworks")
	set(XSTUDIO_INCLUDE_DIRS "${XSTUDIO_ROOT}/include;${XSTUDIO_ROOT}/extern/include")

	# Create imported targets for each xstudio library needed by this plugin
	foreach(_lib plugin_manager bookmark utility module media media_reader
				audio_output ui_base viewport_ui global_store json_store global)
		add_library(xstudio::${_lib} SHARED IMPORTED)
		set_target_properties(xstudio::${_lib} PROPERTIES
			IMPORTED_LOCATION "${XSTUDIO_FRAMEWORKS_DIR}/lib${_lib}.dylib"
			INTERFACE_INCLUDE_DIRECTORIES "${XSTUDIO_INCLUDE_DIRS}"
		)
	endforeach()

	# Pick up XSTUDIO_GLOBAL_VERSION from xstudio source if not already set
	if(NOT XSTUDIO_GLOBAL_VERSION)
		set(XSTUDIO_GLOBAL_VERSION "1.1.0")
	endif()
endif()

set(CMAKE_AUTORCC ON)

# Platform-conditional DeckLink SDK dispatch source
if(APPLE)
	set(DECKLINK_DISPATCH_SRC extern/mac/DeckLinkAPIDispatch.cpp)
else()
	set(DECKLINK_DISPATCH_SRC extern/linux/DeckLinkAPIDispatch.cpp)
endif()

set(SOURCES
	decklink_plugin.cpp
	decklink_output.cpp
	decklink_audio_device.cpp
	pixel_swizzler.cpp
	qml/decklink_plugin.qrc
	${DECKLINK_DISPATCH_SRC}
	${blackmagic_decklink_MOC_SRC}
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_link_libraries(${PROJECT_NAME}
	PUBLIC
		xstudio::plugin_manager
		xstudio::bookmark
		xstudio::utility
		xstudio::module
		xstudio::media
		xstudio::media_reader
		xstudio::audio_output
		xstudio::ui_base
		xstudio::viewport_ui
		xstudio::global_store
		xstudio::json_store
		xstudio::global
		CAF::core
		Qt6::Core
	PRIVATE
		OpenEXR::OpenEXR
		Imath::Imath
		spdlog::spdlog
		fmt::fmt
)

# Platform-specific link dependencies
if(APPLE)
	target_link_libraries(${PROJECT_NAME} PRIVATE "-framework CoreFoundation")
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE dl)
endif()

# Include directories for DeckLink SDK platform headers
target_include_directories(${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)

message("Building plugin for xSTUDIO version ${XSTUDIO_GLOBAL_VERSION}")

target_compile_definitions(${PROJECT_NAME}
		PRIVATE XSTUDIO_GLOBAL_VERSION=\"${XSTUDIO_GLOBAL_VERSION}\"
		PRIVATE PROJECT_VERSION=\"${PROJECT_VERSION}\")

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS_NO_SHARED true)

# On macOS, xstudio headers define static globals (e.g. BroadcastActor::NAME) that also
# exist in xstudio's dylibs. Without hidden visibility, the dynamic linker resolves our
# plugin's references to the host library's __DATA_CONST copies, causing SIGBUS when our
# static initializers try to write to that read-only memory.
# Fix: hide all symbols by default so our globals stay local to the plugin.
if(APPLE)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		CXX_VISIBILITY_PRESET hidden
		VISIBILITY_INLINES_HIDDEN ON
	)
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib/xstudio/cpp-plugins)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/plugin_decklink_prefs.json DESTINATION lib/xstudio/cpp-plugins/preferences)

add_subdirectory(qml)
