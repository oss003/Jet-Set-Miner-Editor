;========================================================================
;DECLARE SPACE
;========================================================================

;--- Local variables ----------------------------------------------------

tmp         equ $60

;--- Global variables ---------------------------------------------------

WSP         equ $80
SPR_ACT     equ WSP+1
XPTR        equ WSP+2
YPTR        equ WSP+3
CNT_KEY     equ WSP+4
XBYTE       equ WSP+6
YBYTE       equ WSP+7
LEVEL       equ WSP+8
KEY_FLAG    equ WSP+9
DOOR_FLAG   equ WSP+10
;ENE_FLAG    equ WSP+11
LEV_HEAD    equ WSP+12
QUIT_FLAG   equ WSP+14

;--- Systemcalls --------------------------------------------------------

SCRBASE     equ $8000
OSTEXT      equ $f7d1
OSWRCH      equ $fff4
WAITSYNC    equ $fe66
GETKEY      equ $fe71
WAIT_HSEC   equ $fb81
LOADVEC     equ $ffe0
SAVEVEC     equ $ffdd
MODESEL     equ $b000
SHIFTKEY    equ $b001

;--- Data storage -------------------------------------------------------

jsm_data    equ $3a4e
data_adr    equ $5000

load_adr    equ data_adr+$c00 ; Pointers to game headers
hdr_ptr     equ data_adr+$c10 ; Level headers
level_dat   equ data_adr+$c88 ; Compressed data

;--- Init vars ----------------------------------------------------------

        lda #0
        sta XPTR          ;X-pos cursor
        sta YPTR          ;Y-pos cursor
        sta LEVEL         ;Level counter
        sta CNT_KEY       ;Key counter
        sta SPR_ACT       ;Active tile nr
        sta KEY_FLAG      ;Set for key move
        sta DOOR_FLAG     ;Set for door move
        sta QUIT_FLAG     ;Quit flag

;========================================================================
;MAIN PROGRAM
;========================================================================

start:
        jsr calc_head_ptr ;LEV_HEAD = current header address
        jsr clear         ;Clear screen
        jsr set_raster    ;Draw raster
        jsr drawlev       ;Draw level(LEVEL)
	jsr mkdispb       ;Display status bar

;--- Main loop ----------------------------------------------------------

main:
        ldx XPTR          ;Cursor on
        ldy YPTR
        jsr INV_CURSOR
        jsr wait_time

        ldx XPTR          ;Cursor off
        ldy YPTR
        jsr INV_CURSOR
        jsr wait_time

        lda KEY_FLAG      ;Moving key?
         beq chk_door
         jsr invert_key   ;Mark key
         jsr wait_time
         jsr invert_key   ;Unmark key
         jmp no_move
chk_door:
        lda DOOR_FLAG     ;Moving door?
         beq no_move
         jsr invert_door  ;Mark door
         jsr wait_time
         jsr invert_door  ;Unmark door
no_move:
        jsr keyscan       ;Scan and execute keypress

        lda XPTR          ;Print x-pos in bytes
        ldx #4
        ldy #22
        jsr hex2dec

        lda YPTR          ;Print x-pos in bytes
        ldx #8
        ldy #22
        jsr hex2dec

        lda XPTR          ;Print x-pos in pixels
        asl a
        asl a
        asl a
        ldx #4
        ldy #21
        jsr hex2dec

        lda YPTR          ;Print x-pos in pixels
        asl a
        asl a
        asl a
        ldx #8
        ldy #21
        jsr hex2dec

        lda QUIT_FLAG
        bne quit_prog
        jmp main
quit_prog:
        lda #12
        jmp OSWRCH
   
;--- Calculate address level header -------------------------------------

calc_head_ptr:
        ldx LEVEL
        lda hdr_ptr_lb,x
        sta LEV_HEAD
        lda hdr_ptr_hb,x
        sta LEV_HEAD+1
        rts

hdr_ptr_lb:
        .db <(hdr_ptr),<(hdr_ptr+30),<(hdr_ptr+60),<(hdr_ptr+90)
hdr_ptr_hb:
        .db >(hdr_ptr),>(hdr_ptr+30),>(hdr_ptr+60),>(hdr_ptr+90)

;========================================================================
;KEYSCAN
;========================================================================

UP       equ $1b ; ;
DOWN     equ $1e ; .
LEFT     equ $3a ; Z
RIGHT    equ $38 ; X
ACTION   equ $00 ; Space

TILE0    equ $11 ; 1
TILE1    equ $12 ; 2
TILE2    equ $13 ; 3
TILE3    equ $14 ; 4

KEY      equ $2b ; K
DOOR     equ $24 ; D
NEXTLEV  equ $2e ; N

ENEM     equ $25 ; E
LOAD     equ $2c ; L
SAVE     equ $33 ; S
QUIT     equ $31 ; Q

