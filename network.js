// this is the network script that we load where we will register the callback
// for the networking events
var enet_states = [
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

network.server = function (eventName, host, peer) {
    console.log("received event ", eventName);
    console.log("peer address ", peer.address);
    console.log("peer bandwidth ", "out: ", peer.bandwidth.outgoing, " incoming: ", peer.bandwidth.incoming);
    console.log("peer state ", enet_states[peer.state]);
};


network.client = function (eventName, host, peer) {
    console.log("received event ", eventName);
}
