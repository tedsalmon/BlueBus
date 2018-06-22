;
; File: sfr_setters.h
; Author: Ted Salmon <tass2001@gmail.com>
; Description:
;     Set various special function registers via ASM
;
.include "p24Fxxxx.inc"
.text

.macro SINGLE_BIT_SET name
    .global _Set\name
    _Set\name:
        sl  w0, #1, w0 ; Shift Left
        add w0, w1, w0 ; Add
        sl  w0, #1, w0 ; Shift Left
        bra w0 ; Branch unconditionally
.endm

.macro SINGLE_BIT reg, bit
    bclr \reg, #\bit ; Clear the `bit` in register `reg`
    return
    bset \reg, #\bit ; Set the bit in `reg` to `bit`
    return
.endm

.macro MULTI_BIT_SET name, width
    .global _Set\name
    _Set\name:
        mov    #((1 << \width) - 1), w2
        and    w1, w2, w1
        mul.uu w0, #6, w4
        bra    w4
.endm

.macro MULTI_BIT reg, bnum
    sl   w2, #\bnum, w0
    com  w0, w0
    and  \reg
    sl   w1, #\bnum, w0
    ior  \reg
    return
.endm

; UART RX
SINGLE_BIT_SET UARTRXIE
SINGLE_BIT IEC0 U1RXIE
SINGLE_BIT IEC1 U2RXIE
SINGLE_BIT IEC5 U3RXIE
SINGLE_BIT IEC5 U4RXIE

SINGLE_BIT_SET UARTRXIF
SINGLE_BIT IFS0 U1RXIF
SINGLE_BIT IFS1 U2RXIF
SINGLE_BIT IFS5 U3RXIF
SINGLE_BIT IFS5 U4RXIF

MULTI_BIT_SET UARTRXIP 3
MULTI_BIT IPC2  U1RXIP0
MULTI_BIT IPC7  U2RXIP0
MULTI_BIT IPC20 U3RXIP0
MULTI_BIT IPC22 U4RXIP0

; UART TX
SINGLE_BIT_SET UARTTXIE
SINGLE_BIT IEC0 U1TXIE
SINGLE_BIT IEC1 U2TXIE
SINGLE_BIT IEC5 U3TXIE
SINGLE_BIT IEC5 U4TXIE

SINGLE_BIT_SET UARTTXIF
SINGLE_BIT IFS0 U1TXIF
SINGLE_BIT IFS1 U2TXIF
SINGLE_BIT IFS5 U3TXIF
SINGLE_BIT IFS5 U4TXIF

MULTI_BIT_SET UARTTXIP 3
MULTI_BIT IPC3  U1TXIP0
MULTI_BIT IPC7  U2TXIP0
MULTI_BIT IPC20 U3TXIP0
MULTI_BIT IPC22 U4TXIP0
