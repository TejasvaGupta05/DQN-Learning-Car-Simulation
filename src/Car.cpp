#include "Car.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>
static float toRad(float deg) {
    return deg * std::numbers::pi_v<float> / 180.f;
}
Car::Car(sf::Vector2f startPos, float startAngleDeg, sf::Color color)
    : startPosition(startPos), startAngle(startAngleDeg)
{
    shape.setSize({50.f, 25.f});
    shape.setOrigin({25.f, 12.5f});
    shape.setFillColor(color);
    shape.setOutlineColor(sf::Color(255, 255, 255, 180));
    shape.setOutlineThickness(1.5f);
    reset();
}
void Car::reset() {
    position = startPosition;
    angle    = startAngle;
    speed    = 0.f;
    shape.setPosition(position);
    shape.setRotation(sf::degrees(angle));
}
void Car::update(Action action, float dt) {
    switch (action) {
        case Action::Accelerate:
            speed += ACCELERATION * dt;
            break;
        case Action::Brake:
            speed -= BRAKING * dt;
            break;
        case Action::Left:
            if (speed > 5.f) angle -= TURN_RATE * dt;
            break;
        case Action::Right:
            if (speed > 5.f) angle += TURN_RATE * dt;
            break;
        case Action::None:
            break;
    }
    speed = std::clamp(speed, 0.f, MAX_SPEED);
    speed = std::max(0.f, speed - FRICTION * dt);  // friction
    float rad  = toRad(angle);
    position.x += std::cos(rad) * speed * dt;
    position.y += std::sin(rad) * speed * dt;
    shape.setPosition(position);
    shape.setRotation(sf::degrees(angle));
}
void Car::draw(sf::RenderWindow& window) const {
    window.draw(shape);
}
