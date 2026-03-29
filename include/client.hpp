#ifndef CLIENT_HPP
#define CLIENT_HPP
#include<SFML/Network.hpp>
#include <iostream>
#include<string>
class Client{
private:
    sf::TcpSocket socket;
    std::optional<sf::IpAddress> ip;
    unsigned short port;
public:
    Client(const std::string& ip, unsigned short port){
        this->ip = sf::IpAddress::resolve(ip);
        this->port = port;
    };
    ~Client() = default;
    void connect(){
        if (!ip.has_value()) {
            std::cout << "Invalid IP\n";
            return;
        }
        if (socket.connect(*ip, port) != sf::Socket::Status::Done) {
            std::cout << "Connection failed\n";
            return;
        }
        else{
            std::cout << "Connected to " << ip->toString() << ":" << port << std::endl;
        }
    };
    void send(const std::string& message){
        if (socket.send(message.c_str(), message.size()) != sf::Socket::Status::Done) {
            std::cout << "Send failed\n";
            return;
        }
    };
    std::string receive(){
        char buffer[1024];
        std::size_t received;
        if (socket.receive(buffer, sizeof(buffer), received) == sf::Socket::Status::Done) {
            std::string response(buffer, received);
            std::cout << "Server: " << response << std::endl;
        }
        return "";
    };
};

#endif