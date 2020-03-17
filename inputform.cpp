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

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#endif

#include <fstream>
#include <wx/spinctrl.h>

#include "inputform.hpp"

void InputForm::Initialize() {
  m_count = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10000, 100);
  m_amin = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0.0, 100.0, 1.0, 0.1);
  for (auto &&offset : m_offset)
    offset = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10000.0, 10000.0, 0.0, 1.0);
  
  auto grid = new wxFlexGridSizer(2);

  const auto flags = wxSizerFlags().Expand().Center().Border(wxALL, 5);
  grid->Add(new wxStaticText(this, wxID_ANY, "Number of lines"), flags);
  grid->Add(m_count, flags);
  grid->Add(new wxStaticText(this, wxID_ANY, "Minimum area (%)"), flags);
  grid->Add(m_amin, flags);
  grid->Add(new wxStaticText(this, wxID_ANY, "X offset"), flags);
  grid->Add(m_offset[0], flags);
  grid->Add(new wxStaticText(this, wxID_ANY, "Y offset"), flags);
  grid->Add(m_offset[1], flags);
  grid->Add(new wxStaticText(this, wxID_ANY, "Z offset"), flags);
  grid->Add(m_offset[2], flags);
  SetSizerAndFit(grid);
}

Parameters InputForm::GetParameters() const noexcept {
  Parameters parameters;
  parameters.resize(1);
  parameters[0].next.resize(1);
  parameters[0].next[0].next.resize(1);
  parameters[0].next[0].next[0].next.resize(1);
  parameters[0].value.offset = Vector(m_offset[0]->GetValue(), m_offset[1]->GetValue(), m_offset[2]->GetValue());
  parameters[0].next[0].value = m_count->GetValue();
  parameters[0].next[0].next[0].value = m_amin->GetValue() / 100.0;
  parameters[0].next[0].next[0].next[0] = FIRST;
  return parameters;
}
