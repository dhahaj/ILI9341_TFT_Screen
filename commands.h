#pragma once

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// Touchscreen connection:
#define Y1 A3  // need two analog inputs
#define X1 A2  // 
#define Y2 9   // 
#define X2 8   //

int16_t PTR_COL = 0; // LCD cursor pointer
int16_t PTR_ROW = 0;
int16_t TS_COL = 0; // TOUCHSCREEN(TS) detected value
int16_t TS_ROW = 0;

// TS calibration
uint16_t ROW_F = 110; // TS first row
uint16_t ROW_L = 920; // TS last row
uint16_t COL_F = 110; // TS first column
uint16_t COL_L = 930; // TS last column

uint8_t  FONT_SIZE  = 3; // font size
uint16_t FRGD_COLOR = GREEN; // foreground color
uint16_t BKG_COLOR  = BLACK; // background color


void BD_as_input(void) {
    DDRD = DDRD & 0x03;// Pins 7-2 as input, no change for pins 1,0 (RX TX)
    DDRB = DDRB & 0xFC; // Pins 8-9 as input
}

void BD_as_output(void) {
    DDRD = DDRD | 0xFC;// Pins 7-2 as output, no change for pins 1,0 (RX TX)
    DDRB = DDRB | 0x03;// Pins 8-9 as output
}

void lcdWrite(uint8_t d) {
    PORTC = PORTC & 0xFD; // WR 0    // ILI9341 reads data pins when WR rises from LOW to HIGH (A1 pin on arduino)
    PORTD = (PORTD & 0x03) | ((d) & 0xFC); // data pins of ILI9341 connected to two arduino ports
    PORTB = (PORTB & 0xFC) | ((d) & 0x03);
    PORTC = PORTC | 0x02; // WR 1
}

void lcdCommandWrite(uint8_t d) {
    PORTC = PORTC & 0xFB; // LCD_RS = 0, arduino pin A2
    lcdWrite(d);  // write data pins
}

void lcdDataWrite(uint8_t d) {
    PORTC = PORTC | 0x04; // LCD_RS = 1, arduino pin A2
    lcdWrite(d);  // write data pins
}

uint8_t lcdRead(void) {
    PORTC = PORTC | 0x04; // RS 1  // CS LOW, WR HIGH, RD HIGH->LOW>HIGH, RS(D/C) HIGH

    // LCD_RD - arduino pin A0 After RD falls from HIGH to LOW ILI9341 outputs data until RD returns to HIGH
    PORTC = PORTC & 0xFE; // RD 0
    BD_as_input(); // Set arduino pins as input
    uint8_t pin72 = PIND & 0xFC; // Read data pins 7-2
    uint8_t pin10 = PINB & 0x03; // Read data pins 1-0
    PORTC = PORTC | 0x01; // RD 1
    BD_as_output(); // Re-Set arduino pins as output
    return pin72 | pin10;
}


void lcd_init(void) {
    PORTC = PORTC | 0x10; // 1  // LCD_RESET 1 - 0 - 1, arduino pin A4
    delay(10);
    PORTC = PORTC & 0xEF; // 0
    delay(20);
    PORTC = PORTC | 0x10; // 1
    delay(20);

    // CS HIGH, WR HIGH, RD HIGH, CS LOW
    PORTC = PORTC | 0x08; // CS 1
    PORTC = PORTC | 0x02; // WR 1
    PORTC = PORTC | 0x01; // RD 1
    PORTC = PORTC & 0xF7; // CS 0

    lcdCommandWrite(0xF7); // Pump ratio control
    lcdDataWrite(0x20); //

    lcdCommandWrite(0x3A); // COLMOD: Pixel Format Set
    lcdDataWrite(0x55);

    lcdCommandWrite(0x36); // Memory Access Control
    // MY  - Row Address Order (bit7)
    // MX  - Column Address Order
    // MV  - Row / Column Exchange
    // ML  - Vertical Refresh Order
    // BGR - RGB-BGR Order
    // MH  - Horizontal Refresh ORDER(bit2)
    lcdDataWrite(0x08);
    lcdCommandWrite(0x11); // Sleep OUT
    lcdCommandWrite(0x29); // Display ON
    delay(50);
}

