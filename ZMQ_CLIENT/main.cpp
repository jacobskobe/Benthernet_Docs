#include <iostream>
#include <string>
#include <vector>
#include <zmq.hpp>
#include <chrono>
#include <thread>
#include <atomic> // For std::atomic_bool to control the chat thread

// Helper function to check if a message starts with a specific topic
bool startsWith(const std::string& fullString, const std::string& prefix) {
    return fullString.rfind(prefix, 0) == 0;
}

// Global atomic boolean to signal the chat listener thread to stop
std::atomic_bool stopChatListener(false);

// Forward declaration for the chat listener thread function
// This thread will now use its own dedicated SUB socket.
void chatListenerThread(zmq::socket_t& chatSubSocket, const std::string& currentChannel, const std::string& userName);

class ZMQClient {
private:
    zmq::context_t context;
    zmq::socket_t pushSocket; // This pushes messages to the server's PULL socket
    zmq::socket_t serviceSubSocket; // NEW: Dedicated socket for service replies
    zmq::socket_t chatSubSocket;    // NEW: Dedicated socket for chat messages

    std::string userName; // Client's chosen name (e.g., "kobe")
    std::string channel;
    std::string password;
    std::string generatedUsername; // The username assigned by the server (e.g., "User_asdf123")
    std::vector<std::string> gamesToPlay;

public:
    ZMQClient(const std::string& user, const std::string& chan)
        : context(1),
        pushSocket(context, zmq::socket_type::push),
        serviceSubSocket(context, zmq::socket_type::sub), // Initialize dedicated service SUB
        chatSubSocket(context, zmq::socket_type::sub),     // Initialize dedicated chat SUB
        userName(user), channel(chan)
    {
        // Connect PUSH socket to server's PULL socket
        pushSocket.connect("tcp://localhost:24041"); // Or "tcp://benternet.pxl-ea-ict.be:24041"

        // Connect BOTH SUB sockets to the server's PUB socket
        serviceSubSocket.connect("tcp://localhost:24042"); // Or "tcp://benternet.pxl-ea-ict.be:24042"
        chatSubSocket.connect("tcp://localhost:24042");    // Or "tcp://benternet.pxl-ea-ict.be:24042"

        // Subscribe serviceSubSocket to all relevant service topics
        serviceSubSocket.setsockopt(ZMQ_SUBSCRIBE, "service>username!>", 17);
        serviceSubSocket.setsockopt(ZMQ_SUBSCRIBE, "service>password!>", 17);
        serviceSubSocket.setsockopt(ZMQ_SUBSCRIBE, "service>login!>", 14);
        serviceSubSocket.setsockopt(ZMQ_SUBSCRIBE, "service>game!>", 12);
        serviceSubSocket.setsockopt(ZMQ_SUBSCRIBE, "service>clients!>", 16);
        serviceSubSocket.setsockopt(ZMQ_SUBSCRIBE, "service>logout!>", 15);

        // Set a default timeout for the serviceSubSocket
        serviceSubSocket.setsockopt(ZMQ_RCVTIMEO, 2000); // 2000 ms = 2 seconds timeout for service receives
    }

    void sendMessage(const std::string& msg) {
        pushSocket.send(msg.c_str(), msg.size(), 0);
    }

    // This method now exclusively uses serviceSubSocket
    std::string receiveSpecificMessage(const std::string& expectedTopicPrefix, int timeoutMs = 5000) {
        serviceSubSocket.setsockopt(ZMQ_RCVTIMEO, timeoutMs); // Temporarily set timeout

        zmq::message_t reply;
        std::string fullResponse;

        // This loop will retry receiving until it gets the expected message or times out.
        // It no longer needs to worry about discarding chat messages.
        while (true) {
            if (serviceSubSocket.recv(&reply, 0)) { // Blocking receive with timeout
                fullResponse = std::string(static_cast<char*>(reply.data()), reply.size());
                std::cout << "[Client Debug] Received (Service Socket): " << fullResponse << std::endl;

                if (startsWith(fullResponse, expectedTopicPrefix)) {
                    serviceSubSocket.setsockopt(ZMQ_RCVTIMEO, 2000); // Reset timeout
                    return fullResponse;
                } else {
                    // This case indicates an unexpected service message, or a misconfigured subscription.
                    // It should be rare if subscriptions are correct.
                    std::cout << "[Client Debug] Discarding unexpected service message (doesn't start with '"
                              << expectedTopicPrefix << "'): " << fullResponse << std::endl;
                    // Continue waiting for the correct message
                }
            } else {
                int error = zmq_errno();
                if (error == ETIMEDOUT) {
                    std::cerr << "[Client] Timeout while waiting for " << expectedTopicPrefix << std::endl;
                } else {
                    std::cerr << "[Client] ZMQ Receive Error on Service Socket: " << zmq_strerror(error) << std::endl;
                }
                serviceSubSocket.setsockopt(ZMQ_RCVTIMEO, 2000); // Reset timeout
                return ""; // Return empty string on timeout or error
            }
        }
    }

