#pragma once
#include <SFML/Graphics.hpp>
#include "GameState.hpp"
class Car {
public:
    Car(sf::Vector2f startPos, float startAngleDeg,
        sf::Color color = sf::Color(60, 220, 120));
    // Apply action and advance physics by dt seconds
    void update(Action action, float dt);
    // Reset car to starting position/angle
    void reset();
    // Draw the car rectangle
    void draw(sf::RenderWindow& window) const;
    sf::Vector2f getPosition() const { return position; }
    float        getAngle()    const { return angle; }
    float        getSpeed()    const { return speed; }
private:
    sf::Vector2f      position;
    float             angle;   // degrees; 0=right, 90=down (SFML Y-down)
    float             speed;
    sf::Vector2f      startPosition;
    float             startAngle;
    sf::RectangleShape shape;  // 40 x 20 px rectangle
    static constexpr float MAX_SPEED    = 220.f; // px/s
    static constexpr float ACCELERATION = 160.f;
    static constexpr float BRAKING      = 160.f;
    static constexpr float FRICTION     = 50.f;
    static constexpr float TURN_RATE    = 85.f;  // deg/s
};