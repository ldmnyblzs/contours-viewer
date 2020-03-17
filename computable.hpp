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

#ifndef COMPUTABLE_HPP
#define COMPUTABLE_HPP

#include <functional>

class Computable {
  bool m_running = true;
  std::function<void(void)> m_running_changed = []{};
protected:
  void SetRunning(bool running = true) {
	m_running = running;
	m_running_changed();
  }
public:
  virtual void Compute() = 0;
  virtual void Cancel() = 0;
  virtual void Save() = 0;
  bool Running() const {
	return m_running;
  }
  void SetRunningChanged(std::function<void(void)> running_changed) {
	m_running_changed = running_changed;
  }
};

#endif // COMPUTABLE_HPP
