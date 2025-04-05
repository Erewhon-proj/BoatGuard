#ifndef SENSORI_H
#define SENSORI_H

void leggi_accelerometro(float &x, float &y, float &z, bool in_movimento);
void leggi_giroscopio(float &x, float &y, float &z, bool in_movimento);
float leggi_solcometro(bool in_movimento);

#endif // SENSORI_H
