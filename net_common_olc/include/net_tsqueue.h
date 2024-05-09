#ifndef NET_TSQUEUE_H
#define NET_TSQUEUE_H
#include "net_common.h"

namespace olc {
namespace net {
template <typename T> class tsqueue {
public:
  tsqueue() = default;
  tsqueue(const tsqueue<T> &) = delete;
  ~tsqueue() { clear(); };

public:
  const T &front() const {
    std::scoped_lock lk{muxQueue};
    return deQueue.front();
  }
  const T &back() const {
    std::scoped_lock lk{muxQueue};
    return deQueue.back();
  }

  void push_back(const T &item) {
    std::scoped_lock lk{muxQueue};
    deQueue.emplace_back(std::move(item));
    mCv.notify_one();
  }
  void push_front(const T &item) {
    std::scoped_lock lk{muxQueue};
    deQueue.emplace_front(std::move(item));
    mCv.notify_one();
  }

  bool empty() const {
    std::scoped_lock lk{muxQueue};
    return deQueue.empty();
  }

  std::size_t count() const {
    std::scoped_lock lk{muxQueue};
    return deQueue.size();
  }

  void clear() {
    std::scoped_lock lk{muxQueue};
    deQueue.clear();
  }

  T pop_front() {
    std::scoped_lock lk{muxQueue};
    T f{std::move(deQueue.front())};
    deQueue.pop_front();

    return f;
  }
  T wait_pop_front() {
    std::unique_lock lk{muxQueue};
    mCv.wait(lk, [&] { return !deQueue.empty(); });
    T f{std::move(deQueue.front())};
    deQueue.pop_front();

    return f;
  }

  template <typename timeFor>
  std::optional<T> wait_pop_front(const timeFor &tFor) {
    std::unique_lock lk{muxQueue};
    mCv.wait_for(lk, tFor, [&] { return !deQueue.empty(); });

    if (!deQueue.empty()) {
      T f{std::move(deQueue.front())};
      deQueue.pop_front();

      return f;
    } else
      return {};
  }

  T pop_back() {
    std::scoped_lock lk{muxQueue};
    T f{std::move(deQueue.back())};
    deQueue.pop_back();

    return f;
  }

  T wait_pop_back() {
    std::unique_lock lk{muxQueue};
    mCv.wait(lk, [&] { return !deQueue.empty(); });
    T f{std::move(deQueue.back())};
    deQueue.pop_back();

    return f;
  }

  template <typename timeFor>
  std::optional<T> wait_pop_back(const timeFor &tFor) {
    std::unique_lock lk{muxQueue};
    mCv.wait_for(lk, tFor, [&] { return !deQueue.empty(); });

    if (!deQueue.empty()) {
      T f{std::move(deQueue.back())};
      deQueue.pop_back();

      return f;
    } else
      return {};
  }

protected:
  std::condition_variable mCv{};
  mutable std::mutex muxQueue{};
  std::deque<T> deQueue{};
};
} // namespace net
} // namespace olc

#endif
