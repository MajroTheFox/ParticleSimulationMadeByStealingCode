#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <random>

const int NUM_PARTICLES = 64;
const float WINDOW_WIDTH = 1200.f;
const float WINDOW_HEIGHT = 800.f;
const float REPULSION_STRENGTH = 80000.f;
const float DAMPING = 0.995f;
const float MAX_FORCE = 5000.f;
const float PARTICLE_RADIUS = 6.f;
const float MIN_DISTANCE = 15.f; // prevents extreme forces at very close range

struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Vector2f force;
    sf::Color color;
};

int main() {
    sf::RenderWindow window(
        sf::VideoMode((unsigned)WINDOW_WIDTH, (unsigned)WINDOW_HEIGHT),
        "Particle Repulsion Simulation - 64 Particles"
    );
    window.setFramerateLimit(60);

    // Random setup
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> xDist(50.f, WINDOW_WIDTH - 50.f);
    std::uniform_real_distribution<float> yDist(50.f, WINDOW_HEIGHT - 50.f);
    std::uniform_real_distribution<float> vDist(-50.f, 50.f);
    std::uniform_int_distribution<int> cDist(80, 255);

    std::vector<Particle> particles(NUM_PARTICLES);
    for (auto& p : particles) {
        p.position = { xDist(rng), yDist(rng) };
        p.velocity = { vDist(rng), vDist(rng) };
        p.force = { 0.f, 0.f };
        p.color = sf::Color(
            (sf::Uint8)cDist(rng),
            (sf::Uint8)cDist(rng),
            (sf::Uint8)cDist(rng)
        );
    }

    sf::CircleShape shape(PARTICLE_RADIUS);
    shape.setOrigin({ PARTICLE_RADIUS, PARTICLE_RADIUS });

    sf::Clock clock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape)
                window.close();
        }

        float dt = clock.restart().asSeconds();
        // Clamp dt to avoid explosion on lag spikes
        if (dt > 0.05f) dt = 0.05f;

        // Reset forces
        for (auto& p : particles)
            p.force = { 0.f, 0.f };

        // Compute pairwise repulsion
        for (size_t i = 0; i < particles.size(); ++i) {
            for (size_t j = i + 1; j < particles.size(); ++j) {
                sf::Vector2f diff = particles[i].position - particles[j].position;
                float distSq = diff.x * diff.x + diff.y * diff.y;
                float dist = std::sqrt(distSq);

                if (dist < 0.001f) {
                    // jitter to avoid div-by-zero
                    diff = { 0.001f, 0.001f };
                    dist = 0.0014f;
                }

                // Clamp minimum distance to prevent huge forces
                float effDist = std::max(dist, MIN_DISTANCE);

                // Repulsion force magnitude: F = k / d^2
                float forceMag = REPULSION_STRENGTH / (effDist * effDist);
                if (forceMag > MAX_FORCE) forceMag = MAX_FORCE;

                sf::Vector2f dir = diff / dist;

                particles[i].force += dir * forceMag;
                particles[j].force -= dir * forceMag;
            }
        }

        // Integrate
        for (auto& p : particles) {
            // Semi-implicit Euler
            p.velocity += p.force * dt;
            p.velocity *= DAMPING; // add some drag for stability
            p.position += p.velocity * dt;

            // Wall repulsion (soft bounce)
            const float margin = 20.f;
            const float wallForce = 3000.f;
            if (p.position.x < margin)
                p.velocity.x += wallForce * dt * (margin - p.position.x) / margin;
            if (p.position.x > WINDOW_WIDTH - margin)
                p.velocity.x -= wallForce * dt * (p.position.x - (WINDOW_WIDTH - margin)) / margin;
            if (p.position.y < margin)
                p.velocity.y += wallForce * dt * (margin - p.position.y) / margin;
            if (p.position.y > WINDOW_HEIGHT - margin)
                p.velocity.y -= wallForce * dt * (p.position.y - (WINDOW_HEIGHT - margin)) / margin;

            // Hard clamp as final safety
            p.position.x = std::clamp(p.position.x, 0.f, WINDOW_WIDTH);
            p.position.y = std::clamp(p.position.y, 0.f, WINDOW_HEIGHT);
        }

        // Draw
        window.clear(sf::Color(15, 15, 25));
        for (auto& p : particles) {
            shape.setPosition(p.position);
            shape.setFillColor(p.color);
            window.draw(shape);
        }
        window.display();
    }

    return 0;
}