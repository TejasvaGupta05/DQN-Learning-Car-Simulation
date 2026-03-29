#include "Track.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>
static sf::Vector2f ellipsePoint(float cx, float cy, float rx, float ry, int i, int n) {
    float a = 2.f * std::numbers::pi_v<float> * i / n;
    return {cx + rx * std::cos(a), cy + ry * std::sin(a)};
}
// Returns n evenly-spaced {midline-position, clockwise-heading-deg} pairs
std::vector<std::pair<sf::Vector2f, float>> Track::getStartPoses(int n) const {
    const float pi    = std::numbers::pi_v<float>;
    const float midRX = (OUTER_RX + INNER_RX) * 0.5f;
    const float midRY = (OUTER_RY + INNER_RY) * 0.5f;
    const float baseI = float(N) * 3.f / 4.f;  // index of top of oval
    std::vector<std::pair<sf::Vector2f, float>> poses;
    poses.reserve(n);
    for (int k = 0; k < n; ++k) {
        float idx = baseI + float(k) * float(N) / float(n);
        float t   = 2.f * pi * idx / float(N);
        sf::Vector2f pos{CX + midRX * std::cos(t), CY + midRY * std::sin(t)};
        // Clockwise tangent of the mid-ellipse at angle t
        sf::Vector2f tang{-midRX * std::sin(t), midRY * std::cos(t)};
        float angle = std::atan2(tang.y, tang.x) * 180.f / pi;
        poses.push_back({pos, angle});
    }
    return poses;
}
Track::Track()
    : outerFill(sf::PrimitiveType::TriangleFan, N + 2)
    , innerFill(sf::PrimitiveType::TriangleFan, N + 2)
    , outerLine(sf::PrimitiveType::LineStrip, N + 1)
    , innerLine(sf::PrimitiveType::LineStrip, N + 1)
    , startLine(sf::PrimitiveType::Lines, 2)
{
    buildOvalTrack();
}
void Track::buildOvalTrack() {
    const sf::Color asphalt(55, 55, 65);
    const sf::Color grass(35, 105, 40);
    const sf::Color wall(220, 220, 220);
    // --- Filled polygons (TriangleFan from center) ---
    outerFill[0].position = {CX, CY};
    outerFill[0].color    = asphalt;
    innerFill[0].position = {CX, CY};
    innerFill[0].color    = grass;
    for (int i = 0; i <= N; ++i) {
        outerFill[i + 1].position = ellipsePoint(CX, CY, OUTER_RX, OUTER_RY, i, N);
        outerFill[i + 1].color    = asphalt;
        innerFill[i + 1].position = ellipsePoint(CX, CY, INNER_RX, INNER_RY, i, N);
        innerFill[i + 1].color    = grass;
    }
    // --- Wall outlines (LineStrip loops) ---
    for (int i = 0; i <= N; ++i) {
        outerLine[i].position = ellipsePoint(CX, CY, OUTER_RX, OUTER_RY, i, N);
        outerLine[i].color    = wall;
        innerLine[i].position = ellipsePoint(CX, CY, INNER_RX, INNER_RY, i, N);
        innerLine[i].color    = wall;
    }
    // --- Collision walls (pairs of consecutive segment endpoints) ---
    for (int i = 0; i < N; ++i) {
        sf::Vector2f oa = ellipsePoint(CX, CY, OUTER_RX, OUTER_RY, i,       N);
        sf::Vector2f ob = ellipsePoint(CX, CY, OUTER_RX, OUTER_RY, (i+1)%N, N);
        sf::Vector2f ia = ellipsePoint(CX, CY, INNER_RX, INNER_RY, i,       N);
        sf::Vector2f ib = ellipsePoint(CX, CY, INNER_RX, INNER_RY, (i+1)%N, N);
        walls.push_back({oa, ob});
        walls.push_back({ia, ib});
    }
    // --- Start line (top of oval, angle = -pi/2 on ellipse) ---
    // That is point index N*3/4 (270 degrees around, i.e. top) for a clockwise layout,
    // but with standard math angle 0=right: top = i at angle=-pi/2 = i = N*3/4
    sf::Vector2f outerTop = ellipsePoint(CX, CY, OUTER_RX, OUTER_RY, N * 3 / 4, N);
    sf::Vector2f innerTop = ellipsePoint(CX, CY, INNER_RX, INNER_RY, N * 3 / 4, N);
    startLine[0].position = outerTop;
    startLine[0].color    = sf::Color::Yellow;
    startLine[1].position = innerTop;
    startLine[1].color    = sf::Color::Yellow;
    // Car starts midway between inner/outer at the top, heading right (0 deg)
    startPosition = (outerTop + innerTop) / 2.f;
    startAngle    = 0.f;  // heading = right = clockwise around top
}
void Track::draw(sf::RenderWindow& window) const {
    window.draw(outerFill);
    window.draw(innerFill);
    window.draw(outerLine);
    window.draw(innerLine);
    window.draw(startLine);
}
bool Track::checkCollision(sf::Vector2f pos, float radius) const {
    for (const auto& [a, b] : walls) {
        if (segmentPointDist(a, b, pos) < radius) return true;
    }
    return false;
}
float Track::segmentPointDist(sf::Vector2f a, sf::Vector2f b, sf::Vector2f p) const {
    sf::Vector2f ab = b - a;
    sf::Vector2f ap = p - a;
    float len2 = ab.x * ab.x + ab.y * ab.y + 1e-8f;
    float t = std::clamp((ap.x * ab.x + ap.y * ab.y) / len2, 0.f, 1.f);
    sf::Vector2f closest = a + ab * t;
    sf::Vector2f d = p - closest;
    return std::sqrt(d.x * d.x + d.y * d.y);
}
