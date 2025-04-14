#ifndef STATE_H
#define STATE_H

typedef enum
{
    ORMEGGIATA,
    RUBATA,
    IN_MOVIMENTO,
} StatoBarca;

inline const char *statoToString(StatoBarca stato)
{
    switch (stato)
    {
    case ORMEGGIATA:
        return "ORMEGGIATA";
    case RUBATA:
        return "RUBATA";
    case IN_MOVIMENTO:
        return "IN_MOVIMENTO";
    default:
        return "STATO_SCONOSCIUTO";
    }
}

#endif
