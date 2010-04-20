hex

: w@  dup >r c@ r> char+ c@ 8 lshift or ;
: w!  2dup c! >r 8 rshift r> char+ c! ;

10000 CONSTANT memsize
CREATE mem memsize 2* allot   mem memsize 2* erase

VOCABULARY CROSS ALSO CROSS DEFINITIONS

VARIABLE tdp

1 CONSTANT tcell
: tcell+  tcell + ;

: t!  2* mem + w! ;
: t@  2* mem + w@ ;

: tallot  tdp +! ;
: there  tdp @ ;

: t,   there tcell tallot t! ;
: talign  ;

\ : upc  dup [char] a [char] z 1+ within IF [char] a - [char] A + THEN ;
\ : t,"  dup tc, bounds ?DO i c@ tc, LOOP ;
\ : t,"upc  dup tc, bounds ?DO i c@ upc tc, LOOP ;

INCLUDE engine.fs

ONLY FORTH ALSO CROSS DEFINITIONS

VARIABLE tlast
VARIABLE tlatest
VOCABULARY TARGET
ALSO TARGET context @ CONSTANT target-wordlist PREVIOUS

: theader  talign there tlatest !
  there ALSO TARGET DEFINITIONS CONSTANT PREVIOUS DEFINITIONS ;
: treveal  tlatest @ tlast ! ;
\ : immediate  tlast @ tcell+ dup c@ 1 or swap tc! ;
: t'  name also TARGET evaluate PREVIOUS ;

\ here s" &&code_DOCOL" string, CONSTANT <docol>
\ 0 VALUE xt-do
\ 0 VALUE xt-?do
\ 0 VALUE xt-loop
\ 0 VALUE xt-+loop
\ 0 VALUE xt-var
\ 0 VALUE xt-val
\ 0 VALUE xt-dfr

\ VARIABLE leaves
\ : resolve-loop  leaves @ BEGIN ?dup WHILE
\                 dup @ swap there over - swap t! REPEAT
\                 there - t, leaves ! ;

: call, ( x -- )  f040 t, t, ;
: ret,            9a90 t, ;

: <mark ( -- dest )       there ;
: <jump ( dest insn -- )  there 1+ rot -
                          dup 40 >= ABORT" backward jump offset too big"
                          40 or or t, ;
: >mark ( insn -- orig )  there swap t, ;
: >jump ( orig -- )  there over - 1-
                     dup 40 >= ABORT" forward jump offset too big"
                     over t@ or swap t! ;

: 0test,  2841 t, 968d t, ;                    \ r4 -= 1 ; pop r4,r4 from [bp]

: *if    ( -- orig )  0test, 0e00 >mark ;      \ jb
: *ahead ( -- orig )         ee00 >mark ;      \ jmp
: *then  ( orig -- )              >jump ;

: *begin ( -- dest )              <mark ;
: *until ( dest -- )  0test, 0e00 <jump ;      \ jb
: *again ( dest -- )         ee00 <jump ;      \ jmp


: lit, ( x -- )  d88d t,                                  \ push r4,r4 to [bp]
                 dup 0 40 within IF 9840 or t, EXIT THEN          \ r4 = NN
                 dup -3f 0 within IF negate 6840 or t, EXIT THEN  \ r4 =- NN
                 990c t, t, ;                                     \ r4 = NNNN


VOCABULARY MACRO ALSO MACRO
context @ CROSS CONSTANT macro-wordlist MACRO DEFINITIONS PREVIOUS

\ : (  [char] ) parse 2drop ;
\ : \  0        parse 2drop ;
\ : code      theader treveal here name string, ts, ;
\ : variable  theader treveal <docol> ts, xt-var ta, 0 t, ;
\ : value     theader treveal <docol> ts, xt-val ta, t, ;
\ : defer     theader treveal <docol> ts, xt-dfr ta, 0 ta, ;
: :
  theader BEGIN name
  BEGIN ?dup 0= WHILE drop refill 0= ABORT" refill failed" name REPEAT
  2dup macro-wordlist search-wordlist IF nip nip execute ELSE
  2dup target-wordlist search-wordlist IF nip nip execute call, ELSE
  evaluate lit, THEN THEN AGAIN ;
: ;  ret, treveal r> drop ;
\ : lits  xt-lit ta, here name string, tl, ;
\ : [char]  xt-lit ta, name drop c@ t, ;
\ : docol  <docol> ts, ;

: IF    ( -- orig )  *if ;
: AHEAD ( -- orig )  *ahead ;
: THEN  ( orig -- )  *then ;

: BEGIN ( -- dest )  *begin ;
: UNTIL ( dest -- )  *until ;
: AGAIN ( dest -- )  *again ;

: ELSE  ( orig1 -- orig2 )      *ahead swap *then ;
: WHILE  ( dest -- orig dest )  *if swap ;
: REPEAT ( orig dest -- )       *again *then ;
\
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

: emit  c200 call, ;

ONLY FORTH ALSO CROSS ALSO MACRO

8000 tdp !


: cold  53 emit ;
: dada  BEGIN 53 emit AGAIN ;
: dadb  BEGIN 53 emit UNTIL ;
: dadc  1 IF 79 emit THEN  0 IF 6e emit THEN ;
: dadd  1 IF 79 emit ELSE 6e emit THEN ;
: dade  BEGIN emit WHILE emit REPEAT ;
: bla  1234 -1 0 1 3e 3f 40 -40 -3f -2 -1 5432 ;
: ook  5678 bla bla bla ;
: dat  9abc ook bla ook ;



ONLY FORTH DEFINITIONS

: make-file ( addr len name len -- )
  w/o bin open-file ABORT" couldn't create file"
  dup >r write-file ABORT" couldn't write file"
      r> close-file ABORT" couldn't close file" ;

mem 20000 s" t.b" make-file
