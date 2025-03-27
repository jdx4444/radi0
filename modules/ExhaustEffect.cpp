#include "ExhaustEffect.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>

ExhaustEffect::ExhaustEffect() {}

void ExhaustEffect::Trigger(const ImVec2& position) {
    const int numParticles = 8; // Number of particles in the shot.
    std::cout << "DEBUG: ExhaustEffect::Trigger - Emitting " 
              << numParticles << " particles at position (" 
              << position.x << ", " << position.y << ")." << std::endl;
    
    float centralAngle = 180.0f * 3.1415926f / 180.0f; // leftward
    float spread = 20.0f * 3.1415926f / 180.0f;         // 20° spread
    
    particles.clear();  // Start fresh.
    for (int i = 0; i < numParticles; ++i) {
        Particle p;
        // Start exactly at the given position, then add a small random offset:
        p.position = position;
        p.position.x += ((std::rand() % 11) - 5) / 10.0f; // ±0.5 pixels offset
        p.position.y += ((std::rand() % 11) - 5) / 10.0f; // ±0.5 pixels offset
        
        // Evenly fan out particles.
        float t = (numParticles > 1) ? static_cast<float>(i) / (numParticles - 1) : 0.5f;
        float angle = centralAngle + (t - 0.5f) * spread;
        
        // Speed: 5 to 10 pixels per second.
        float speed = 5.0f + (std::rand() % 6);
        p.velocity = ImVec2(std::cos(angle) * speed, std::sin(angle) * speed);
        
        // Lifetime: between 0.5 and 0.7 seconds.
        p.initialLifetime = p.lifetime = 0.5f + (std::rand() % 21) / 100.0f;
        
        particles.push_back(p);
    }
}

void ExhaustEffect::Update(float deltaTime) {
    for (auto& p : particles) {
        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;
        p.lifetime -= deltaTime;
    }
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
    for (const auto& p : particles) {
        float alpha = p.lifetime / p.initialLifetime;
        // Use the same UI green color (109,254,149) with alpha modulation.
        ImU32 color = IM_COL32(109, 254, 149, static_cast<int>(alpha * 255));
        // Draw a single pixel square.
        ImVec2 pos = p.position;
        draw_list->AddRectFilled(pos, ImVec2(pos.x + 1.0f, pos.y + 1.0f), color);
    }
}
