#ifndef PLAYER_H
#define PLAYER_H

#include <stdlib.h>
#include <string>
using namespace std;


const int PLAYER_HIGHT = 0;
const int PLAYER_WIDTH = 0;

struct Character{

    string name;
    string role;

    string walk1, walk2, fightStand, deadPosion;

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

// Character Array Declaration and Initialization
// inline helps us to make it like a shared array 
Character CH[3] = {
    // CH[0] - The Alchemist
    {
        "The Alchemist",      // name
        "Witchcraft Expert",  
        "Images//Alchemist_walk1.png", // images
        "Images//Alchemist_walk2.png", 
        "Images//Alchemist card.jpg",  // card image shown on the CHARACTER_SELECT screen
        "Images//Alchemist_deadPosion.png",
        100.0f,               // health
        100.0f,               // maxHealth
        200.0f,               // moveSpeed
        450.0f,               // attackRange
        0.75f,                // resourceCostMultiplier (-25%)
        1.50f,                // potionConsumeSpeed
        0.0f,                 // dashDistance
        0.0f,                 // energyCost
        0.0f,                 // shieldDuration
        0.0f                  // shieldStrength
    },
    // CH[1] - The Ranger
    {
        "The Ranger",         // name
        "Shadow Dasher",      // role
        "Images//Ranger_walk1.png",  // images
        "Images//Ranger_walk2.png", 
        "Images//Ranger card.jpg",   // card image shown on the CHARACTER_SELECT screen
        "Images//Ranger_deadPosion.png",
        70.0f,                // health
        70.0f,                // maxHealth
        320.0f,               // moveSpeed
        500.0f,               // attackRange
        1.00f,                // resourceCostMultiplier
        1.00f,                // potionConsumeSpeed
        150.0f,               // dashDistance
        25.0f,                // energyCost
        0.0f,                 // shieldDuration
        0.0f                  // shieldStrength
    },
    // CH[2] - The Guardian
    {
        "The Guardian",       // name
        "Light Protector",    // role
        "Images//Guardian_walk1.png",  // images
        "Images//Guardian_walk2.png", 
        "Images//Guardian card.jpg",   // card image shown on the CHARACTER_SELECT screen
        "Images//Guardian_deadPosion.png",
        180.0f,               // health
        180.0f,               // maxHealth
        130.0f,               // moveSpeed
        100.0f,               // attackRange
        1.00f,                // resourceCostMultiplier
        1.00f,                // potionConsumeSpeed
        0.0f,                 // dashDistance
        0.0f,                 // energyCost
        4.0f,                 // shieldDuration
        100.0f                // shieldStrength
    }
};
#endif