void lcdRect(int16_t col, int16_t row, int16_t width, int16_t height, int16_t color) {
    lcdCommandWrite(0x2a); // Column Address Set
    lcdDataWrite(row >> 8);
    lcdDataWrite(row);
    lcdDataWrite((row + height - 1) >> 8);
    lcdDataWrite(row + height - 1);
    lcdCommandWrite(0x2b); // Page Address Set
    lcdDataWrite(col >> 8);
    lcdDataWrite(col);
    lcdDataWrite((col + width - 1) >> 8);
    lcdDataWrite(col + width - 1);
    lcdCommandWrite(0x2c); // Memory Write

    byte color_high = color >> 8, color_low = color;
    int i, j;
    for(i = 0; i < width; i++)
        for(j = 0; j < height; j++) {
            lcdDataWrite(color_high);
            lcdDataWrite(color_low);
        }
}

/*
  Accelerate screen clearing sacrifing color depth. Instead of writing
    to data bits high and low byte of the color for each pixel, which takes more
    than 300ms to fill the screen, set once data bits to 0's for black or
    to 1's for white and start changing control bit WR from LOW to HIGH to
    write whole area. It takes cca 70 ms. In this way the color of screen are
    limited to those with the same high and low byte. For example setting color
    to 0x0C fills the screen with color 0x0C0C.
    Writing two pixels in one cycle lowering cycle count from 76800 (240x320) to
    38400 clears screen in less then 30ms.
*/
void lcdClear(byte color) {
    lcdCommandWrite(0x2a);
    lcdDataWrite(0);
    lcdDataWrite(0);
    lcdDataWrite(0);
    lcdDataWrite(0xEC);
    lcdCommandWrite(0x2b);
    lcdDataWrite(0);
    lcdDataWrite(0);
    lcdDataWrite(1);
    lcdDataWrite(0x3F);
    lcdCommandWrite(0x2c);

    PORTC = PORTC | 0x04; // LCD_RS = 1 - DATA
    PORTD = (PORTD & 0x03) | ((color) & 0xFC);
    PORTB = (PORTB & 0xFC) | ((color) & 0x03);

    uint16_t x = 240 * 320 / 2; // 38400; // 240*320/2
    byte wr0 = PORTC & 0xFD; // set WR 0
    byte wr1 = PORTC | 0x02; // set WR 1
    for(uint16_t y = 0; y < x; y++)
    {
        PORTC = wr0;
        PORTC = wr1;
        PORTC = wr0;
        PORTC = wr1;
        PORTC = wr0;
        PORTC = wr1;
        PORTC = wr0;
        PORTC = wr1;
    }
}

