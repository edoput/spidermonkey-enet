// this is the network script that we load where we will register the callback
// for the networking events
let enet_states = [
        "disconnected",
        "ack-connect",
        "connection-pending",
        "connection-succeeded",
        "connected",
        "disconnect-later",
        "disconnecting",
        "ack-disconnect",
        "zombie",
];

class ServerState {
    Unconnected = 0;
    Pregame     = 1;
    Loading     = 2;
    InGame      = 3;
    PostGame    = 4;
};

var currentState = ServerState.Unconnected;

//var host = network.serve({address: "127.0.0.1", port: 20595});

function logPeer(peer) {
  let message = `peer
      state   ${enet_states[peer.state]}
      ID      ${peer.id}
      address ${peer.address}
      session ${peer.session}`;
  console.log(message);
}

function logEvent(event) {
  let message = `event
      channel ${event.channel}
      peer    ${event.peer}
      type    ${event.type}`;
  console.log(message);
}

function decodeString(ab) {
  return String.fromCharCode.apply(null, new Uint16Array(ab));
}

function encodeString(str) {
  var buf = new ArrayBuffer(str.length*2); // 2 bytes for each char
  var bufView = new Uint16Array(buf);
  for (var i=0, strLen=str.length; i < strLen; i++) {
    bufView[i] = str.charCodeAt(i);
  }
  return buf;
}

network.handler = (host, event) => {
  switch (event.type) {
  case "connect":
    logPeer(event.peer);
    host.sendPacket(`+${event.peer.id}`);
    break;
  case "disconnect":
    logPeer(event.peer);
    host.sendPacket(`-${event.peer.id}`);
    break;
  case "receive":
    //TODO(edoput) forward message to other peers
    if (event.packet !== undefined) {
      console.log(`received ${event.packet.byteLength} bytes from ${event.peer.id}`)
      console.log(decodeString(event.packet));
    }
    break;
  case "none":
    console.log("received none :(")
  }
}

//currentState = ServerState.Pregame;
//function tick () {
//        console.log("tick");
//        setTimeout(tick, 1000);
//}
//setTimeout(tick);

network.start()
