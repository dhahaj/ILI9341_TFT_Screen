/*
 Connect data pins LCD_D 0-7 to arduino UNO:
 LCD_D 0 -- D8
 LCD_D 1 -- D9
 LCD_D 2 -- D2
 LCD_D 3 -- D3
 LCD_D 4 -- D4
 LCD_D 5 -- D5
 LCD_D 6 -- D6
 LCD_D 7 -- D7
 Connect command pins:
 LCD_RST -- A4   1 -> 0 min 15 micros 0 -> 1
 LCD_CS  -- A3   chip select, active LOW
 LCD_RS  -- A2   data/command select, 0 command, 1 data
 LCD_WR  -- A1   0 -> 1, HIGH when not used
 LCD_RD  -- A0   0 -> 1, HIGH when not used

 arduino uno porty:
 B (digital pin 8 to 13)
 C (analog input pins)
 D (digital pins 0 to 7)   0 1 are RX TX, don't use
*/

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

byte readTouch(void) {
    //Y1 A3
    //X1 A2
    //Y2 9
    //X2 8
    int16_t row, col;
    int8_t touch, wait_touch = 1, valid = 0;
    while (wait_touch)
    {
        pinMode(Y1, INPUT);
        pinMode(Y2, INPUT_PULLUP);
        pinMode(X1, OUTPUT);
        pinMode(X2, OUTPUT);
        digitalWrite(X1, LOW);
        digitalWrite(X2, LOW);

        touch = !digitalRead(Y1); // 0 - touched
        if (touch)
        {
            //delay(5);
            digitalWrite(X1, HIGH);   // X variant A
            //digitalWrite(X2, HIGH);   // X variant B
            delay(1);
            row = analogRead(Y1);
            delay(4);
            if (abs(analogRead(Y1) - row) > 3)
                return 0;
            delay(3);
            if (abs(analogRead(Y1) - row) > 3)
                return 0;
            //if (analogRead(Y1)!=row) { return 0;}

            pinMode(X1, INPUT);
            pinMode(X2, INPUT_PULLUP);
            pinMode(Y1, OUTPUT);
            pinMode(Y2, OUTPUT);
            //digitalWrite(Y1, HIGH);  // Y variant A
            //digitalWrite(Y2, LOW);  // Y variant A
            digitalWrite(Y1, LOW);  // Y variant B
            digitalWrite(Y2, HIGH);  // Y variant B
            delay(1);
            col = analogRead(X1);
            delay(4);
            if (abs(analogRead(X1) - col) > 3)
                return 0;
            delay(3);
            if (abs(analogRead(X1) - col) > 3)
                return 0;
            //if (analogRead(X1)!=col) { return 0;}

            //digitalWrite(Y1, LOW);  // Y variant A
            digitalWrite(Y2, LOW);  // Y variant B
            //delay(5);
            touch = !digitalRead(X1); // 0 - dotyk
            if (touch) {
                int16_t rows = ROW_L - ROW_F;
                int16_t cols = COL_L - COL_F;
                float row1 = float(row - ROW_F) / rows * 240;
                float col1 = float(col - COL_F) / cols * 320;
                TS_ROW = int(row1);
                TS_COL = int(col1);
                valid = 1;
            }
            wait_touch = 0;
        }
    }

    BD_as_output(); // Re-Set A2 A3 8 9 for ILI9341
    DDRC = DDRC | B00011111; // A0-A4 as outputs

    // To find out values for calibration F_ROW, L_ROW, F_COL, F_COL
    BKG_COLOR = 0x0C0C;
    FONT_SIZE = 2;
    PTR_COL = 230;
    PTR_ROW = 200;
    displayInteger(row);
    PTR_COL = 280;
    PTR_ROW = 200;
    displayInteger(col);

    PTR_COL = 230;
    PTR_ROW = 220;
    displayClearChar(3);
    displayInteger(TS_ROW);
    PTR_COL = 280;
    PTR_ROW = 220;
    displayClearChar(3);
    displayInteger(TS_COL);
    BKG_COLOR = GREEN;
    FONT_SIZE = 3;

    // lcdRect(TS_COL - 1, TS_ROW - 1, 2, 2, FRGD_COLOR); // Draw a point where touched
    return valid;
}

// draw keypad
String KEYPAD_LABEL[]  =
{
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "*",
    "0",
    "#"
};

uint16_t KEYPAD_ROW[]  =
{
    150,
    150,
    150,
    100,
    100,
    100,
    50,
    50,
    50,
    200,
    200,
    200
};

uint16_t KEYPAD_COL[]  =
{
    10,
    50,
    90,
    10,
    50,
    90,
    10,
    50,
    90,
    10,
    50,
    90
};

uint16_t D_COL, D_ROW;

void setup()
{
    //  Serial.begin(9600);
    BD_as_output(); // Set pins 1-8 as output
    DDRC = DDRC | B00011111;// Set pins A0-A4 as output

    lcd_init();
    lcdClear(BLACK);

    BKG_COLOR = BLACK;
    FRGD_COLOR = GREEN;
    
    lcdRect(0, 0, 320, 240, BKG_COLOR); // clear whole screen, slow
    
    for (int i = 0; i < 12; i++) { // draw a keypad
        PTR_COL = KEYPAD_COL[i];
        PTR_ROW = KEYPAD_ROW[i];
        lcdRect(PTR_COL, PTR_ROW, 35, 38, GREEN);
        PTR_COL = KEYPAD_COL[i] + 12;
        PTR_ROW = KEYPAD_ROW[i] + 10;
        displayString(KEYPAD_LABEL[i]);
    }
    D_COL = 10; // where to display number from keypad
    D_ROW = 10;
}

void loop()
{
    if(!readTouch()) return;

    for (int i = 0; i < 12; i++)
    {
        if ( TS_COL > KEYPAD_COL[i] && TS_COL < KEYPAD_COL[i] + 35 && TS_ROW > KEYPAD_ROW[i] && TS_ROW < KEYPAD_ROW[i] + 38 ) {
            PTR_COL = KEYPAD_COL[i];
            PTR_ROW = KEYPAD_ROW[i];
            lcdRect(PTR_COL, PTR_ROW, 35, 38, RED);
            PTR_COL = KEYPAD_COL[i] + 12;
            PTR_ROW = KEYPAD_ROW[i] + 10;
            BKG_COLOR = RED;
            displayString(KEYPAD_LABEL[i]);
            if (KEYPAD_LABEL[i] == "#") {
                if (D_COL > 10) {
                    D_COL -= FONT_SIZE * 6;
                    PTR_COL = D_COL;
                    PTR_ROW = D_ROW;
                    BKG_COLOR = BLACK;
                    displayClearChar(1);
                }
            } else if( (D_COL + (FONT_SIZE * 6)) < 320 ) {
                PTR_COL = D_COL;
                PTR_ROW = D_ROW;
                BKG_COLOR = BLACK;
                displayString(KEYPAD_LABEL[i]);
                D_COL = PTR_COL;
                D_ROW = PTR_ROW;
            }
            delay(85);
            PTR_COL = KEYPAD_COL[i];
            PTR_ROW = KEYPAD_ROW[i];
            lcdRect(PTR_COL, PTR_ROW, 35, 38, BLUE);
            PTR_COL = KEYPAD_COL[i] + 12;
            PTR_ROW = KEYPAD_ROW[i] + 10;
            BKG_COLOR = BLACK;
            displayString(KEYPAD_LABEL[i]);
        }
    }
}