L_KEY    equ $2c ; L
R_KEY    equ $32 ; R
X_KEY    equ $38 ; X
Y_KEY    equ $39 ; Y

CLRLEV   equ $23 ; C
ESC      equ $3b ; Esc

keyscan:
        jsr GETKEY

;--- Check cursor movement ---------------------------------------------

chk_key1:
        cpy #UP           ;Cursor up
          bne chk_key2
          lda YPTR
          beq chk_key2
          dec YPTR
          rts
chk_key2:
        cpy #DOWN         ;Cursor down
          bne chk_key3
          lda YPTR
          cmp #19
          beq chk_key3
          inc YPTR
          rts
chk_key3:
        cpy #LEFT         ;Cursor left
          bne chk_key4
          lda XPTR
          beq chk_key4
          dec XPTR
          rts
chk_key4:
        cpy #RIGHT        ;Cursor right
          bne chk_key5
          lda XPTR
          cmp #31
          beq chk_key5
          inc XPTR
          rts

;--- Check tile selection ----------------------------------------------

chk_key5:
        cpy #TILE0        ;Select tile0
          bne chk_key6
set_tile0:
          lda #0
          jmp clr_tileptr
chk_key6:
          cpy #TILE1      ;Select tile1
          bne chk_key7
          lda #1
          jmp clr_tileptr
chk_key7:
        cpy #TILE2        ;Select tile2
          bne chk_key8
          lda #2
          jmp clr_tileptr
chk_key8:
        cpy #TILE3        ;Select tile3
          bne chk_key9
          lda #3
          jmp clr_tileptr

;--- Check moving objects ---------------------------------------------

chk_key9:
        cpy #KEY          ;Move key
          bne chk_key10
          jmp move_key
chk_key10:
        cpy #DOOR         ;Move door
          bne chk_key11
          jmp move_door
chk_key11:
        cpy #ESC          ;Escape
          bne chk_key13
          lda #0
          sta KEY_FLAG    ;Cancel key move
          sta DOOR_FLAG   ;Cancel door move
          jmp set_tile0

;--- Check show menu keys ---------------------------------------------

chk_key13:
        cpy #LOAD         ;Load file
          bne chk_key14
          jmp load_file
chk_key14:
        cpy #SAVE         ;Save file
          bne chk_key15
          jmp save_file
chk_key15:
        cpy #NEXTLEV      ;Next level
          bne chk_key16
          jmp next_level
chk_key16:
        cpy #CLRLEV       ;Clear level
          bne chk_key17
          jmp clr_lev
chk_key17:
        cpy #ENEM         ;Enemies properties
         bne chk_key19
         jmp ene_prop
chk_key19:
        cpy #QUIT         ;Quit
          bne chk_key20
          inc QUIT_FLAG
          rts

;--- Check action -----------------------------------------------------

chk_key20:
        cpy #ACTION       ;Space
          bne chk_key99
          lda SPR_ACT
          cmp #4          ;Moving key?
            bne chk_act1
            jmp set_key
chk_act1:
          cmp #5          ;Moving door?
            bne chk_act3
            jmp set_door
chk_act3:        
          ldx XPTR        ;Get current tilenr
          ldy YPTR
          jsr getblock
          ldx XPTR        ;Erase tile
          ldy YPTR
          jsr drawtile
          lda SPR_ACT     ;Update leveldata
          ldx XPTR
          ldy YPTR
          jsr setblock
          lda SPR_ACT     ;Set new tile
          ldx XPTR
          ldy YPTR
          jmp drawtile

;--- No action for pressed key ----------------------------------------

chk_key99:
        rts

;========================================================================
;K-KEY, MOVE KEY
;========================================================================

;--- Move selected key ------------------------------------------------

move_key:
        inc CNT_KEY
        lda CNT_KEY
        and #3
        sta CNT_KEY
        clc
        adc #$30
        ldx #20
        ldy #22
        jsr PUTCHAR
        lda #4
        jsr clr_tileptr
        inc KEY_FLAG
wait_release:
        jsr GETKEY
        cpy #$ff
        bne wait_release
        jmp chk_key99
        rts

;--- Invert selected key ----------------------------------------------

invert_key:
        lda CNT_KEY
        asl a
        tay
        lda (LEV_HEAD),y
        tax
        iny
        lda (LEV_HEAD),y
        tay
        jmp INV_CURSOR

;--- Set selected key -------------------------------------------------

set_key:
        lda CNT_KEY       ;Erase old key
        asl a
        tay
        lda (LEV_HEAD),y
        tax
        iny
        lda (LEV_HEAD),y
        tay
        lda #4
        jsr drawtile

        lda #4            ;Draw new key
        ldx XPTR
        ldy YPTR
        jsr drawtile

        lda CNT_KEY       ;Set new key coords
        asl a
        tay
        lda XPTR
        sta (LEV_HEAD),y
        lda YPTR
        iny
        sta (LEV_HEAD),y

        lda #0            ;End key move
        sta KEY_FLAG
        jmp set_tile0

