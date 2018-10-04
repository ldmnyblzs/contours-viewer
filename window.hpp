#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <wx/frame.h>

class wxAuiNotebook;
class wxAuiNotebookEvent;

class Window final : public wxFrame {
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
	explicit Window(Args&&... args) :
	wxFrame(std::forward<Args>(args)...) {
		Initialize();
	}
};

#endif // WINDOW_HPP
