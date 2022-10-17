#include <assert.h>
#include <jsapi.h>
#include <js/ArrayBuffer.h>
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
    void withNetwork(JSContext *ctx, JS::HandleObject global, JS::MutableHandleObject out) {
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
    auto result = static_cast<ENetHost *>(JS_GetInstancePrivate(ctx, h, &hostClass, NULL));
    return(result);
  }

  static JSPropertySpec hostProperties[] = {
    JS_PS_END,
  };

  JSNATIVE(sendPacket) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    JS::HandleValue jsdata = args.get(0);
    size_t dataSize = JS_GetStringEncodingLength(ctx, jsdata.toString());
    if (dataSize == -1) {
      //TODO(edoput) throw exception
      return false;
    }

    //TODO(edoput) who owns this buffer and for how long?
    void* buffer = malloc(dataSize);
    if (JS_EncodeStringToBuffer(ctx, jsdata.toString(), (char*) buffer, dataSize)) {
      //TODO(edoput) throw exception
      return false;
    }
    ENetPacket * packet = enet_packet_create(buffer, dataSize, ENET_PACKET_FLAG_RELIABLE);
    if (!packet) {
      //TODO(edoput) throw exception
      return false;
    }
 
    ENetHost *self = getHostPrivate(ctx, args.thisv());
    //TODO(edoput) channel what?
    enet_host_broadcast(self, 0, packet);
    return true;
  }

  static JSFunctionSpec hostMethods[] = {
    JS_FN("sendPacket", sendPacket, 1, 0),
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
    auto result = static_cast<ENetPeer *>(JS_GetPrivate(h));
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


  /** ENetEvent */
  static JSClass eventClass = {
    "event",
    JSCLASS_HAS_PRIVATE,
  };

  ENetEvent *getEventPrivate(JSContext *ctx, JS::HandleValue THIS) {
    assert(THIS.isObject());
    JS::RootedObject h(ctx, &THIS.toObject());
    //TODO(edoput) JS_GetInstancePrivate does not work?
    auto result = static_cast<ENetEvent *>(JS_GetPrivate(h));
    return(result);
  }

  JSNATIVE(getEventChannel) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    ENetEvent *self = getEventPrivate(ctx, args.thisv());
    //TODO(edoput) args.rval().setNumber(self->channelID);
    args.rval().setUndefined();
    return true;
  }

  JSNATIVE(getEventPeer) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    ENetEvent *self = getEventPrivate(ctx, args.thisv());
    JS::RootedObject eventPeer(ctx, JS_NewObject(ctx, &peerClass));
    JS_DefineProperties(ctx, eventPeer, peerProperties);
    JS_DefineFunctions(ctx, eventPeer, peerMethods);
    JS_SetPrivate(eventPeer, self->peer);
    args.rval().setObjectOrNull(eventPeer);
    return true;
  }

  static char* eventNames[] = {
    "none",
    "connect",
    "disconnect",
    "receive",
  };

  JSNATIVE(getEventType) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    ENetEvent *self = getEventPrivate(ctx, args.thisv());
    args.rval().setString(JS_NewStringCopyZ(ctx, eventNames[(uint32_t)self->type]));
    return true;
  }

  void enetPacketFree(void* packetData, void* enetPacket) {
    enet_packet_destroy((ENetPacket*) enetPacket);
  }

  // getEventPacket exposes the data contained in the packet
  JSNATIVE(getEventPacket) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    ENetEvent *self = getEventPrivate(ctx, args.thisv());
    if (self->packet) {
      // We received a packet and there may be data associated with it.
      // Expose it  as an array buffer, we will figure it out later.
      // The packet owns the data but we share it with the
      // JS runtime. Someone has to free this data eventually.
      // when it's done with it; self->packet->freeCallBack
      JS::RootedObject packet(ctx, JS::NewExternalArrayBuffer(ctx, self->packet->dataLength, (void*) self->packet->data, &enetPacketFree, (void*) self));
      args.rval().setObjectOrNull(packet);
    } else {
      // there was no packet associated
      args.rval().setUndefined();
    }
    return true;
  }

  static JSPropertySpec eventProperties[] = {
    JS_PSG("channel", getEventChannel, 0),
    JS_PSG("peer", getEventPeer, 0),
    JS_PSG("type", getEventType, 0),
    JS_PSG("packet", getEventPacket, 0),
    JS_PS_END,
  };

  static JSFunctionSpec eventMethods[] = {
    JS_FS_END,
  };

};
