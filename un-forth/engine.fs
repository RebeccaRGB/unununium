: OP  CREATE , DOES> @ t, ;
: OPnnnn  CREATE , DOES> @ t, t, ;
: OPnn  CREATE , DOES> @ or t, ;

VOCABULARY ASM ALSO ASM DEFINITIONS

: org  tdp ! ;

4e00 OPnn   jne-forw
5e00 OPnn   jeq-forw
4e40 OPnn   jne-back
5e40 OPnn   jeq-back
ee40 OPnn   jmp-back
f040 OPnnnn call
f140 OP     int-off
fe80 OPnnnn goto
9a90 OP     retf

92d3 OP     r1=[r3++]
92e3 OP     r1=d[r3]
92f3 OP     r1=d[r3++]
9339 OP     r1>>=4
b30b OPnnnn r1=r3&#
9303 OP     r1=r3
9304 OP     r1=r4

470b OPnnnn r3==#
490c OPnnnn r4==#
0841 OP     r4++

d688 OP     push-r3
9488 OP     pop-r3

0240 OPnn   r1+=##
9240 OPnn   r1=##
9640 OPnn   r3=##
9840 OPnn   r4=##
a240 OPnn   r1|=##
b240 OPnn   r1&=##

9108 OPnnnn sp=#
9309 OPnnnn r1=#
970b OPnnnn r3=#
a309 OPnnnn r1|=#
b309 OPnnnn r1&=#
b50a OPnnnn r2&=#

9311 OPnnnn r1=[]
9512 OPnnnn r2=[]
d319 OPnnnn []=r1
d91c OPnnnn []=r4

4444 OP     noppie

PREVIOUS DEFINITIONS


ALSO ASM

c000 org

int-off
55aa r1=#  3d24 []=r1
3d23 dup r1=[]  fff9 r1&=#  04 r1|=##  []=r1
3d20 dup r1=[]  7fff r1&=#  []=r1
4006 r1=#  3d20 []=r1
02 r1=##  3d25 []=r1
08 r1=##  3d00 []=r1
3d0a dup r1=[]  07 r1|=##  []=r1
3d09 dup r1=[]  07 r1&=##  17 r1|=##  []=r1
3d08 dup r1=[]  07 r1&=##  17 r1|=##  []=r1
3d07 dup r1=[]  0f r1&=##  ff r1|=#  []=r1
88c0 r1=#  3d0e []=r1  3d0d []=r1
f77f r1=#  3d0b []=r1
0 r1=##  3d04 []=r1  3d03 []=r1
ffff r1=#  3d01 []=r1
27ff sp=#

c3 r1=#  3d30 []=r1
3d31 r1=[]
\ ff r1=#  3d34 []=r1  a8 r1=#  3d33 []=r1   \ baud rate 19200
ff r1=#  3d34 []=r1  f1 r1=#  3d33 []=r1   \ baud rate 115200
03 r1=##  3d31 []=r1
3d0f dup r1=[]  6000 r1|=#  []=r1
3d0e dup r1=[]  6000 r1|=#  []=r1
noppie noppie
3d0d dup r1=[]  4000 r1|=#  []=r1
noppie noppie
c100 goto


\ dump
c100 org

3d23 dup r1=[]  80 r1|=#  []=r1
30 r4=##  0 r3=##

3d2f []=r4

ff r1=r3&# 2 jne-forw c300 call

r1=d[r3]  c200 call
r1=d[r3++]  r1>>=4 r1>>=4 c200 call
0 r3==#  10 jne-back
r4++  40 r4==#  16 jne-back  1f jmp-back


c200 org

3d31 r2=[]  40 r2&=#  5 jne-back
3d35 []=r1
retf


\ message
c300 org

push-r3
r1=r4  c200 call
r1=r3  r1>>=4 r1>>=4  c200 call
r1=r3  c200 call

d000 r3=#

r1=[r3++]  3 jeq-forw
c200 call
5 jmp-back

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
