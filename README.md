
# 🧠 Username Registration API (ZeroMQ over Benthernet)

Welcome to the **Username Registration Service** — a simple API that registers a user-provided name and returns a randomly generated username. It's built in C++ using **ZeroMQ** and tested in **Qt Creator 6.8.2**. Communication happens via the **Benthernet** message broker.

---

## ⚙️ Requirements

Before you begin, make sure you have the following:

- ✅ **Qt Creator 6.8.2** (required)
- ✅ C++17 or later
- ✅ **No need to install ZeroMQ manually** – all required libraries are **included in the project files**.
- ✅ Internet connection (Benthernet is a remote broker)

> 📦 **ZeroMQ and all required libraries are already bundled** with the project. You can clone and run the project right away.

---

## 🏗️ Project Setup

1. **Download or clone the project folder**.
2. Open the `.pro` file (for both client and server) in **Qt Creator 6.8.2**.
3. Wait for Qt to auto-detect the kits and configure everything.
4. Build the project using the green **"Run"** button.

> ⚠️ The **client and server** are separate apps and **must be run in separate terminals or windows**.

---

## 🚀 Running the Application

### 🖥️ Step 1: Run the Server

1. Open the **server-side project** in Qt.
2. Build and run it.
3. Terminal will print:  
   ```
   Service actief: wacht op subscribers...
   ```
4. The server is now listening for incoming client messages.

---

### 👤 Step 2: Run the Client

1. Open the **client-side project** in a new Qt window.
2. Edit your name in the following line:
   ```cpp
   std::string naam = "Your Name Here";
   ```
3. Build and run.
4. Terminal will print something like:
   ```
   Verzonden: service>username?>Your Name Here>
   Ontvangen reactie: service>username!>Your Name Here>je bent geregistreerd als: User_AbC123xy>
   ```

---

## 🔄 Message Format

### ➡️ Client → Server

```text
service>username?>{your name}>
```

| Field          | Description             |
|----------------|-------------------------|
| `{your name}`  | Your chosen display name |

---

### ⬅️ Server → Client

```text
service>username!>{your name}>je bent geregistreerd als: {generated_username}>
```

| Field                   | Description                             |
|-------------------------|-----------------------------------------|
| `{your name}`           | Matches the name you submitted          |
| `{generated_username}`  | Random system-generated identifier      |

---

## 🧠 How It Works

- The client sends a message via **PUSH** socket to register a name.
- The server listens via a **SUB** socket for messages like:
  ```text
  service>username?>John Doe>
  ```
- If the name is new:
  - It generates a random username like `User_X8r1Tz7p`
  - Sends it back to the client using **PUSH** → **SUB**
- The client receives this via its **SUB** socket and prints it.

---

## ✨ Features

- ✅ One-time registration check (avoids duplicates)
- ✅ Random username generation
- ✅ Terminal logging for easy debugging
- ✅ Clean ZeroMQ pattern usage: PUSH + SUB

---

## 📦 Demo Output

### 📤 Client Output
```text
Verzonden: service>username?>Kobe Jacobs>
Ontvangen reactie: service>username!>Kobe Jacobs>je bent geregistreerd als: User_x7FQ2t8p>
```

### 📥 Server Output
```text
Verstuur bericht naar client: service>username!>Kobe Jacobs>je bent geregistreerd als: User_x7FQ2t8p>
Nieuwe subscriber: Kobe Jacobs (#1), gegeven gebruikersnaam: User_x7FQ2t8p
```

---

## 🙋 Frequently Asked Questions

**Q: Can I run both client and server on one PC?**  
Yes! Just open two separate Qt instances or terminal windows and run both apps.

**Q: Will everyone get a different username?**  
Yes, usernames are generated randomly per registration.

**Q: What if I register the same name twice?**  
You’ll still only be counted once, and won’t receive a new username.

---

## 🧑‍🏫 Project Info

- 📚 **Project by:** Kobe Jacobs  
- 🏫 **School:** PXL University of Applied Sciences and Arts  
- 💻 **Course:** Network Programming

---

## 📖 Additional Resources

For a deeper understanding of the code and logic used in this project, you can check out the detailed **[Code Explanation](project_files/README.md)** document located in the `project_files` subfolder.