    void registerUser() {
        std::string regMsg = "service>username?>" + userName + "|" + channel;
        std::cout << "[Client] Sending registration: " << regMsg << std::endl;
        sendMessage(regMsg);
        std::string response = receiveSpecificMessage("service>username!>");
        if (!response.empty()) {
            std::cout << "[Client] Final Response: " << response << std::endl;
            size_t start = response.find("geregistreerd als: ") + strlen("geregistreerd als: ");
            size_t end = response.find(">", start);
            if (start != std::string::npos && end != std::string::npos) {
                generatedUsername = response.substr(start, end - start);
                std::cout << "[Client] Server assigned username: " << generatedUsername << std::endl;
            }
        } else {
            std::cout << "[Client] Failed to receive expected registration response." << std::endl;
        }
    }

    void requestPassword() {
        int length;
        std::cout << "Geef gewenste lengte van het wachtwoord (minimaal 10 tekens): ";
        std::cin >> length;
        if (length < 10) {
            std::cout << "Wachtwoord moet minimaal 10 tekens zijn. Lengte wordt op 10 gezet." << std::endl;
            length = 10;
        }
        std::string passReq = "service>password?>" + userName + "|" + std::to_string(length);
        std::cout << "[Client] Sending password request: " << passReq << std::endl;
        sendMessage(passReq);
        std::string response = receiveSpecificMessage("service>password!>");
        if (!response.empty()) {
            std::cout << "[Client] Final Response: " << response << std::endl;
            size_t pwPos = response.find("Je wachtwoord is: ");
            if (pwPos != std::string::npos) {
                password = response.substr(pwPos + 18);
                if (!password.empty() && password.back() == '>') password.pop_back();
            }
        } else {
            std::cout << "[Client] Failed to receive expected password response." << std::endl;
        }
    }

    bool login() {
        std::string loginReq = "service>login?>" + userName + "|" + password;
        std::cout << "[Client] Sending login request: " << loginReq << std::endl;
        sendMessage(loginReq);
        std::string response = receiveSpecificMessage("service>login!>");
        if (!response.empty()) {
            std::cout << "[Client] Final Response: " << response << std::endl;
            bool success = response.find("Succesvol ingelogd") != std::string::npos;
            if (success) {
                // If login successful, we assume the server associated our userName with a generatedUsername.
                // We *must* have generatedUsername from registration for chat to work.
                // If client restarted and didn't register, this will be empty.
                // A robust system would ask for generatedUsername explicitly during login if not found.
            }
            return success;
        } else {
            std::cout << "[Client] Failed to receive expected login response." << std::endl;
            return false;
        }
    }

    void logout() {
        std::string logoutMsg = "service>logout?>" + userName;
        std::cout << "[Client] Sending logout request: " << logoutMsg << std::endl;
        sendMessage(logoutMsg);
        std::string response = receiveSpecificMessage("service>logout!>");
        if (!response.empty()) {
            std::cout << "[Client] Final Response: " << response << std::endl;
            std::cout << "Uitgelogd." << std::endl;
            password.clear();
            generatedUsername.clear(); // Clear generated username on logout
        } else {
            std::cout << "[Client] Failed to receive expected logout response." << std::endl;
        }
    }

    void requestRandomGame() {
        std::string gameReq = "service>game?>" + userName + "|" + channel;
        std::cout << "[Client] Requesting random game: " << gameReq << std::endl;
        sendMessage(gameReq);
        std::string response = receiveSpecificMessage("service>game!");
        if (!response.empty()) {
            std::cout << "[Client] Final Response: " << response << std::endl;
            size_t pos = response.find("Random game is: ");
            if (pos != std::string::npos) {
                std::string game = response.substr(pos + 16);
                if (!game.empty() && game.back() == '>') game.pop_back();
                std::cout << "Wil je deze game toevoegen aan je lijst om later te spelen? (j/n): ";
                char keuze;
                std::cin >> keuze;
                if (keuze == 'j' || keuze == 'J') {
                    gamesToPlay.push_back(game);
                    std::cout << "Game '" << game << "' toegevoegd aan je lijst.\n";
                }
            }
        } else {
            std::cout << "[Client] Failed to receive expected game response." << std::endl;
        }
    }

