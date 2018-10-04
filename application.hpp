#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <wx/app.h>

class Application final : public wxApp {
  bool OnInit() final;
};

#endif // APPLICATION_HPP
