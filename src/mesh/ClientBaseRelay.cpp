#include "ClientBaseRelay.h"

static bool clientBaseElevatePrimaryOptIn = false;

bool clientBaseGetElevatePrimaryOptIn()
{
    return clientBaseElevatePrimaryOptIn;
}

void clientBaseSetElevatePrimaryOptIn(bool enabled)
{
    clientBaseElevatePrimaryOptIn = enabled;
}
