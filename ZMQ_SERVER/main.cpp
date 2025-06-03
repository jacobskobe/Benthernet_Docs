#include <string>
#include <iostream>
#include <zmq.hpp>
#include <unordered_map>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm> // For std::remove
#include <mutex>     // For protecting shared data in UserManager if multithreaded later

// Using a mutex for thread-safety in UserManager, though for this single-threaded server,
// it's not strictly necessary yet, but good practice if you expand later.
std::mutex userManagerMutex;

class UserManager {
private:
    std::unordered_map<std::string, std::string> registeredUsers; // key = name|channel, value = generatedUsername
    std::unordered_map<std::string, std::string> passwords;       // key = generatedUsername, value = password
    std::unordered_map<std::string, bool> loggedInUsers; // key = generatedUsername, value = true (logged in)
    // Add a map to store the channel for each logged-in generated username
    std::unordered_map<std::string, std::string> userChannels; // key = generatedUsername, value = channel

public:
    bool isUserRegistered(const std::string& key) const {
        std::lock_guard<std::mutex> lock(userManagerMutex);
        return registeredUsers.find(key) != registeredUsers.end();
    }

    std::string getRegisteredUsername(const std::string& key) const {
        std::lock_guard<std::mutex> lock(userManagerMutex);
        auto it = registeredUsers.find(key);
        if (it != registeredUsers.end()) return it->second;
        return "";
    }

    void registerUser(const std::string& key, const std::string& username, const std::string& channel) {
        std::lock_guard<std::mutex> lock(userManagerMutex);
        registeredUsers[key] = username;
        userChannels[username] = channel; // Store the channel
    }

    void setPassword(const std::string& username, const std::string& password) {
        std::lock_guard<std::mutex> lock(userManagerMutex);
        passwords[username] = password;
    }

    bool verifyPassword(const std::string& username, const std::string& password) const {
        std::lock_guard<std::mutex> lock(userManagerMutex);
        auto it = passwords.find(username);
        return it != passwords.end() && it->second == password;
    }

    std::string findUserKeyByName(const std::string& name) const {
        std::lock_guard<std::mutex> lock(userManagerMutex);
        for (const auto& pair : registeredUsers) {
            if (pair.first.rfind(name + "|", 0) == 0) {
                return pair.first;
            }
        }
        return "";
    }

    // Helper to get the generated username from the client's provided name
    std::string getGeneratedUsernameFromName(const std::string& name) const {
        std::lock_guard<std::mutex> lock(userManagerMutex);
        for (const auto& pair : registeredUsers) {
            if (pair.first.rfind(name + "|", 0) == 0) {
                return pair.second; // Return the generated username
            }
        }
        return "";
    }


    void userLoggedIn(const std::string& username) {
        std::lock_guard<std::mutex> lock(userManagerMutex);
        loggedInUsers[username] = true;
    }

    void userLoggedOut(const std::string& username) {
        std::lock_guard<std::mutex> lock(userManagerMutex);
        loggedInUsers.erase(username);
    }

    std::vector<std::string> getLoggedInUsers() const {
        std::lock_guard<std::mutex> lock(userManagerMutex);
        std::vector<std::string> users;
        for (const auto& pair : loggedInUsers) {
            if (pair.second) {
                users.push_back(pair.first);
            }
        }
        return users;
    }

    std::string getUserChannel(const std::string& username) const {
        std::lock_guard<std::mutex> lock(userManagerMutex);
        auto it = userChannels.find(username);
        if (it != userChannels.end()) {
            return it->second;
        }
        return ""; // Or throw an exception if user not found
    }
};

// Helper function to extract parts from messages like service>username?>name|channel
std::string extractNameAndChannel(const std::string& message, std::string& channel) {
    std::size_t pos = message.find("?>");
    if (pos != std::string::npos) {
        std::string part = message.substr(pos + 2);
        std::size_t sep = part.find("|");
        if (sep != std::string::npos) {
            channel = part.substr(sep + 1);
            return part.substr(0, sep); // Returns the name
        }
        return part; // If no channel specified, return name
    }
    return "";
}

