: OP  CREATE , DOES> @ t, ;
: OPnnnn  CREATE , DOES> @ t, t, ;
: OPnn  CREATE , DOES> @ or t, ;
: OP#   CREATE , , DOES> over 0 40 within IF cell+ @ or t, ELSE @ t, t, THEN ;



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
\   NN [bp] D ld          000
\   NN # D ld             040
\   (push/pop)            080
\   S  @ @+ +@ @-  D ld   0c0/0c8/0d0/0d8   d: 020
\   S D ld                100
\   NNNN # D ld           108
\   NNNN [] D ld          110   (store: 118)
\                         asr 120, lsl 140, lsr 160, rol 180. ror 1a0
\   NN [] D ld            1c0

VOCABULARY ASM ALSO ASM DEFINITIONS

: org  tdp ! ;

100 CONSTANT sp   101 CONSTANT r1   102 CONSTANT r2   103 CONSTANT r3
104 CONSTANT r4   105 CONSTANT bp   106 CONSTANT sr   107 CONSTANT pc

: ld  7 and 9 lshift or 9000 or t, ;

f040 OPnnnn call
f140 OP     int-off
fe80 OPnnnn goto
9a90 OP     retf    \ a special pop

92d3 OP     r1=[r3++]
92e3 OP     r1=d[r3]
92f3 OP     r1=d[r3++]
9339 OP     r1>>=4
b30b OPnnnn r1=r3&#

470b OPnnnn r3==#
490c OPnnnn r4==#
0841 OP     r4++

d688 OP     push-r3
9488 OP     pop-r3

0240 0309 OP# r1+=imm

9040 9108 OP# sp=imm
9240 9309 OP# r1=imm
9440 950a OP# r2=imm
9640 970b OP# r3=imm
9840 990c OP# r4=imm

a240 a309 OP# r1|=imm

b240 b309 OP# r1&=imm
b440 b50a OP# r2&=imm

9311 OPnnnn r1=[]
9512 OPnnnn r2=[]
d319 OPnnnn []=r1
d91c OPnnnn []=r4

4444 OP     noppie

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
55aa r1=imm  3d24 []=r1
3d23 dup r1=[]  fff9 r1&=imm  04 r1|=imm  []=r1
3d20 dup r1=[]  7fff r1&=imm  []=r1
4006 r1=imm  3d20 []=r1
02 r1=imm  3d25 []=r1
08 r1=imm  3d00 []=r1
3d0a dup r1=[]  07 r1|=imm  []=r1
3d09 dup r1=[]  07 r1&=imm  17 r1|=imm  []=r1
3d08 dup r1=[]  07 r1&=imm  17 r1|=imm  []=r1
3d07 dup r1=[]  0f r1&=imm  ff r1|=imm  []=r1
88c0 r1=imm  3d0e []=r1  3d0d []=r1
f77f r1=imm  3d0b []=r1
0 r1=imm  3d04 []=r1  3d03 []=r1
ffff r1=imm  3d01 []=r1
27ff sp=imm

c3 r1=imm  3d30 []=r1
3d31 r1=[]
\ ff r1=imm  3d34 []=r1  a8 r1=imm  3d33 []=r1   \ baud rate 19200
ff r1=imm  3d34 []=r1  f1 r1=imm  3d33 []=r1   \ baud rate 115200
03 r1=imm  3d31 []=r1
3d0f dup r1=[]  6000 r1|=imm  []=r1
3d0e dup r1=[]  6000 r1|=imm  []=r1
noppie noppie
3d0d dup r1=[]  4000 r1|=imm  []=r1
noppie noppie
c100 goto


\ dump
c100 org

BEGIN
3d23 dup r1=[]  80 r1|=imm  []=r1
30 r4=imm  0 r3=imm

BEGIN
3d2f []=r4

BEGIN
ff r1=r3&# 0= IF c300 call THEN

r1=d[r3]  c200 call
r1=d[r3++]  r1>>=4 r1>>=4 c200 call
0 r3==#  0= UNTIL
r4++  40 r4==#  = UNTIL
AGAIN



c200 org

BEGIN 3d31 r2=[]  40 r2&=imm  0= UNTIL
3d35 []=r1
retf


\ message
c300 org

push-r3
r4 r1 ld  c200 call
r3 r1 ld  r1>>=4 r1>>=4  c200 call
r3 r1 ld  c200 call

d000 r3=imm

BEGIN  r1=[r3++]  0<> WHILE  c200 call  REPEAT

pop-r3
retf


d000 org
char O t, char H t, char A t, char I t, bl t, char K t, char I t, char T t,
char T t, char E t, char H t, char S t, char ! t, 0 t,


fff5 org

ffff t, ffff t, c000 t,
ffff t, ffff t, ffff t, ffff t, ffff t, ffff t, ffff t, ffff t,


: make-file ( addr len name len -- )
  w/o bin open-file ABORT" couldn't create file"
  dup >r write-file ABORT" couldn't write file"
      r> close-file ABORT" couldn't close file" ;

mem 20000 s" t.b" make-file
