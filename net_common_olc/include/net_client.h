#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include "net_common.h"
#include "net_connection.h"
#include "net_message.h"
#include "net_tsqueue.h"

namespace olc {
namespace net {
template <typename T> class ClientInterface {
protected:
  ClientInterface() = default;
  virtual ~ClientInterface() { disConnect(); }

public:
  bool connect(const std::string &host, const uint16_t port) {
    try {
      boost::asio::ip::tcp::resolver resolver{mAsioContext};
      boost::asio::ip::tcp::resolver::results_type endPoints{
          resolver.resolve(host, std::to_string(port))};

      mConnection = std::make_unique<Connection<T>>(
          Connection<T>::Owner::client, mAsioContext,
          boost::asio::ip::tcp::socket(mAsioContext), mQMessageIn);

      mConnection->connectToServer(endPoints);

      thrContext = std::thread([this]() { mAsioContext.run(); });

    } catch (std::exception &e) {
      std::cerr << "Client Exception: " << e.what() << '\n';
      return false;
    }
    return true;
  }
  void disConnect() {
    if (isConnected())
      mConnection->disConnect();
    mAsioContext.stop();
    if (thrContext.joinable())
      thrContext.join();
    mConnection.release();
  }

  bool isConnected() {
    if (mConnection)
      return mConnection->isConnected();
    else
      return false;
  }

  void send(const message<T> &msg) { mConnection->send(msg); }

  tsqueue<owned_message<T>> &inComming() { return mQMessageIn; }

protected:
  boost::asio::io_context mAsioContext{};
  std::thread thrContext{};

  std::unique_ptr<Connection<T>> mConnection{};

private:
  tsqueue<owned_message<T>> mQMessageIn{};
};
//
} // namespace net
} // namespace olc

#endif
