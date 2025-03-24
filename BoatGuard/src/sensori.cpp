#include "sensori.h"
#include <iot_board.h>

void leggi_accelerometro(float &x, float &y, float &z, bool in_movimento)
{
    if (in_movimento)
    {
        x = random(-500, 500) / 1000.0; // [-0.5, 0.5]
        y = random(-500, 500) / 1000.0;
        z = random(900, 980) / 100.0; // [9.0, 9.8]
    }
    else
    {
        x = random(-50, 50) / 1000.0; // [-0.05, 0.05]
        y = random(-50, 50) / 1000.0;
        z = random(975, 985) / 100.0; // [9.75, 9.85]
    }
}

void leggi_giroscopio(float &x, float &y, float &z, bool in_movimento)
{
    if (in_movimento)
    {
        x = random(-500, 500) / 1000.0; // [-0.5, 0.5]
        y = random(-500, 500) / 1000.0;
        z = random(-500, 500) / 1000.0;
    }
    else
    {
        x = random(-20, 20) / 1000.0; // [-0.02, 0.02]
        y = random(-20, 20) / 1000.0;
        z = random(-20, 20) / 1000.0;
    }
}

float leggi_solcometro(bool in_movimento)
{
    if (in_movimento)
    {
        return random(100, 1500) / 100.0; // [1.0, 15.0]
    }
    else
    {
        return random(0, 5) / 100.0; // [0.00, 0.05]
    }
}