void displayChar(char znak) {
    static const byte ASCII[][5] =
    {
        {0x00, 0x00, 0x00, 0x00, 0x00}, // 20   (0)
        {0x00, 0x00, 0x5f, 0x00, 0x00}, // 21 ! (1)
        {0x00, 0x07, 0x00, 0x07, 0x00}, // 22 " (2)
        {0x14, 0x7f, 0x14, 0x7f, 0x14}, // 23 # (3)
        {0x24, 0x2a, 0x7f, 0x2a, 0x12} ,// 24 $ (4)
        {0x23, 0x13, 0x08, 0x64, 0x62}, // 25 % (5)
        {0x36, 0x49, 0x55, 0x22, 0x50}, // 26 &
        {0x00, 0x00, 0x07, 0x05, 0x07}, // 27 '
        {0x00, 0x1c, 0x22, 0x41, 0x00}, // 28 (
        {0x00, 0x41, 0x22, 0x1c, 0x00}, // 29 )
        {0x14, 0x08, 0x3e, 0x08, 0x14}, // 2a * (10)
        {0x08, 0x08, 0x3e, 0x08, 0x08}, // 2b + (11)
        {0x00, 0x50, 0x30, 0x00, 0x00}, // 2c ,
        {0x08, 0x08, 0x08, 0x08, 0x08}, // 2d -
        {0x00, 0x60, 0x60, 0x00, 0x00}, // 2e .
        {0x20, 0x10, 0x08, 0x04, 0x02}, // 2f /
        {0x3e, 0x51, 0x49, 0x45, 0x3e}, // 30 0 (16)
        {0x00, 0x42, 0x7f, 0x40, 0x00}, // 31 1
        {0x42, 0x61, 0x51, 0x49, 0x46}, // 32 2
        {0x21, 0x41, 0x45, 0x4b, 0x31}, // 33 3
        {0x18, 0x14, 0x12, 0x7f, 0x10}, // 34 4
        {0x27, 0x45, 0x45, 0x45, 0x39}, // 35 5
        {0x3c, 0x4a, 0x49, 0x49, 0x30}, // 36 6
        {0x01, 0x71, 0x09, 0x05, 0x03}, // 37 7
        {0x36, 0x49, 0x49, 0x49, 0x36}, // 38 8
        {0x06, 0x49, 0x49, 0x29, 0x1e}, // 39 9
        {0x00, 0x36, 0x36, 0x00, 0x00}, // 3a : (26)
        {0x00, 0x56, 0x36, 0x00, 0x00}, // 3b ;
        {0x08, 0x14, 0x22, 0x41, 0x00}, // 3c <
        {0x14, 0x14, 0x14, 0x14, 0x14}, // 3d =
        {0x00, 0x41, 0x22, 0x14, 0x08}, // 3e >
        {0x02, 0x01, 0x51, 0x09, 0x06}, // 3f ? (31)
        {0x32, 0x49, 0x79, 0x41, 0x3e}, // 40 @ (32)
        {0x7e, 0x11, 0x11, 0x11, 0x7e}, // 41 A (33)
        {0x7f, 0x49, 0x49, 0x49, 0x36}, // 42 B
        {0x3e, 0x41, 0x41, 0x41, 0x22}, // 43 C
        {0x7f, 0x41, 0x41, 0x22, 0x1c}, // 44 D
        {0x7f, 0x49, 0x49, 0x49, 0x41}, // 45 E
        {0x7f, 0x09, 0x09, 0x09, 0x01}, // 46 F
        {0x3e, 0x41, 0x49, 0x49, 0x7a}, // 47 G
        {0x7f, 0x08, 0x08, 0x08, 0x7f}, // 48 H
        {0x00, 0x41, 0x7f, 0x41, 0x00}, // 49 I
        {0x20, 0x40, 0x41, 0x3f, 0x01}, // 4a J
        {0x7f, 0x08, 0x14, 0x22, 0x41}, // 4b K
        {0x7f, 0x40, 0x40, 0x40, 0x40}, // 4c L
        {0x7f, 0x02, 0x0c, 0x02, 0x7f}, // 4d M
        {0x7f, 0x04, 0x08, 0x10, 0x7f}, // 4e N
        {0x3e, 0x41, 0x41, 0x41, 0x3e}, // 4f O
        {0x7f, 0x09, 0x09, 0x09, 0x06}, // 50 P
        {0x3e, 0x41, 0x51, 0x21, 0x5e}, // 51 Q
        {0x7f, 0x09, 0x19, 0x29, 0x46}, // 52 R
        {0x46, 0x49, 0x49, 0x49, 0x31}, // 53 S
        {0x01, 0x01, 0x7f, 0x01, 0x01}, // 54 T
        {0x3f, 0x40, 0x40, 0x40, 0x3f}, // 55 U
        {0x1f, 0x20, 0x40, 0x20, 0x1f}, // 56 V
        {0x3f, 0x40, 0x38, 0x40, 0x3f}, // 57 W
        {0x63, 0x14, 0x08, 0x14, 0x63}, // 58 X
        {0x07, 0x08, 0x70, 0x08, 0x07}, // 59 Y
        {0x61, 0x51, 0x49, 0x45, 0x43}, // 5a Z
        {0x00, 0x7f, 0x41, 0x41, 0x00}, // 5b [
        {0x02, 0x04, 0x08, 0x10, 0x20}, // 5c Y
        {0x00, 0x41, 0x41, 0x7f, 0x00}, // 5d ]
        {0x04, 0x02, 0x01, 0x02, 0x04}, // 5e ^
        {0x40, 0x40, 0x40, 0x40, 0x40}, // 5f _
        {0x00, 0x01, 0x02, 0x04, 0x00}, // 60 `
        {0x20, 0x54, 0x54, 0x54, 0x78}, // 61 a
        {0x7f, 0x48, 0x44, 0x44, 0x38}, // 62 b
        {0x38, 0x44, 0x44, 0x44, 0x20}, // 63 c
        {0x38, 0x44, 0x44, 0x48, 0x7f}, // 64 d
        {0x38, 0x54, 0x54, 0x54, 0x18}, // 65 e
        {0x08, 0x7e, 0x09, 0x01, 0x02}, // 66 f
        {0x0c, 0x52, 0x52, 0x52, 0x3e}, // 67 g
        {0x7f, 0x08, 0x04, 0x04, 0x78}, // 68 h
        {0x00, 0x44, 0x7d, 0x40, 0x00}, // 69 i
        {0x20, 0x40, 0x44, 0x3d, 0x00}, // 6a j
        {0x7f, 0x10, 0x28, 0x44, 0x00}, // 6b k
        {0x00, 0x41, 0x7f, 0x40, 0x00}, // 6c l
        {0x7c, 0x04, 0x18, 0x04, 0x78}, // 6d m
        {0x7c, 0x08, 0x04, 0x04, 0x78}, // 6e n
        {0x38, 0x44, 0x44, 0x44, 0x38}, // 6f o
        {0x7c, 0x14, 0x14, 0x14, 0x08}, // 70 p
        {0x08, 0x14, 0x14, 0x18, 0x7c}, // 71 q
        {0x7c, 0x08, 0x04, 0x04, 0x08}, // 72 r
        {0x48, 0x54, 0x54, 0x54, 0x20}, // 73 s
        {0x04, 0x3f, 0x44, 0x40, 0x20}, // 74 t
        {0x3c, 0x40, 0x40, 0x20, 0x7c}, // 75 u
        {0x1c, 0x20, 0x40, 0x20, 0x1c}, // 76 v
        {0x3c, 0x40, 0x30, 0x40, 0x3c}, // 77 w
        {0x44, 0x28, 0x10, 0x28, 0x44}, // 78 x
        {0x0c, 0x50, 0x50, 0x50, 0x3c}, // 79 y
        {0x44, 0x64, 0x54, 0x4c, 0x44}, // 7a z
        {0x00, 0x08, 0x36, 0x41, 0x00}, // 7b {
        {0x00, 0x00, 0x7f, 0x00, 0x00}, // 7c |
        {0x00, 0x41, 0x36, 0x08, 0x00}, // 7d }
        {0x10, 0x08, 0x08, 0x10, 0x08}, // 7e 
        {0x00, 0x06, 0x09, 0x09, 0x06}  // 7f 
    };

    // int8_t size = FONT_SIZE;
    // int16_t color = FRGD_COLOR;
    // int16_t bcolor = BKG_COLOR;

    if( (PTR_COL + (FONT_SIZE * 6)) > 319) {
        PTR_COL = 0;
        PTR_ROW += FONT_SIZE * (8 + 1);
    }

    lcdCommandWrite(0x2a); // ROWS
    lcdDataWrite(PTR_ROW >> 8);
    lcdDataWrite(PTR_ROW);
    lcdDataWrite(((PTR_ROW + FONT_SIZE * 8) - 1) >> 8);
    lcdDataWrite((PTR_ROW + FONT_SIZE * 8) - 1);
    lcdCommandWrite(0x2b); // COLUMNS
    lcdDataWrite(PTR_COL >> 8);
    lcdDataWrite(PTR_COL);
    lcdDataWrite((PTR_COL + (FONT_SIZE * 6)) >> 8);
    lcdDataWrite(PTR_COL + (FONT_SIZE * 6));
    lcdCommandWrite(0x2c);
    byte bkg_clr_high = BKG_COLOR >> 8;
    byte bkg_clr_low = BKG_COLOR;
    byte fgd_clr_high = FRGD_COLOR >> 8;
    byte fgd_clr_low = FRGD_COLOR;
    byte index, nbit, i, j;
    for (index = 0; index < 5; index++)
    {
        char col = ASCII[znak - 0x20][index];
        for ( i = 0; i < FONT_SIZE; i++)
        {
            byte mask = B00000001;
            for (nbit = 0; nbit < 8; nbit++)
            {
                if (col & mask) {
                    for (j = 0; j < FONT_SIZE; j++) {
                        lcdDataWrite(fgd_clr_high);
                        lcdDataWrite(fgd_clr_low);
                    }
                } else {
                    for (j = 0; j < FONT_SIZE; j++) {
                        lcdDataWrite(bkg_clr_high);
                        lcdDataWrite(bkg_clr_low);
                    }
                }
                mask = mask << 1;
            }
        }
    }

    for ( i = 0; i < FONT_SIZE; i++) {
        for (nbit = 0; nbit < 8; nbit++) {
            for (j = 0; j < FONT_SIZE; j++) {
                lcdDataWrite(bkg_clr_high);
                lcdDataWrite(bkg_clr_low);
            }
        }
    }
    PTR_COL += FONT_SIZE * 6;
}


