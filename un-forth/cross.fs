hex

: +order ( wid -- )  >r get-order r> swap 1+ set-order ;

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

wordlist CONSTANT target-wordlist

: theader  talign there tlatest !
  get-current target-wordlist set-current   there CONSTANT   set-current ;
: treveal  tlatest @ tlast ! ;
\ : immediate  tlast @ tcell+ dup c@ 1 or swap tc! ;
: t'  name target-wordlist +order evaluate PREVIOUS ;

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


: push,   d88d t, ;         \ push r4,r4 to [bp]
: pop,    968d t, ;         \ pop r4,r4 from [bp]
: pop3,   948d t, ;         \ pop r3,r3 from [bp]
: pushr,  d888 t, ;         \ push r4,r4 to [sp]
: popr,   9688 t, ;         \ pop r4,r4 from [sp]

: lit, ( x -- )  push,
                 dup 0 40 within IF 9840 or t, EXIT THEN          \ r4 = NN
                 dup -3f 0 within IF negate 6840 or t, EXIT THEN  \ r4 =- NN
                 990c t, t, ;                                     \ r4 = NNNN


: 0test,  2841 t, pop, ;    \ r4 -= 1

: *if    ( -- orig )  0test, 0e00 >mark ;      \ jb
: *ahead ( -- orig )         ee00 >mark ;      \ jmp
: *then  ( orig -- )              >jump ;

: *begin ( -- dest )              <mark ;
: *until ( dest -- )  0test, 0e00 <jump ;      \ jb
: *again ( dest -- )         ee00 <jump ;      \ jmp


VARIABLE ram  100 ram !


wordlist CONSTANT macro-wordlist
macro-wordlist set-current

: (  [char] ) parse 2drop ;
: \         0 parse 2drop ;

: CODE      theader treveal ;
: END-CODE  ret, ;

: CONSTANT  theader treveal lit, ret, ;
: VARIABLE  theader treveal ram @ lit, ret,  1 ram +! ;
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
: [char]  name drop c@ lit, ;
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


\
\ Primitives.
\

: dup   push, ;
: over  push, 9802 t, ;       \ r4 = [bp+2]
: swap  pop3, push, 9903 t, ; \ r4 = r3
: drop  pop, ;
: nip   0a41 t, ;             \ bp += 1

: >r    pushr, pop, ;
: r>    push, popr, ;
: r@    push, popr, pushr, ;

: @     98c4 t, ;             \ r4 = [r4]
: !     pop3, d6c4 t, pop, ;  \ [r4] = r3

: negate  6904 t, ;           \ r4 =- r4

: +     08dd t, ;             \ r4 += [++bp]
: swap- 28dd t, ;             \ r4 -= [++bp]
: -     28dd t, 6904 t, ;
: and   b8dd t, ;             \ r4 &= [++bp]
: or    a8dd t, ;             \ r4 |= [++bp]

: 0=    2841 t, 3904 t, ;     \ r4 -= 1 ; r4 -= r4, carry



8000 tdp !   fe80 t, 0 t,
ONLY FORTH ALSO CROSS macro-wordlist +order

\ temp
: emit  BEGIN 3d31 @ 40 and 0= UNTIL 3d35 ! ;

