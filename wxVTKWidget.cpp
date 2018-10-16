#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/dcclient.h>
#endif

#include "wxVTKWidget.hpp"

#include <iostream>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkWindowToImageFilter.h>
#include <vtkImageShiftScale.h>
#include <vtkImageExport.h>

BEGIN_EVENT_TABLE(wxVTKWidget, wxGLCanvas)
EVT_SIZE(wxVTKWidget::Resize)
EVT_PAINT(wxVTKWidget::Render)
EVT_LEFT_DOWN(wxVTKWidget::LeftDown)
EVT_LEFT_DCLICK(wxVTKWidget::LeftDown)
EVT_LEFT_UP(wxVTKWidget::LeftUp)
EVT_MIDDLE_DOWN(wxVTKWidget::MiddleDown)
EVT_MIDDLE_DCLICK(wxVTKWidget::MiddleDown)
EVT_MIDDLE_UP(wxVTKWidget::MiddleUp)
EVT_RIGHT_DOWN(wxVTKWidget::RightDown)
EVT_RIGHT_DCLICK(wxVTKWidget::RightDown)
EVT_RIGHT_UP(wxVTKWidget::RightUp)
EVT_MOTION(wxVTKWidget::Motion)
EVT_ENTER_WINDOW(wxVTKWidget::Enter)
EVT_LEAVE_WINDOW(wxVTKWidget::Leave)
EVT_MOUSEWHEEL(wxVTKWidget::Wheel)
END_EVENT_TABLE()

void wxVTKWidget::SetRenderWindowInteractor(vtkGenericRenderWindowInteractor *interactor) {
  if (m_interactor == interactor)
    return;
  
  if (m_interactor) {
    m_interactor->RemoveObserver(m_create_timer_callback.GetPointer());
    m_interactor->RemoveObserver(m_destroy_timer_callback.GetPointer());
    m_interactor->RemoveObserver(m_render_callback.GetPointer());
    if (auto window = m_interactor->GetRenderWindow()) {
      window->RemoveObserver(m_make_current_callback.GetPointer());
      window->RemoveObserver(m_swap_buffers_callback.GetPointer());
    }
  }
  
  m_interactor = interactor;
  
  if (m_interactor) {
    m_init_window = true;
    m_interactor->EnableRenderOff();
    
    m_interactor->AddObserver(vtkCommand::CreateTimerEvent, m_create_timer_callback.GetPointer());
    m_interactor->AddObserver(vtkCommand::DestroyTimerEvent, m_destroy_timer_callback.GetPointer());
    m_interactor->AddObserver(vtkCommand::RenderEvent, m_render_callback.GetPointer());
    
    if (auto window = m_interactor->GetRenderWindow()) {
      window->AddObserver(vtkCommand::WindowMakeCurrentEvent, m_make_current_callback.GetPointer());
      window->AddObserver(vtkCommand::WindowFrameEvent, m_swap_buffers_callback.GetPointer());
    }
  }
}

void wxVTKWidget::ResetCamera() {
	if (auto window = m_interactor->GetRenderWindow()) {
		auto renderers = window->GetRenderers();
		auto renderer = renderers->GetFirstRenderer();
		while (renderer) {
			renderer->ResetCamera();
			renderer = renderers->GetNextItem();
		}
	}
	Refresh();
}

GD::Image wxVTKWidget::Screenshot() {
  vtkNew<vtkWindowToImageFilter> image_filter;
  image_filter->SetInput(m_interactor->GetRenderWindow());

  vtkNew<vtkImageShiftScale> to_uchar;
  to_uchar->SetInputConnection(image_filter->GetOutputPort());
  to_uchar->SetOutputScalarTypeToUnsignedChar();

  vtkNew<vtkImageExport> exporter;
  exporter->SetInputConnection(to_uchar->GetOutputPort());
  exporter->ImageLowerLeftOff();
  
  const auto dimensions = exporter->GetDataDimensions();
  std::vector<unsigned char> data(dimensions[0] * dimensions[1] * 3);

  exporter->Export(data.data());
  const auto components = exporter->GetDataNumberOfScalarComponents();

  GD::Image image(dimensions[0], dimensions[1], true);

  std::size_t index = 0;
  for (int y = 0; y < dimensions[1]; ++y)
    for (int x = 0; x < dimensions[0]; ++x) {
      image.SetPixel(x, y, GD::TrueColor(data[index], data[index + 1], data[index + 2]));
      index += components;
    }

  return gdImageCropAuto(image.GetPtr(), GD_CROP_WHITE);
}

