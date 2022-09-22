#include <assert.h>
#include <jsapi.h>
#include <enet/enet.h>

JSNATIVE(noop) {
  return true;
}

/** The JS interpreter needs a global object. You can attach
 * other names to the global objects like the browser does with
 * _window_, _document_ and so on.
 */
namespace global {
  static JSClass globalClass = {
    "global",
    JSCLASS_GLOBAL_FLAGS,
    &JS::DefaultGlobalClassOps,
  };

};

namespace builtin {
  /** For each builtin name, e.g. console, netserver, we make a withX function adding it.
   *  Builtin names may be any JS value but I usually go for an object with a custom class.
   */

  namespace network {


    JS::Heap<JSObject*> clientHandler, serverHandler;

    void traceHandler(JSTracer* tracer, void* data) {
      JS::TraceEdge(tracer, &serverHandler, "net::handler::server");
      JS::TraceEdge(tracer, &clientHandler, "net::handler::client");
    };

    static JSClass serverClass = {
      "Network",
      0
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
  
    static JSPropertySpec serverProperties[] = {
      JS_PSGS("server", getNetServerHandler, setNetServerHandler, 0),
      JS_PSGS("client", getNetClientHandler, setNetClientHandler, 0),
      JS_PS_END,
    };

    static JSFunctionSpec serverMethods[] = {
      JS_FS_END,
    };

    // exposes globally the name _netserver_ for an object of class netserver
    void withNetServer(JSContext *ctx, JS::HandleObject global, JS::MutableHandleObject out) {
      JS::RootedObject result(ctx, JS_DefineObject(ctx, global, "network", &serverClass, 0));
      JS_DefineProperties(ctx, result, serverProperties);
      JS_DefineFunctions(ctx, result, serverMethods);
      out.set(result);
      JS_AddExtraGCRootsTracer(ctx, traceHandler, nullptr);
    }
  };


  namespace console {

    JSNATIVE(consoleLog) {
      JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
      //TODO(edoput) argc == args.length() ???
      for (unsigned i = 0; i < argc; i++) {
	  std::cout << JS_EncodeStringToASCII(ctx, JS::ToString(ctx, args.get(i))).get();
      }
      std::cout << '\n';
      return true;
    }
  
    static JSClass consoleClass = {
      "console",
      0
    };

    static JSFunctionSpec consoleMethods[] = {
      JS_FN("log", consoleLog, 1, 0),
      JS_FS_END,
    };
    void withConsole(JSContext *ctx, JS::HandleObject global, JS::MutableHandleObject out) {
      JS::RootedObject result(ctx, JS_DefineObject(ctx, global, "console", &consoleClass, 0));
      JS_DefineFunctions(ctx, result, consoleMethods);
      out.set(result);
    }
  };

};

namespace enet {
  /** Each enet name I want to use from JS must be exposed
   *  as a JS value. The straightforward way to work with
   *  pointers is to have a _native object_ (firefox naming).
   *
   *  A native object is a JS object with a custom class defined
   *  in C++. Properties and methods are defined as JSNATIVE functions.
   *  The native object may refer to an instance/pointer in C++
   *  by setting its private data value to it.
   */

  /** ENetHost */

  //[Exposed=(network.server, network.client)]
  static JSClass hostClass = {
    "host",
    JSCLASS_HAS_PRIVATE,
  };

  ENetHost *getHostPrivate(JSContext *ctx, JS::HandleValue THIS) {
    assert(THIS.isObject());
    JS::RootedObject h(ctx, &THIS.toObject());
    ENetHost *result = (ENetHost *) JS_GetInstancePrivate(ctx, h, &hostClass, NULL);
    return(result);
  }

  static JSPropertySpec hostProperties[] = {
    JS_PS_END,
  };

  JSNATIVE(disconnectPeer) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    //JS::HandleObject peerArg = args.get(0);
    //ENetHost *currentHost = JS_GetPrivate(hostArg);
    return true;
  }

