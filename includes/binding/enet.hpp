#ifndef BINDING_ENET_H
#define BINDING_ENET_H

#include <jsapi.h>
#include <enet/enet.h>

#include "builtin/macro.hpp"

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

  ENetHost *getHostPrivate(JSContext *ctx, JS::HandleValue THIS);

  static JSPropertySpec hostProperties[] = {
    JS_PS_END,
  };

  JSNATIVE(sendPacket);

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

  /** JS Properties to native struct fields */

  JSNATIVE(getPeerAddress);

  JSNATIVE(getPeerBandwidth);

  JSNATIVE(getPeerState);
  
  JSNATIVE(getPeerID);
  
  JSNATIVE(getPeerSessionID);
  
  JSNATIVE(getPeerStats);

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

  JSNATIVE(disconnectPeer);

  JSNATIVE(pingPeer);

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

  JSNATIVE(getEventChannel);

  JSNATIVE(getEventPeer);

  JSNATIVE(getEventType);

  // getEventPacket exposes the data contained in the packet
  JSNATIVE(getEventPacket);

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
#endif
