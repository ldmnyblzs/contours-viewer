/*
  Copyright 2019 Balázs Ludmány

  This file is part of contours-viewer.

  contours-viewer is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  contours-viewer is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with contours-viewer.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef INPUT_FORM_HPP
#define INPUT_FORM_HPP

#include <array>

#include "model/parameters.hpp"

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
