#include <assert.h>
#include <jsapi.h>
#include <js/ArrayBuffer.h>
#include <enet/enet.h>

#include "binding/enet.hpp"

namespace enet {
  ENetHost *getHostPrivate(JSContext *ctx, JS::HandleValue THIS) {
    assert(THIS.isObject());
    JS::RootedObject h(ctx, &THIS.toObject());
    auto result = static_cast<ENetHost *>(JS_GetInstancePrivate(ctx, h, &hostClass, NULL));
    return(result);
  }

  JSNATIVE(sendPacket) {
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    JS::HandleValue jsdata = args.get(0);
    //TODO(edoput) an ArrayBuffer would fit here better
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
    //TODO(edoput) ENET_PACKET_FLAG_*
    ENetPacket * packet = enet_packet_create(buffer, dataSize, ENET_PACKET_FLAG_RELIABLE);
    if (!packet) {
      //TODO(edoput) JS::ExceptionStackBehavior::{Capture,DoNotCapture}
      //TODO(edoput) what to put as the exception value?
      //JS_SetPendingException(ctx, JS::UndefinedValue());
      return false;
    }
    ENetHost *self = getHostPrivate(ctx, args.thisv());
    //TODO(edoput) channel what?
    enet_host_broadcast(self, 0, packet);
    return true;
  }

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
    JS_DefineProperty(ctx, b, "incoming", self->incomingBandwidth, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineProperty(ctx, b, "outgoing", self->outgoingBandwidth, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
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

  JSNATIVE(getEventType) {
    std::string eventNames[] = {
      "none",
      "connect",
      "disconnect",
      "receive",
    };
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    ENetEvent *self = getEventPrivate(ctx, args.thisv());
    args.rval().setString(JS_NewStringCopyZ(ctx, eventNames[(uint32_t)self->type].c_str()));
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
 
};
