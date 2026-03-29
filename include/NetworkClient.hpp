#pragma once
#include <SFML/Network.hpp>
#include <optional>
#include <string>
#include "GameState.hpp"

// Wraps a TcpSocket for the C++<->Python JSON protocol.
// Messages are newline-delimited ASCII JSON.
class NetworkClient {
public:
    NetworkClient()  = default;
    ~NetworkClient() = default;

    // Connect to inference server. Returns false on failure.
    bool connectToServer(const std::string& ip, unsigned short port);

    void disconnect();

    // Send CarState as JSON, receive action int, return as Action enum.
    // Returns Action::None if disconnected.
    Action queryAction(const CarState& state, float reward, bool done);

    bool isConnected() const { return connected; }

private:
    std::string buildStateJson(const CarState& state, float reward, bool done) const;
    Action      parseActionJson(const std::string& json) const;

    sf::TcpSocket socket;
    bool          connected = false;
};
