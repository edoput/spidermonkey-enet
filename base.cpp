#include <assert.h>
#include <jsapi.h>

#include <js/Initialization.h>

using std::string;

ENetHost * MakeHost() {
  ENetHost *result;
  ENetAddress address;
  enet_address_set_host(&address, "127.0.0.1");
  address.port = 20595;
  result = enet_host_create(&address, 32, 2, 0, 0);
  return(result);
}

ENetHost* MakeClient() {
  ENetHost *result;
  result = enet_host_create(NULL, 1, 2, 0, 0);
  return(result);
}

bool LoadScript(JSContext *ctx, const char *filename, JS::MutableHandleValue rval) {
  JS::OwningCompileOptions options(ctx);
  return JS::EvaluateUtf8Path(ctx, options, filename, rval);
}

void PrintResult(JSContext *ctx, JS::HandleValue val, const std::string prefix = "(): ") {
  JS::RootedString rval_str(ctx, JS::ToString(ctx, val));
  std::cout << prefix << JS_EncodeStringToASCII(ctx, rval_str).get() << '\n';
}

ENetPeer *Connect(ENetHost *client) {
  ENetPeer *result;
  ENetAddress address;
  enet_address_set_host(&address, "127.0.0.1");
  address.port = 20595;
  result = enet_host_connect(client, &address, 2, 0);
  return(result);
}



void Server(ENetHost *host, JSContext *ctx, JS::HandleObject global, JS::HandleObject netserver, JS::HandleObject console) {
  ENetEvent event;
  std::vector<ENetPeer*> peers;

  while (true) {
    JS::RootedValueArray<3> args(ctx);
    JS::RootedObject eventHost(ctx, JS_NewObject(ctx, &enet::hostClass)),
                     eventPeer(ctx, JS_NewObject(ctx, &enet::peerClass));
    JS_DefineProperties(ctx, eventHost, enet::hostProperties);
    JS_DefineProperties(ctx, eventPeer, enet::peerProperties);
    JS_DefineFunctions(ctx, eventHost, enet::hostMethods);
    JS_DefineFunctions(ctx, eventPeer, enet::peerMethods);
    
    JS::RootedValue rval(ctx);
    string eventName;

    while (enet_host_service(host, &event, 1000) > 0) {
      JS_SetPrivate(eventHost, host);
      JS_SetPrivate(eventPeer, event.peer);

      switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT:
	eventName = "connect";
	peers.push_back(event.peer);
	break;
      case ENET_EVENT_TYPE_RECEIVE:
	eventName = "receive";
	enet_packet_destroy(event.packet);
	break;
      case ENET_EVENT_TYPE_DISCONNECT:
	eventName = "diconnect";
	break;
      }
      args[0].setString(JS_NewStringCopyZ(ctx, eventName.c_str()));
      args[1].setObject(*eventHost);
      args[2].setObject(*eventPeer);

      // the enet server has done one iteration of the loop
      // it is time for us to call a function; it can either be a JSNATIVE
      // or a callback implemented by the user
      JS_CallFunctionName(ctx, netserver, "todo", args, &rval);
      //JS_CallFunction(ctx, global, dispatch, args, &rval);
    }

    ENetPacket * packet = enet_packet_create ("packet", strlen ("packet") + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(host, 0, packet);
  }
}

void Client(ENetHost *client, JSContext *ctx, JS::HandleObject global, JS::HandleObject netserver, JS::HandleObject console) {
  ENetPeer* peer = Connect(client); // connection to remote
  JS::RootedObject clientHost(ctx, JS_NewObject(ctx, &enet::hostClass)),
                   eventPeer(ctx, JS_NewObject(ctx, &enet::peerClass));
  JS_DefineFunctions(ctx, clientHost, enet::hostMethods);
  JS_DefineFunctions(ctx, eventPeer, enet::peerMethods);

  ENetEvent event;

  while (true) {
    JS::RootedValueArray<3> args(ctx);

    JS::RootedValue rval(ctx);
    string eventName;

    while (enet_host_service(client, &event, 1000) > 0) {
      switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT:
	eventName = "connected";
        break;
      case ENET_EVENT_TYPE_RECEIVE:
	eventName = "receive";
	break;
      case ENET_EVENT_TYPE_DISCONNECT:
	eventName = "disconnect";
	break;
      }
      args[0].setString(JS_NewStringCopyZ(ctx, eventName.c_str()));
      args[1].setObject(*clientHost);
      args[2].setObject(*eventPeer);
      
      //JS::Call(ctx, netserver, nullptr, args, &rval);
      // JS_CallFunction(ctx, global, dispatch, args, &rval);
    }
  }
}

void Setup() {
  enet_initialize();
  JS_Init();
}

void Teardown() {
  JS_ShutDown();
  atexit(enet_deinitialize);
}
