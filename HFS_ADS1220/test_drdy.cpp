#include <iostream>
#include <pigpio.h>
#include <unistd.h>

#define DRDY_GPIO 25

int main() {
    if (gpioInitialise() < 0) {
        std::cerr << "Erreur lors de l'initialisation de pigpio." << std::endl;
        return -1;
    }

    gpioSetMode(DRDY_GPIO, PI_INPUT);
    gpioSetPullUpDown(DRDY_GPIO, PI_PUD_UP); // Active pull-up interne

    while (true) {
        int etat = gpioRead(DRDY_GPIO);
        std::cout << "DRDY = " << etat << std::endl;
        time_sleep(0.1); // toutes les 100 ms
    }

    gpioTerminate();
    return 0;
}
