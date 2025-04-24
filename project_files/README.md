# 📦 Code Overview: Benthernet Username Service

This section breaks down the implementation of both the **client** and **server** components of the Benthernet Username Service.

---

## ✅ Client Code Explained

### 📄 File: `client.cpp`

```cpp
#include <iostream>
#include <zmq.hpp>
#include <string>

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#define sleep(n)    Sleep(n)
#endif
```
This block sets up includes and cross-platform compatibility for the `sleep()` function.

```cpp
zmq::context_t context{1};
```
Creates a ZeroMQ context that manages sockets.

```cpp
zmq::socket_t push_socket{context, zmq::socket_type::push};
push_socket.connect("tcp://benternet.pxl-ea-ict.be:24041");
```
Connects to the server with a **PUSH** socket for sending requests.

```cpp
zmq::socket_t sub_socket{context, zmq::socket_type::sub};
sub_socket.connect("tcp://benternet.pxl-ea-ict.be:24042");
```
Sets up a **SUB** socket to listen for responses.

```cpp
std::string naam = "Kobe Jacobs";
std::string sub_topic = "service>username!>" + naam + ">";
sub_socket.setsockopt(ZMQ_SUBSCRIBE, sub_topic.c_str(), sub_topic.length());
```
Subscribes to a topic specifically for receiving a personalized username.

```cpp
std::string push_message = "service>username?>" + naam + ">";
push_socket.send(push_message.c_str(), push_message.length(), 0);
```
Sends a request to the server asking to register the user.

```cpp
while (true) {
    zmq::message_t reply;
    sub_socket.recv(&reply, 0);

    std::string antwoord(static_cast<char*>(reply.data()), reply.size());

    std::cout << "Ontvangen reactie: " << antwoord << std::endl;
}
```
The loop listens for responses and prints them out.

---

## ✅ Server Code Explained

### 📄 File: `server.cpp`

```cpp
#include <string>
#include <iostream>
#include <zmq.hpp>
#include <unordered_set>
#include <ctime>
```
Includes required headers. `unordered_set` is used to keep track of registered names.

```cpp
std::string extract_name(const std::string& message) { ... }
```
Extracts the user's name from the incoming request message.

```cpp
std::string generate_random_username() { ... }
```
Generates a unique random username using alphanumeric characters.

```cpp
zmq::context_t context{1};
```
ZeroMQ context object for managing communication.

```cpp
zmq::socket_t push_socket{context, zmq::socket_type::push};
push_socket.connect("tcp://benternet.pxl-ea-ict.be:24041");

zmq::socket_t sub_socket{context, zmq::socket_type::sub};
sub_socket.connect("tcp://benternet.pxl-ea-ict.be:24042");
```
Both sockets are connected (client-side) to receive and respond to messages.

```cpp
std::string topic = "service>username?>";
sub_socket.setsockopt(ZMQ_SUBSCRIBE, topic.c_str(), topic.length());
```
Server listens for all requests to the `username` service.

```cpp
std::unordered_set<std::string> geregistreerde_namen;
```
Maintains a set of already registered names to prevent duplicate logging.

```cpp
while (true) {
    zmq::message_t request;
    sub_socket.recv(&request, 0);

    std::string bericht(static_cast<char*>(request.data()), request.size());
    std::string naam = extract_name(bericht);

    if (geregistreerde_namen.find(naam) == geregistreerde_namen.end()) {
        geregistreerde_namen.insert(naam);
    }

    std::string random_username = generate_random_username();
    std::string antwoord = "service>username!>" + naam + ">je bent geregistreerd als: " + random_username + ">";
    push_socket.send(antwoord.c_str(), antwoord.length(), 0);
}
```
The main loop processes each incoming request, generates a response, and sends it back.

---

## 🔧 Additional Notes
- The project uses ZeroMQ for async communication.
- Topics are structured like a pub-sub service (`username?>` for requests, `username!>` for responses).
- All names are encoded into the message to allow personalized responses.