void wxVTKWidget::Resize(wxSizeEvent &event) {
  if (m_interactor) {
    auto const size = event.GetSize();
    m_interactor->UpdateSize(size.GetWidth(), size.GetHeight());
  }
  //event.Skip();
}

void wxVTKWidget::RenderCallback(vtkObject* vtkNotUsed(source),
			      unsigned long vtkNotUsed(eid),
			      void* clientData,
			      void* vtkNotUsed(callData)) {
  auto self = reinterpret_cast<wxVTKWidget *>(clientData);
  self->Refresh();
}

void wxVTKWidget::MakeCurrentCallback(vtkObject* vtkNotUsed(source),
				      unsigned long vtkNotUsed(eid),
				      void* clientData,
				      void* vtkNotUsed(callData)) {
  auto self = reinterpret_cast<wxVTKWidget *>(clientData);
  self->m_context->SetCurrent(*self);
  auto window = reinterpret_cast<vtkGenericOpenGLRenderWindow*>(self->m_interactor->GetRenderWindow());
  if (self->m_init_window && window) {
    if (!window->InitializeFromCurrentContext())
      window->OpenGLInit();
    self->m_init_window = false;
  }
}

void wxVTKWidget::SwapBuffersCallback(vtkObject* vtkNotUsed(source),
				      unsigned long vtkNotUsed(eid),
				      void* clientData,
				      void* vtkNotUsed(callData)) {
  auto self = reinterpret_cast<wxVTKWidget *>(clientData);
  self->SwapBuffers();
}

void wxVTKWidget::Render(wxPaintEvent &WXUNUSED(event)) {
  if (!IsShown())
    return;
  
  if (m_interactor) {
    wxPaintDC(this);
    auto window = m_interactor->GetRenderWindow();
    window->Render();
  }
}

void wxVTKWidget::SetButtonInformation(const wxMouseEvent &event) {
  m_interactor->SetEventInformationFlipY(event.GetX(),
					 event.GetY(),
					 event.ControlDown() ? 1 : 0,
					 event.ShiftDown() ? 1 : 0,
					 0,
					 event.GetClickCount());
  m_interactor->SetAltKey(event.AltDown());  
}

void wxVTKWidget::LeftDown(wxMouseEvent &event) {
  if (!m_interactor)
    return;
  SetButtonInformation(event);
  m_interactor->LeftButtonPressEvent();
  event.Skip();
}

void wxVTKWidget::LeftUp(wxMouseEvent &event) {  
  if (!m_interactor)
    return;
  SetButtonInformation(event);
  m_interactor->LeftButtonReleaseEvent();
  event.Skip();
}

void wxVTKWidget::MiddleDown(wxMouseEvent &event) {
  if (!m_interactor)
    return;
  SetButtonInformation(event);
  m_interactor->MiddleButtonPressEvent();
  event.Skip();
}

void wxVTKWidget::MiddleUp(wxMouseEvent &event) {  
  if (!m_interactor)
    return;
  SetButtonInformation(event);
  m_interactor->MiddleButtonReleaseEvent();
  event.Skip();
}

void wxVTKWidget::RightDown(wxMouseEvent &event) {
  if (!m_interactor)
    return;
  SetButtonInformation(event);
  m_interactor->RightButtonPressEvent();
  event.Skip();
}

