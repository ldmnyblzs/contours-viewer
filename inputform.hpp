#ifndef INPUT_FORM_HPP
#define INPUT_FORM_HPP

#include <wx/panel.h>
#include <array>

#include "parameters.hpp"

class wxSpinCtrl;
class wxSpinCtrlDouble;

class InputForm final : public wxPanel {
	wxSpinCtrl *m_count;
	wxSpinCtrlDouble *m_amin;
	std::array<wxSpinCtrlDouble *, 3> m_offset;
	void Initialize();
public:
	template <typename... Args>
	explicit InputForm(Args&&... args) :
	wxPanel(std::forward<Args>(args)...) {
		Initialize();
	}
	
	Parameters GetParameters() const noexcept;
};

#endif // INPUT_FORM_HPP
