#ifndef EXHAUST_EFFECT_H
#define EXHAUST_EFFECT_H

#include "imgui.h"
#include <vector>

class ExhaustEffect {
public:
    ExhaustEffect();
    // Trigger a new exhaust puff at the given position.
    void Trigger(const ImVec2& position);
    // Update particles based on delta time.
    void Update(float deltaTime);
    // Draw particles to the provided draw list.
    void Draw(ImDrawList* draw_list);
    
private:
    struct Particle {
        ImVec2 position;
        ImVec2 velocity;
        float lifetime;       // Remaining lifetime (seconds)
        float initialLifetime; // Initial lifetime (for fade calculations)
    };
    std::vector<Particle> particles;
};

#endif // EXHAUST_EFFECT_H
