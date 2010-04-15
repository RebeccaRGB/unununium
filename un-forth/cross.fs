hex

\ here 1 , c@ -1 cells allot 0= CONSTANT host-big-endian?

10000 CONSTANT memsize
CREATE mem memsize 2* allot   mem memsize 2* erase

VARIABLE tdp

VOCABULARY CROSS ALSO CROSS DEFINITIONS

1 CONSTANT tcell
: tcell+  tcell + ;
\ : mem>offset  mem - tcell / 2/ ;
\ : (.)  dup >r abs 0 <# #s r> sign #> ;
\ : .offset  ?dup 0= IF ." 0" EXIT THEN
\            ." dict+" base @ swap decimal mem>offset (.) type base ! ;

: w@  dup >r c@ r> char+ c@ 8 lshift or ;
: w!  2dup c! >r 8 rshift r> char+ c! ;

: t!  2* mem + w! ;
\ : ta!  1 over >tag c!  t! ;
\ : tc!  2 over >tag c!  c! ;
\ : ts!  3 over >tag c!  t! ;
\ : tl!  4 over >tag c!  t! ;

: tallot  tdp +! ;
: there  tdp @ ;

: t,   there tcell tallot t! ;
\ : ta,  there tcell tallot ta! ;
\ : tc,  there     1 tallot tc! ;
\ : ts,  there tcell tallot ts! ;
\ : tl,  there tcell tallot tl! ;

: talign  ;

\ : upc  dup [char] a [char] z 1+ within IF [char] a - [char] A + THEN ;
\ : t,"  dup tc, bounds ?DO i c@ tc, LOOP ;
\ : t,"upc  dup tc, bounds ?DO i c@ upc tc, LOOP ;

VARIABLE tlast
VARIABLE tlatest
VOCABULARY TARGET
ALSO TARGET context @ CONSTANT target-wordlist PREVIOUS

\ : t(header)
\   there tlatest !  talign tlast @ ta,
\   there 4 - >tag dup c@ 80 or swap c!
\   0 tc, t,"upc talign ;
\ : theader  save-input name t(header) restore-input
\   there ALSO TARGET DEFINITIONS CONSTANT PREVIOUS DEFINITIONS ;
\ : treveal  tlatest @ tlast ! ;
\ : immediate  tlast @ tcell+ dup c@ 1 or swap tc! ;
\ : t'  name also TARGET evaluate PREVIOUS ;
\
\ : t.char
\   dup 20 - 5f U< over 22 <> and over 5c <> and IF emit EXIT THEN
\   base @ >r 8 base ! 0 <# # # # [char] \ hold #> type r> base ! ;
\ : t.chars  4 bounds DO i >tag c@ 0= ?LEAVE i c@ t.char LOOP ;
\ : tfinish
\   mem BEGIN dup there < WHILE
\   dup >tag c@ dup 80 and IF cr ELSE space THEN ." {"
\   7f and CASE
\   0 OF ." .n=" base @ decimal over @ (.) type base ! ENDOF
\   1 OF ." .a=" dup @ .offset ENDOF
\   2 OF ." .c=" [char] " emit dup t.chars [char] " emit ENDOF
\   3 OF ." .a=" dup @ count type ENDOF
\   4 OF ." .n=" dup @ count type ENDOF
\   ." UNKNOWN TAG " dup .
\   ENDCASE ." },"
\   tcell+ REPEAT drop ;
\
\ here s" &&code_DOCOL" string, CONSTANT <docol>
\ 0 VALUE xt-exit
\ 0 VALUE xt-lit
\ 0 VALUE xt-branch
\ 0 VALUE xt-0branch
\ 0 VALUE xt-do
\ 0 VALUE xt-?do
\ 0 VALUE xt-loop
\ 0 VALUE xt-+loop
\ 0 VALUE xt-var
\ 0 VALUE xt-val
\ 0 VALUE xt-dfr
\
\ : resolve-orig  there over tcell+ - swap t! ;
\ : *ahead  xt-branch ta, there 0 t, ;
\ : *if    xt-0branch ta, there 0 t, ;
\ : *then  resolve-orig ;
\ : resolve-dest  there tcell+ - t, ;
\ : *begin  there ;
\ : *again  xt-branch ta, resolve-dest ;
\ : *until  xt-0branch ta, resolve-dest ;
\
\ VARIABLE leaves
\ : resolve-loop  leaves @ BEGIN ?dup WHILE
\                 dup @ swap there over - swap t! REPEAT
\                 there - t, leaves ! ;

VOCABULARY MACRO ALSO MACRO
context @ CROSS CONSTANT macro-wordlist MACRO DEFINITIONS PREVIOUS

\ : (  [char] ) parse 2drop ;
\ : \  0        parse 2drop ;
\ : code      theader treveal here name string, ts, ;
\ : variable  theader treveal <docol> ts, xt-var ta, 0 t, ;
\ : value     theader treveal <docol> ts, xt-val ta, t, ;
\ : defer     theader treveal <docol> ts, xt-dfr ta, 0 ta, ;
\ : :
\   theader <docol> ts, BEGIN name
\   BEGIN ?dup 0= WHILE drop refill 0= ABORT" refill failed" name REPEAT
\   2dup macro-wordlist search-wordlist IF nip nip execute ELSE
\   2dup target-wordlist search-wordlist IF nip nip execute ta, ELSE
\   evaluate xt-lit ta, t, THEN THEN AGAIN ;
\ : ;  xt-exit ta, treveal r> drop ;
\ : lits  xt-lit ta, here name string, tl, ;
\ : [char]  xt-lit ta, name drop c@ t, ;
\ : docol  <docol> ts, ;
\
\ : ahead  *ahead ;
\ : if     *if ;
\ : then   *then ;
\ : else   *ahead swap *then ;
\
\ : begin  *begin ;
\ : again  *again ;
\ : until  *until ;
\ : while   *if swap ;
\ : repeat  *again *then ;
\
\ : do  leaves @ there xt-do ta, 0 leaves ! ;
\ : ?do  leaves @ xt-?do ta, there leaves ! there 0 t, ;
\ : loop  xt-loop ta, resolve-loop ;
\ : +loop  xt-+loop ta, resolve-loop ;
\ \ : leave dotick xxdoleave compile, leaves @ here leaves ! compile, ; IMMEDIATE
\ \ : ?leave dotick xxdo?leave compile, leaves @ here leaves ! compile, ; IMMEDIATE
\ \ imm(CASE 0)
\ \ imm(ENDCASE DOTICK DROP COMPILE, ?DUP 0BRANCH(5) 1- SWAP THEN BRANCH(-8))
\ \ imm(OF 1+ >R DOTICK OVER COMPILE, DOTICK = COMPILE, IF DOTICK DROP COMPILE, R>)
\ \ imm(ENDOF >R ELSE R>)
\
\ : [compile]
\   name target-wordlist search-wordlist 0= ABORT" bad [compile]"
\   execute ta, ;



ALSO MACRO

INCLUDE engine.fs
