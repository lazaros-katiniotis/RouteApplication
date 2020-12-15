cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)

# Use the CMakeLists.txt's parent-directory-name for the project's id/name
get_filename_component(PROJECT_ID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" PROJECT_ID ${PROJECT_ID})
project(${PROJECT_ID})

find_package(CURL CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)

set(ROUTE_APP_SRC
	Helper.h
	Main.cpp
	HTTPHandler.cpp
	HTTPHandler.h
	Renderer.cpp
	Renderer.h
	Model.cpp
	Model.h
	ArgumentParser.cpp
	ArgumentParser.h
	Pathfinder.cpp
	Pathfinder.h	
)

add_executable(${PROJECT_ID} ${ROUTE_APP_SRC})
target_compile_features(${PROJECT_ID} PUBLIC cxx_std_17)

target_link_libraries(${PROJECT_ID} io2d)
target_link_libraries(${PROJECT_ID} CURL::libcurl)
target_link_libraries(${PROJECT_ID} pugixml)
