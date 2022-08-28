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

  
    // class NetServer {
    //   ENetHost* _host;
    //   JS::Heap<JSFunction*> handler;
    //   // JS::Heap<JSFunction*> onConnect;
    //   // JS::Heap<JSFunction*> onDisconnect;
    //   // JS::Heap<JSFunction*> onMessage;
  
    //   void trace(JSTracer* tracer, void *data) {
    //     JS::TraceEdge(tracer, &handler, "handler");
    //     // JS::TraceEdge(tracer, &onConnect, "onConnect");
    //     // JS::TraceEdge(tracer, &onDisconnect, "onDisconnect");
    //     // JS::TraceEdge(tracer, &onMessage, "onMessage");
    //   } 
    // };

  // the netserver global object exposes a handler function
  // that receives all the ENet events from the loop.
  namespace net {

    //TODO(edoput) this must now be garbage collected
    // trace also the stuff we add to _listeners_
    // JS_AddExtraGCRootsTracer(ctx, &trace_listeners, nullptr);

    JS::Heap<JSObject*> clientHandler, serverHandler;

    void traceHandler(JSTracer* tracer, void* data) {
      JS::TraceEdge(tracer, &serverHandler, "net::handler::server");
      JS::TraceEdge(tracer, &clientHandler, "net::handler::client");
    };

    static JSClass serverClass = {
      "Netserver",
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
      JS_FN("dispatch", noop, 2, 0),
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
      JS::RootedString rval_str(ctx, JS::ToString(ctx, args.get(0)));
      std::cout << JS_EncodeStringToASCII(ctx, rval_str).get() << '\n';
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

  //[Exposed=netserver.handler]
  static JSClass hostClass = {
    "host",
    JSCLASS_HAS_PRIVATE,
  };

  ENetHost *getHostPrivate(JSContext *ctx, JS::CallArgs args) {
    JS::HandleValue  THIS = args.thisv();
    assert(THIS.isObject());
    JS::RootedObject h(ctx, &THIS.toObject());
    ENetHost *result = (ENetHost *) JS_GetInstancePrivate(ctx, h, &hostClass, NULL);
    return(result);
  }


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
  
  // [Exposed=netserver.handler]
  static JSClass peerClass = {
    "peer",
    JSCLASS_HAS_PRIVATE,
  };

  ENetPeer *getPeerPrivate(JSContext *ctx, JS::CallArgs args) {
    JS::HandleValue  THIS = args.thisv();
    assert(THIS.isObject());
    JS::RootedObject h(ctx, &THIS.toObject());
    ENetPeer *result = (ENetPeer *) JS_GetInstancePrivate(ctx, h, &hostClass, NULL);
    return(result);
  }

  JSNATIVE(getPeerAddress) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    ENetPeer *self = getPeerPrivate(ctx, args);
    ENetAddress address = self->address;
    args.rval().setNumber(address.host);
    return true;
  }
  JSNATIVE(getPeerBandwidth) {
    return true;
  }
  JSNATIVE(getPeer) {
    return true;
  }
  JSNATIVE(getPeerState) {
    return true;
  }
  JSNATIVE(getPeerSession) {
    return true;
  }
  JSNATIVE(getPeerStats) {
    return true;
  }
  static JSPropertySpec peerProperties[] = {
    JS_PSG("address", getPeerAddress, 0),
    JS_PSG("state", getPeerState, 0),
    JS_PSG("bandwidth", getPeerBandwidth, 0),
    JS_PSG("peer", getPeer, 0),
    JS_PSG("session", getPeerSession, 0),
    JS_PSG("stats", getPeerStats, 0),
    JS_PS_END,
  };

  static JSFunctionSpec peerMethods[] = {
    JS_FS_END,
  };

  static JSFunctionSpec serverMethods[] = {
    JS_FS_END,
  };

};

