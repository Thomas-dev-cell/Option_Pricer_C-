#include "BlackScholesModel.h" // Mod�le Black-Scholes pour le pricing et les param�tres du march�
#include "CallOption.h"        // Classe pour les options call
#include "PutOption.h"         // Classe pour les options put
#include "BarrierOption.h"     // Classe pour les options barri�re
#include "AsianOption.h"       // Classe pour les options asiatiques
#include "LookbackOption.h"    // Classe pour les options lookback
#include <iostream>            // Pour les entr�es/sorties standard
#include <memory>              // Pour std::unique_ptr (non utilis� ici, mais peut �tre pertinent pour les extensions)
#include <vector>              // Pour g�rer les collections (non utilis� dans ce code)

// Fonction pour afficher le menu des types d'options disponibles
void displayMenu() {
    std::cout << "\n--- Menu des options disponibles ---\n";
    std::cout << "1. Call Option\n";
    std::cout << "2. Put Option\n";
    std::cout << "3. Barrier Option (Up-and-Out)\n";
    std::cout << "4. Barrier Option (Down-and-Out)\n";
    std::cout << "5. Barrier Option (Up-and-In)\n";
    std::cout << "6. Barrier Option (Down-and-In)\n";
    std::cout << "7. Asian Option (Call)\n";
    std::cout << "8. Asian Option (Put)\n";
    std::cout << "9. Lookback Option (Call)\n";
    std::cout << "10. Lookback Option (Put)\n";
    std::cout << "0. Quitter\n";
}

// Point d'entr�e principal du programme
int main() {
    // Initialisation des param�tres du mod�le Black-Scholes
    double spot, rate, volatility, dividend;
    std::cout << "Entrez les param�tres du mod�le Black-Scholes :\n";
    std::cout << "Spot price (S0) : ";
    std::cin >> spot; // Prix initial du sous-jacent
    std::cout << "Risk-free rate (r) : ";
    std::cin >> rate; // Taux sans risque
    std::cout << "Volatility (sigma) : ";
    std::cin >> volatility; // Volatilit�
    std::cout << "Dividend yield (q) : ";
    std::cin >> dividend; // Taux de dividende

    BlackScholesModel model(spot, rate, volatility, dividend); // Cr�ation du mod�le Black-Scholes

    while (true) {
        displayMenu(); // Affiche le menu des options disponibles
        int choice;
        std::cout << "Choisissez une option : ";
        std::cin >> choice; // Lecture du choix de l'utilisateur

        if (choice == 0) {
            // Quitter le programme
            std::cout << "Merci d'avoir utilis� le programme.\n";
            break;
        }

        // Lecture des param�tres communs � toutes les options
        double strike, maturity;
        std::cout << "Entrez le strike (K) : ";
        std::cin >> strike; // Prix d'exercice
        std::cout << "Entrez la maturit� (T, en ann�es) : ";
        std::cin >> maturity; // Maturit� de l'option

        int numPaths = 10000;  // Nombre de simulations Monte-Carlo
        int steps = 100;       // Nombre de pas temporels

        if (choice == 1) {
            // Option call
            CallOption callOption(strike, maturity);
            double price = model.priceAnalytic(&callOption, true); // Calcul analytique du prix
            double hedgeCost = callOption.hedgeCost(model, steps); // Calcul du co�t de r�plication
            std::cout << "Prix du call option : " << price << "\n";
            std::cout << "Co�t de r�plication : " << hedgeCost << "\n";

        } else if (choice == 2) {
            // Option put
            PutOption putOption(strike, maturity);
            double price = model.priceAnalytic(&putOption, false); // Calcul analytique du prix
            double hedgeCost = putOption.hedgeCost(model, steps); // Calcul du co�t de r�plication
            std::cout << "Prix du put option : " << price << "\n";
            std::cout << "Co�t de r�plication : " << hedgeCost << "\n";

        } else if (choice >= 3 && choice <= 6) {
            // Options barri�res
            double barrier;
            std::cout << "Entrez le niveau de la barri�re (B) : ";
            std::cin >> barrier; // Niveau de la barri�re

            // Demande du type d'option (call ou put)
            OptionType optionType;
            int optionTypeChoice;
            std::cout << "Type d'option : 1 pour Call, 2 pour Put : ";
            std::cin >> optionTypeChoice;
            if (optionTypeChoice == 1) {
                optionType = OptionType::Call;
            } else if (optionTypeChoice == 2) {
                optionType = OptionType::Put;
            } else {
                std::cout << "Choix invalide pour le type d'option.\n";
                continue;
            }

            // S�lection du type de barri�re
            BarrierType barrierType;
            switch (choice) {
                case 3:
                    barrierType = BarrierType::UpAndOut;
                    break;
                case 4:
                    barrierType = BarrierType::DownAndOut;
                    break;
                case 5:
                    barrierType = BarrierType::UpAndIn;
                    break;
                case 6:
                    barrierType = BarrierType::DownAndIn;
                    break;
            }

            BarrierOption barrierOption(strike, maturity, barrier, barrierType, optionType);
            double price = barrierOption.price(model, numPaths, steps); // Calcul du prix par Monte-Carlo
            double hedgeCost = barrierOption.hedgeCost(model, steps);  // Calcul du co�t de r�plication
            std::cout << "Prix de l'option barri�re : " << price << "\n";
            std::cout << "Co�t de r�plication : " << hedgeCost << "\n";

        } else if (choice == 7 || choice == 8) {
            // Options asiatiques
            OptionType optionType = (choice == 7) ? OptionType::Call : OptionType::Put;

            AsianOption asianOption(strike, maturity, optionType);
            double price = asianOption.price(model, numPaths, steps); // Calcul du prix par Monte-Carlo
            double hedgeCost = asianOption.hedgeCost(model, steps);  // Calcul du co�t de r�plication
            std::cout << "Prix de l'option asiatique : " << price << "\n";
            std::cout << "Co�t de r�plication : " << hedgeCost << "\n";

        } else if (choice == 9 || choice == 10) {
            // Options lookback
            OptionType optionType = (choice == 9) ? OptionType::Call : OptionType::Put;

            LookbackOption lookbackOption(strike, maturity, optionType);
            double price = lookbackOption.price(model, numPaths, steps); // Calcul du prix par Monte-Carlo
            double hedgeCost = lookbackOption.hedgeCost(model, steps);  // Calcul du co�t de r�plication
            std::cout << "Prix de l'option lookback : " << price << "\n";
            std::cout << "Co�t de r�plication : " << hedgeCost << "\n";

        } else {
            std::cout << "Choix invalide. Veuillez r�essayer.\n";
        }
    }

    return 0; // Fin du programme
}