;========================================================================
;D-KEY, MOVE DOOR
;========================================================================

;--- Move door --------------------------------------------------------

tmp_x     equ tmp
tmp_y     equ tmp+1

move_door:
        lda #5
        jsr clr_tileptr
        inc DOOR_FLAG
        jmp wait_release

;--- Invert door ------------------------------------------------------

invert_door:
        ldy #8
        lda (LEV_HEAD),y
        sta tmp_x
        iny
        lda (LEV_HEAD),y
        sta tmp_y
       
        ldx tmp_x
        ldy tmp_y
        jsr INV_CURSOR
        ldx tmp_x
        inx
        ldy tmp_y
        jsr INV_CURSOR
        ldx tmp_x
        ldy tmp_y
        iny
        jsr INV_CURSOR
        ldx tmp_x
        inx
        ldy tmp_y
        iny
        jmp INV_CURSOR

;--- Set door ---------------------------------------------------------

set_door:
        ldy #8
        lda (LEV_HEAD),y
        sta tmp_x
        iny
        lda (LEV_HEAD),y
        sta tmp_y
       
        lda #5            ;Erase door
        ldx tmp_x
        ldy tmp_y
        jsr drawtile
        lda #7
        ldx tmp_x
        inx
        ldy tmp_y
        jsr drawtile
        lda #6
        ldx tmp_x
        ldy tmp_y
        iny
        jsr drawtile
        lda #8
        ldx tmp_x
        inx
        ldy tmp_y
        iny
        jsr drawtile

        lda #5            ;Set new door
        ldx XPTR
        ldy YPTR
        jsr drawtile
        lda #6
        ldx XPTR
        inx
        ldy YPTR
        jsr drawtile
        lda #7
        ldx XPTR
        ldy YPTR
        iny
        jsr drawtile
        lda #8
        ldx XPTR
        inx
        ldy YPTR
        iny
        jsr drawtile

        ldy #8
        lda XPTR
        sta (LEV_HEAD),y
        iny
        lda YPTR
        sta (LEV_HEAD),y

        lda #0
        sta DOOR_FLAG
        jmp set_tile0

;--- Point to selected tile -------------------------------------------

clr_tileptr:
        sta SPR_ACT
tile_upd:
        jsr PUTSTRING
        .db 12,23,'       _     ',0
        nop
set_tileptr:
        lda SPR_ACT
        asl a
        clc
        adc #12
        tax
        ldy #23
        lda #'^'
        jsr PUTCHAR
        rts

;========================================================================
;N-KEY, SELECT NEXT LEVEL
;========================================================================

next_level:
        inc LEVEL
        lda LEVEL
        and #3
        sta LEVEL
upd_scr:
        jsr calc_head_ptr ;LEV_HEAD = current header address
        jsr set_raster
        jsr drawlev
        jmp mkdispb       ;Display status bar

;========================================================================
;L-KEY, LOAD JET SET MINER DATA FILE
;========================================================================

loadadr    equ tmp

load_file:
        jsr show_menu
        jsr PUTSTRING
        .db 14,8,'LOAD',0
        nop
lf0:
        jsr GETKEY
        cpy #UP           ;Up
          bne lf2
          inc filenr
          lda filenr
          cmp #$3a
          bne lf1
          dec filenr
lf1:
          jsr show_file
          jmp lf0
lf2:
        cpy #DOWN         ;Down
          bne lf4
          dec filenr
          lda filenr
          cmp #$2f
          bne lf3
          inc filenr
lf3:
          jmp lf1
lf4:
        cpy #ESC          ;Escape
          bne lf5
lf6:
          jmp upd_scr

lf5:
        cpy #ACTION       ;Space
          bne lf0

;--- Load file --------------------------------------------------------

         lda #12
         jsr OSWRCH
         jsr OSTEXT
         .db 'LOADING '
         nop

         lda filenr       ;Set filenr
         sta tmpfile+3

         ldx #$ff
prt_name:
         inx
         lda tmpfile,x
         jsr OSWRCH
         cmp #$0d
         bne prt_name
         lda #$0a
         jsr OSWRCH

;--- Load datafile DATx with tiles, sprites and leveldata -------------

loadfile:
         lda #<tmpfile
         sta loadadr
         lda #>tmpfile
         sta loadadr+1          

         lda #<load_adr
         sta loadadr+2
         lda #>load_adr
         sta loadadr+3          

         lda #$b2
         sta loadadr+4
         lda #$c2
         sta loadadr+5

         ldx #loadadr
         jsr LOADVEC

         jsr WAIT_HSEC    ; Wait 0,5 sec

         jsr decomp

         lda #$F0         ; Clear4
         sta MODESEL

         jmp lf6
tmpfile:
        .db 'DATx',$0D

;========================================================================
;S-KEY, SAVE JET SET MINER DATA FILE
;========================================================================