\ \ \ code tib      &&code_TIB                  \ XXX: shouldn't be primitive
\ \ \ code pockets  &&code_POCKETS                  \ XXX: shouldn't be primitive
\ \ \ code dotick   &&code_DOTICK
\ \ \ code pick     &&code_PICK
\ \ \ code swap     &&code_SWAP
\ \ \ code depth    &&code_DEPTH
\ \ \ code depth!   &&code_DEPTH_X21
\ \ \ code rdepth   &&code_RDEPTH
\ \ \ code rdepth!  &&code_RDEPTH_X21
\ \ \ code *        &&code_X2a
\ \ \ code lshift   &&code_LSHIFT
\ \ \ code rshift   &&code_RSHIFT
\ \ \ code ashift   &&code_ASHIFT
\ \ \ code xor      &&code_XOR
: c@  @ ;
: c!  ! ;
\ \ \ code w@       &&code_W_X40
\ \ \ code w!       &&code_W_X21
\ \ \ code l@       &&code_L_X40
\ \ \ code l!       &&code_L_X21
\ \ \ code u<       &&code_U_X3c
\ \ \ code =        &&code_X3d
\ \ \ code dodo     &&code_DODO
\ \ \ code do?do    &&code_DO_X3f_DO
\ \ \ code doloop   &&code_DOLOOP
\ \ \ code do+loop  &&code_DO_X2b_LOOP
\ \ \ code doleave  &&code_DOLEAVE
\ \ \ code do?leave &&code_DO_X3f_LEAVE
\ \ \ code exit     &&code_EXIT
\ \ \ code execute  &&code_EXECUTE
\ \ \ code move     &&code_MOVE
\ \ \ code findit   &&code_FINDIT
\ \ \
\ \ \
\ \ \
\ \ \ : /char  1 ;
\ \ \ : /cell  LITS CELLSIZE ;
\ \ \ : char+  /char + ;
\ \ \ : cell+  /cell + ;
\ \ \ : char-  /char - ;
\ \ \ : cell-  /cell - ;
\ \ \ : chars  /char * ;
\ \ \ : cells  /cell * ;
\ \ \ : chars+  chars + ;
\ \ \ : cells+  cells + ;
\ \ \
\ \ \
\ \ \ : var  r> cell+ ;
\ \ \ : val  r> cell+ @ ;
\ \ \ : dfr  r> cell+ @ execute ;
\ \ \
\ \ \ t' var to xt-var
\ \ \ t' val to xt-val
\ \ \ t' dfr to xt-dfr


VARIABLE dp
: here  dp @ ;


