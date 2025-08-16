#include "GameManager.h"
#include "ConsoleColor.h"
#include <iostream>
#include <sstream>
#include <limits>
#include <chrono>
#include <thread>

GameManager::GameManager(Player& local, Player& remote, NetworkManager& network)
    : localPlayer(local), remotePlayer(remote), net(network), engine(std::random_device{}()) {}

void GameManager::rollNumber() {
    std::uniform_int_distribution<int> dist(1, 20);
    targetNumber = dist(engine);
}

void GameManager::startGame() {
    printColorLine("Welcome to Networked Number Guessing Game!", ConsoleColor::Aqua);

    int clientScore = 0;
    bool playAgain = true;

    while (playAgain) {
        if (localPlayer.getIsHost()) {
            // Host Side
            rollNumber();
            printColorLine("Secret number generated.", ConsoleColor::Gray);
            net.sendMessage("START|");  // tell client to begin

            bool remoteWin = false;
            int clientGuesses = 0;

            while (clientGuesses < 3 && !remoteWin) {
                std::string guessMsg = net.receiveMessage();
                int guess = std::stoi(guessMsg.substr(guessMsg.find('|') + 1));

                if (guess == targetNumber) {
                    remoteWin = true;
                    net.sendMessage("HINT|Correct!");
                } else if (guess < targetNumber) {
                    net.sendMessage("HINT|Too low!");
                } else {
                    net.sendMessage("HINT|Too high!");
                }
                clientGuesses++;
            }

            // Reveal the number
            net.sendMessage("NUMBER|" + std::to_string(targetNumber));

            // Result
            if (remoteWin) {
                printColorLine("Guessed correctly!", ConsoleColor::LightGreen);
                clientScore++;
            } else {
                printColorLine("Did not guess the number.", ConsoleColor::LightRed);
            }

            // Scoreboard
            std::ostringstream sb;
            sb << "Client Wins: " << clientScore;
            printColorLine(sb.str(), ConsoleColor::Aqua);

            // Wait for client decision
            printColorLine("Waiting for client to decide whether to continue...", ConsoleColor::Gray);
            std::string contMsg;
            do {
                contMsg = net.receiveMessage();
            } while (contMsg == "WAITING");

            if (contMsg.rfind("EXIT|", 0) == 0) {
                printColorLine("Client chose to quit. Ending game.", ConsoleColor::LightRed);
                playAgain = false;
            } else if (contMsg.rfind("CONT|", 0) == 0) {
                printColorLine("Client chose to play another round!", ConsoleColor::Aqua);
            }

        } else {
            // Client Side
            std::string msg;
            do {
                msg = net.receiveMessage();
            } while (msg == "WAITING");

            if (msg.rfind("START|", 0) == 0) {
                int attemptsLeft = 3;
                bool guessedCorrectly = false;

                while (attemptsLeft > 0 && !guessedCorrectly) {
                    int guess;
                    printColor("Enter your guess (1-20): ", ConsoleColor::Yellow);
                    if (!(std::cin >> guess)) {
                        std::cin.clear();
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        printColorLine("Invalid input. Try again.", ConsoleColor::LightRed);
                        continue;
                    }

                    net.sendMessage("GUESS|" + std::to_string(guess));

                    // Receive hint
                    std::string hintMsg;
                    do {
                        hintMsg = net.receiveMessage();
                    } while (hintMsg == "WAITING");

                    if (hintMsg.rfind("HINT|", 0) == 0) {
                        std::string hint = hintMsg.substr(hintMsg.find('|') + 1);
                        printColorLine(hint, ConsoleColor::Gray);
                        if (hint == "Correct!") {
                            guessedCorrectly = true;
                        }
                    }
                    attemptsLeft--;
                }

                // Reveal number
                std::string resultMsg;
                do {
                    resultMsg = net.receiveMessage();
                } while (resultMsg == "[WAIT]");

                if (resultMsg.rfind("NUMBER|", 0) == 0) {
                    targetNumber = std::stoi(resultMsg.substr(resultMsg.find('|') + 1));
                    printColorLine("The secret number was: " + std::to_string(targetNumber), ConsoleColor::Aqua);
                }

                // Continue?
                std::string cont;
                printColor("Play another round? (y/n): ", ConsoleColor::LightPurple);
                std::cin >> cont;
                std::cin.ignore();

                if (cont != "y" && cont != "Y") {
                    net.sendMessage("EXIT|Client quit the game.");
                    playAgain = false;
                } else {
                    net.sendMessage("CONT|");
                }
            }
        }
    }

    printColorLine("\nGame Over. Thanks for playing!", ConsoleColor::Aqua);
    net.closeConnection();
}