save_file:
        jsr show_menu
        jsr PUTSTRING
        .db 14,8,'SAVE',0
        nop
sf0:
        jsr GETKEY
        cpy #UP           ;Up
          bne sf2
          inc filenr
          lda filenr
          cmp #$3a
          bne sf1
          dec filenr
sf1:
          jsr show_file
          jmp sf0
sf2:
        cpy #DOWN         ;Down
          bne sf4
          dec filenr
          lda filenr
          cmp #$2f
          bne sf3
          inc filenr
sf3:
          jmp sf1
sf4:
        cpy #ESC          ;Escape
          bne sf5
sf6:
          jmp upd_scr
sf5:
        cpy #ACTION       ;Space
          bne sf0

          jsr compress

;--- Save file --------------------------------------------------------

         lda #12
         jsr OSWRCH
         jsr OSTEXT
         .db 'SAVING '
         nop

         lda filenr       ;Set filenr
         sta tmpfile+3

         ldx #$ff
prt_name1:
         inx
         lda tmpfile,x
         jsr OSWRCH
         cmp #$0d
         bne prt_name1
         lda #$0a
         jsr OSWRCH

;--- Save datafile DATx with tiles, sprites and leveldata -------------

savefile:
         lda #<tmpfile   ; String
         sta loadadr
         lda #>tmpfile
         sta loadadr+1          

         lda #<load_adr  ; Startaddress
         sta loadadr+2
         lda #>load_adr
         sta loadadr+3          

         lda #<jsm_data  ; Exec address
         sta loadadr+4
         lda #>jsm_data
         sta loadadr+5

         lda #<load_adr  ; Start address
         sta loadadr+6
         lda #>load_adr
         sta loadadr+7          

         lda wr_ptr      ; Endaddresss
         sta loadadr+8
         lda wr_ptr+1
         sta loadadr+9

         ldx #loadadr
         jsr SAVEVEC

         jsr WAIT_HSEC    ; Wait 0,5 sec

         lda #$F0         ; Clear4
         sta MODESEL

         jmp sf6       jmp sf6

;--- Draw file menu ---------------------------------------------------

show_menu:
        jsr PUTSTRING
        .db 12, 7,5,3,3,3,3,3,3,6,0
        .db 12, 8,8,'      ',8,0
        .db 12, 9,1,3,3,3,3,3,3,2,0
        nop
show_file:
        jsr PUTSTRING
        .db 12,10,8,' DAT'
filenr:
        .db     '0',13,8,0
        nop
        jsr PUTSTRING
        .db 12,11,7,3,3,3,3,3,3,4,0
        nop

;        lda filenr
;        ldx #10
;        ldy #23
;        jsr PUTCHAR

wait_rel2:
        jsr GETKEY
        cpy #$ff
        bne wait_rel2
        rts    

;--- Compress levels -------------------------------------------------

max_col    equ 32
max_row    equ 20

rd_ptr     equ tmp+3
counter    equ tmp+5
curblock   equ tmp+6
xptr       equ tmp+7
yptr       equ tmp+8
tmpsave    equ tmp+9
wr_ptr     equ tmp+10

compress:
    lda #<level_dat   ; wr_ptr = address to write compressed data to
    sta wr_ptr
    lda #>level_dat
    sta wr_ptr+1

    lda #0
    sta LEVEL         ; Reset level counter
cp1:
    lda #<data_adr    ; rd_ptr = address to read uncompressed data from
    sta rd_ptr
    lda #>data_adr
    clc
    adc LEVEL
    adc LEVEL
    adc LEVEL
    sta rd_ptr+1

    lda LEVEL         ; Set startadres level
    asl a
    asl a
    tax
    sec
    lda wr_ptr
    sbc #<(load_adr-jsm_data) ;Loadaddress JSM is $3806
    sta load_adr,x
    lda wr_ptr+1
    sbc #>(load_adr-jsm_data)
    sta load_adr+1,x

    ldy LEVEL         ; Header address
    sec
    lda hdr_ptr_lb,y
    sbc #<(load_adr-jsm_data)
    sta load_adr+2,x
    lda hdr_ptr_hb,y
    sbc #>(load_adr-jsm_data)
    sta load_adr+3,x

    lda #0
    sta counter       ; counter = tilecounter
    sta xptr          ; xptr = columncounter
    sta yptr          ; yptr = rowcounter

    tax               ; Get first blocknr
    tay
    jsr getblock
    sta curblock

cp_loop:
    ldx xptr          ; Check end of column
    cpx #max_col
    bne no_endcol

    ldx #0            ; End of column reached
    stx xptr          ; Reset column counter
    inc yptr          ; Increment rowcounter
no_endcol:
    ldy yptr          ; Check end of rows
    cpy #max_row
    beq endrow

    jsr getblock      ; Get blocknr. at X,Y
    cmp curblock      ; Compare with last read block
    beq inc_cnt
    jsr exit1         ; Different blocks, so write byte
    jmp inc_ptr
