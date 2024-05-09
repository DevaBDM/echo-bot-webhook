#ifndef NET_SERVER_H
#define NET_SERVER_H
#include "net_common.h"
#include "net_connection.h"
#include "net_message.h"
#include "net_tsqueue.h"

namespace olc {
namespace net {
template <typename T> class serverInterface {
public:
  serverInterface(std::uint16_t port)
      : mAsioAcceptor{mAsioIoconstext, boost::asio::ip::tcp::endpoint{
                                           boost::asio::ip::tcp::v4(), port}} {}
  virtual ~serverInterface() { stop(); }
  bool start() {
    try {
      waitForClientConnection();
      mThreadContext = std::thread{[this]() { mAsioIoconstext.run(); }};
    } catch (std::exception &e) {
      std::cerr << "[SERVER] Exception: " << e.what() << '\n';
      return false;
    }

    std::cout << "[SERVER] Started!\n";
    return true;
  }
  void stop() {
    mAsioIoconstext.stop();
    if (mThreadContext.joinable())
      mThreadContext.join();
    std::cerr << "[SERVER] Stopped!\n";
  }
  void waitForClientConnection() {
    mAsioAcceptor.async_accept([this](const std::error_code &ec,
                                      boost::asio::ip::tcp::socket sock) {
      if (!ec) {
        std::cerr << "[SERVER] New connection: " << sock.remote_endpoint()
                  << '\n';
        std::shared_ptr<Connection<T>> newConn{std::make_shared<Connection<T>>(
            Connection<T>::Owner::server, mAsioIoconstext, std::move(sock),
            mQmessageIn)};

        if (onClientConnect(newConn)) {
          mDeqConnections.push_back(std::move(newConn));
          mDeqConnections.back()->connectToClient(this, mIDCounter++);
          std::cerr << '[' << mDeqConnections.back()->getID()
                    << "] connection approved\n";

        } else {
          std::cerr << "[--------] connection Denied\n";
        }

      } else {
        std::cerr << "[SERVER] New connection error: " << ec.message() << '\n';
      }
      waitForClientConnection();
    });
  }
  bool messageClient(std::shared_ptr<Connection<T>> client,
                     const message<T> &msg) {
    if (client && client->isConnected()) {
      client->send(msg);
      return true;
    } else {
      onClientDisconnect(client);
      mDeqConnections.erase(
          std::remove(mDeqConnections.begin(), mDeqConnections.end(), client),
          mDeqConnections.end());
      return false;
    }
  }
  void
  messageAllClients(const message<T> &msg,
                    std::shared_ptr<Connection<T>> pIgnoreClient = nullptr) {
    bool bInvalidClientExists{false};
    for (auto &client : mDeqConnections) {
      if (client && client->isConnected()) {
        if (client != pIgnoreClient)
          client->send(msg);
      } else {
        onClientDisconnect(client);
        client.reset();
        bInvalidClientExists = true;
      }
    }

    if (bInvalidClientExists) {
      mDeqConnections.erase(
          std::remove(mDeqConnections.begin(), mDeqConnections.end(), nullptr),
          mDeqConnections.end());
    }
  }
  void update(std::size_t mMaxMessages = static_cast<std::size_t>(-1)) {
    std::size_t mMessageCount{0};
    while (mMessageCount < mMaxMessages) {

      auto msg = mQmessageIn.wait_pop_front();
      onMessage(msg.remote, msg.msg);

      mMessageCount++;
    }
  }

protected:
  virtual bool onClientConnect(std::shared_ptr<Connection<T>> /*client*/) {
    return false;
  }
  virtual void onClientDisconnect(std::shared_ptr<Connection<T>> /*client*/) {}
  virtual void onMessage(std::shared_ptr<Connection<T>> /*client*/,
                         const message<T> & /*msg*/) {}

public:
  virtual void onClientValidated(std::shared_ptr<Connection<T>> /* client */) {}

protected:
  boost::asio::io_context mAsioIoconstext{};
  boost::asio::ip::tcp::acceptor mAsioAcceptor;

  tsqueue<owned_message<T>> mQmessageIn{};
  std::deque<std::shared_ptr<Connection<T>>> mDeqConnections{};
  std::thread mThreadContext{};
  std::uint32_t mIDCounter = 10000;
};
} // namespace net
} // namespace olc
#endif