void displayInteger(int16_t n) {
    String str = String(n);
    byte l = str.length();
    char b[l + 1]; // +1 for the null terminator
    str.toCharArray(b, l + 1);
    for(int n = 0; n < l; n++)
        displayChar(b[n]);
}

void displayString(String str) {
    byte l = str.length();
    char b[l + 1]; // +1 for the null terminator
    str.toCharArray(b, l + 1);
    for(int n = 0; n < l; n++) {
        displayChar(b[n]);
    }
}

void displayCharArray(const char* a) {
    displayString(String(a));
}

void displayClearChar(byte n) {

    // int8_t size = FONT_SIZE; // delete n chars
    // int16_t bcolor = BKG_COLOR;

    lcdCommandWrite(0x2a); // ROWS
    lcdDataWrite(PTR_ROW >> 8);
    lcdDataWrite(PTR_ROW);
    lcdDataWrite(((PTR_ROW + FONT_SIZE * 8) - 1) >> 8);
    lcdDataWrite((PTR_ROW + FONT_SIZE * 8) - 1);
    lcdCommandWrite(0x2b); // COLUMNS
    lcdDataWrite(PTR_COL >> 8);
    lcdDataWrite(PTR_COL);
    lcdDataWrite((PTR_COL + (FONT_SIZE * 6 * n)) >> 8);
    lcdDataWrite(PTR_COL + (FONT_SIZE * 6 * n));
    lcdCommandWrite(0x2c);

    byte bkg_clr_high = BKG_COLOR >> 8, bkg_clr_low = BKG_COLOR;
    int16_t cyc = FONT_SIZE * 8 * FONT_SIZE * 6 * n;
    for (int16_t i = 0; i < cyc; i++)
    {
        lcdDataWrite(bkg_clr_high);
        lcdDataWrite(bkg_clr_low);
    }
}
