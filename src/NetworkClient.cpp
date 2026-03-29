#include "NetworkClient.hpp"
#include <sstream>
#include <iomanip>

bool NetworkClient::connectToServer(const std::string& ip, unsigned short port) {
    auto resolved = sf::IpAddress::resolve(ip);
    if (!resolved.has_value()) return false;

    if (socket.connect(*resolved, port) != sf::Socket::Status::Done)
        return false;

    socket.setBlocking(true);
    connected = true;
    return true;
}

void NetworkClient::disconnect() {
    socket.disconnect();
    connected = false;
}

Action NetworkClient::queryAction(const CarState& state, float reward, bool done) {
    if (!connected) return Action::None;

    // Build and send JSON with newline delimiter
    std::string msg = buildStateJson(state, reward, done) + "\n";
    if (socket.send(msg.c_str(), msg.size()) != sf::Socket::Status::Done) {
        connected = false;
        return Action::None;
    }

    // Read response one byte at a time until newline
    std::string response;
    while (true) {
        char c;
        std::size_t rec = 0;
        if (socket.receive(&c, 1, rec) != sf::Socket::Status::Done) {
            connected = false;
            return Action::None;
        }
        if (c == '\n') break;
        response += c;
    }

    return parseActionJson(response);
}

std::string NetworkClient::buildStateJson(const CarState& s, float reward, bool done) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    // Normalize state for Neural Network (DQN)
    // Sensor MAX_RANGE is 200.f, Car MAX_SPEED is 220.f
    oss << "{\"state\":["
        << s.distLeft / 200.f      << ","
        << s.distFrontLeft / 200.f << ","
        << s.distFront / 200.f     << ","
        << s.distFrontRight / 200.f<< ","
        << s.distRight / 200.f     << ","
        << s.velocity / 220.f      << "],"
        << "\"reward\":" << reward << ","
        << "\"done\":" << (done ? "true" : "false") << "}";
    return oss.str();
}

Action NetworkClient::parseActionJson(const std::string& json) const {
    // Look for "action": N
    auto pos = json.find("\"action\":");
    if (pos == std::string::npos) return Action::None;
    pos += 9;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':')) ++pos;
    try {
        int id = std::stoi(json.substr(pos));
        if (id >= 0 && id <= 3) return static_cast<Action>(id);
    } catch (...) {}
    return Action::None;
}
