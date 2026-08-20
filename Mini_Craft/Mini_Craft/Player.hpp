#ifndef PLAYER_H
#define PLAYER_H

#include <stdlib.h>
#include <string>
using namespace std;


const int PLAYER_HIGHT;
const int PLAYER_WIDTH;

struct Character{

    string name;
    string role;
    float health;
    float maxHealth;
    float moveSpeed;
    float attackRange;
    
    float resourceCostMultiplier;
    float potionConsumeSpeed;     // Multiplier for consumption time
    float dashDistance;
    float energyCost;
    float shieldDuration;
    float shieldStrength;
};

// wrinting f is not necessary, without it the float is 64 bit
// but we write is fot the compiler to treat it as 32bit

// 1. The Alchemist
Character alchemist = {
    "The Alchemist",      // name
    "Witchcraft Expert",  // role
    100.0f,               // health
    100.0f,               // maxHealth
    200.0f,               // moveSpeed (pixels/sec)
    450.0f,               // attackRange (high)
    0.75f,                // resourceCostMultiplier (-25%)
    2.0f,                // potionConsumeSpeed (50% faster)
    0.0f,                 // dashDistance (N/A)
    0.0f,                 // energyCost (N/A)
    0.0f,                 // shieldDuration (N/A)
    0.0f                  // shieldStrength (N/A)
};

// 2. The Ranger
Character ranger = {
    "The Ranger",         // name
    "Shadow Dasher",      // role
    70.0f,                // health (low)
    70.0f,                // maxHealth
    320.0f,               // moveSpeed (high)
    500.0f,               // attackRange (high)
    1.00f,                // resourceCostMultiplier
    1.00f,                // potionConsumeSpeed
    150.0f,               // dashDistance
    25.0f,                // energyCost
    0.0f,                 // shieldDuration (N/A)
    0.0f                  // shieldStrength (N/A)
};

// 3. The Guardian
Character guardian = {
    "The Guardian",       // name
    "Light Protector",    // role
    180.0f,               // health (high)
    180.0f,               // maxHealth
    130.0f,               // moveSpeed (slow)
    100.0f,               // attackRange (low)
    1.00f,                // resourceCostMultiplier
    1.00f,                // potionConsumeSpeed
    0.0f,                 // dashDistance (N/A)
    0.0f,                 // energyCost (N/A)
    4.0f,                 // shieldDuration (seconds)
    100.0f                // shieldStrength
};
#endif