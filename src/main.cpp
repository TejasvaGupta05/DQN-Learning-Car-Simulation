// ============================================================
//  RL Car Simulation — main.cpp (3-car parallel learning)
//  Project Juru | SFML 3.x
//
//  Three cars run simultaneously on the same track.
//  Each car:
//    • Has its own sensors, episode counters and start pose
//    • Logs independently to the SAME CSV (3× data collection rate)
//    • Holds its own TCP connection to the inference server (Infer phase)
//
//  PHASE SWITCHING:
//    Phase::Collect  — random actions, log to data/training_data.csv
//    Phase::Infer    — query Python server via TCP, use model action
//
//  CONTROLS:
//    M     — toggle Manual / AI for car 0 only  (others always AI/random)
//    R     — reset ALL cars to start positions
//    ESC   — quit  |  Arrow keys — drive car 0 manually
// ============================================================
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include "Car.hpp"
#include "Track.hpp"
#include "Sensor.hpp"
#include "NetworkClient.hpp"
#include "GameState.hpp"
// ---- Configuration -------------------------------------------------
constexpr int            NUM_CARS      = 1;
constexpr unsigned short SERVER_PORT   = 5000;
constexpr const char*    SERVER_IP     = "127.0.0.1";
constexpr int            MAX_STEPS     = 1200;
constexpr float          CAR_RADIUS    = 19.f;
// Distinct colours for the three cars
constexpr std::array<uint32_t, NUM_CARS> CAR_RGBA = {
    0x3CDC78FF,  // green
};
static sf::Color carColor(int i) {
    uint32_t rgba = CAR_RGBA[i];
    return sf::Color(rgba >> 24, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF, rgba & 0xFF);
}
int main(int argc, char* argv[]) {
    int targetEpisodes = -1;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--episodes" && i + 1 < argc) targetEpisodes = std::stoi(argv[++i]);
    }

    std::srand(static_cast<unsigned>(std::time(nullptr)));
    // ---- Window --------------------------------------------------------
    sf::RenderWindow window(
        sf::VideoMode({1280u, 720u}),
        "RL Learning"
    );
    window.setFramerateLimit(60);
    // ---- Track ---------------------------------------------------------
    Track track;
    auto startPoses = track.getStartPoses(NUM_CARS);
    // ---- Cars & sensors ------------------------------------------------
    std::vector<Car>    cars;
    std::vector<Sensor> sensors(NUM_CARS);
    cars.reserve(NUM_CARS);
    for (int i = 0; i < NUM_CARS; ++i) {
        cars.emplace_back(startPoses[i].first, startPoses[i].second, carColor(i));
        sensors[i].update(cars[i], track);
    }
    // ---- Per-car episode state -----------------------------------------
    std::array<int,          NUM_CARS> stepCounts    = {};
    std::array<int,          NUM_CARS> episodeCounts = {};
    std::array<float,        NUM_CARS> totalRewards  = {};
    std::array<sf::Vector2f, NUM_CARS> prevPositions;
    std::array<float,        NUM_CARS> lastRewards   = {};
    std::array<bool,         NUM_CARS> lastDones     = {};
    for (int i = 0; i < NUM_CARS; ++i)
        prevPositions[i] = startPoses[i].first;
    // ---- Network clients (one per car in Infer phase) ------------------
    std::vector<NetworkClient> netClients(NUM_CARS);
    std::cout << "Connecting " << NUM_CARS << " clients to "
                << SERVER_IP << ":" << SERVER_PORT << " ...\n";
    for (int i = 0; i < NUM_CARS; ++i) {
        if (!netClients[i].connectToServer(SERVER_IP, SERVER_PORT)) {
            std::cerr << "Car " << i << ": connection failed!\n";
            return 1;
        }
        std::cout << "Car " << i << " connected.\n";
    }
    // ---- Font / HUD ----------------------------------------------------
    sf::Font font;
    bool fontLoaded = font.openFromFile("C:/Windows/Fonts/arial.ttf");
    sf::Text hudText(font, "", 14);
    hudText.setFillColor(sf::Color(230, 230, 230));
    hudText.setPosition({10.f, 8.f});
    sf::RectangleShape hudBg({380.f, 92.f});
    hudBg.setFillColor(sf::Color(0, 0, 0, 155));
    hudBg.setPosition({6.f, 4.f});
    // Car-label badges drawn next to each car
    sf::Text labelText(font, "", 12);
    const std::array<std::string, NUM_CARS> labelsStr = {"CAR0"};
    bool manualControl = false;
    int  totalRowsLogged = 0;
    sf::Clock clock;
    std::cout << "[DEBUG] Window opened, starting main loop.\n";
    // ---- Game loop -----------------------------------------------------
    while (window.isOpen()) {
        std::cout << "[DEBUG] Main loop start.\n";
        float dt = std::min(clock.restart().asSeconds(), 0.05f);
        // ---- Events ----
        while (const std::optional<sf::Event> ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) window.close();
            if (const auto* k = ev->getIf<sf::Event::KeyPressed>()) {
                if (k->code == sf::Keyboard::Key::Escape) window.close();
                if (k->code == sf::Keyboard::Key::M) {
                    manualControl = !manualControl;
                    std::cout << "Car0 control: " << (manualControl ? "MANUAL" : "AI") << "\n";
                }
                if (k->code == sf::Keyboard::Key::R) {
                    for (int i = 0; i < NUM_CARS; ++i) {
                        cars[i].reset();
                        sensors[i].update(cars[i], track);
                        stepCounts[i]    = 0;
                        totalRewards[i]  = 0.f;
                        prevPositions[i] = startPoses[i].first;
                    }
                }
            }
        }
        // ---- Per-car update ----
        for (int i = 0; i < NUM_CARS; ++i) {
            // --- Current state from sensors ---
            auto dists  = sensors[i].getDistances();
            CarState st { dists[0], dists[1], dists[2], dists[3], dists[4],
                          cars[i].getSpeed() };
            // --- Choose action ---
            Action action = Action::None;
            if (i == 0 && manualControl) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))    action = Action::Accelerate;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))  action = Action::Brake;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  action = Action::Left;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) action = Action::Right;
            } else {
                std::cout << "[DEBUG] Requesting action from server...\n";
                action = netClients[i].queryAction(st, lastRewards[i], lastDones[i]);
                std::cout << "[DEBUG] Received action.\n";
            }
            // --- Reward ---
            bool crashed  = track.checkCollision(cars[i].getPosition(), CAR_RADIUS);
            sf::Vector2f d = cars[i].getPosition() - prevPositions[i];
            float distDelta = std::sqrt(d.x * d.x + d.y * d.y);
            prevPositions[i] = cars[i].getPosition();
            float reward = computeReward(st, crashed, distDelta);
            totalRewards[i] += reward;
            bool done = crashed || (stepCounts[i] >= MAX_STEPS);
            
            // Save reward and done for next query
            lastRewards[i] = reward;
            lastDones[i]   = done;
            // --- Episode reset or advance ---
            if (done) {
                std::cout << "[Car" << i << " Ep" << ++episodeCounts[i]
                          << "] steps=" << stepCounts[i]
                          << " reward=" << static_cast<int>(totalRewards[i])
                          << (crashed ? " CRASH" : " TIMEOUT") << "\n";
                cars[i].reset();
                sensors[i].update(cars[i], track);
                stepCounts[i]    = 0;
                totalRewards[i]  = 0.f;
                prevPositions[i] = startPoses[i].first;
            } else {
                cars[i].update(action, dt);
                sensors[i].update(cars[i], track);
                ++stepCounts[i];
            }
        }
        // ---- HUD ----
        if (fontLoaded) {
            int totalEps = episodeCounts[0];
            
            if (targetEpisodes > 0 && totalEps >= targetEpisodes) {
                window.close();
                break;
            }
            
            std::string hud = std::string("Phase: ONLINE INFER") +
                "  Car0: " + (manualControl ? "MANUAL" : "AI") + " (M toggle) | R=ResetAll | ESC=Quit\n";
            for (int i = 0; i < NUM_CARS; ++i) {
                hud += "  Car" + std::to_string(i) +
                       ": Ep=" + std::to_string(episodeCounts[i]) +
                       " Step=" + std::to_string(stepCounts[i]) +
                       " Spd=" + std::to_string(static_cast<int>(cars[i].getSpeed())) +
                       " R=" + std::to_string(static_cast<int>(totalRewards[i])) + "\n";
            }
            hud += "  Total eps: " + std::to_string(totalEps);
            hudText.setString(hud);
        }
        // ---- Render ----
        window.clear(sf::Color(25, 80, 30));
        track.draw(window);
        for (int i = 0; i < NUM_CARS; ++i) {
            sensors[i].draw(window);
            cars[i].draw(window);
            // Car label badge
            if (fontLoaded) {
                labelText.setString(labelsStr[i]);
                labelText.setFillColor(carColor(i));
                sf::Vector2f p = cars[i].getPosition();
                labelText.setPosition({p.x - 14.f, p.y - 22.f});
                window.draw(labelText);
            }
        }
        if (fontLoaded) {
            window.draw(hudBg);
            window.draw(hudText);
        }
        window.display();
    }
    // ---- Cleanup ----
    for (int i = 0; i < NUM_CARS; ++i) netClients[i].disconnect();
    return 0;
}
