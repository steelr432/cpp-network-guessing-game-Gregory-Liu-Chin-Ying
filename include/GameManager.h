#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include "Player.h"
#include "NetworkManager.h"
#include <random>

class GameManager {
private:
    Player& localPlayer;
    Player& remotePlayer;
    NetworkManager& net;
    int targetNumber;
    std::default_random_engine engine;

public:
    GameManager(Player& local, Player& remote, NetworkManager& network);
    void startGame();
    bool playRound();
    void rollNumber();
    int getNumberResult() const;
};

#endif
