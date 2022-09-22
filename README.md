enetjs
======

## Build

make main

## Run

To run the sample application make two symlinks, server and clide, point to main executable.

## Debugging

To debug the sample application use gdb. As the sample application looks up the role using
the name of the current executable the trick to make gdb launch it correctly is to load main
and the run server/client.

```bash
$ gdb main
(gdb) run server
```

```bash
$ gdb main
(gdb) run client
```

## Shared memory

Usually there are only two parties when working with ENet as a C library. The application
and the ENet server. This means that data such as packets is shared between at most 2 parties.

By introducing Spidermonkey we increased that number to three. Now three can allocate data
and three can free it, e.g. ENet receives a packet, allocates some data, the application loop
makes the event available to Spidermonkey. The event has associated pointers:  a _data_ pointer
that the application can set and the packet _message_ buffer.

The _message_ buffer is now exposed to JS as a [_ArrayBuffer_][ArrayBufferMDN] so if JS 
land holds a reference to it it should not be freed. In this example the buffer ownership
passed from ENet to Spidermonkey.

[ArrayBufferMDN]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer

The other direction is also possible. Suppose your serialization module takes a structure and
gives you back the _ArrayBuffer_ containing the serialized bytes. If you add a binding
to `enet_packet_create` then ownership of that data is shared between Spidermonkey and ENet.

Spidermonkey may free the data while ENet is still using it. And this is without considering
round-trips of buffers. Consider this example.

```
ENet Client sends packet  --> ENet Host receive packet --> ENet host forward packet to all connected
```

On step 1 Spidermonkey receives a buffer from ENet. On step 2 Spidermonkey gives back that buffer
to ENet. It may still hold a reference to it though.

This simple example shows that most likely Spidermonkey will be the problem here (actually the JS program will be).
To satisfy this needed use-case then every buffer must be freed by Spidermonkey alone.

I have still not found the perfect scheme to work on this problem but the `js/ArrayBuffer.h` headers
describe multiple schemes of allocation/freeing responsibilities.

### Buffer protocol

ArrayBuffers offer a big opportunity for moving data between different C/C++ libraries
without serialization. The simplest example is between a consumer, receiver pair of libraries.

If the JS bindings of the producer expose a ArrayBuffer and the JS bindings of the consumer
take an ArrayBuffer then there has been no intermediate copy.

The [Python buffer protocol](https://docs.python.org/3/c-api/buffer.html), [PEP3118](https://peps.python.org/pep-3118/), [PEP688](https://peps.python.org/pep-0688/)
documents this exact approach and use-case.

As such I have decided to expose C/C++ pointers as _ArrayBuffers_ so that I may move them around
without serialization.
