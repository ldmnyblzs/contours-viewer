#ifndef MESHVIEW_HPP
#define MESHVIEW_HPP

#include <array>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkAxesActor.h>

#include "wxVTKWidget.hpp"

class Mesh;
class LevelGraph;

class MeshView final : public wxVTKWidget {
	vtkNew<vtkPolyDataMapper> m_meshMapper;
	std::array<vtkNew<vtkPolyDataMapper>, 3> m_contourMappers;
	vtkNew<vtkAxesActor> m_axesActor;
	void Initialize();
public:
	template <typename... Args>
	explicit MeshView(Args&&... args) :
	  wxVTKWidget(std::forward<Args>(args)...) {
	  Initialize();
	}
	void UpdateMesh(const Mesh &mesh);
	void UpdateTexture(const LevelGraph &graph);
};

#endif // MESHVIEW_HPP