inc_cnt:
    inc counter       ; Same block, increment blockcounter
    lda counter
    cmp #63           ; Check blockcounter=63 (max=1111 11..)
    bne inc_ptr
    jsr exit1         ; If reached, write byte
inc_ptr:
    inc rd_ptr        ; Increment read pointer
    bne no_incrd
    inc rd_ptr+1
no_incrd:     
    inc xptr          ; Next column

    jmp cp_loop       ; Loop
endrow:
    lda counter       ; Last byte read, check to write last byte
    beq ending
    lda curblock
    jsr exit1         ; Write last byte
    lda #$ff
    sta (wr_ptr),y
    jsr ptr_inc_wr
ending:
    inc LEVEL         ; Next level
    lda LEVEL
    cmp #4            ; All levels done?
    beq cp_end
    jmp cp1
cp_end:
    and #3
    sta LEVEL
    rts               ; End of routine

exit1:
    and #3            ; Blocknr & 3
    sta tmpsave       ; Save 
    lda counter       ; Shift counter 2 bits to the right
    asl a
    asl a
    clc               ; Add blocknr
    adc curblock
    ldy #0
    sta (wr_ptr),y    ; Write compressed byte
ptr_inc_wr:
    inc wr_ptr        ; Increment write pointer
    bne no_incwr
    inc wr_ptr+1
no_incwr:

    sty counter       ; Reset blockcounter
    inc counter       ; 1 block present
    lda tmpsave
    sta curblock
    
    rts               ; Back to program

;========================================================================
;SUBROUTINES
;========================================================================

;--- Level decompression ----------------------------------------------

u_lev     equ tmp         ;Uncompressed data for level
c_lev     equ tmp+2       ;Compressed data for level
tmp_dat   equ tmp+4

decomp:
        lda #0
        sta LEVEL

        lda #<level_dat   ;Compressed data address
        sta c_lev   
        lda #>level_dat
        sta c_lev+1
dloop:
        lda #<data_adr    ;Uncompressed data store address
        sta u_lev
        lda #>data_adr
        clc
        adc LEVEL
        adc LEVEL
        adc LEVEL
        sta u_lev+1

        ldy #0
deloop:
        lda (c_lev),y     ;Get next byte
        cmp #$ff          ;If $FF then end of level data
        beq deend
	sta tmp_dat       ;Save
	lsr a
	lsr a
	tax               ;Get run length number (top 6 bits)
	lda tmp_dat
	and #3            ;Get tile number
	
        inc c_lev         ;Increase source address
        bne nodinc2
        inc c_lev+1
nodinc2:
        nop
dewloop:
        sta (u_lev),y     ;Store tile
        inc u_lev         ;Increase dest address
        bne nodeinc
        inc u_lev+1
nodeinc:
        dex               ;Decrease run length count
        bne dewloop       ;and loop
        jmp deloop        ;Byte over - jump to start of loop

deend:
        inc c_lev         ;Increase source address
        bne nodinc3
        inc c_lev+1
nodinc3:
        inc LEVEL
        lda LEVEL
        cmp #4
        bne dloop
        and #3
        sta LEVEL
        rts

;---------------------------------------------------------------
;Draw sprite a,x,y
;---------------------------------------------------------------

spriteadr   equ tmp+5
screenadr   equ tmp+7
tmpnr       equ tmp+9
tmpx        equ tmp+10
tmpy        equ tmp+11

drawsprite:
        sta tmpnr
        txa
        lsr a
        lsr a
        lsr a
        sta tmpx
        tya
        lsr a
        lsr a
        lsr a
        sta tmpy

        lda #12
        ldx tmpx
        ldy tmpy
        jsr PUTCHAR

        lda #12
        ldx tmpx
        inx
        ldy tmpy
        jsr PUTCHAR
 
        ldx tmpx
        ldy tmpy
        iny
        lda tmpnr
        beq spr_left
        lda #10
        jmp put_spr
spr_left: 
        sec               ;$0e,$13,$18,$1d (14,19,24,29)
        lda dat_tmp
        sbc #14
        and #3
        clc
        adc #14
put_spr:
        jsr PUTCHAR

        ldx tmpx
        inx
        ldy tmpy
        iny
        lda tmpnr
        beq spr_left2
        sec
        lda dat_tmp
        sbc #14
        and #3
        clc
        adc #14
        jmp put_spr2
spr_left2: 
        lda #9
put_spr2:
        jsr PUTCHAR

        rts

;---------------------------------------------------------------
;Make status display (why did I call it 'display bottom'?)
;---------------------------------------------------------------