// Helper function to extract name and password from login/password requests
std::string extractNameAndPassword(const std::string& message, std::string& passwordOrLength) {
    std::size_t pos = message.find("?>");
    if (pos != std::string::npos) {
        std::string part = message.substr(pos + 2);
        std::size_t sep = part.find("|");
        if (sep != std::string::npos) {
            passwordOrLength = part.substr(sep + 1);
            return part.substr(0, sep); // Returns the name
        }
        return part; // If no password/length, return name
    }
    return "";
}

// Helper function to extract parts from chat messages
std::string extractChatInfo(const std::string& message, std::string& channel, std::string& senderUsername) {
    // Expected format: chat>channel>sender_username>message_text
    std::size_t firstSep = message.find(">");
    if (firstSep == std::string::npos) return ""; // "chat"
    std::size_t secondSep = message.find(">", firstSep + 1);
    if (secondSep == std::string::npos) return ""; // channel
    std::size_t thirdSep = message.find(">", secondSep + 1);
    if (thirdSep == std::string::npos) return ""; // sender_username

    if (message.substr(0, firstSep) != "chat") return ""; // Ensure it's a chat message

    channel = message.substr(firstSep + 1, secondSep - (firstSep + 1));
    senderUsername = message.substr(secondSep + 1, thirdSep - (secondSep + 1));
    return message.substr(thirdSep + 1); // Returns the actual message text
}


std::string generateRandomUsername() {
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string username;
    for (int i = 0; i < 8; ++i) {
        username += chars[rand() % chars.size()];
    }
    return "User_" + username;
}

std::string generateRandomPassword(int length) {
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    std::string password;
    for (int i = 0; i < length; ++i) {
        password += chars[rand() % chars.size()];
    }
    return password;
}

