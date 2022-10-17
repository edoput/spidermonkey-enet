#include "builtin/macro.hpp"
#include "builtin/network.hpp"

namespace builtin {
  namespace network {

    JS::Heap<JSObject*> clientHandler, serverHandler;

    void traceHandler(JSTracer* tracer, void* data) {
      JS::TraceEdge(tracer, &serverHandler, "net::handler::server");
      JS::TraceEdge(tracer, &clientHandler, "net::handler::client");
    };

    void getServerHandlerFunction(JSContext *ctx, JS::MutableHandleValue rval) {
      rval.setObjectOrNull(serverHandler.get());
    }
    
    void getClientHandlerFunction(JSContext *ctx, JS::MutableHandleValue rval) {
      rval.setObjectOrNull(clientHandler.get());
    }

    JSNATIVE(getNetServerHandler) {
      JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
      args.rval().setObjectOrNull(*serverHandler.address());
      return true;
    }

    JSNATIVE(setNetServerHandler) {
      JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
      serverHandler.set(&args.get(0).toObject());
      args.rval().setUndefined();
      return true;
    }
    
    JSNATIVE(getNetClientHandler) {
      JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
      args.rval().setObjectOrNull(*clientHandler.address());
      return true;
    }

    JSNATIVE(setNetClientHandler) {
      JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
      clientHandler.set(&args.get(0).toObject());
      args.rval().setUndefined();
      return true;
    }
  
    /** postMessage is exposed on network.
     * I need to make sure always creating a ENetHost is intended
     * and I have a pointer for it.
     *
     * The server is intended to receive packets from the clients
     * and forward them. This is accomplished using enet_host_broadcast.
     *
     * A client is intended to send and receive packets from the server.
     * This is accomplished using enet_peer_send or enet_host_broadcast?
     * As long as a client only holds one peer in its connections would
     * they work the same?
     */
    JSNATIVE(postMessage) {
      JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
      args.rval().setUndefined();
      return true;
    }

    // exposes globally the name _netserver_ for an object of class netserver
    void withNetwork(JSContext *ctx, JS::HandleObject global, JS::MutableHandleObject out) {
      JS::RootedObject result(ctx, JS_DefineObject(ctx, global, "network", &serverClass, 0));
      JS_DefineProperties(ctx, result, serverProperties);
      JS_DefineFunctions(ctx, result, serverMethods);
      out.set(result);
      JS_AddExtraGCRootsTracer(ctx, traceHandler, nullptr);
    }
  };
}
