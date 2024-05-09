
#ifndef NET_CONNECTION_H
#define NET_CONNECTION_H

#include "net_common.h"
#include "net_message.h"
#include "net_tsqueue.h"

namespace olc {
namespace net {
template <typename T> class serverInterface;

template <typename T>
class Connection : public std::enable_shared_from_this<Connection<T>> {
public:
  enum class Owner {
    server,
    client,
  };
  Connection(Owner parent, boost::asio::io_context &asioContext,
             boost::asio::ip::tcp::socket sock, tsqueue<owned_message<T>> &qIn)
      : mAsioContext{asioContext}, mSock{std::move(sock)}, mQMessageIn{qIn} {
    mOwnerType = parent;
    if (parent == Owner::server) {
      mHandShakeOut = std::uint64_t(
          std::chrono::system_clock::now().time_since_epoch().count());
      mHandShakeCheck = scrample(mHandShakeOut);
    }
  }
  Connection() {}
  virtual ~Connection() {}

public:
  // aftter successfully new client is connected start validating the client
  // the server point paprameter is used to call user provided onClientValidated
  // function
  void connectToClient(olc::net::serverInterface<T> *server,
                       std::uint32_t uid = 0) {
    if (mOwnerType == Owner::server) {
      if (mSock.is_open()) {
        mId = uid;
        writeValidation();
        readValidation(server);
      }
    }
  }
  void connectToServer(
      const boost::asio::ip::tcp::resolver::results_type &endPoints) {
    if (mOwnerType == Owner::client) {
      boost::asio::async_connect(
          mSock, endPoints,
          [this](const std::error_code &er,
                 boost::asio::ip::tcp::endpoint /* endPoint */) {
            if (!er) {
              readValidation();
            }
          });
    }
  };
  void disConnect() {
    if (isConnected())
      boost::asio::post(mAsioContext, [this] { mSock.close(); });
  }

  bool isConnected() const { return mSock.is_open(); };

  void send(const message<T> &msg) {
    boost::asio::post(mAsioContext, [this, msg]() {
      bool bWriteMessage{!mQMessageOut.empty()};
      mQMessageOut.push_back(msg);
      if (!bWriteMessage) {
        writeHeader();
      }
    });
  }

  std::uint32_t getID() const { return mId; }

private:
  void readHeader() {
    boost::asio::async_read(
        mSock,
        boost::asio::buffer(&mMsgTemporaryIn.header, sizeof(message_header<T>)),
        [this](const std::error_code &ec, std::size_t /* bytes */) {
          if (!ec) {
            if (mMsgTemporaryIn.header.size > 0) {
              mMsgTemporaryIn.body.resize(mMsgTemporaryIn.header.size);
              readBody();
            } else {
              addToIncommingMessageQueue();
            }
          } else {
            std::cerr << '[' << mId << "] read header failed.\n";
            mSock.close();
          }
        });
  }
  void readBody() {
    boost::asio::async_read(mSock, boost::asio::buffer(mMsgTemporaryIn.body),
                            [this](const std::error_code &ec, std::size_t) {
                              if (!ec) {
                                addToIncommingMessageQueue();
                              } else {
                                std::cerr << '[' << mId
                                          << "] read body failed.\n";
                                mSock.close();
                              }
                            });
  }
  void writeHeader() {
    boost::asio::async_write(
        mSock,
        boost::asio::buffer(&mQMessageOut.front().header,
                            sizeof(message_header<T>)),
        [this](const std::error_code &er, std::size_t /*bytes*/) {
          if (!er) {
            if (mQMessageOut.front().body.size() > 0) {
              writeBody();
            } else {
              mQMessageOut.pop_front();
              if (!mQMessageOut.empty())
                writeHeader();
            }
          } else {
            std::cerr << '[' << mId << "] write header failed.\n";
            mSock.close();
          }
        });
  }
  void writeBody() {
    boost::asio::async_write(
        mSock, boost::asio::buffer(mQMessageOut.front().body),
        [this](const boost::system::error_code er, std::size_t /* bytes */) {
          if (!er) {
            mQMessageOut.pop_front();
            if (!mQMessageOut.empty())
              writeHeader();
          } else {
            std::cerr << '[' << mId << "] write body failed.\n";
            mSock.close();
          }
        });
  }

  // add the mMsgTemporaryIn message to outgoing message queue
  void addToIncommingMessageQueue() {
    if (mOwnerType == Owner::server) {
      mQMessageIn.push_back({this->shared_from_this(), mMsgTemporaryIn});
    } else
      mQMessageIn.push_back({nullptr, mMsgTemporaryIn});

    readHeader();
  }

  std::uint64_t scrample(std::uint64_t nInput) {
    std::uint64_t out{nInput ^ 0xDEADBDADFEDFEAFF};
    out = (out & 0xF0F00FDebabdF0F0) >> 4 | (out & 0xF0F0A0A0A0bdF0F0) << 4;
    return out ^ 0x0ACFDCEFB5123456;
  }

  void writeValidation() {
    boost::asio::async_write(
        mSock, boost::asio::buffer(&mHandShakeOut, sizeof(mHandShakeOut)),
        [this](const std::error_code &ec, std::size_t /* bytes */) {
          if (!ec) {
            if (mOwnerType == Owner::client)
              readHeader();
          } else {
            mSock.close();
          }
        });
  }

  void readValidation(olc::net::serverInterface<T> *server = nullptr) {
    boost::asio::async_read(
        mSock, boost::asio::buffer(&mHandShakeIn, sizeof(mHandShakeIn)),
        [this, server](const std::error_code &ec, std::size_t /* bytes */) {
          if (!ec) {

            if (mOwnerType == Owner::server) {
              if (mHandShakeIn == mHandShakeCheck) {
                std::cerr << "Client Validated\n";
                server->onClientValidated(this->shared_from_this());
                readHeader();
              } else {
                std::cerr << "Client disConnected (fail validation)\n";
                mSock.close();
              }
            } else {
              mHandShakeOut = scrample(mHandShakeIn);
              writeValidation();
            }
          } else {
            std::cerr << "Client disConnected (readValidation)\n";
            mSock.close();
          }
        });
  }

protected:
  // the asio operation work on context but it's one and provided by
  // client/server
  boost::asio::io_context &mAsioContext;

  // as connection it contains it's own socket for paticulart client or server
  boost::asio::ip::tcp::socket mSock;

  // each connection has ID identifier to identify clients from the server side
  // for clients or from clients side for server identification
  std::uint32_t mId{0};

  // identifies what is this connection for i.e server or client to act
  // accordingly
  Owner mOwnerType{};

  // for outgoing messages
  tsqueue<message<T>> mQMessageOut{};

  // for current reading message
  message<T> mMsgTemporaryIn{};

  // for incomming message from this connection
  // deposit it to server/client queue for processing
  // so it needs to reference server or client queue
  tsqueue<owned_message<T>> &mQMessageIn;

  std::uint64_t mHandShakeOut{0};
  std::uint64_t mHandShakeIn{0};
  std::uint64_t mHandShakeCheck{0};
};
//
} // namespace net
} // namespace olc

#endif
