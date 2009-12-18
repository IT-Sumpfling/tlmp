#pragma once

#include "DataTypes.h"
#include "Common.h"

#include "../ExternalLibs/raknet/include/RakPeerInterface.h"
#include "../ExternalLibs/raknet/include/RakNetworkFactory.h"
#include "../ExternalLibs/raknet/include/MessageIdentifiers.h"
#include "../ExternalLibs/raknet/include/BitStream.h"

#include "network.pb.h"


namespace TLMP {

  namespace Network {

    class Server {
    public:
      typedef void (*OnListening)(void *);
      typedef void (*OnShutdown)(void *);
      typedef void (*OnClientConnect)(void *);
      typedef void (*OnClientDisconnect)(void *);
      

      static Server& getSingleton();

      void Listen(u16 port, u16 maxconnections);
      void Shutdown();

      void SetCallback_OnListening(OnListening callback);
      void SetCallback_OnShutdown(OnShutdown callback);
      void SetCallback_OnClientConnect(OnClientConnect callback);
      void SetCallback_OnClientDisconnect(OnClientDisconnect callback);

      void ReceiveMessages();

      void SendMessage(NetworkMessages::Player message);

    protected:
      Server();
      Server(const Server&);
      Server& operator=(const Server&);
      ~Server();

    private:
      void ParseMessage(u8 *packetData, u32 length);

      RakPeerInterface *m_pServer;
      RakNet::BitStream *m_pBitStream;
      
      OnListening         m_pOnListening;
      OnShutdown          m_pOnShutdown;
      OnClientConnect     m_pOnClientConnect;
      OnClientDisconnect  m_pOnClientDisconnect;
    };

  };

};