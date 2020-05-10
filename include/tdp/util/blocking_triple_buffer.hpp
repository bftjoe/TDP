#ifndef TDP_BLOCKING_TRIPLE_BUFFER_HPP
#define TDP_BLOCKING_TRIPLE_BUFFER_HPP

#include <condition_variable>
#include <mutex>
#include <optional>

namespace tdp::util {

template <typename T>
class blocking_triple_buffer {
 public:
  void push(T val) {
    {
      std::unique_lock lock{_mutex};
      _buffer[_in] = std::move(val);
      std::swap(_in, _buf);
      available = true;
    }
    _condition.notify_one();
  }

  T pop() {
    {
      std::unique_lock lock{_mutex};
      _condition.wait(lock, [&] { return available; });
      std::swap(_out, _buf);
      available = false;
    }
    return std::move(_buffer[_out]);
  }

  template <typename Pred>
  std::optional<T> pop_unless(Pred&& p) {
    {
      std::unique_lock lock{_mutex};
      _condition.wait(lock, [&] { return p() || available; });

      if (!available)
        return std::nullopt;

      std::swap(_out, _buf);
      available = false;
    }

    return std::move(_buffer[_out]);
  }

  bool empty() const noexcept { return !available; }

  void wake() { _condition.notify_all(); }

 private:
  std::array<T, 3> _buffer;  // TODO: aligned_storage_t to prevent default construction?
  bool available = false;
  std::size_t _in = 0;
  std::size_t _buf = 1;
  std::size_t _out = 2;

  std::mutex _mutex;
  std::condition_variable _condition;
};

}  // namespace tdp::util

#endif