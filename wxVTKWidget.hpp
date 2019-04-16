#ifndef WXVTKWIDGET_HPP
#define WXVTKWIDGET_HPP

#include <unordered_map>
#include <memory>

#include <dependencies.hpp>

#include <vtkGenericRenderWindowInteractor.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkCallbackCommand.h>

#ifdef GD_FOUND
#  include <gdpp.h>
#endif //GD_FOUND

#include <wx/timer.h>
#include <wx/glcanvas.h>

#undef TrueColor

class wxVTKWidget : public wxGLCanvas {
  wxGLContext *m_context;
  vtkSmartPointer<vtkGenericRenderWindowInteractor> m_interactor;
public:
  template<typename... Args>
  explicit wxVTKWidget(Args&&... args) :
    wxGLCanvas(std::forward<Args>(args)...) {
	wxGLContextAttrs attrs;
    attrs.PlatformDefaults().CoreProfile().OGLVersion(3, 2).EndList();
	m_context = new wxGLContext(this, nullptr, &attrs);
	
    m_create_timer_callback->SetClientData(this);
    m_create_timer_callback->SetCallback(&wxVTKWidget::CreateTimer);
    m_destroy_timer_callback->SetClientData(this);
    m_destroy_timer_callback->SetCallback(&wxVTKWidget::DestroyTimer);
    m_render_callback->SetClientData(this);
    m_render_callback->SetCallback(&wxVTKWidget::RenderCallback);
    m_make_current_callback->SetClientData(this);
    m_make_current_callback->SetCallback(&wxVTKWidget::MakeCurrentCallback);
    m_swap_buffers_callback->SetClientData(this);
    m_swap_buffers_callback->SetCallback(&wxVTKWidget::SwapBuffersCallback);
  }
  void SetRenderWindowInteractor(vtkGenericRenderWindowInteractor *interactor);
  void ResetCamera();
#ifdef GD_FOUND
  GD::Image Screenshot();
#endif //GD_FOUND

private:
  // Window events
  void Resize(wxSizeEvent &event);

  // Rendering events
  bool m_init_window = false;
  static void RenderCallback(vtkObject* source, unsigned long eid, void* clientData, void* callData);
  static void MakeCurrentCallback(vtkObject* source, unsigned long eid, void* clientData, void* callData);
  static void SwapBuffersCallback(vtkObject* source, unsigned long eid, void* clientData, void* callData);
  void Render(wxPaintEvent &event);

  int m_rotation = 0;
  inline void SetButtonInformation(const wxMouseEvent &event);
  void LeftDown(wxMouseEvent &event);
  void LeftUp(wxMouseEvent &event);
  void MiddleDown(wxMouseEvent &event);
  void MiddleUp(wxMouseEvent &event);
  void RightDown(wxMouseEvent &event);
  void RightUp(wxMouseEvent &event);
  void Motion(wxMouseEvent &event);
  void Enter(wxMouseEvent &event);
  void Leave(wxMouseEvent &event);
  void Wheel(wxMouseEvent &event);
  
  class CustomTimer final : public wxTimer {
    wxVTKWidget *m_target;
    int m_platformId;
  public:
    CustomTimer(wxVTKWidget *target, int platformId) :
      m_target(target), m_platformId(platformId) {
    }
    void Notify() final {
      m_target->Timer(m_platformId);
    }
  };
  std::unordered_map<int, std::unique_ptr<CustomTimer> > m_timers;
  int m_next_timer = 1;
  static void CreateTimer(vtkObject* source, unsigned long eid, void* clientData, void* callData);
  static void DestroyTimer(vtkObject* source, unsigned long eid, void* clientData, void* callData);
  void Timer(int platformId);
  
  // Callback commands
  vtkNew<vtkCallbackCommand> m_create_timer_callback;
  vtkNew<vtkCallbackCommand> m_destroy_timer_callback;
  vtkNew<vtkCallbackCommand> m_render_callback;
  vtkNew<vtkCallbackCommand> m_make_current_callback;
  vtkNew<vtkCallbackCommand> m_swap_buffers_callback;

  DECLARE_EVENT_TABLE()
};

#endif // WXVTKWIDGET_HPP
