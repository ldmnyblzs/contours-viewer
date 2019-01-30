cmake_minimum_required(VERSION 3.9)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set (CMAKE_CXX_STANDARD 17)

#include(cotire)
#include(ExternalProject)

project(tr)

find_package(Boost REQUIRED filesystem iostreams)
find_package(wxWidgets REQUIRED core base gl aui adv)
include(${wxWidgets_USE_FILE})
find_package(VTK REQUIRED vtkRenderingOpenGL2 vtkRenderingAnnotation vtkInteractionStyle vtkInfovisLayout vtkViewsInfovis)
include(${VTK_USE_FILE})
find_package(TBB CONFIG REQUIRED)
find_package(CGAL REQUIRED Core)
find_package(GD REQUIRED)
include_directories(${GD_INCLUDE_DIRS})
find_package(bliss REQUIRED)
find_package(shape REQUIRED)

#add_executable(tr WIN32 parameters.cpp mesh.cpp levelgraph.cpp subrangegraph.cpp reebgraph.cpp ratios.cpp computation.cpp window.cpp singlefile.cpp inputform.cpp meshview.cpp graphview.cpp outputview.cpp wxVTKWidget.cpp batch.cpp batchfile.cpp parametersview.cpp filesview.cpp)
add_executable(tr WIN32 window.cpp singlefile.cpp inputform.cpp meshview.cpp graphview.cpp outputview.cpp wxVTKWidget.cpp batchfile.cpp parametersview.cpp filesview.cpp model/batch.cpp model/execute.cpp model/ratios.cpp)
target_compile_definitions(tr PRIVATE WXUSINGDLL)
target_compile_definitions(tr PRIVATE NOMINMAX)
target_compile_definitions(tr PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_definitions(tr PRIVATE HAVE_LIBPNG)

#target_link_libraries(tr PRIVATE Boost::filesystem Boost::iostreams ${VTK_LIBRARIES} ${wxWidgets_LIBRARIES} CGAL::CGAL CGAL::CGAL_Core tbb ${GD_LIBRARIES} ${install_dir}/lib/libbliss.a shape)
target_link_libraries(tr PRIVATE Boost::filesystem Boost::iostreams ${VTK_LIBRARIES} ${wxWidgets_LIBRARIES} CGAL::CGAL CGAL::CGAL_Core ${GD_LIBRARIES} TBB::tbb shape)
#cotire(tr)

install(TARGETS tr RUNTIME DESTINATION .)
install(CODE
"
	include(BundleUtilities)
	fixup_bundle(\"\$\{CMAKE_INSTALL_PREFIX\}/tr.exe\" \"\" \"\" IGNORE_ITEM \"opengl32.dll\")
"
DESTINATION .)

set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
include(InstallRequiredSystemLibraries)
include(CPack)