\ \ \ VARIABLE forth-wordlist tcell negate tallot 3039 t,
\ \ \ .( #define FWL () t' forth-wordlist .offset .( +2) char ) emit
\ \ \ 0 VALUE current         tcell negate tallot here s" (type_u)FWL" string, tl,
\ \ \ VARIABLE search-order   tcell negate tallot here s" FWL" string, ts,
\ \ \          0 ta, 0 ta, 0 ta, 0 ta, 0 ta, 0 ta, 0 ta, 0 ta,
\ \ \          0 ta, 0 ta, 0 ta, 0 ta, 0 ta, 0 ta, 0 ta,
\ \ \ cr .( #define SO () t' search-order .offset .( +2) char ) emit
\ \ \ 0 VALUE context         tcell negate tallot here s" (type_u)SO" string, tl,
\ \ \
\ \ \
\ \ \ DEFER emit
\ \ \ DEFER key
\ \ \ DEFER key?
\ \ \ DEFER accept
\ \ \
\ \ \ DEFER .exception
\ \ \ DEFER print-status
\ \ \
\ \ \ DEFER (reveal)
\ \ \ DEFER (find)
\ \ \
\ \ \
\ \ \ VARIABLE catcher
\ \ \ VARIABLE abort"-str
\ \ \
\ \ \ VARIABLE #tib
\ \ \ 0 VALUE ib
\ \ \ VARIABLE #ib
\ \ \ 0 VALUE source-id
\ \ \ VARIABLE >in
\ \ \ VARIABLE span

VARIABLE base

\ \ \ VARIABLE latest
\ \ \ VARIABLE whichpocket
\ \ \ VARIABLE state
\ \ \
\ \ \
\ \ \ : 0  LITS 0 ;
\ \ \ : 1  LITS 1 ;
\ \ \ : 2  LITS 2 ;
\ \ \ : 3  LITS 3 ;
\ \ \ : true   -1 ;
\ \ \ : false LITS 0 ;
\ \ \
\ \ \
\ \ \ : doto  r> cell+ dup >r @ cell+ cell+ ! ;
\ \ \ : sliteral  r> cell+ dup dup c@ + LITS -CELLSIZE and >r ;
\ \ \
\ \ \
\ \ \ : ?dup   dup IF dup THEN ;
: tuck   swap over ;
: 2dup   over over ;
\ \ \ : 3dup   2 pick 2 pick 2 pick ;
\ \ \ : 2over  3 pick 3 pick ;
: 2drop  drop drop ;
\ \ \ : 3drop  drop drop drop ;
\ \ \ : clear  0 depth! ;
: rot    >r swap r> swap ;
\ \ \ : -rot   swap >r swap r> ;
\ \ \ : 2swap  >r -rot r> -rot ;
\ \ \ : 2rot   >r >r 2swap r> r> 2swap ;


: 2*   dup + ;
\ \ \ : u2/  1 rshift ;
\ \ \ : 2/   1 ashift ;
\ \ \ : <<   lshift ;
\ \ \ : >>   rshift ;
\ \ \ : >>a  ashift ;
\ \ \ : invert true xor ;
\ \ \ : not  invert ;


CODE <   48dd t, ae02 t, 6841 t, ee01 t, 9840 t, END-CODE
CODE >   48dd t, 2e02 t, 6841 t, ee01 t, 9840 t, END-CODE
CODE u<  48dd t, 8e02 t, 6841 t, ee01 t, 9840 t, END-CODE

: 0<  0 < ;
\ \ \ : >    swap < ;
\ \ \ : u>   swap u< ;
\ \ \ : <=   > 0= ;
\ \ \ : <>   = 0= ;
\ \ \ : >=   < 0= ;
\ \ \ : 0<=  0 <= ;
\ \ \ : 0<>  0 <> ;
\ \ \ : 0>   0 > ;
\ \ \ : 0>=  0 >= ;
\ \ \ : u<=  u> 0= ;
: u>=  u< 0= ;
\ \ \ : within  over - >r - r> u< ;
\ \ \ : between  >r over <= swap r> <= and ;

CODE (do)
\ pop r2,r2 from sp		ret
\ r3 = [++bp]			limit
\ r4 -= r3			index-limit
\ push r3,r4 to sp
\ r4 = [++bp]
\ pc = r2
END-CODE

: d2*   2* over 0< - >r 2* r> ;
\ \ \ : ud2/  >r u2/ r@ LITS 8*CELLSIZE-1 lshift or r> u2/ ;
\ \ \ : d2/   >r u2/ r@ LITS 8*CELLSIZE-1 lshift or r> 2/ ;
\ \ \
\ \ \
\ \ \ : negate  0 swap - ;
: abs  dup 0< IF negate THEN ;
\ \ \ : max  2dup < IF swap THEN drop ;
\ \ \ : min  2dup > IF swap THEN drop ;
\ \ \ : u*  * ;
: 1+  1 + ;
: 1-  1 - ;
\ \ \ : 2+  2 + ;
\ \ \ : 2-  2 - ;
\ \ \ : even  dup 1 and + ;
\ \ \ : bounds  over + swap ;
\ \ \
\ \ \
\ \ \ : s>d dup 0< ;
\ \ \ : dnegate  invert >r negate dup 0= r> swap - ;
\ \ \ : dabs  dup 0< IF dnegate THEN ;
\ \ \ : m+  swap >r dup >r + dup r> u< r> swap - ;
\ \ \ : d+  >r m+ r> + ;
\ \ \ : d-  dnegate d+ ;
\ \ \ : *'  >r dup 0< >r d2* r> IF r@ m+ THEN r> ;
\ \ \ : um*  0 -rot LITS 8*CELLSIZE 0 DO *' LOOP drop ;
\ \ \ : m*  2dup xor >r >r abs r> abs um* r> 0< IF dnegate THEN ;
: ....  10 BEGIN dup WHILE >r dup 8000 and IF 31 ELSE 30 THEN emit 2* r> 1- REPEAT 2drop 20 emit ;
\ : /'  7777 drop  >r dup 0< >r d2* r> over r@    0a emit over .... dup ....   u>= or IF >r 1 or r> r@ - THEN r> ;
: /'  7777 drop  >r dup 0< >r d2* r> over r@ u>= or IF >r 1 or r> r@ - THEN r> ;


\ \ \ : um/mod  LITS 8*CELLSIZE 0 DO /' LOOP drop swap ;
: um/mod  10 BEGIN dup WHILE >r /' r> 1- REPEAT drop drop swap ;
\ \ \ : sm/rem
\ \ \   over >r >r dabs r@ abs um/mod
\ \ \   r> 0< IF negate THEN
\ \ \   r> 0< IF negate swap negate swap THEN ;
\ \ \ : fm/mod
\ \ \   dup >r 2dup xor 0< >r sm/rem
\ \ \   over 0<> r> and IF 1- swap r> + swap EXIT THEN
\ \ \   r> drop ;
\ \ \
\ \ \
: u/mod  0 swap um/mod ;
\ \ \ : /mod   >r s>d r> fm/mod ;
\ \ \ : /      /mod nip ;
\ \ \ : mod    /mod drop ;
\ \ \ : */mod  >r m* r> fm/mod ;
\ \ \ : */     */mod nip ;
\ \ \
\ \ \
\ \ \ : aligned  /cell 1- + /cell negate and ;
\ \ \
\ \ \
\ \ \ : i  r> r@ swap >r ;
\ \ \ : j  r> r> r> r@ swap >r swap >r swap >r ;
\ \ \ : unloop  r> r> r> 2drop >r ;
\ \ \
\ \ \
\ \ \ : +!  tuck @ + swap ! ;
\ \ \ : comp
\ \ \   0 ?DO over i + c@ over i + c@
\ \ \         2dup < IF 2drop unloop 2drop -1 EXIT THEN
\ \ \              > IF       unloop 2drop  1 EXIT THEN
\ \ \   LOOP 2drop 0 ;
\ \ \ : off  false swap ! ;
\ \ \ : on   true swap ! ;
\ \ \ : <w@  w@ dup LITS 0x8000 >= IF LITS 0x10000 - THEN ;
\ \ \ : 2@   dup cell+ @ swap @ ;
\ \ \ : 2!   dup >r ! r> cell+ ! ;
\ \ \ : fill  -rot bounds ?DO dup i c! LOOP drop ;
\ \ \ : blank  20 fill ;
\ \ \ : erase   0 fill ;
\ \ \
\ \ \
\ \ \ : catch
\ \ \   depth >r  catcher @ >r
\ \ \   rdepth catcher !  execute
\ \ \   r> catcher !  r> drop 0 ;
\ \ \ : throw
\ \ \   ?dup IF catcher @ rdepth!
\ \ \           r> catcher !  r> swap >r depth!
\ \ \           drop r> THEN ;
\ \ \ : abort  -1 throw ;
\ \ \
\ \ \
\ \ \ : source  ib #ib @ ;
\ \ \ : terminal  tib doto ib  #tib @ #ib !  0 doto source-id ;
\ \ \
\ \ \
: bl       20 ;
\ \ \ : bell     07 ;
\ \ \ : bs       08 ;
: carret   0d ;
: linefeed 0a ;
\ \ \
\ \ \
: emit  BEGIN 3d31 @ 40 and 0= UNTIL 3d35 ! ;
\ \ \ : type    bounds ?DO i c@ emit LOOP ;
: type  BEGIN dup WHILE over c@ emit 1- >r 1+ r> REPEAT 2drop ;
: cr      carret emit linefeed emit ;
: space   bl emit ;
\ \ \ : spaces  BEGIN dup 0> WHILE space 1- REPEAT drop ;
\ \ \
\ \ \
\ \ \ : count    dup char+ swap c@ ;
\ \ \ : /string  tuck - >r chars + r> ;
\ \ \
\ \ \ : upc  dup [char] a [char] z between IF 20 - THEN ;
\ \ \ : lcc  dup [char] A [CHAR] Z between IF 20 + THEN ;
\ \ \
\ \ \
\ \ \ : expect  accept span ! ;
\ \ \ : refill
\ \ \   source-id 0= IF source expect 0 >in ! true EXIT THEN
\ \ \   source-id -1 = IF false EXIT THEN
\ \ \   6502 throw ;
\ \ \
\ \ \
\ \ \ : decimal  LITS 10   base ! ;
: hex      10 base ! ;
\ \ \ : octal    LITS 010  base ! ;


: pad  here 100 + ;
: todigit  dup 9 > 27 and + [char] 0 + ;
: mu/mod  dup >r u/mod r> swap >r um/mod r> ;
: <#  pad dup ! ;
: hold  pad dup @ 1- tuck swap ! c! ;
: sign  0< IF [char] - hold THEN ;
: #  base @ mu/mod rot todigit hold ;
: #s  BEGIN # 2dup or WHILE REPEAT ;
: #>  666 drop   2drop pad dup @ tuck - ;
: (.)  <# dup >r abs 0 #s r> sign #> ;
\ \ \ : u#  base @ u/mod swap todigit hold ;
\ \ \ : u#s  BEGIN u# dup WHILE REPEAT ;
\ \ \ : u#>  drop pad dup @ tuck - ;
\ \ \ : (u.)  <# u#s u#> ;
: .  (.) type space ;
\ \ \ : s.  . ;
\ \ \ : u.  (u.) type space ;
\ \ \ : .r   >r (.)  r> over - spaces type ;
\ \ \ : u.r  >r (u.) r> over - spaces type ;
\ \ \ : .d  base @ swap decimal . base ! ;
\ \ \ : .h  base @ swap hex . base ! ;
\ \ \ : .s  depth dup 0< IF drop EXIT THEN 0 ?DO depth i - 1- pick . LOOP ;
\ \ \ : ?  @ . ;
\ \ \
\ \ \
\ \ \ : digit? ( c -- u true | false )
\ \ \   upc dup 30 3a within IF 30 - ELSE
\ \ \       dup 41 5b within IF 37 - ELSE drop false EXIT THEN THEN
\ \ \   dup base @ < IF true EXIT THEN drop false ;
\ \ \
\ \ \ : digit+ ( ud u -- ud' )
\ \ \   swap base @ u* >r >r base @ um* r> r> d+ ;
\ \ \
\ \ \ : >number ( ud str len -- ud' str' len' )
\ \ \   BEGIN dup WHILE over c@ digit? WHILE
\ \ \   -rot 1 /string >r >r digit+ r> r> REPEAT THEN ;
\ \ \
\ \ \ : $number
\ \ \   dup 0= IF 2drop true EXIT THEN
\ \ \   >r dup >r c@ [char] - = dup IF
\ \ \     r> char+ r> 1- dup 0= IF 3drop true EXIT THEN
\ \ \     >r >r THEN
\ \ \   0 0 r> r> >number nip 0= IF
\ \ \     drop swap IF negate THEN
\ \ \   false EXIT THEN
\ \ \   3drop true ;
\ \ \
\ \ \
\ \ \ : allot  dp +! ;
\ \ \ : ,  here ! /cell allot ;
\ \ \ : c,  here c! /char allot ;
\ \ \ : align  BEGIN here /cell 1- and WHILE 0 c, REPEAT ;
\ \ \ : place  2dup c! char+ swap chars bounds ?DO dup c@ i c! char+ 1 chars +LOOP drop ;
\ \ \ : string,  here over 1+ chars allot place ;
\ \ \
\ \ \
\ \ \ : noop  ;
\ \ \
\ \ \
\ \ \ : last  current ;
\ \ \
\ \ \
\ \ \ : link>name  cell+ ;
\ \ \ : name>  char+ dup c@ 1+ chars+ aligned ;
\ \ \ : link>  link>name name> ;
\ \ \ : name>string  char+ count ;
\ \ \
\ \ \
\ \ \ : header  align here last @ , latest ! 0 c, string, align ;
\ \ \ : reveal  latest @ link>name name>string (reveal) latest @ last ! ;
\ \ \
\ \ \
\ \ \ : string=ci  >r swap dup r> <> IF 3drop false EXIT THEN chars bounds ?DO dup c@ upc i c@ upc <> IF drop unloop false EXIT THEN char+ 1 chars +LOOP drop true ;
\ \ \ : (find-order)  context BEGIN dup >r search-order u>= WHILE 2dup r@ @ @ (find) ?dup IF nip nip r> drop EXIT THEN r> cell- REPEAT r> 3drop 0 ;
\ \ \ : $find  (find-order) dup IF link>name dup name> swap c@ true THEN ;
\ \ \
\ \ \
\ \ \ : 'immediate  1 ;
\ \ \ : immediate?  'immediate and 0<> ;
\ \ \ : immediate  last @ cell+ dup c@ 'immediate or swap c! ;
\ \ \
\ \ \
\ \ \ : words  last @ BEGIN ?dup WHILE dup cell+ char+ count type space @ REPEAT ;
\ \ \
\ \ \
\ \ \ : findchar  swap 0 ?DO over i + c@ over dup bl = IF <= ELSE = THEN IF i unloop nip nip true EXIT THEN LOOP 2drop false ;
\ \ \ : parse  >r ib >in @ + span @ >in @ - 2dup r> findchar IF nip dup 1 + ELSE dup THEN >in +! ;
\ \ \ : skipws  ib span @ BEGIN dup >in @ > WHILE over >in @ + c@ bl <= WHILE 1 >in +! REPEAT THEN 2drop ;
\ \ \ : parse-word  skipws bl parse ;
\ \ \ : pocket  LITS POCKETSIZE whichpocket @ * pockets + 1 whichpocket @ - whichpocket ! ;
\ \ \ : word  pocket >r parse dup r@ c! bounds r> dup 2swap ?DO char+ i c@ over c! LOOP drop ;
\ \ \
\ \ \
\ \ \ : char  parse-word drop c@ ;
\ \ \ : (  [char] ) parse 2drop ; IMMEDIATE
\ \ \ : \  linefeed parse 2drop ; IMMEDIATE
\ \ \
\ \ \
\ \ \ : [  state off ; IMMEDIATE
\ \ \ : ]  state on ;
\ \ \ : compile,  , ;
\ \ \ : :  parse-word header dotick docol compile, ] ;
\ \ \ : :noname  align here dotick docol compile, ] ;
\ \ \ : ;  dotick exit compile, reveal [ ; IMMEDIATE
\ \ \
\ \ \
\ \ \ : c"  [char] " parse dotick sliteral compile, dup c, bounds ?DO i c@ c, LOOP align ; IMMEDIATE
\ \ \ : s"  state @ IF c" dotick count compile, EXIT THEN [char] " parse dup >r pocket dup >r swap move r> r> ; IMMEDIATE
\ \ \ : ."  s" dotick type compile, ; IMMEDIATE
\ \ \ : .(  [char] ) parse type ; IMMEDIATE
\ \ \
\ \ \
\ \ \ : resolve-orig  here over cell+ - swap ! ;
\ \ \ : ahead  dotick branch compile, here 0 compile, ; IMMEDIATE
\ \ \ : if  dotick 0branch compile, here 0 compile, ; IMMEDIATE
\ \ \ : then  resolve-orig ; IMMEDIATE
\ \ \ : else  dotick branch compile, here 0 compile, swap resolve-orig ; IMMEDIATE
\ \ \ : case  0 ; IMMEDIATE
\ \ \ : endcase  dotick drop compile, BEGIN ?dup WHILE 1- swap [compile] then REPEAT ; IMMEDIATE
\ \ \ : of  1+ >r dotick over compile, dotick = compile, [compile] if dotick drop compile, r> ; IMMEDIATE
\ \ \ : endof  >r [compile] else r> ; IMMEDIATE
\ \ \ : resolve-dest  here cell+ - compile, ;
\ \ \ : begin  here ; IMMEDIATE
\ \ \ : again  dotick branch compile, resolve-dest ; IMMEDIATE
\ \ \ : until  dotick 0branch compile, resolve-dest ; IMMEDIATE
\ \ \ : while  [compile] if swap ; IMMEDIATE
\ \ \ : repeat  [compile] again [compile] then ; IMMEDIATE
\ \ \
\ \ \
\ \ \ VARIABLE leaves
\ \ \ : resolve-loop  leaves @ BEGIN ?dup WHILE dup @ swap here over - swap ! REPEAT here - compile, leaves ! ;
\ \ \ : do  leaves @ here dotick dodo compile, 0 leaves ! ; IMMEDIATE
\ \ \ : ?do  leaves @ dotick do?do compile, here here leaves ! 0 compile, ; IMMEDIATE
\ \ \ : loop  dotick doloop compile, resolve-loop ; IMMEDIATE
\ \ \ : +loop  dotick do+loop compile, resolve-loop ; IMMEDIATE
\ \ \ : leave  dotick doleave compile, leaves @ here leaves ! compile, ; IMMEDIATE
\ \ \ : ?leave  dotick do?leave compile, leaves @ here leaves ! compile, ; IMMEDIATE
\ \ \
\ \ \
\ \ \ : save-source  r> ib >r #ib @ >r source-id >r span @ >r >in @ >r >r ;
\ \ \ : restore-source  r> r> >in ! r> span ! r> doto source-id r> #ib ! r> doto ib >r ;
\ \ \
\ \ \
\ \ \ : compile-word  2dup $find IF immediate? IF nip nip execute EXIT THEN compile, 2drop EXIT THEN 2dup $number IF type -63 throw THEN dotick lit compile, compile, 2drop ;
\ \ \ : interpret-word  2dup $find IF drop nip nip execute EXIT THEN 2dup $number IF type -63 throw THEN >r 2drop r> ;
\ \ \ : interpret  0 >in ! BEGIN parse-word dup WHILE state @ IF compile-word ELSE interpret-word THEN REPEAT 2drop ;
\ \ \
\ \ \
\ \ \ : evaluate  save-source -1 doto source-id dup #ib ! span ! doto ib dotick interpret catch restore-source throw ;
\ \ \ : eval  evaluate ;
\ \ \
\ \ \
\ \ \ : doabort"  swap IF abort"-str ! -2 throw THEN drop ;
\ \ \ : abort"  c" dotick doabort" compile, ; IMMEDIATE
\ \ \
\ \ \
\ \ \ : quit
\ \ \   BEGIN 0 rdepth! [ terminal
\ \ \   BEGIN depth . [char] > emit space refill 0= IF EXIT THEN
\ \ \   space dotick interpret catch dup print-status cr UNTIL AGAIN ;
\ \ \
\ \ \
\ \ \ : literal  dotick lit compile, compile, ; IMMEDIATE
\ \ \
\ \ \ : compile  r> cell+ dup @ compile, >r ;



3141 CONSTANT pip
VARIABLE v0
VARIABLE v1
VARIABLE v2
VARIABLE v3
: urgh  v0 v1 v2 v3 ;
: init
  55aa 3d24 !
  3d23 @ fff9 and 4 or 3d23 !
  3d20 @ 7fff and 3d20 !
  4006 3d20 !
  2 3d25 !
  8 3d00 !
  3d0a @ 7 or 3d0a !
  3d09 7 and 17 or 3d09 !
  3d08 7 and 17 or 3d08 !
  3d07 7 and 17 or 3d07 !
  88c0 3d0e !
  88c0 3d0d !
  f77f 3d0b !
  0 3d04 !
  0 3d03 !
  ffff 3d01 !  ;

: serial-init
  c3 3d30 !    3d31 @ drop
  \ ff 3d34 !  a8 3d33 !  \ baud rate 19200
  ff 3d34 !  f1 3d33 !  \ baud rate 115200
  03 3d31 !

  3d0f @ 6000 or 3d0f !
  3d0e @ 6000 or 3d0e !
  3d0d @ 4000 or 3d0d ! ;

: emit  BEGIN 3d31 @ 40 and 0= UNTIL 3d35 ! ;

: ....  10 BEGIN dup WHILE >r dup 8000 and IF 31 ELSE 30 THEN emit 2* r> 1- REPEAT 2drop space ;
: ttt   50 emit here ....   0 0 <#  35 hold 36 hold #>   cr 52 emit here .... cr   21 emit pad .... pad @ .... cr    over .... dup .... cr   type ;
: cold
  init
  serial-init
  65 dup emit emit

  20 emit
  30 emit 3 4 < IF 79 ELSE 6e THEN emit
  31 emit 4 3 < IF 79 ELSE 6e THEN emit
  32 emit 3 3 < IF 79 ELSE 6e THEN emit
  33 emit -1 0 < IF 79 ELSE 6e THEN emit
  34 emit 0 -1 < IF 79 ELSE 6e THEN emit

  20 emit
  30 emit 3 4 > IF 79 ELSE 6e THEN emit
  31 emit 4 3 > IF 79 ELSE 6e THEN emit
  32 emit 3 3 > IF 79 ELSE 6e THEN emit
  33 emit -1 0 > IF 79 ELSE 6e THEN emit
  34 emit 0 -1 > IF 79 ELSE 6e THEN emit

  20 emit
  30 emit 3 4 u>= IF 79 ELSE 6e THEN emit
  31 emit 4 3 u>= IF 79 ELSE 6e THEN emit
  32 emit 3 3 u>= IF 79 ELSE 6e THEN emit
  33 emit -1 0 u>= IF 79 ELSE 6e THEN emit
  34 emit 0 -1 u>= IF 79 ELSE 6e THEN emit

  cr
  3 todigit emit  1 todigit emit  4 todigit emit
  9 todigit emit  a todigit emit  f todigit emit

  hex
  0400 dp !

  cr base @ BEGIN dup WHILE 2a emit 1- REPEAT drop

  1234 0 10
     cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....
  /' cr >r >r dup .... r> dup .... r> dup ....

cr 2a emit 2a emit  5678    0 10 um/mod swap .... ....
cr 2a emit 2a emit  5678 1234 10 um/mod swap .... ....

  cr 1234 ....
  cr 51 emit  9 2 tuck - todigit emit todigit emit
  cr ttt
cr base @ ....
cr 65 0 5 mu/mod .... .... ....
cr 2a emit 5678 1234 d2* .... ....
cr 2a emit abcd 1234 d2* .... ....
  cr 4 0 <# # #> type
  cr 1234 . 5678 .

  BEGIN AGAIN ;



t' cold 8001 t!



ONLY FORTH DEFINITIONS

: make-file ( addr len name len -- )
  w/o bin open-file ABORT" couldn't create file"
  dup >r write-file ABORT" couldn't write file"
      r> close-file ABORT" couldn't close file" ;

mem 20000 s" t.b" make-file
