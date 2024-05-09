#ifndef NEW_MESSAGE_H
#define NEW_MESSAGE_H

#include "net_common.h"

namespace olc {
namespace net {
//
template <typename T> struct message_header {
  T id{};
  std::uint32_t size{0};
};

template <typename T> struct message {
  using body_type = std::string;
  message_header<T> header{};
  body_type body{};

  std::size_t size() const { return (sizeof header + body.size()); };

  friend std::ostream &operator<<(std::ostream &out, const message<T> msg) {
    out << "ID: " << static_cast<int>(msg.header.id)
        << " Size: " << msg.header.size << '\n';
    return out;
  }

  template <typename DataType> message<T> &operator<<(const DataType &data) {
    static_assert(std::is_standard_layout<DataType>::value,
                  "Data is to complicated");
    std::size_t sizeBefore{body.size()};
    body.resize(body.size() + sizeof data);
    std::memcpy(body.data() + sizeBefore, &data, sizeof data);
    header.size = static_cast<std::uint32_t>(body.size());

    return *this;
  }

  message<T> &operator<<(const std::string &data) {
    body += data;
    header.size = static_cast<std::uint32_t>(body.size());

    return *this;
  }

  template <typename DataType>
  friend message<T> &operator>>(message<T> &msg, DataType &data) {
    static_assert(std::is_standard_layout<DataType>::value,
                  "Data is to complicated");
    std::size_t newSize{msg.body.size() - sizeof data};
    std::memcpy(&data, msg.body.data() + newSize, sizeof data);
    msg.body.resize(newSize);
    msg.header.size = static_cast<std::uint32_t>(msg.body.size());
    return msg;
  }
};

template <typename T> class Connection;

template <typename T> struct owned_message {
  std::shared_ptr<Connection<T>> remote{nullptr};
  message<T> msg;
  friend std::ostream &operator<<(std::ostream &out,
                                  const owned_message<T> &message) {
    out << message.msg;
    return out;
  }
  //
};

} // namespace net
} // namespace olc

#endif
