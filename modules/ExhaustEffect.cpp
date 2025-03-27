#include "ExhaustEffect.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

ExhaustEffect::ExhaustEffect() {}

void ExhaustEffect::Trigger(const ImVec2& position) {
    // Spawn a burst of particles (for example, 20 particles)
    const int numParticles = 20;
    for (int i = 0; i < numParticles; ++i) {
        Particle p;
        p.position = position;
        // Randomize a direction (0 to 360 degrees) and speed.
        float angle = (std::rand() % 360) * 3.1415926f / 180.0f;
        float speed = 10.0f + std::rand() % 20; // speed between 10 and 30 pixels per second
        p.velocity = ImVec2(std::cos(angle) * speed, std::sin(angle) * speed);
        p.initialLifetime = p.lifetime = 0.5f + (std::rand() % 50) / 100.0f; // between 0.5 and ~1.0 sec
        particles.push_back(p);
    }
}

void ExhaustEffect::Update(float deltaTime) {
    // Update each particle’s position and lifetime.
    for (auto& p : particles) {
        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;
        p.lifetime -= deltaTime;
    }
    // Remove particles that have expired.
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return p.lifetime <= 0.0f; }),
        particles.end());
}

void ExhaustEffect::Draw(ImDrawList* draw_list) {
    // For each particle, compute an alpha based on remaining lifetime
    // and draw a small filled circle.
    for (const auto& p : particles) {
        float alpha = p.lifetime / p.initialLifetime;
        ImU32 color = IM_COL32(128, 128, 128, static_cast<int>(alpha * 255));
        float radius = 2.0f; // 2 pixels radius
        draw_list->AddCircleFilled(p.position, radius, color, 8);
    }
}