  // This function is defined on an object called eventHost
  // in the Client/Server loops. On each iteration of the loop
  // we set the private data of the eventHost to point to the
  // current ENetHost* that we received from the setup phase.
  // The `this` identifier in JS is bound to this eventHost
  // object but we can also recover it using the call arguments.
  // JSNATIVE(todo) {
  //   JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  //   JS::HandleValue  THIS = args.thisv();
  //   assert(THIS.isObject());
  //   JS::RootedObject h(ctx, &THIS.toObject());
  //   ENetHost *host = (ENetHost *) JS_GetInstancePrivate(ctx, h, &hostClass, NULL);
  //   assert(host);
  
  //   //ENetHost *host = (ENetHost *) JS_GetPrivate(THIS);
  //   return true;
  // }

  static JSPropertySpec hostProperties[] = {
    JS_PS_END,
  };

  static JSFunctionSpec hostMethods[] = {
    //JS_FN("todo", todo, 0, 0),
    JS_FS_END,
  };

  /** ENetPeer */
  
  // [Exposed=(network.server, network.client)]
  static JSClass peerClass = {
    "peer",
    JSCLASS_HAS_PRIVATE,
  };

  ENetPeer *getPeerPrivate(JSContext *ctx, JS::HandleValue THIS) {
    assert(THIS.isObject());
    JS::RootedObject h(ctx, &THIS.toObject());
    //TODO(edoput) JS_GetInstancePrivate does not work?
    ENetPeer *result = (ENetPeer *) JS_GetPrivate(h);
    return(result);
  }

  /** JS Properties to native struct fields */

  JSNATIVE(getPeerAddress) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    ENetPeer *self = getPeerPrivate(ctx, args.thisv());
    char address[15];
    assert(!enet_address_get_host_ip(&self->address, address, 15));
    args.rval().setString(JS_NewStringCopyN(ctx, address, 16));
    return true;
  }
  
  JSNATIVE(getPeerBandwidth) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    ENetPeer *self = getPeerPrivate(ctx, args.thisv());
    JS::RootedObject b(ctx, JS_NewPlainObject(ctx));
    JS_DefineProperty(ctx, b, "incoming", self->incomingBandwidth, JSPROP_ENUMERATE || JSPROP_READONLY || JSPROP_PERMANENT);
    JS_DefineProperty(ctx, b, "outgoing", self->outgoingBandwidth, JSPROP_ENUMERATE || JSPROP_READONLY || JSPROP_PERMANENT);
    args.rval().setObjectOrNull(b);
    return true;
  }
  

  JSNATIVE(getPeerState) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    ENetPeer *self = getPeerPrivate(ctx, args.thisv());
    //TODO(edoput) mmm delicious magic values
    args.rval().setInt32(self->state);
    return true;
  }
  
  JSNATIVE(getPeerID) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    ENetPeer *self = getPeerPrivate(ctx, args.thisv());
    args.rval().setInt32(self->incomingPeerID);
    return true;
  }
  
  JSNATIVE(getPeerSessionID) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    ENetPeer *self = getPeerPrivate(ctx, args.thisv());
    args.rval().setInt32(self->incomingSessionID);
    return true;
  }
  
  JSNATIVE(getPeerStats) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    ENetPeer *self = getPeerPrivate(ctx, args.thisv());
    args.rval().setUndefined();
    return true;
  }

  static JSPropertySpec peerProperties[] = {
    JS_PSG("address", getPeerAddress, 0),
    JS_PSG("state", getPeerState, 0),
    JS_PSG("bandwidth", getPeerBandwidth, 0),
    JS_PSG("id", getPeerID, 0),
    JS_PSG("session", getPeerSessionID, 0),
    JS_PSG("stats", getPeerStats, 0),
    JS_PS_END,
  };

  /** JS methods to enet_peer_* functions */

  JSNATIVE(disconnectPeer) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    ENetPeer *self = getPeerPrivate(ctx, args.thisv());
    //TODO(edoput) accept optional values: data, now, force
    enet_peer_disconnect(self, 0);
    args.rval().setUndefined();
    return true;
  }

  JSNATIVE(pingPeer) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    ENetPeer *self = getPeerPrivate(ctx, args.thisv());
    enet_peer_ping(self);
    return true;
  }

  static JSFunctionSpec peerMethods[] = {
    JS_FN("disconnect", disconnectPeer, 1, 0),
    JS_FN("ping", pingPeer, 1, 0),
    JS_FS_END,
  };

  static JSFunctionSpec serverMethods[] = {
    JS_FS_END,
  };

};
