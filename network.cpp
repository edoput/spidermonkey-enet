void Server(ENetHost *host,  JSContext *ctx) {
  ENetEvent event;
  std::vector<ENetPeer*> peers;

  while (true) {
    while (enet_host_service(host, &event, 1000) > 0) {
      switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT:
	printf("A new client connected from %x:%u.\n",
		 event.peer->address.host,
		 event.peer->address.port);
	peers.push_back(event.peer);
	break;
      case ENET_EVENT_TYPE_RECEIVE:
	printf ("A packet of length %u containing %s was received from %s on channel %u.\n",
		event.packet -> dataLength,
		event.packet -> data,
		event.peer -> data,
		event.channelID);
	enet_packet_destroy(event.packet);
	break;
      case ENET_EVENT_TYPE_DISCONNECT:
	printf("peer disconnected\n");

	//TODO(edoput) what fields of the ENetPeer structure can be used
	// to tell apart one peer from the other?
	break;
      }
    }

    ENetPacket * packet = enet_packet_create ("packet", strlen ("packet") + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(host, 0, packet);
  }
}

void Client(ENetHost *client,  JSContext *ctx) {
  ENetPeer* peer = Connect(client); // connection to remote

  ENetEvent event;

  while (true) {
    while (enet_host_service(client, &event, 1000) > 0) {
      switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT:
	std::cout << "client: connected\n";
        break;
      case ENET_EVENT_TYPE_RECEIVE:
	std::cout << "client: receive\n";
	break;
      case ENET_EVENT_TYPE_DISCONNECT:
	std::cout << "client: disconnected\n";

	break;
      }
    }
  }
}
