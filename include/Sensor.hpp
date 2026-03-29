#pragma once
#include <SFML/Graphics.hpp>
#include <array>

// Forward declarations
class Car;
class Track;

class Sensor {
public:
    static constexpr int   NUM_RAYS  = 5;
    static constexpr float MAX_RANGE = 200.f;

    // Ray angles (degrees) relative to car heading
    // Left=-60, FrontLeft=-30, Front=0, FrontRight=+30, Right=+60
    static constexpr std::array<float, NUM_RAYS> ANGLES = {-60.f, -30.f, 0.f, 30.f, 60.f};

    // Cast all rays against track walls, update distances and hit points
    void update(const Car& car, const Track& track);

    // Draw semi-transparent ray lines and red hit dots
    void draw(sf::RenderWindow& window) const;

    std::array<float, NUM_RAYS> getDistances() const { return distances; }

private:
    // Cast a single ray; returns distance to nearest wall (capped at MAX_RANGE)
    float castRay(sf::Vector2f origin, float angleDeg, const Track& track) const;

    // Parametric ray-segment intersection; returns t>=0 or -1 if none
    float raySegIntersect(sf::Vector2f orig, sf::Vector2f dir,
                          sf::Vector2f a,    sf::Vector2f b) const;

    std::array<float,        NUM_RAYS> distances{};
    std::array<sf::Vector2f, NUM_RAYS> hitPoints{};
    sf::Vector2f carPos{};
};
