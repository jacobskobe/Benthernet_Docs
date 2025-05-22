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
#define sleep(n) Sleep(n)
#endif
```
This block includes essential headers and handles cross-platform compatibility for the `sleep()` function.

```cpp
zmq::context_t context{1};
```
Creates a ZeroMQ context, which manages the sockets used for messaging.

```cpp
zmq::socket_t push_socket{context, zmq::socket_type::push};
push_socket.connect("tcp://benternet.pxl-ea-ict.be:24041");
```
Initializes and connects a **PUSH** socket to send requests to the Benthernet server.

```cpp
zmq::socket_t sub_socket{context, zmq::socket_type::sub};
sub_socket.connect("tcp://benternet.pxl-ea-ict.be:24042");
```
Sets up a **SUB** socket to subscribe and listen for server responses.

```cpp
std::string naam = "Kobe Jacobs";
std::string sub_topic = "service>username!>" + naam + ">";
sub_socket.setsockopt(ZMQ_SUBSCRIBE, sub_topic.c_str(), sub_topic.length());
```
Subscribes to a topic tailored to the username, ensuring the client only receives relevant messages.

```cpp
std::string push_message = "service>username?>" + naam + ">";
push_socket.send(push_message.c_str(), push_message.length(), 0);
```
Sends a username registration request to the server.

```cpp
while (true) {
    zmq::message_t reply;
    sub_socket.recv(&reply, 0);

    std::string antwoord(static_cast<char*>(reply.data()), reply.size());

    std::cout << "Ontvangen reactie: " << antwoord << std::endl;
}
```
Continuously listens for server responses and outputs them to the console.

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
Includes necessary headers; `unordered_set` helps track registered usernames to prevent duplicates.

```cpp
std::string extract_name(const std::string& message) { ... }
```
Extracts the username from incoming client requests.

```cpp
std::string generate_random_username() { ... }
```
Generates a unique, random username with alphanumeric characters.

```cpp
zmq::context_t context{1};
```
Creates a ZeroMQ context for socket management.

```cpp
zmq::socket_t push_socket{context, zmq::socket_type::push};
push_socket.connect("tcp://benternet.pxl-ea-ict.be:24041");

zmq::socket_t sub_socket{context, zmq::socket_type::sub};
sub_socket.connect("tcp://benternet.pxl-ea-ict.be:24042");
```
Both sockets connect to the Benthernet endpoints: **PUSH** to send replies, **SUB** to receive client requests.

```cpp
std::string topic = "service>username?>";
sub_socket.setsockopt(ZMQ_SUBSCRIBE, topic.c_str(), topic.length());
```
Subscribes to all username registration requests.

```cpp
std::unordered_set<std::string> geregistreerde_namen;
```
Keeps a set of registered usernames to avoid duplicates.

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
The main server loop listens for requests, processes them by generating a username, stores the name, and sends back confirmation.

---

## 🔧 Additional Notes

- The project uses **ZeroMQ** for asynchronous communication.
- Message topics follow a pub-sub pattern: requests end with `?>`, responses with `!>`.
- Personalization is achieved by encoding usernames in the message topics.
- This design supports scalability and multiple concurrent clients.
