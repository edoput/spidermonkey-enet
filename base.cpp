#include <assert.h>
#include <iostream>

#include <jsapi.h>
#include <js/Conversions.h>
#include <js/Initialization.h>

#include "builtin/network.hpp"
#include "binding/enet.hpp"

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

  JS::RootedObject eventHost(ctx, JS_NewObject(ctx, &enet::hostClass)),
                   eventObject(ctx, JS_NewObject(ctx, &enet::eventClass));

  JS_DefineProperties(ctx, eventHost, enet::hostProperties);
  JS_DefineProperties(ctx, eventObject, enet::eventProperties);
  JS_DefineFunctions(ctx, eventHost, enet::hostMethods);
  JS_DefineFunctions(ctx, eventObject, enet::eventMethods);

  JS::RootedValue rval(ctx), handler(ctx);

  while (true) {
    builtin::network::getServerHandlerFunction(ctx, &handler);

    JS::RootedValueArray<2> args(ctx);

    while (enet_host_service(host, &event, 1000) > 0) {
      JS_SetPrivate(eventHost, host);
      JS_SetPrivate(eventObject, &event);

      args[0].setObject(*eventHost);
      args[1].setObject(*eventObject);

      if (!JS_CallFunctionValue(ctx, global, handler, args, &rval)) {
        if (JS_IsExceptionPending(ctx)) {
          JS::ExceptionStack exnStack(ctx);
          JS::StealPendingExceptionStack(ctx, &exnStack);
          JS::ErrorReportBuilder builder(ctx);
          builder.init(ctx, exnStack, JS::ErrorReportBuilder::NoSideEffects);
          JS::PrintError(ctx, stderr, builder, false);
        }
      }
    }

    //ENetPacket * packet = enet_packet_create ("packet", strlen ("packet") + 1, ENET_PACKET_FLAG_RELIABLE);
    //enet_host_broadcast(host, 0, packet);
  }
}

void Client(ENetHost *client, JSContext *ctx, JS::HandleObject global, JS::HandleObject netserver, JS::HandleObject console) {
  ENetEvent event;

  ENetPeer* peer = Connect(client); // connection to remote
  JS::RootedObject clientHost(ctx, JS_NewObject(ctx, &enet::hostClass)),
                   eventObject(ctx, JS_NewObject(ctx, &enet::eventClass));

  JS_DefineProperties(ctx, clientHost,  enet::hostProperties);
  JS_DefineProperties(ctx, eventObject, enet::eventProperties);
  JS_DefineFunctions(ctx,  clientHost,  enet::hostMethods);
  JS_DefineFunctions(ctx,  eventObject, enet::eventMethods);

  JS::RootedValue rval(ctx), handler(ctx);
    

  while (true) {
    builtin::network::getClientHandlerFunction(ctx, &handler);

    JS::RootedValueArray<2> args(ctx);

    //TODO(edoput) test == 0, timeout reached
    //TODO(edoput) test <  0, enet error
    while (enet_host_service(client, &event, 1000) > 0) {
      JS_SetPrivate(clientHost, client);
      JS_SetPrivate(eventObject, &event);

      args[0].setObject(*clientHost);
      args[1].setObject(*eventObject);

        // the enet server has done one iteration of the loop
      // it is time for us to call a function; it can either be a JSNATIVE
      // or a callback implemented by the user

      if (!JS_CallFunctionValue(ctx, global, handler, args, &rval)) {
        if (JS_IsExceptionPending(ctx)) {
          JS::ExceptionStack exnStack(ctx);
          JS::StealPendingExceptionStack(ctx, &exnStack);
          JS::ErrorReportBuilder builder(ctx);
          builder.init(ctx, exnStack, JS::ErrorReportBuilder::NoSideEffects);
          JS::PrintError(ctx, stderr, builder, false);
        }
      }
    }
    //uint8_t message[6] = {'H', 'e', 'l', 'l', 'o', '!'};
    uint16_t message[6] = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x21};
    ENetPacket * packet = enet_packet_create (&message, 12, ENET_PACKET_FLAG_RELIABLE);
    // NOTE(edoput) both options work here
    enet_peer_send(peer, 0, packet);
    //enet_host_broadcast(client, 0, packet);
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
