#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include "io_mappings.h"
#include "sys_config.h"
#include "lib/bc127.h"
#include "lib/ibus.h"

/*
 * Main loop for our PIC24FJ
 */
int main(void)
{
    BC127_t *bt = BC127Init();
    IBus_t *ibus = IBusInit();

    // Turn on LED D10 on our Dev Board
    TRISAbits.TRISA7 = 0;
    LATAbits.LATA7 = 1;

    while (1) {
        ibus->process(ibus);
        bt->process(bt);
    }
    bt->destroy(bt);
    ibus->destroy(ibus);
    return 0;
}
