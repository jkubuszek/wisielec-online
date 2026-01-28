#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

void HangmanTitle(void){
    system("clear");
    printf("H      H       A     N      NNN   GGGG    MMM       MM        A     N      NNN\n");
    printf("HH     HH     AAAA  NNNN     NN GGG       MMMMM    MMMM      AAAA  NNNN     NN\n");
    printf("HHHHHHHHH   AAA  AA NN  NNN  NN GG   GGG  MMM MMM MM MMM   AAA  AA  NN  NNN  NN\n");
    printf(" HH    HH  AAAAAAAA NN    NNN N GG     GG  MM   MMM   MM  AAAAAAAA  NN    NNN N\n");
    printf(" HH    HH AA      AA NN     NNN  GGGGGGG  MM         MMM AA      AA  NN     NNN\n");
    printf("\n");
    printf(" HHHHHHHHH AAAAAAAAA  NNNNNNNN   GGGGGGGGG    MMMMMMMMM   AAAAAAAA    NNNNNNNN\n");
    printf("                             |\n");
    printf("                             O\n");
    printf("                            /|\\\n");
    printf("                            / \\\n");
}

void rysujWisielca(int bledy) {
    printf("                            %s\n", (bledy >= 2 ? "████████████████████████████":""));
    printf("                           %s\n", (bledy >= 2 ? "██████████████████████████████":""));
    printf("                             %s     %s           %s\n", (bledy >= 1 ? "███":""), (bledy >= 2 ? "████":""), (bledy >= 3 ? "█":""));
    printf("                             %s   %s            %s\n", (bledy >= 1 ? "███":""), (bledy >= 2 ? "█████":""), (bledy >= 3 ? "█":""));
    printf("                             %s %s              %s\n", (bledy >= 1 ? "███":""), (bledy >= 2 ? "█████":""), (bledy >= 3 ? "█":""));
    printf("                             %s%s             %s\n", (bledy >= 1 ? "███":""), (bledy >= 2 ? "████":""), (bledy >= 3 ? "███████":""));
    printf("                             %s%s             %s\n", (bledy >= 1 ? "███":""), (bledy >= 2 ? "██":""), (bledy >= 3 ? "████   ████":""));
    printf("                             %s              %s\n", (bledy >= 1 ? "███":""), (bledy >= 3 ? "███       ███":""));
    printf("                             %s             %s\n", (bledy >= 1 ? "███":""), (bledy >= 3 ? "███         ███":""));
    printf("                             %s              %s\n", (bledy >= 1 ? "███":""), (bledy >= 3 ? "███       ███":""));
    printf("                             %s               %s\n", (bledy >= 1 ? "███":""), (bledy >= 3 ? "████   ████":""));
    printf("                             %s                 %s\n", (bledy >= 1 ? "███":""), (bledy >= 3 ? "███████":""));
    printf("                             %s                   %s\n", (bledy >= 1 ? "███":""), (bledy >= 4 ? "████":""));
    printf("                             %s                 %s%s%s\n", (bledy >= 1 ? "███":""), (bledy >= 5 ? "██":"  "), (bledy >= 4 ? "████":""), (bledy >= 6 ? "██":""));
    printf("                             %s               %s%s%s\n", (bledy >= 1 ? "███":""), (bledy >= 5 ? "████":"    "), (bledy >= 4 ? "████":""), (bledy >= 6 ? "████":""));
    printf("                             %s              %s  %s  %s\n", (bledy >= 1 ? "███":""), (bledy >= 5 ? "███":"   "), (bledy >= 4 ? "████":""), (bledy >= 6 ? "███":""));
    printf("                             %s             %s    %s    %s\n", (bledy >= 1 ? "███":""), (bledy >= 5 ? "██":"  "), (bledy >= 4 ? "████":""), (bledy >= 6 ? "██":""));
    printf("                             %s                   %s\n", (bledy >= 1 ? "███":""), (bledy >= 4 ? "████":""));
    printf("                             %s                   %s\n", (bledy >= 1 ? "███":""), (bledy >= 4 ? "████":""));
    printf("                             %s                  %s%s%s\n", (bledy >= 1 ? "███":""), (bledy >= 7 ? "██":"  "), (bledy >= 4 ? "██":""), (bledy >= 8 ? "██":""));
    printf("                             %s                 %s  %s\n", (bledy >= 1 ? "███":""), (bledy >= 7 ? "███":""), (bledy >= 8 ? "███":""));
    printf("                             %s                %s    %s\n", (bledy >= 1 ? "███":""), (bledy >= 7 ? "███":""), (bledy >= 8 ? "███":""));
    printf("                             %s               %s      %s\n", (bledy >= 1 ? "███":""), (bledy >= 7 ? "███":""), (bledy >= 8 ? "███":""));
    printf("                             %s              %s        %s\n", (bledy >= 1 ? "███":""), (bledy >= 7 ? "███":""), (bledy >= 8 ? "███":""));
    printf("                             %s\n", (bledy >= 1 ? "███":""));
    printf("                             %s\n", (bledy >= 1 ? "███":""));
    printf("                       ██████████████████████████████████████████\n");
    printf("                       ██████████████████████████████████████████\n");
}

int main() {
    char haslo[] = "PROGRAMOWANIE"; // Możesz zmienić na dowolne słowo (dużymi literami)
    int dlugosc = strlen(haslo);
    char zgadniete[dlugosc + 1];
    int bledy = 0;
    int trafienia = 0;
    char litera;

    // Inicjalizacja tablicy zgadniętych liter podkreślnikami
    for (int i = 0; i < dlugosc; i++) {
        zgadniete[i] = '_';
    }
    zgadniete[dlugosc] = '\0';

    printf("Witaj w grze Wisielec!\n");

    while (bledy < 8 && trafienia < dlugosc) {
        HangmanTitle();
        rysujWisielca(bledy);
        printf("\nHaslo: ");
        for (int i = 0; i < dlugosc; i++) printf("%c ", zgadniete[i]);
        
        printf("\nPodaj litere: ");
        scanf(" %c", &litera);
        litera = toupper(litera);

        int znaleziono = 0;
        for (int i = 0; i < dlugosc; i++) {
            if (haslo[i] == litera && zgadniete[i] == '_') {
                zgadniete[i] = litera;
                trafienia++;
                znaleziono = 1;
            }
        }

        if (!znaleziono) {
            bledy++;
            printf("Nie ma takiej litery!\n");
        } else {
            printf("Brawo!\n");
        }
    }
    HangmanTitle();
    rysujWisielca(bledy);
    if (trafienia == dlugosc) {
        printf("\nGRATULACJE! Wygrales! Haslo to: %s\n", haslo);
    } else {
        printf("\nPRZEGRANA. Wisielec gotowy. Haslo to: %s\n", haslo);
    }

    return 0;
}