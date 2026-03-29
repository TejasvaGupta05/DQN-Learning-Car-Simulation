#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <utility>
#include <vector>
class Track {
public:
    Track();
    // Draw the asphalt and grass fills, then wall outlines
    void draw(sf::RenderWindow& window) const;
    // Returns true if position is within `radius` px of any wall segment
    bool checkCollision(sf::Vector2f pos, float radius) const;
    sf::Vector2f getStartPosition() const { return startPosition; }
    float        getStartAngle()    const { return startAngle; }
    // Returns n evenly-spaced {position, angleDeg} pairs along the track centre
    std::vector<std::pair<sf::Vector2f, float>> getStartPoses(int n) const;
    // Wall segments used by sensor raycasting
    const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& getWalls() const {
        return walls;
    }
private:
    void buildOvalTrack();
    float segmentPointDist(sf::Vector2f a, sf::Vector2f b, sf::Vector2f p) const;
    std::vector<std::pair<sf::Vector2f, sf::Vector2f>> walls;
    // Filled polygons for rendering
    sf::VertexArray outerFill;  // asphalt (entire outer ellipse)
    sf::VertexArray innerFill;  // grass   (inner ellipse, drawn on top)
    sf::VertexArray outerLine;  // outer wall outline (LineStrip)
    sf::VertexArray innerLine;  // inner wall outline (LineStrip)
    sf::VertexArray startLine;  // start/finish line (Lines)
    sf::Vector2f startPosition;
    float        startAngle = 0.f;
    static constexpr int   N        = 48;     // polygon resolution
    static constexpr float CX       = 640.f;
    static constexpr float CY       = 360.f;
    static constexpr float OUTER_RX = 420.f;
    static constexpr float OUTER_RY = 230.f;
    static constexpr float INNER_RX = 285.f;
    static constexpr float INNER_RY = 130.f;
};