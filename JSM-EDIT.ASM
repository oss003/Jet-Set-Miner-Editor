;Jet Set Miner level editor

codebase     equ $2a00

               org codebase-22

;****************************************
; Header for Atom emulator Wouter Ras
		 .db "EDITOR          "
		 .dw codebase
		 .dw codebase
		 .dw eind_asm-start_asm
;****************************************
start_asm    
               include macros.inc
               include editor.inc
               include scrtab.inc
               include font-ed.inc
               include mtiles.inc
;               include msprites.inc
;               include levels.inc
eind_asm
