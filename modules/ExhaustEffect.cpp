#include "ExhaustEffect.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>  // For debugging output

ExhaustEffect::ExhaustEffect() {}

void ExhaustEffect::Trigger(const ImVec2& position) {
    const int numParticles = 20;
    std::cout << "DEBUG: ExhaustEffect::Trigger - Emitting " 
              << numParticles << " particles at position (" 
              << position.x << ", " << position.y << ")." << std::endl;
    for (int i = 0; i < numParticles; ++i) {
        Particle p;
        p.position = position;
        // Randomize a direction (0 to 360 degrees) and speed.
        float angle = (std::rand() % 360) * 3.1415926f / 180.0f;
        float speed = 10.0f + std::rand() % 20; // speed between 10 and 30 pixels per second
        p.velocity = ImVec2(std::cos(angle) * speed, std::sin(angle) * speed);
        // Temporarily increase particle lifetime: between 2.0 and 2.5 seconds.
        p.initialLifetime = p.lifetime = 2.0f + (std::rand() % 50) / 100.0f;
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
    // Remove expired particles.
    size_t before = particles.size();
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return p.lifetime <= 0.0f; }),
        particles.end());
    size_t removed = before - particles.size();
    if (removed > 0) {
        std::cout << "DEBUG: ExhaustEffect::Update - Removed " << removed 
                  << " expired particles. " << particles.size() 
                  << " remain." << std::endl;
    }
}

void ExhaustEffect::Draw(ImDrawList* draw_list) {
    // Draw each particle using the same green as your UI elements.
    for (const auto& p : particles) {
        float alpha = p.lifetime / p.initialLifetime;
        // Use the UI green: (109, 254, 149), with alpha modulation.
        ImU32 color = IM_COL32(109, 254, 149, static_cast<int>(alpha * 255));
        float radius = 2.0f; // 2 pixels radius
        draw_list->AddCircleFilled(p.position, radius, color, 8);
    }
}