mkdispb:
        jsr PUTSTRING
        .db 0,20,'     X   Y _ TILES _ GAME  _ENEM',0
        .db 0,21,'PIX 000,000_1 2 3 4_K D LEV_LOAD',0 
        .db 0,22,'BYT 000,000_       _0    0 _SAVE',0
        .db 0,23,'FILE : DAT0_^      _       _QUIT',0
        nop

        lda #0           ;Set tile0
        ldx #12
        ldy #22
        jsr drawtile

        lda #1           ;Set tile1
        ldx #14 
        ldy #22
        jsr drawtile

        lda #2           ;Set tile2
        ldx #16
        ldy #22
        jsr drawtile

        lda #3           ;Set tile3
        ldx #18
        ldy #22
        jsr drawtile

        lda CNT_KEY      ;Set key nr
        clc
        adc #$30
        ldx #20
        ldy #22
        jsr PUTCHAR

        lda LEVEL        ;Set level nr
        clc
        adc #$30
        ldx #25
        ldy #22
        jsr PUTCHAR

        lda filenr
        ldx #10
        ldy #23
        jsr PUTCHAR

        jsr tile_upd
	rts

;---------------------------------------------------------------
;Show status display properties enemies
;---------------------------------------------------------------

ene_counter    equ tmp
ene_line       equ tmp+1
ene_cursor     equ tmp+2

ene_prop:
        lda #0
        sta ene_cursor

prop_start:
        jsr prop_val
prop_redraw:
        jsr wait_rel2
        jsr show_cursor
prop_scan:
        jsr GETKEY
prop_key1:
        cpy #UP           ;Cursor up
          bne prop_key2
          lda ene_cursor
          beq prop_scan
          dec ene_cursor
          jmp prop_redraw
prop_key2:
        cpy #DOWN         ;Cursor down
          bne prop_key3
          lda ene_cursor
          cmp #3
          beq prop_scan
          inc ene_cursor
          jmp prop_redraw
prop_key3:
        cpy #DOOR         ;Direction
          bne prop_key4
          lda ene_cursor
          asl a
          asl a
          clc
          adc ene_cursor
          adc #14
          tay
          lda (LEV_HEAD),y
          bne prop2
          clc
          adc #1
          sta (LEV_HEAD),y
          jmp p_end 
prop2:
          cmp #1
          bne prop3
          lda #255
          sta (LEV_HEAD),y
          jmp p_end
prop3:
          lda #0
          sta (LEV_HEAD),y
p_end:
          pha
          ldx #24
          lda ene_cursor
          ora #8
          tay
          pla
          jsr hex2dec
          jmp prop_redraw
prop_key4:
        cpy #L_KEY         ;Left border
          bne prop_key5
          lda ene_cursor
          asl a
          asl a
          clc
          adc ene_cursor
          adc #12
          tay
          lda (LEV_HEAD),y
          bit SHIFTKEY
          bmi prop4
          sec
          sbc #2
          jmp prop5
prop4:
          clc
          adc #2
prop5:
          and #$fe
          sta (LEV_HEAD),y
          pha
          ldx #16
          lda ene_cursor
          ora #8
          tay
          pla
          jsr hex2dec
          jmp prop_redraw
prop_key5:
        cpy #R_KEY         ;Right border
          bne prop_key6
          lda ene_cursor
          asl a
          asl a
          clc
          adc ene_cursor
          adc #13
          tay
          lda (LEV_HEAD),y
          bit SHIFTKEY
          bmi prop6
          sec
          sbc #2
          jmp prop7
prop6:
          clc
          adc #2
prop7:
          and #$fe
          sta (LEV_HEAD),y
          pha
          ldx #20
          lda ene_cursor
          ora #8
          tay
          pla
          jsr hex2dec
          jmp prop_redraw
prop_key6:
        cpy #X_KEY         ;X-coord
          bne prop_key7
          lda ene_cursor
          asl a
          asl a
          clc
          adc ene_cursor
          adc #10
          tay
          lda (LEV_HEAD),y
          bit SHIFTKEY
          bmi prop8
          sec
          sbc #2
          jmp prop9
prop8:
          clc
          adc #2
prop9:
          sta (LEV_HEAD),y
          pha
          ldx #8
          lda ene_cursor
          ora #8
          tay
          pla
          jsr hex2dec
          jmp prop_redraw
prop_key7:
        cpy #Y_KEY         ;Y-coord
          bne prop_key8
          lda ene_cursor
          asl a
          asl a
          clc
          adc ene_cursor
          adc #11
          tay
          lda (LEV_HEAD),y
          bit SHIFTKEY
          bmi prop10
          sec
          sbc #2
          jmp prop11
prop10:
          clc
          adc #2
prop11:
          sta (LEV_HEAD),y
          pha
          ldx #12
          lda ene_cursor
          ora #8
          tay
          pla
          jsr hex2dec
          jmp prop_redraw
prop_key8:
        cpy #ESC
         beq prop_end
         jmp prop_scan
prop_end:
        jsr set_raster
        jsr drawlev
        rts

