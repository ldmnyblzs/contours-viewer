cmake_minimum_required(VERSION 3.9)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set (CMAKE_CXX_STANDARD 17)

project(tr)

find_package(Boost REQUIRED filesystem)
find_package(wxWidgets REQUIRED core base gl aui adv)
include(${wxWidgets_USE_FILE})
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})
find_package(CGAL REQUIRED Core)


find_package(TBB REQUIRED)
find_package(GD REQUIRED)
include_directories(${GD_INCLUDE_DIRS})

add_executable(tr WIN32 parameters.cpp mesh.cpp levelgraph.cpp subrangegraph.cpp reebgraph.cpp ratios.cpp computation.cpp application.cpp window.cpp singlefile.cpp inputform.cpp meshview.cpp graphview.cpp outputview.cpp wxVTKWidget.cpp batch.cpp batchfile.cpp parametersview.cpp filesview.cpp)
target_compile_definitions(tr PRIVATE WXUSINGDLL)
target_compile_definitions(tr PRIVATE NOMINMAX)
target_compile_definitions(tr PRIVATE _CRT_SECURE_NO_WARNINGS)

target_link_libraries(tr PRIVATE Boost::filesystem ${VTK_LIBRARIES} ${wxWidgets_LIBRARIES} CGAL::CGAL CGAL::CGAL_Core tbb ${GD_LIBRARIES})
