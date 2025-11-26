include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/macros.cmake")

project(blackmagic_decklink VERSION 0.1.0 LANGUAGES CXX)

find_package(xStudio REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CAF COMPONENTS core REQUIRED)
find_package(OpenEXR REQUIRED)
find_package(Imath REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(Qt6 COMPONENTS Core REQUIRED)

set(CMAKE_AUTORCC ON)

set(SOURCES
	decklink_plugin.cpp
	decklink_output.cpp
	decklink_audio_device.cpp
	pixel_swizzler.cpp
	qml/decklink_plugin.qrc
	extern/DeckLinkAPIDispatch.cpp
	${blackmagic_decklink_MOC_SRC}
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_link_libraries(${PROJECT_NAME}
	PUBLIC
		xstudio::plugin_manager
		xstudio::bookmark
		caf::core
		Qt6::Core
)

message("Building plugin for xSTUDIO version ${XSTUDIO_GLOBAL_VERSION}")

target_compile_definitions(${PROJECT_NAME}
		PRIVATE XSTUDIO_GLOBAL_VERSION=\"${XSTUDIO_GLOBAL_VERSION}\")
		
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS_NO_SHARED true)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib/xstudio/cpp-plugins)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/plugin_decklink_prefs.json DESTINATION lib/xstudio/cpp-plugins/preferences)

add_subdirectory(qml)
