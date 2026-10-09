#include <windows.h>

#define CT_MEMGUARD_SLEEPER_INTERVAL_MS 1000

int main(void) {
    for (;;) {
        Sleep(CT_MEMGUARD_SLEEPER_INTERVAL_MS);
    }
}
