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

network.server = (host, event) => {
  switch (event.type) {
  case "connect":
    //host.sendPacket(`+${event.peer.id}`);
    break;
  case "disconnect":
    //host.sendPacket(`-${event.peer.id}`);
    break;
  }
  logPeer(event.peer);
}

network.client = (host, event) => {
  switch (event.type) {
  case "connect":
    logPeer(event.peer);
    break;
  case "disconnect":
    break;
  case "receive":
    if (event.packet.data !== undefined) {
      console.log(event.packet.data);
    }
    break;
  }
}

//currentState = ServerState.Pregame;