    void showGamesToPlay() {
        if (gamesToPlay.empty()) {
            std::cout << "Je lijst met games om later te spelen is leeg.\n";
        } else {
            std::cout << "Games om later te spelen:\n";
            for (size_t i = 0; i < gamesToPlay.size(); ++i) {
                std::cout << i + 1 << ". " << gamesToPlay[i] << '\n';
            }
        }
    }

    void requestClientList() {
        std::string clientListReq = "service>clients?>";
        std::cout << "[Client] Requesting client list: " << clientListReq << std::endl;
        sendMessage(clientListReq);
        std::string response = receiveSpecificMessage("service>clients!>");
        if (!response.empty()) {
            std::cout << "[Client] Final Response: " << response << std::endl;
            size_t pos = response.find(">");
            if (pos != std::string::npos) {
                std::string clientInfo = response.substr(pos + 1);
                if (!clientInfo.empty() && clientInfo.back() == '>') clientInfo.pop_back();
                std::cout << "Geregistreerde clients op de server: " << clientInfo << std::endl;
            }
        } else {
            std::cout << "[Client] Failed to receive expected client list response." << std::endl;
        }
    }

    void enterChatroom() {
        if (generatedUsername.empty()) {
            std::cout << "Je moet eerst registreren en inloggen om de chatroom te betreden." << std::endl;
            return;
        }

        std::cout << "\n--- Welkom in de chatroom (Kanaal: " << channel << ") ---\n";
        std::cout << "Type je bericht en druk op Enter. Type 'exit' om de chat te verlaten.\n";

        // Subscribe chatSubSocket to the specific channel for chat messages
        std::string chatTopic = "chat!>" + channel + ">";
        chatSubSocket.setsockopt(ZMQ_SUBSCRIBE, chatTopic.c_str(), chatTopic.length());
        std::cout << "[Client] Subscribed chatSubSocket to chat topic: " << chatTopic << std::endl;

        stopChatListener.store(false); // Reset atomic flag for new chat session
        // Pass a reference to chatSubSocket and the channel to the thread
        std::thread listener(chatListenerThread, std::ref(chatSubSocket), channel, generatedUsername);

        std::string chatInput;
        std::cin.ignore(); // Clear the newline character left by previous cin
        while (true) {
            std::cout << "[" << generatedUsername << " in " << channel << "]> ";
            std::getline(std::cin, chatInput);

            if (chatInput == "exit") {
                std::cout << "Chatroom verlaten.\n";
                break;
            }

            // Send chat message to server
            std::string chatMsg = "chat>" + channel + ">" + generatedUsername + ">" + chatInput;
            sendMessage(chatMsg);
        }

        stopChatListener.store(true); // Signal the listener thread to stop
        if (listener.joinable()) {
            listener.join(); // Wait for the listener thread to finish
            std::cout << "Chat listener thread gestopt.\n";
        }

        // Unsubscribe chatSubSocket from chat topic when leaving chatroom
        chatSubSocket.setsockopt(ZMQ_UNSUBSCRIBE, chatTopic.c_str(), chatTopic.length());
        std::cout << "[Client] Unsubscribed chatSubSocket from chat topic: " << chatTopic << std::endl;
    }


    bool loginFlow() {
        std::cout << "Voer je wachtwoord in: ";
        std::string inputPw;
        std::cin >> inputPw;
        password = inputPw;
        if (login()) {
            bool loggedInMenu = true;
            while (loggedInMenu) {
                std::cout << "\nJe bent ingelogd. Kies een optie:\n";
                std::cout << "1. Vraag random game aan\n";
                std::cout << "2. Toon lijst met games om later te spelen\n";
                std::cout << "3. Toon ingelogde clients\n";
                std::cout << "4. Betreed chatroom\n";
                std::cout << "5. Uitloggen\n";
                std::cout << "Je keuze: ";
                int loggedChoice;
                std::cin >> loggedChoice;
                if (loggedChoice == 1) {
                    requestRandomGame();
                } else if (loggedChoice == 2) {
                    showGamesToPlay();
                } else if (loggedChoice == 3) {
                    requestClientList();
                } else if (loggedChoice == 4) {
                    enterChatroom();
                } else if (loggedChoice == 5) {
                    logout();
                    loggedInMenu = false;
                } else {
                    std::cout << "Ongeldige keuze." << std::endl;
                }
            }
            return true;
        } else {
            std::cout << "Inloggen mislukt. Probeer opnieuw." << std::endl;
            return false;
        }
    }