void wxVTKWidget::RightUp(wxMouseEvent &event) {  
  if (!m_interactor)
    return;
  SetButtonInformation(event);
  m_interactor->RightButtonReleaseEvent();
  event.Skip();
}

void wxVTKWidget::Motion(wxMouseEvent &event) {
  if (!m_interactor)
    return;
  m_interactor->SetEventInformationFlipY(event.GetX(),
					 event.GetY(),
					 event.ControlDown() ? 1 : 0,
					 event.ShiftDown() ? 1 : 0,
					 0,
					 0);
  m_interactor->SetAltKey(event.AltDown());
  m_interactor->MouseMoveEvent();
  event.Skip();
}

void wxVTKWidget::Enter(wxMouseEvent &event) {
  if (!m_interactor)
    return;
  m_interactor->SetEventInformationFlipY(event.GetX(),
					 event.GetY(),
					 event.ControlDown() ? 1 : 0,
					 event.ShiftDown() ? 1 : 0,
					 0,
					 0);
  m_interactor->SetAltKey(event.AltDown());
  m_interactor->EnterEvent();
  event.Skip();
}

void wxVTKWidget::Leave(wxMouseEvent &event) {
  if (!m_interactor)
    return;
  m_interactor->SetEventInformationFlipY(event.GetX(),
					 event.GetY(),
					 event.ControlDown() ? 1 : 0,
					 event.ShiftDown() ? 1 : 0,
					 0,
					 0);
  m_interactor->SetAltKey(event.AltDown());
  m_interactor->LeaveEvent();
  event.Skip();
}

void wxVTKWidget::Wheel(wxMouseEvent &event) {
  if (m_interactor) {
    const auto delta = event.GetWheelDelta();
    const auto neg_delta = -1 * delta;
    m_rotation += event.GetWheelRotation();
    if (m_rotation >= delta) {
      m_interactor->SetEventInformationFlipY(event.GetX(),
					     event.GetY(),
					     event.ControlDown() ? 1 : 0,
					     event.ShiftDown() ? 1 : 0,
					     0,
					     0);
      m_interactor->SetAltKey(event.AltDown());
      m_interactor->MouseWheelForwardEvent();
      m_rotation %= delta;
    } else if (m_rotation <= neg_delta) {
      m_interactor->SetEventInformationFlipY(event.GetX(),
					     event.GetY(),
					     event.ControlDown() ? 1 : 0,
					     event.ShiftDown() ? 1 : 0,
					     0,
					     0);
      m_interactor->SetAltKey(event.AltDown());
      m_interactor->MouseWheelBackwardEvent();
      m_rotation %= neg_delta;
    }
  }
  event.Skip();
}

void wxVTKWidget::Timer(int platformId) {
  m_interactor->InvokeEvent(vtkCommand::TimerEvent, (void*)&platformId);
}

void wxVTKWidget::CreateTimer(vtkObject* vtkNotUsed(source),
			      unsigned long vtkNotUsed(eid),
			      void* clientData,
			      void* vtkNotUsed(callData)) {
  auto self = reinterpret_cast<wxVTKWidget *>(clientData);
  
  self->m_interactor->SetTimerEventPlatformId(self->m_next_timer);
  auto timer = self->m_timers.emplace(self->m_next_timer,
				      std::make_unique<CustomTimer>(self, self->m_next_timer));
  switch (self->m_interactor->GetTimerEventType()) {
  case vtkRenderWindowInteractor::RepeatingTimer:
    timer.first->second->Start(self->m_interactor->GetTimerEventDuration());
    break;
  default:
    timer.first->second->StartOnce(self->m_interactor->GetTimerEventDuration());
  }
  ++self->m_next_timer;
}

void wxVTKWidget::DestroyTimer(vtkObject* vtkNotUsed(source),
			       unsigned long vtkNotUsed(eid),
			       void* clientData,
			       void* callData) {
  auto self = reinterpret_cast<wxVTKWidget *>(clientData);
  self->m_timers.erase(*reinterpret_cast<int*>(callData));
}
