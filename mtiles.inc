;MONOCHROME VERSION

tiles:
        .BYTE %00000000      ;Tiles - all 8x8 1 bit per pixel, so easy to change!
        .BYTE %00000000      ;This one is air
        .BYTE %00000000
        .BYTE %00000000
        .BYTE %00000000
        .BYTE %00000000
        .BYTE %00000000
        .BYTE %00000000

        .BYTE %11111111      ;Floor that can be walked and jumped through
        .BYTE %10101011
        .BYTE %11010101
        .BYTE %10101011
        .BYTE %11010101
        .BYTE %10101011
        .BYTE %11010101
        .BYTE %11111111

        .BYTE %11101110      ;Brick wall
        .BYTE %11101110
        .BYTE %11101110
        .BYTE %00000000
        .BYTE %01110111
        .BYTE %01110111
        .BYTE %01110111
        .BYTE %00000000

        .BYTE %00010000      ;Spike
        .BYTE %00010000
        .BYTE %00111000
        .BYTE %00111000
        .BYTE %01111100
        .BYTE %01111100
        .BYTE %11111110
        .BYTE %11111110

keyspr: .BYTE %00001100  ;Key sprite - 8x8, 1 bit per pixel
        .BYTE %00010010
        .BYTE %00100001
        .BYTE %00100001
        .BYTE %00010010
        .BYTE %10101100
        .BYTE %01000000
        .BYTE %10000000

doorspr:.BYTE %11011011   ;Door sprite - 16x16, 1 bit per pixel
        .BYTE %10110110   ;This is stored in screen format (ie 4 8x8 chunks)
        .BYTE %01101101
        .BYTE %11011011
        .BYTE %10110110
        .BYTE %01101101
        .BYTE %11011011
        .BYTE %10110110
        
        .BYTE %01101101
        .BYTE %11011011
        .BYTE %10110110
        .BYTE %01101101
        .BYTE %11011011
        .BYTE %10110110
        .BYTE %01101101
        .BYTE %11011011

        .BYTE %01101101
        .BYTE %11011011
        .BYTE %10110110
        .BYTE %01101101
        .BYTE %11011011
        .BYTE %10110110
        .BYTE %01101101
        .BYTE %11011011
        
        .BYTE %10110110
        .BYTE %01101101
        .BYTE %11011011
        .BYTE %10110110
        .BYTE %01101101
        .BYTE %11011011
        .BYTE %10110110
        .BYTE %01101101