prop_val:
        jsr PUTSTRING
        .db 3, 4,5,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,6,0
        .db 3, 5,'_   ENEMIE PROPERTIES    _',0
        .db 3, 6,1,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,0
        .db 3, 7,'_ NR  X   Y  <-L R-> DIR _',0
        .db 3, 8,'_  0 000 000 000 000 000 _',0
        .db 3, 9,'_  1 000 000 000 000 000 _',0
        .db 3,10,'_  2 000 000 000 000 000 _',0
        .db 3,11,'_  3 000 000 000 000 000 _',0
        .db 3,12,7,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,0
        nop

        lda #0
        sta ene_counter
        lda #8
        sta ene_line
        ldy #10
        sty dat_tmp
prop_loop:
        ldy dat_tmp
        lda (LEV_HEAD),y
        ldx #8
        ldy tmp+1
        jsr hex2dec
        inc dat_tmp

        ldy dat_tmp
        lda (LEV_HEAD),y
        ldx #12
        ldy tmp+1
        jsr hex2dec
        inc dat_tmp

        ldy dat_tmp
        lda (LEV_HEAD),y
        ldx #16
        ldy tmp+1
        jsr hex2dec
        inc dat_tmp

        ldy dat_tmp
        lda (LEV_HEAD),y
        ldx #20
        ldy tmp+1
        jsr hex2dec
        inc dat_tmp

        ldy dat_tmp
        lda (LEV_HEAD),y
        ldx #24
        ldy tmp+1
        jsr hex2dec
        inc dat_tmp

        inc tmp+1
        inc tmp
        lda tmp
        cmp #4
        bne prop_loop
        rts

show_cursor:
        ldx #4
        ldy #8
        lda #' '
        jsr PUTCHAR
        ldx #4
        ldy #9
        lda #' '
        jsr PUTCHAR
        ldx #4
        ldy #10
        lda #' '
        jsr PUTCHAR
        ldx #4
        ldy #11
        lda #' '
        jsr PUTCHAR

        ldx #4
        clc
        lda ene_cursor
        adc #8
        tay
        lda #'>'
        jmp PUTCHAR


;---------------------------------------------------------------
;Draw level background + keys
;---------------------------------------------------------------

dat_adr    equ tmp     ; TMPVAR+2/3
row_cnt    equ tmp+2   ;Temp row counter      +8
col_cnt    equ tmp+3   ;Temp column counter   +9
dat_tmp    equ tmp+4

drawlev:
        lda #<data_adr        ;Get address to store uncompressed data
        sta dat_adr
        lda #>data_adr
        clc
        adc LEVEL
        adc LEVEL
        adc LEVEL
        sta dat_adr+1

;Draw level without keys
        lda #0          ;Row counter
        sta row_cnt

dr_loop1:
        lda #0
        sta col_cnt     ;Column counter
dr_loop2:
        ldy #0
        lda (dat_adr),y
        ldx col_cnt
        ldy row_cnt
        jsr drawtile

        inc dat_adr
        bne dr_noinc
        inc dat_adr+1
dr_noinc:
        inc col_cnt
        lda col_cnt
        cmp #32
        bne dr_loop2

        inc row_cnt
        lda row_cnt
        cmp #20
        bne dr_loop1

;--- Draw key sprites onto screen -----------------------------

        ldy #0
        sty dat_tmp
keyloop:
        ldy dat_tmp
        lda (LEV_HEAD),y  ;x-coord key
        tax
        iny
        lda (LEV_HEAD),y  ;Y-coord key
        tay
        lda #4
        jsr drawtile

        inc dat_tmp
        inc dat_tmp
        lda dat_tmp
        cmp #8
        bne keyloop
        
;--- Draw door onto screen -----------------------------------

drawdoor:
        ldy #8
        lda (LEV_HEAD),y
        sta dat_tmp
        iny
        lda (LEV_HEAD),y
        sta dat_tmp+1

        ldx dat_tmp
        ldy dat_tmp+1
        lda #5
        jsr drawtile

        inc dat_tmp
        ldx dat_tmp
        ldy dat_tmp+1
        lda #6
        jsr drawtile

        inc dat_tmp+1
        ldx dat_tmp
        ldy dat_tmp+1
        lda #8
        jsr drawtile

        dec dat_tmp
        ldx dat_tmp
        ldy dat_tmp+1
        lda #7
        jsr drawtile

;--- Draw enemies onto screen -----------------------------------

drawene:
        ldy #14
        sty dat_tmp
eneloop:
        ldy dat_tmp
        lda (LEV_HEAD),y
        cmp #$ff
        beq next_ene
        pha
        dey
        dey
        dey
        dey
        lda (LEV_HEAD),y  ;x-coord ene
        tax
        iny
        lda (LEV_HEAD),y  ;Y-coord ene
        tay
        pla
        jsr drawsprite
next_ene:
        inc dat_tmp
        inc dat_tmp
        inc dat_tmp
        inc dat_tmp
        inc dat_tmp
        lda dat_tmp
        cmp #34
        bne eneloop

        rts 

