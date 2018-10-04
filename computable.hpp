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