int main() {
    srand((unsigned int)time(nullptr));

    zmq::context_t context{1};
    zmq::socket_t pullSocket{context, zmq::socket_type::pull};
    pullSocket.bind("tcp://*:24041");

    zmq::socket_t pubSocket{context, zmq::socket_type::pub};
    pubSocket.bind("tcp://*:24042");

    UserManager userManager;

    std::vector<std::string> games = {
        "The Legend of Zelda: Breath of the Wild", "Minecraft", "Among Us", "Fortnite",
        "Overwatch", "Celeste", "Hades", "Stardew Valley", "Dark Souls", "GTA V",
        "COD Zombies", "Warzone", "Pacman", "Tetris", "League of Legends", "SOGGY BISCUIT"
    };

    std::cout << "Service actief: wacht op client requests..." << std::endl;

    while (true) {
        zmq::message_t request;
        pullSocket.recv(&request, 0);

        std::string message(static_cast<char*>(request.data()), request.size());
        std::cout << "[Server] Received: " << message << std::endl;

        if (message.rfind("service>username?>", 0) == 0) {
            std::string channel;
            std::string name = extractNameAndChannel(message, channel);
            if (name.empty() || channel.empty()) {
                std::cerr << "[Server] Ongeldig username bericht" << std::endl;
                continue;
            }

            std::string generatedUsername = generateRandomUsername();
            userManager.registerUser(name + "|" + channel, generatedUsername, channel); // Pass channel here

            std::string reply = "service>username!>" + name + "|" + channel + ">je bent geregistreerd als: " + generatedUsername + ">";
            std::cout << "Verstuur bericht naar client: " << reply << std::endl;
            pubSocket.send(reply.c_str(), reply.size(), 0);

        } else if (message.rfind("service>password?>", 0) == 0) {
            std::string lengthStr;
            std::string name = extractNameAndPassword(message, lengthStr);
            if (name.empty()) {
                std::cerr << "[Server] Ongeldig password bericht" << std::endl;
                continue;
            }
            int pwLength = std::atoi(lengthStr.c_str());
            if (pwLength <= 0) pwLength = 8;

            std::string genUsername = userManager.getGeneratedUsernameFromName(name);
            if (genUsername.empty()) {
                std::cerr << "[Server] Geen geregistreerde gebruiker gevonden voor wachtwoordaanvraag: " << name << std::endl;
                // Send an error reply to client
                std::string reply = "service>password!>" + name + ">Fout: Gebruiker niet gevonden. Registreer eerst.>";
                pubSocket.send(reply.c_str(), reply.size(), 0);
                continue;
            }

            std::string genPassword = generateRandomPassword(pwLength);
            userManager.setPassword(genUsername, genPassword);

            std::string reply = "service>password!>" + name + "|" + lengthStr + ">Je wachtwoord is: " + genPassword + ">";
            std::cout << "Verstuur wachtwoord naar client: " << reply << std::endl;
            pubSocket.send(reply.c_str(), reply.size(), 0);

        } else if (message.rfind("service>login?>", 0) == 0) {
            std::string providedPassword;
            std::string name = extractNameAndPassword(message, providedPassword);
            if (name.empty()) {
                std::cerr << "[Server] Ongeldig login bericht" << std::endl;
                continue;
            }

            std::string genUsername = userManager.getGeneratedUsernameFromName(name);
            if (genUsername.empty()) {
                std::string reply = "service>login!>" + name + ">Gebruiker niet gevonden>";
                pubSocket.send(reply.c_str(), reply.size(), 0);
                continue;
            }

            if (!userManager.verifyPassword(genUsername, providedPassword)) {
                std::string reply = "service>login!>" + name + ">Wachtwoord ongeldig>";
                pubSocket.send(reply.c_str(), reply.size(), 0);
                continue;
            }

            userManager.userLoggedIn(genUsername);
            std::string reply = "service>login!>" + name + ">Succesvol ingelogd>";
            pubSocket.send(reply.c_str(), reply.size(), 0);

        } else if (message.rfind("service>logout?>", 0) == 0) {
            std::string name = message.substr(strlen("service>logout?>"));

            std::string genUsername = userManager.getGeneratedUsernameFromName(name);
            if (!genUsername.empty()) {
                userManager.userLoggedOut(genUsername);
                std::cout << "[Server] User " << genUsername << " logged out." << std::endl;
                std::string reply = "service>logout!>" + name + ">Uitgelogd>";
                pubSocket.send(reply.c_str(), reply.size(), 0);
            } else {
                std::cerr << "[Server] Could not find user to log out: " << name << std::endl;
                std::string reply = "service>logout!>" + name + ">Fout bij uitloggen: gebruiker niet gevonden>";
                pubSocket.send(reply.c_str(), reply.size(), 0);
            }

        } else if (message.rfind("service>game?>", 0) == 0) {
            std::string data = message.substr(strlen("service>game?>"));
            std::string username_and_channel = data; // This is the original name|channel from client

            std::string randomGame = games[rand() % games.size()];

            std::string reply = "service>game!>" + username_and_channel + ">Random game is: " + randomGame + ">";
            std::cout << "Verstuur random game naar client: " << reply << std::endl;
            pubSocket.send(reply.c_str(), reply.size(), 0);

        } else if (message.rfind("service>clients?>", 0) == 0) {
            std::vector<std::string> loggedInUsers = userManager.getLoggedInUsers();
            std::string clientList = "service>clients!>";
            if (loggedInUsers.empty()) {
                clientList += "Geen clients momenteel ingelogd.";
            } else {
                clientList += "Ingelogde clients: ";
                for (size_t i = 0; i < loggedInUsers.size(); ++i) {
                    clientList += loggedInUsers[i];
                    if (i < loggedInUsers.size() - 1) {
                        clientList += ", ";
                    }
                }
            }
            clientList += ">";
            std::cout << "[Server] Sending client list: " << clientList << std::endl;
            pubSocket.send(clientList.c_str(), clientList.size(), 0);

        } else if (message.rfind("chat>", 0) == 0) { // NEW: Handle incoming chat messages
            std::string channel, senderUsername, chatMessageText;
            chatMessageText = extractChatInfo(message, channel, senderUsername);

            if (channel.empty() || senderUsername.empty() || chatMessageText.empty()) {
                std::cerr << "[Server] Ongeldig chat bericht: " << message << std::endl;
                // Optionally send an error back to the sender
                continue;
            }

            // The server re-publishes the chat message to all subscribers of that channel
            std::string chatBroadcast = "chat!>" + channel + ">" + senderUsername + ">" + chatMessageText;
            std::cout << "[Server] Broadcasting chat: " << chatBroadcast << std::endl;
            pubSocket.send(chatBroadcast.c_str(), chatBroadcast.size(), 0);

        } else {
            std::cerr << "[Server] Onbekend bericht: " << message << std::endl;
        }
    }

    return 0;
}