    void run() {
        bool registered = false;
        bool hasPassword = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        while (true) {
            std::cout << "\nKies een optie:\n";
            // Determine if user has been "registered" in this session,
            // or if they might be a returning user with a known username.
            // For now, "registered" means we successfully got a generatedUsername.
            if (generatedUsername.empty()) { // Check if generatedUsername is known
                std::cout << "1. Registreren\n";
            }
            if (!generatedUsername.empty() && password.empty()) { // Only ask for password if registered but no password
                std::cout << "2. Wachtwoord aanvragen\n";
            }
            std::cout << "3. Inloggen\n";
            std::cout << "4. Afsluiten\n";
            std::cout << "Je keuze: ";
            int choice;
            std::cin >> choice;

            // Updated logic for menu flow based on generatedUsername existence
            if (choice == 1 && generatedUsername.empty()) {
                registerUser();
                // Check if generatedUsername was successfully set by registerUser
                if (!generatedUsername.empty()) {
                    registered = true; // Mark as registered for current session
                }
            } else if (choice == 2 && !generatedUsername.empty() && password.empty()) {
                requestPassword();
                if (!password.empty()) {
                    hasPassword = true;
                }
            } else if (choice == 3) {
                loginFlow();
            } else if (choice == 4) {
                std::cout << "Programma afgesloten." << std::endl;
                return;
            } else {
                std::cout << "Ongeldige keuze, probeer opnieuw." << std::endl;
            }
        }
    }
};

// This function runs in a separate thread to listen for chat messages
void chatListenerThread(zmq::socket_t& chatSubSocket, const std::string& currentChannel, const std::string& currentGeneratedUsername) {
    std::cout << "[Chat Listener] Thread started, listening for messages on channel " << currentChannel << "...\n";
    chatSubSocket.setsockopt(ZMQ_RCVTIMEO, 100); // Set a short timeout for the thread's receive

    while (!stopChatListener.load()) {
        zmq::message_t reply;
        if (chatSubSocket.recv(&reply, 0)) { // Blocking receive with timeout
            std::string fullResponse = std::string(static_cast<char*>(reply.data()), reply.size());

            // Parse the chat message: chat!>channel>sender_username>message_text
            size_t firstSep = fullResponse.find(">"); // "chat!"
            size_t secondSep = fullResponse.find(">", firstSep + 1); // channel
            size_t thirdSep = fullResponse.find(">", secondSep + 1); // sender_username

            if (firstSep != std::string::npos && secondSep != std::string::npos && thirdSep != std::string::npos) {
                std::string receivedChannel = fullResponse.substr(firstSep + 1, secondSep - (firstSep + 1));
                std::string senderUsername = fullResponse.substr(secondSep + 1, thirdSep - (secondSep + 1));
                std::string chatMessageText = fullResponse.substr(thirdSep + 1);

                // Only display if it's for the current channel and not your own sent message
                if (receivedChannel == currentChannel && senderUsername != currentGeneratedUsername) {
                    std::cout << "\n[" << senderUsername << " in " << receivedChannel << "]> " << chatMessageText << "\n";
                    // Reprompt the user after printing the incoming message
                    std::cout << "[" << currentGeneratedUsername << " in " << currentChannel << "]> ";
                    std::cout.flush(); // Ensure prompt is displayed immediately
                }
            } else {
                // This shouldn't happen with correct server broadcasting and chat-specific subscription
                std::cout << "[Chat Listener] Received malformed chat message or unexpected message: " << fullResponse << "\n";
            }
        }
    }
    std::cout << "[Chat Listener] Thread stopped.\n";
}


int main() {
    std::string user, channel;
    std::cout << "Geef je gebruikersnaam op: ";
    std::getline(std::cin, user);

    std::cout << "Geef het kanaal op: ";
    std::getline(std::cin, channel);

    ZMQClient client(user, channel);
    client.run();

    return 0;
}
