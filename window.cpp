#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/app.h>
#include <wx/window.h>
#include <wx/menu.h>
#include <wx/toolbar.h>
#include <wx/filedlg.h>
#endif

#include <wx/aui/aui.h>
#include <wx/artprov.h>
#include <wx/stdpaths.h>
#include <wx/wupdlock.h>

#include "singlefile.hpp"
#include "batchfile.hpp"

static const wxWindowID NotebookID = wxID_HIGHEST + 1;

class Application final : public wxApp {
  bool OnInit() final;
};

class MainWindow final : public wxFrame {
	wxAuiNotebook *tabBar;
	void Initialize();
	void OnOpen(wxCommandEvent &event);
	void OnSave(wxCommandEvent &event);
	void OnCompute(wxCommandEvent &event);
	void OnCancel(wxCommandEvent &event);
	void OnRunningChanged();
	void OnTabChanged(wxAuiNotebookEvent &event);
public:
	template <typename... Args>
	explicit MainWindow(Args&&... args) :
	wxFrame(std::forward<Args>(args)...) {
		Initialize();
	}
};

wxIMPLEMENT_APP(Application);

bool Application::OnInit() {
  auto window = new MainWindow(nullptr, wxID_ANY, "Shape descriptors");
  window->SetEventHandler(window);
  window->Show(true);
  return true;
}

void MainWindow::Initialize() {
	auto fileMenu = new wxMenu;
	fileMenu->Append(wxID_OPEN, "Open");
	fileMenu->Append(wxID_SAVE, "Save");
	fileMenu->AppendSeparator();
	fileMenu->Append(wxID_EXIT, "Exit");
	
	auto toolsMenu = new wxMenu;
	toolsMenu->Append(wxID_EXECUTE, "Compute");
	toolsMenu->Append(wxID_CANCEL, "Cancel");
	
	auto menuBar = new wxMenuBar;
	menuBar->Append(fileMenu, "File");
	menuBar->Append(toolsMenu, "Tools");
	
	SetMenuBar(menuBar);
	
	auto toolbar = CreateToolBar(wxTB_DEFAULT_STYLE | wxTB_TEXT);
	toolbar->AddTool(wxID_OPEN, "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR));
	toolbar->AddTool(wxID_SAVE, "Save", wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR));
	toolbar->AddSeparator();
	toolbar->AddTool(wxID_EXECUTE, "Compute", wxArtProvider::GetBitmap(wxART_EXECUTABLE_FILE, wxART_TOOLBAR));
	toolbar->AddTool(wxID_CANCEL, "Cancel", wxArtProvider::GetBitmap(wxART_DELETE, wxART_TOOLBAR));
	toolbar->Realize();
	
	Bind(wxEVT_MENU, &MainWindow::OnOpen, this, wxID_OPEN);
	Bind(wxEVT_MENU, &MainWindow::OnSave, this, wxID_SAVE);
	Bind(wxEVT_MENU, &MainWindow::OnCompute, this, wxID_EXECUTE);
	Bind(wxEVT_MENU, &MainWindow::OnCancel, this, wxID_CANCEL);
	
	tabBar = new wxAuiNotebook(this, NotebookID);
	Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &MainWindow::OnTabChanged, this, NotebookID);
}

void MainWindow::OnOpen(wxCommandEvent & WXUNUSED(event)) {
  wxFileDialog dialog(this,
		      wxFileSelectorPromptStr,
		      wxStandardPaths::Get().GetUserDir(wxStandardPaths::Dir_Documents),
		      "",
		      "Pebble files (*.stl;*.off)|*.stl;*.off|Batch files (*.csv)|*.csv",
		      wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
  if (dialog.ShowModal() == wxID_CANCEL)
    return;

  wxArrayString paths, filenames;
  dialog.GetPaths(paths);
  dialog.GetFilenames(filenames);
  wxWindowUpdateLocker lock(tabBar);
  switch(dialog.GetFilterIndex()) {
  case 0:
    for (std::size_t i = 0; i < paths.GetCount(); ++i) {
	  auto tab = new SingleFile(paths[i], tabBar, wxID_ANY);
	  tab->SetRunningChanged(std::bind(&MainWindow::OnRunningChanged, this));
      tabBar->AddPage(tab, filenames[i], true);
	}
    break;
  default:
    for (std::size_t i = 0; i < paths.GetCount(); ++i) {
	  auto tab = new BatchFile(paths[i], tabBar, wxID_ANY);
	  tab->SetRunningChanged(std::bind(&MainWindow::OnRunningChanged, this));
      tabBar->AddPage(tab, filenames[i], true);
	}
  }
  GetToolBar()->EnableTool(wxID_EXECUTE, false);
  GetMenuBar()->Enable(wxID_EXECUTE, false);
}

void MainWindow::OnSave(wxCommandEvent & WXUNUSED(event)) {
  auto file = dynamic_cast<Computable *>(tabBar->GetCurrentPage());
  if (file)
    file->Save();
}

void MainWindow::OnCompute(wxCommandEvent & WXUNUSED(event)) {
  auto file = dynamic_cast<Computable *>(tabBar->GetCurrentPage());
  if (file)
    file->Compute();
}

void MainWindow::OnCancel(wxCommandEvent & WXUNUSED(event)) {
  auto file = dynamic_cast<Computable *>(tabBar->GetCurrentPage());
  if (file)
    file->Cancel();
}

void MainWindow::OnRunningChanged() {
  auto tab = dynamic_cast<Computable *>(tabBar->GetCurrentPage());
  if (tab) {
	  GetToolBar()->EnableTool(wxID_EXECUTE, !tab->Running());
	  GetToolBar()->EnableTool(wxID_CANCEL, tab->Running());
	  GetMenuBar()->Enable(wxID_EXECUTE, !tab->Running());
	  GetMenuBar()->Enable(wxID_CANCEL, tab->Running());
  }
}

void MainWindow::OnTabChanged(wxAuiNotebookEvent &event) {
  auto tab = dynamic_cast<Computable *>(tabBar->GetCurrentPage());
  GetToolBar()->EnableTool(wxID_EXECUTE, !tab->Running());
  GetToolBar()->EnableTool(wxID_CANCEL, tab->Running());
  GetMenuBar()->Enable(wxID_EXECUTE, !tab->Running());
  GetMenuBar()->Enable(wxID_CANCEL, tab->Running());
  event.Skip();
}