;--- Get tile nr from uncompressed leveldata ------------------

LEVADR    equ tmp
TMPBLOCK  equ tmp+2

getblock:
        jsr calc_adr
        ldy #0          ;Read blocknr
        lda (LEVADR),y
        rts

;--- Set tile nr into uncompressed data -----------------------

setblock:
        sta TMPBLOCK
        jsr calc_adr
        lda TMPBLOCK
        ldy #0          ;Read blocknr
        sta (LEVADR),y
        rts

calc_adr:
        sty LEVADR
        lda #0
        sta LEVADR+1

        asl LEVADR      ;Calculate Y*32
        asl LEVADR
        asl LEVADR
        asl LEVADR
        rol LEVADR+1
        asl LEVADR
        rol LEVADR+1

        txa             ;Add X + baseadres leveldata
        clc
        adc LEVADR
        adc #<data_adr
        sta LEVADR
        lda LEVADR+1
        adc #>data_adr
        clc
        adc LEVEL
        adc LEVEL
        adc LEVEL
        sta LEVADR+1

        rts

;--- Draw tile a,x,y -------------------------------------------

scradr     equ tmp+6
tileadr    equ tmp+8

drawtile:
        asl a
        asl a
        asl a
        clc
        adc #<tiles
        sta tileadr
        lda #>tiles
        adc #0
        sta tileadr+1

        stx scradr
        tya
        clc
        adc #>SCRBASE
        sta scradr+1

        ldx #0
t_loop:
        ldy #0
        lda (tileadr),y
        ldy scrptr,x
        eor (scradr),y
        sta (scradr),y
        inc tileadr
        bne no_tileinc
        inc tileadr+1
no_tileinc:
        inx
        cpx #8
        bne t_loop
        rts

;---------------------------------------------------------------
;Convert hex nr to decimal string
;---------------------------------------------------------------

hex2dec:
        stx digits
        sty digits+1
        ldx #'0'
        stx digits+2
        stx digits+3

        sec
chk100:
        sbc #100
        bcc goon10
        inc digits+2
        jmp chk100
goon10:
        clc
        adc #100
                
        sec
chk10:
        sbc #10
        bcc go_on1
        inc digits+3
        jmp chk10
go_on1:
        clc
        adc #10+'0'
        sta digits+4

        jsr PUTSTRING
digits  .db 0,0,0,0,0,0
        nop

        rts

;===============================================================
;ROUTINES
;===============================================================

;---------------------------------------------------------------
;CLEAR
; Clear screen 4
;---------------------------------------------------------------
; input : -
; output: -
;
; reg.  : a,x,y
;---------------------------------------------------------------

scr_adr  equ tmp

clear:  
        lda #$F0        ; Clear4
        sta MODESEL

        lda #$80
        sta scr_adr+1
        lda #0
        sta scr_adr
        tay
clrloop:
        sta (scr_adr),y  ; Clear byte
        iny
        bne clrloop
        inc scr_adr+1
        ldx scr_adr+1
        cpx #$98
        bne clrloop
        rts

;---------------------------------------------------------------
;SET_RASTER
; Set raster 32 x 20 chars on screen
;---------------------------------------------------------------
; input : -
; output: -
;
; reg.  : a,x,y
;---------------------------------------------------------------

row_ptr    equ tmp
col_ptr    equ tmp+1

set_raster: 
        lda #19
        sta row_ptr     ; y-ptr
yloop:
        lda #31
        sta col_ptr     ; x-ptr
xloop:
        lda #0          ; char(0)
        ldx col_ptr
        ldy row_ptr
        jsr PUTCHAR
        dec col_ptr
        bpl xloop
        dec row_ptr
        bpl yloop

        rts

;---------------------------------------------------------------
;CLR_LEV
; Clear uncompressed level
;---------------------------------------------------------------
; input : -
; output: -
;
; reg.  : a,y
;---------------------------------------------------------------

adr     equ tmp
cnt     equ tmp+2

clr_lev:
        lda #<data_adr
        sta adr
        lda #>data_adr
        clc
        adc LEVEL
        adc LEVEL
        adc LEVEL
        sta adr+1

        lda #0
        sta cnt
        sta cnt+1

        ldy #0
clr_loop:
        lda #0
        sta (adr),y

        inc adr
        bne clu1
        inc adr+1
clu1:
        inc cnt
        bne clu2
        inc cnt+1
clu2:
        lda cnt+1
        cmp #>640
        bne clr_loop
        lda cnt
        cmp #<640
        bne clr_loop

        jmp upd_scr
        
;---------------------------------------------------------------
;WAIT_TIME
; Wait a while
;---------------------------------------------------------------
; input : -
; output: -
;
; reg.  : -
;---------------------------------------------------------------

wait_time:
        jsr WAITSYNC 
        jsr WAITSYNC
        jmp WAITSYNC
