#include <assert.h>
#include <stdio.h>
#include "../../src/PAW3805EK.c"

int main(void)
{
    signed short x = 1, y = 2;
    PAW3805EK_Init();
    assert(PAW3805EK_GetState() == PAW3805EK_UNCONFIGURED);
    assert(!PAW3805EK_ReadMotion(&x, &y) && x == 0 && y == 0);
    assert(!PAW3805EK_PowerDown());
    puts("PASS unconfigured build links without GPIO/TMOS symbols");
    return 0;
}
