: OPx CREATE , DOES> @ t, ;
: OPnnnn  CREATE , DOES> @ t, t, ;
: OPnn  CREATE , DOES> @ or t, ;
: OP#   CREATE , , DOES> over 0 40 within IF cell+ @ or t, ELSE @ t, t, THEN ;


VARIABLE imm
: imm?  ( op -- f )  1f8 and 108 120 within ;
: short?   ( -- f )  imm @ 0 40 within ;

: 2OP  CREATE , DOES>
  @ >r 7 and 9 lshift or r> or dup imm? IF short? IF
  dup fe00 and swap 38 and 08 = IF 40 ELSE 1c0 THEN or imm @ or t, ELSE
  dup f038 and d010 = IF 8 + THEN
  dup 9 rshift 7 and or t, imm @ t, THEN EXIT THEN
  t, ;


\   x D 0 imm   [bp+NN]                    6 cyc           D <> 7         st
\   x D 1 imm   NN                         2 cyc           D <> 7
\   x D 2 s S   r{D-s-1},rD to/from [rS]   4+2*s cyc                      st
\   x D 3 x S   [rS] and friends           6 cyc                          st
\   x D 4 0 S   rS                         3 cyc
\   x D 4 1 S   NNNN                       4 cyc           ld ignores S
\   x D 4 2 S   [NNNN]                     7 cyc           ld ignores S
\   x D 4 3 S   [NNNN] = rD op rS          7 cyc           st ignores S   st
\   x D 4 x S   rS asr                     3 cyc
\   x D 5 x S   rS lsl/lsr                 3 cyc
\   x D 6 x S   rS rol/ror                 3 cyc
\   x D 7 imm   [NN]                       5 cyc                          st



\   load:
\   NN [bp] D :=          000
\ \   NN # D :=             040
\   (push/pop)            080
\ \   S  @ @- @+ +@  D :=   0c0/0c8/0d0/0d8   d: 020
\ \   S D :=                100
\ \   NNNN # D :=           108
\ \   NNNN [] D :=          110   (store: 118)
\                         asr 120, lsl 140, lsr 160, rol 180. ror 1a0
\ \   NN [] D :=            1c0

VOCABULARY ASM ALSO ASM DEFINITIONS

: org  tdp ! ;

\ The registers.  Sneakily encode "register direct" addressing mode in these.
100 CONSTANT sp   101 CONSTANT r1   102 CONSTANT r2   103 CONSTANT r3
104 CONSTANT r4   105 CONSTANT bp   106 CONSTANT sr   107 CONSTANT pc

\ The addressing modes.
: d:  20 or ;
: @   ff and 0c0 or ;
: @-  ff and 0c8 or ;
: @+  ff and 0d0 or ;
: +@  ff and 0d8 or ;
: #   imm ! 108 ;
: []  imm ! 110 ;

\ The ops.
0000 2OP +=
4000 2OP cmp
9000 2OP :=
a000 2OP |=
b000 2OP &=
d000 2OP st



f040 OPnnnn call
f140 OPx    int-off
fe80 OPnnnn goto
9a90 OPx    retf    \ a special pop

9339 OPx    r1>>=4
b30b OPnnnn r1=r3&#

d688 OPx    push-r3
9488 OPx    pop-r3

4444 OPx    noppie



: IF    ( insn -- orig )  there swap t, ;
: THEN  ( orig -- )  there over - 1-
                     dup 40 >= ABORT" forward jump offset too big"
                     over t@ or swap t! ;
: BEGIN  ( -- dest )  there ;
: UNTIL  ( dest insn -- )  there 1+ rot -
                           dup 40 >= ABORT" backward jump offset too big"
                           40 or or t, ;
: AGAIN  ( dest -- )  ee00 UNTIL ;
: WHILE  ( dest insn -- orig dest )  IF swap ;
: REPEAT ( orig dest -- )  AGAIN THEN ;
: AHEAD  ( -- orig )  ee00 IF ;
: ELSE   ( orig1 -- orig2 )  AHEAD swap THEN ;

4e00 CONSTANT 0=    4e00 CONSTANT =
5e00 CONSTANT 0<>   5e00 CONSTANT <>


PREVIOUS DEFINITIONS


ALSO ASM

c000 org

int-off
55aa # r1 :=   3d24 [] r1 st
3d23 dup [] r1 :=   fff9 # r1 &=   04 # r1 |=   [] r1 st
3d20 dup [] r1 :=   7fff # r1 &=   [] r1 st
4006 # r1 :=   3d20 [] r1 st
02 # r1 :=   3d25 [] r1 st
08 # r1 :=   3d00 [] r1 st
3d0a dup [] r1 :=    07 # r1 |=   [] r1 st
3d09 dup [] r1 :=    07 # r1 &=   17 # r1 |=   [] r1 st
3d08 dup [] r1 :=    07 # r1 &=   17 # r1 |=   [] r1 st
3d07 dup [] r1 :=    0f # r1 &=   ff # r1 |=   [] r1 st
88c0 # r1 :=   3d0e [] r1 st  3d0d [] r1 st
f77f # r1 :=   3d0b [] r1 st
0 # r1 :=   3d04 [] r1 st  3d03 [] r1 st
ffff # r1 :=   3d01 [] r1 st
27ff # sp :=
23ff # bp :=

8000 goto
c3 # r1 :=   3d30 [] r1 st
3d31 [] r1 :=
\ ff # r1 :=   3d34 [] r1 st  a8 # r1 :=   3d33 [] r1 st   \ baud rate 19200
ff # r1 :=   3d34 [] r1 st  f1 # r1 :=   3d33 [] r1 st   \ baud rate 115200
03 # r1 :=   3d31 [] r1 st
3d0f dup [] r1 :=   6000 # r1 |=   [] r1 st
3d0e dup [] r1 :=   6000 # r1 |=   [] r1 st
noppie noppie
3d0d dup [] r1 :=   4000 # r1 |=   [] r1 st
noppie noppie
\ c100 goto


\ dump
c100 org

BEGIN
  3d23 dup [] r1 :=    80 # r1 |=   [] r1 st
  30 # r4 :=  0 # r3 :=
  BEGIN
    3d2f [] r4 st
    BEGIN
      ff r1=r3&# 0= IF c300 call THEN
      r3 d: @  r1 :=                 c200 call
      r3 d: @+ r1 :=  r1>>=4 r1>>=4  c200 call
    0 # r3 cmp  = UNTIL
  1 # r4 +=   40 # r4 cmp  = UNTIL
AGAIN



c200 org

BEGIN 3d31 [] r2 :=   40 # r2 &=   0= UNTIL
3d35 [] r1 st
retf


\ message
c300 org

push-r3
r4 r1 :=  c200 call
r3 r1 :=  r1>>=4 r1>>=4  c200 call
r3 r1 :=  c200 call

d000 # r3 :=

BEGIN  r3 @+ r1 :=  0<> WHILE  c200 call  REPEAT

pop-r3
retf


d000 org
char O t, char H t, char A t, char I t, bl t, char K t, char I t, char T t,
char T t, char E t, char H t, char S t, char ! t, 0 t,


fff5 org

ffff t, ffff t, c000 t,
ffff t, ffff t, ffff t, ffff t, ffff t, ffff t, ffff t, ffff t,
