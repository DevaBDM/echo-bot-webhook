// copyright net_olc_simpleServer
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////  net_olc_simpleServer  ////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "include/olc_net.h"

enum class CustomMsgTypes : std::uint32_t {
  ServerAccept,
  ServerDeny,
  ServerPing,
  ClientID,
  MessageAll,
  ServerMessage,
  SendToClient,
  NoSuchClient,
  ClientDisconnected,
};

std::unordered_map<std::uint32_t,
                   std::shared_ptr<olc::net::Connection<CustomMsgTypes>>>
    k_clientsList;

class CustomServer : public olc::net::serverInterface<CustomMsgTypes> {
public:
  CustomServer(std::uint16_t nPort)
      : olc::net::serverInterface<CustomMsgTypes>{nPort} {}

protected:
  virtual bool onClientConnect(
      std::shared_ptr<olc::net::Connection<CustomMsgTypes>> /* client */)
      override {
    return true;
  }

  virtual void onClientDisconnect(
      std::shared_ptr<olc::net::Connection<CustomMsgTypes>> client) override {
    k_clientsList[client->getID()].reset();
    std::cout << "Removing client [" << client->getID() << "]\n";
  }

  virtual void onClientValidated(
      std::shared_ptr<olc::net::Connection<CustomMsgTypes>> client) override {
    olc::net::message<CustomMsgTypes> msg;
    msg.header.id = CustomMsgTypes::ServerAccept;
    msg << client->getID();
    messageClient(client, msg);

    k_clientsList.insert_or_assign(client->getID(), client);
  }

  virtual void
  onMessage(std::shared_ptr<olc::net::Connection<CustomMsgTypes>> client,
            const olc::net::message<CustomMsgTypes> &msg) override {
    switch (msg.header.id) {
    case CustomMsgTypes::ServerPing: {
      std::cout << '[' << client->getID() << "] server ping\n";
      messageClient(client, msg);
    } break;
    case CustomMsgTypes::ServerAccept:
      break;
    case CustomMsgTypes::ServerDeny:
      break;
    case CustomMsgTypes::MessageAll: {
      std::cout << '[' << client->getID() << "] message all\n";
      olc::net::message<CustomMsgTypes> msgg;
      msgg.header.id = CustomMsgTypes::ServerMessage;
      msgg << client->getID();
      messageAllClients(msgg, client);

    } break;
    case CustomMsgTypes::ServerMessage:
      break;
    case CustomMsgTypes::ClientID:
      break;
    case CustomMsgTypes::SendToClient: {
      olc::net::message<CustomMsgTypes> msgg{msg};
      std::uint32_t ClientID;
      msgg >> ClientID;

      if (k_clientsList.end() != k_clientsList.find(ClientID)) {
        std::shared_ptr<olc::net::Connection<CustomMsgTypes>> found =
            k_clientsList[ClientID];
        if (found) {
          if (found != client) {
            msgg << client->getID();

            if (messageClient(found, msgg))
              break;

            msgg.header.id = CustomMsgTypes ::ClientDisconnected;
          }
        } else {
          msgg.header.id = CustomMsgTypes ::ClientDisconnected;
        }
      } else {
        msgg.header.id = CustomMsgTypes ::NoSuchClient;
      }

      olc::net::message<CustomMsgTypes> reMsg;
      reMsg.header.id = msgg.header.id;
      reMsg << ClientID;
      messageClient(client, reMsg);
    } break;
    case CustomMsgTypes::NoSuchClient:
      break;
    case CustomMsgTypes::ClientDisconnected:
      break;
    }
  }
};

int main() {
  CustomServer server(8080);
  server.start();

  while (1)
    try {
      server.update();
    } catch (std::exception &e) {
      std::cerr << "[SERVER Loop] " << e.what() << '\n';
    }

  return 0;
}
