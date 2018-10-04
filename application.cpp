#include "application.hpp"
#include "window.hpp"

bool Application::OnInit() {
  auto window = new Window(nullptr, wxID_ANY, "VTK window");
  window->SetEventHandler(window);
  window->Show(true);
  return true;
}

IMPLEMENT_APP(Application)
