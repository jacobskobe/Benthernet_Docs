# 🧠 Username Registration & Game Service API (ZeroMQ over Benthernet)

Welcome to the **Username Registration & Game Service** — a C++ API using **ZeroMQ** for messaging over the **Benthernet** broker. This project allows users to register usernames, request passwords, log in, and receive random game suggestions to build a personal game list.

---

## ⚙️ Requirements

Make sure you have:

- ✅ **Qt Creator 6.8.2** (recommended)
- ✅ C++17 or newer
- ✅ ZeroMQ libraries included in the project (no manual install needed)
- ✅ Internet connection (Benthernet is a remote message broker)

> All libraries are bundled for quick setup.

---

## 🏗️ Setup & Build Instructions

1. Clone or download the project repository.
2. Open the `.pro` files for both **client** and **server** in Qt Creator 6.8.2.
3. Let Qt configure the project automatically.
4. Build both projects.
5. Run the **server** and **client** in separate terminal windows or Qt instances.

---

## 🚀 Running the Application

### Server

- Starts and waits for client subscriptions.
- Handles username registrations, password generation, login validation, and game suggestions.

### Client

- Allows users to:
  - Register with a username and channel.
  - Request a password.
  - Log in with credentials.
  - Receive and optionally save random game recommendations.

---

## 🔄 Communication Protocol

### Message Patterns

- Client sends requests over **PUSH** socket.
- Server listens and responds over **SUB** socket.
- Responses from server use **PUSH** → client listens on **SUB** socket.

### Message Formats

| Direction       | Format                                               | Description                         |
|-----------------|-----------------------------------------------------|-----------------------------------|
| Client → Server | `service>username?>username|channel`                | Register username                 |
| Client → Server | `service>password?>username|length`                  | Request password of given length  |
| Client → Server | `service>login?>username|password`                   | Request login authentication      |
| Client → Server | `service>game?>username|channel`                     | Request random game               |

| Server → Client | Format                                              | Description                      |
|----------------|-----------------------------------------------------|---------------------------------|
| Server → Client | `service>username!>username|channel>confirmation`   | Username registration confirmation |
| Server → Client | `service>password!>username>Je wachtwoord is: <pw>`| Password response                |
| Server → Client | `service>login!>username>Succesvol ingelogd`       | Login success                   |
| Server → Client | `service>game!>username|channel>Random game is: <game>` | Random game suggestion        |

---

## 🧩 Features

- User registration with unique usernames per channel.
- Password generation with customizable length.
- Login authentication.
- Random game suggestions.
- User-managed list of games to play later.
- Saves game lists locally in `games.txt`.

---

## 💻 Usage Overview

1. Register your username with a channel.
2. Request a password (minimum length 10).
3. Log in using your username and password.
4. Request random games and choose whether to save them.
5. View your saved games list.
6. Log out or exit the application.

---

## 🙋 FAQ

**Can I run client and server on the same machine?**  
Yes, just use separate terminals or Qt instances.

**Are usernames unique per channel?**  
Yes, usernames are registered per channel to avoid duplicates.

**How do I save games I like?**  
After receiving a random game, confirm saving it to your local list.

---

## 🧑‍🏫 About

- Developed by: Kobe Jacobs  
- PXL University of Applied Sciences and Arts  
- Network Programming Course  

---

For detailed code explanations and more, check the project files.

