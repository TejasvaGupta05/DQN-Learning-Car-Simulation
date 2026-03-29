#include "Sensor.hpp"
#include "Car.hpp"
#include "Track.hpp"
#include <cmath>
#include <numbers>
#include <limits>

static float toRad(float deg) {
    return deg * std::numbers::pi_v<float> / 180.f;
}

void Sensor::update(const Car& car, const Track& track) {
    carPos = car.getPosition();
    float heading = car.getAngle();

    for (int i = 0; i < NUM_RAYS; ++i) {
        float rayAngle = heading + ANGLES[i];
        distances[i] = castRay(carPos, rayAngle, track);

        float rad = toRad(rayAngle);
        hitPoints[i] = carPos + sf::Vector2f{
            std::cos(rad) * distances[i],
            std::sin(rad) * distances[i]
        };
    }
}

void Sensor::draw(sf::RenderWindow& window) const {
    // Colour rays by proximity: green=safe, red=close
    for (int i = 0; i < NUM_RAYS; ++i) {
        float t = distances[i] / MAX_RANGE;  // 0=close, 1=far
        sf::Color rayCol(
            static_cast<uint8_t>((1.f - t) * 255),
            static_cast<uint8_t>(t * 200),
            0, 140
        );

        // Draw thicker line using RectangleShape
        sf::RectangleShape thickLine({distances[i], 3.f}); // 3.f is the thickness
        thickLine.setOrigin({0.f, 1.5f});
        thickLine.setPosition(carPos);
        
        float angle = std::atan2(hitPoints[i].y - carPos.y, hitPoints[i].x - carPos.x) * 180.f / std::numbers::pi_v<float>;
        thickLine.setRotation(sf::degrees(angle));
        thickLine.setFillColor(rayCol);
        window.draw(thickLine);

        sf::CircleShape dot(3.f);
        dot.setOrigin({3.f, 3.f});
        dot.setPosition(hitPoints[i]);
        dot.setFillColor(sf::Color(255, 80, 80));
        window.draw(dot);
    }
}

float Sensor::castRay(sf::Vector2f origin, float angleDeg, const Track& track) const {
    float rad = toRad(angleDeg);
    sf::Vector2f dir{std::cos(rad), std::sin(rad)};

    float minDist = MAX_RANGE;
    for (const auto& [a, b] : track.getWalls()) {
        float t = raySegIntersect(origin, dir, a, b);
        if (t > 0.f && t < minDist)
            minDist = t;
    }
    return minDist;
}

// Parametric 2D ray-segment intersection using perpendicular-vector method.
// Ray:  P(t) = o + t*d          (t >= 0)
// Seg:  Q(u) = a + u*(b-a)      (u in [0,1])
// Returns t to first intersection, or -1 if none.
float Sensor::raySegIntersect(sf::Vector2f o, sf::Vector2f d,
                               sf::Vector2f a, sf::Vector2f b) const {
    sf::Vector2f v1  = o - a;          // vector from segment start to ray origin
    sf::Vector2f v2  = b - a;          // segment direction
    sf::Vector2f v3  = {-d.y, d.x};   // ray direction rotated 90°

    float dot = v2.x * v3.x + v2.y * v3.y;
    if (std::abs(dot) < 1e-8f) return -1.f;  // ray and segment are parallel

    float t = (v2.x * v1.y - v2.y * v1.x) / dot;  // cross(v2, v1) / dot
    float u = (v1.x * v3.x + v1.y * v3.y) / dot;  // dot(v1, v3)   / dot

    if (t >= 0.f && u >= 0.f && u <= 1.f) return t;
    return -1.f;
}
