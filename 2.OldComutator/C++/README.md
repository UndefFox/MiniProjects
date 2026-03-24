Was curious of making some kind of network application, so I've gone headfirst without making any prior research. It definitely has a bunch of problems, but... it works and I call it good enough for the first blind attempt into this :P


# Implementation

All the logic is done via a single class. Each logic is separated into a different thread so that nothing gets stuck. Communication is done via sockets and a custom protocol over TCP. All data is encrypted via XOR using a 64-bit symmetric key. Key exchange is done with Diffie-Hellman. The socket is wrapped into an owning RAII class that should provide better control over raw file descriptors. No busy waiting; using ::poll to wait for events while threads are blocked.


# Networking

Architecture of the net is: a non-fully connected mesh. Nodes connect to each other and combine into one big net. Once a node is connected, it can send a trace signal to update all routing caches of other nodes in the network. The fastest path is guaranteed since each node only accepts the first trace package and it arrives by the quickest path, discarding all the next ones to prevent infinite loops. Once the node has announced its presence in the net, anyone can send them a message, and nodes will transfer the message by the path in their cache.

During the connection, a handshake is performed: exchanging a symmetric key using Diffie-Hellman and then encrypting whole messages using XOR. The ID of the node is also shared at this point.


# Example of use

Instance 1 - o
./oldcom -p 10100 -n 1

Instance 2 - x
./oldcom -p 10101 -n 2

Commands to run (in this order):

o connect 127.0.0.1:10101
x connect 127.0.0.1:10100
o trace
x trace
o send 2 Hello, world!

Adding more nodes to the network should be trivial.


### Points of interest

- src/backend.h : Backend class - Multithreading, non-bussy waiting, processing incoming/outcoming data.
- src/foxtunnel.h : FoxTunnel - Basic, custom protocol.
- src/foxpackage.h : FoxPackage - Implementation of processing packages over the net.


# Conclusions

There is definitely a lot of stuff that can be improved in the code. A basic Socket wrapper is nice, but gets in the way once you start doing server-like logic. You need to make something that allows for easier use in both cases: client and server. The owning style for FoxTunnel definitely makes it harder to manage a multitude of connections and to switch between protocols. Less coupling between the two should make server-side implementation easier. The networking side, while *workable*, needs a lot more complexity to be robust: improve routing, add a way to ensure message delivery, and create some kind of authorization.
