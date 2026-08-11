# A Low-Latency C++ Multiplayer Server

I was writing the netcode for a Unity game I was making with my friends, when I realized that I do not have a very good grasp of how networking actually works.

So, I decided to build tinynet! This project is just for my enjoyment of recreational programming and to broaden my understanding of the topic, so I've avoided using LLMs. While an extraordinarily useful tool, I find that I learn best when I write the code myself.

I attribute most of my work to a few different sources, with the most prominent (while this project is still in its infancy) being Glenn Fiedler's [Game Networking series](https://gafferongames.com/categories/game-networking/).

---

To build and run the client (make sure your working directory is the project's root):

```bash
make client && make run-client
```

The server build is still a work in progress ;)

---

I wanted to add a little section here (mostly for myself) to document some of the decisions I've made on this project and why.

- !

_UDP vs TCP_ is likely something that you may have been wondering about. I opted for UDP here for a couple of reasons.

First of all, UDP is _fast_. We aren't really getting any blocking loops and are able to transmit data at speeds much quicker than TCP. We don't have to concern ourselves with handshakes, we send entire packets at once rather than in segments, and we don't have to wait to acknowledge that the client and server have both fully sent/received packets. As will all things, of course, this comes with a few large tradeoffs; data can be unreliable and out of order; UDP's checksum is unreliable - and so on. However, this project is meant to be as high performance as possible, and TCP simply loses to UDP on that front.

Also, UDP gives me a bit more intimate control over the way that I handle my data. Because of the unstructured nature of the protocol, I can choose how to break up my packets, serialize, deseralize, and so on. I like knowing that I have the low level control over the happenings in my code.

- !

_Packet Integration_! As I write this, I'm at the stage of reading, writing, bitpacking, serializing, etc. all of my packets. When considering how we should be treating packets, I initially thought OOP would be a good choice, as it allows for some clean polymorphism. However, as you begin to think about the scale of your packets, this quickly becomes undesirable from a memory perspective. Deserializing a packet in this case would likely require some heap allocation, which, in a 60+ FPS tick loop with hundreds of packets per frame, will likely fragment the heap (alongside GC overhead) and I have little desire to write a custom pool allocator at the moment.

Right now, the raw data is passed around via the _Buffer_ struct (currently and likely for the forseeable future under clinet/Packets.h). This decouples the logic from Socket, Client, Server, and other relevant networking classes, is inline friendly, and allocates nothing on the heap. It's not amazing to work with from a developer standpoint, though std::variant is an attractive option here.

On a more fundamental level, I need to decide what the job of each layer (Socket/Server, Game Logic, and so on) is. Does Socket want to care if its reading a PlayerInputPacket, or just a buffer? This will take a little more thinking on my end, and I still haven't come to a full conclusion :)

Jaden from a couple hours later here. I think I'm happy with the design I've set out for; we pass around buffers of data, which are prepended with our protocol hash on send and receive. It's important to note that we're working on a byte-by-byte level here; later down the line, an optimization I'm looking to make would be turning this to a bit-by-bit abstraction. It's not particularly relevant unless we're working with massive streams of data, but I do want this to be a _high performance_ project, so it is certainly something on the todo list.

- !
  _Acks_ confused me for a bit here. todo later finish this; but some thoughts before i forget: let's say we're the server, and are about to send a packet to the client. we look at our sequence number. we put that in the packet header. then, we look at
  our received queue for n - 32 and put them all as ack'd in our ack bitfield, which we place in the header. the client receives this packet, looks at their sent queue, and then acks each of the ones we received in the header, takes them off the sent queue, and then resends if a packet hasn't been acked yet newer ones have been

---

Thanks for taking a gander at this repository and hopefully it turns into something valuable for your learning and mine!

- jaden
