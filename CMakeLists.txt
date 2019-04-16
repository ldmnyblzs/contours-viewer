cmake_minimum_required(VERSION 3.9)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set (CMAKE_CXX_STANDARD 17)

project(tr)

include(CMakeDependentOption)
option(USE_VTK "Use VTK for visualization" ON)
option(USE_GD "Use libGD for saving screenshots" ON)
option(USE_TBB "Use TBB for multithreading" ON)

find_package(Boost REQUIRED filesystem iostreams)
find_package(wxWidgets REQUIRED core base gl aui adv)
include(${wxWidgets_USE_FILE})
find_package(CGAL REQUIRED Core)
find_package(bliss REQUIRED)
find_package(shape REQUIRED)

set(SOURCE_FILES window.cpp singlefile.cpp inputform.cpp outputview.cpp batchfile.cpp parametersview.cpp filesview.cpp model/batch.cpp model/execute.cpp model/ratios.cpp)

if(USE_VTK)
  find_package(VTK COMPONENTS vtkRenderingOpenGL2 vtkRenderingAnnotation vtkInteractionStyle vtkInfovisLayout vtkViewsInfovis)
  include(${VTK_USE_FILE})
  list(APPEND SOURCE_FILES meshview.cpp graphview.cpp wxVTKWidget.cpp)
endif(USE_VTK)
if(USE_GD)
  find_package(GD)
  include_directories(${GD_INCLUDE_DIRS})
endif(USE_GD)
if(USE_TBB)
  find_package(TBB)
endif(USE_TBB)

configure_file(dependencies.hpp.in dependencies.hpp)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(tr WIN32 ${SOURCE_FILES})
target_compile_definitions(tr PRIVATE WXUSINGDLL)
target_compile_definitions(tr PRIVATE NOMINMAX)
target_compile_definitions(tr PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_definitions(tr PRIVATE HAVE_LIBPNG)

target_link_libraries(tr PRIVATE Boost::filesystem Boost::iostreams ${VTK_LIBRARIES} ${wxWidgets_LIBRARIES} CGAL::CGAL CGAL::CGAL_Core ${GD_LIBRARIES} tbb shape)

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

