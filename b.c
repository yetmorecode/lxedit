#include <stdio.h>

// We want this in the data object so a fixup is created
// that we can overwrite
char run_game_dummy[] = "\xc3";

int main (int ac, char **av) {
    // Print some banner and wait for input
    printf("F1 Manager Professional Megapack 2022\nPress any key to continue..");
    getchar();

    // Start the game
    void(*f)() = (void(*)())run_game_dummy;
    f();

    return 0;
}
