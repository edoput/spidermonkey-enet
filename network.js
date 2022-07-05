// this is the network script that we load where we will register the callbacks

// there will be a global object called network
// there will be one two classes, one bound to ENetPeer
// the other to ENetHost. Both classes are not available
// to JS and instances of that type are available only
// as arguments to the callbacks.

// we expect to have the following function signatures
// network.registerEventListener("connect", (host, client) => undefined);
// network.removeEventListener("connect, (host, client) => undefined);
// network.dispatch("connect");

// network.registerEventListener("disconnect", (host, client) => undefined);
// network.removeEventListener("disconnect, (host, client) => undefined);
// network.dispatch("disconnect");

// network.registerEventListener("message", (host, client) => undefined);
// network.removeEventListener("message, (host, client) => undefined);
// network.dispatch("message");

function noop () {};

netserver.addEventListener("connect", (host, peer) => {
    console.log("new connection!");
    console.log(host);
});
