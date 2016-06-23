#include <UTFT.h>
#include <Time.h>

#include <Ethernet.h>
#include <EthernetUdp.h>

// udp port
#define UDP_PORT 33333

// screen
#define COLOR_TOP_BACK 0x39E7
#define COLOR_TOP_FONT 0x8C51
#define COLOR_BODY_BACK 0x0000
#define COLOR_CHART 0x2104
#define COLOR_CHART_GREEN 0x2765
#define COLOR_CHART_YELLOW 0xFFC6
#define COLOR_CHART_RED 0xF863
#define COLOR_GEAR 0xF863
#define COLOR_RPM 0x2FFF
#define COLOR_SPEED 0x2FC6
#define COLOR_BODY_DESC_FONT 0x4228

// debug mode
// #define DEBUG

// define car info struct
typedef struct {
    uint8_t game;
    uint16_t speed;
    uint8_t gear;
    uint16_t rpm;
    uint16_t maxRpm;
} CarInfo;

// car info
CarInfo carInfo;
unsigned long time;

// mac address
byte mac[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

// segment draw function
uint8_t segmentPos[21][3] = {
    {0, 0, 0},
    {76, 0, 1},
    {76, 76, 1},
    {0, 152, 0},
    {0, 76, 1},
    {0, 0, 1},
    {0, 76, 0},
    {0, 0, 0},
    {36, 0, 1},
    {36, 36, 1},
    {0, 72, 0},
    {0, 36, 1},
    {0, 0, 1},
    {0, 36, 0},
    {0, 0, 0},
    {20, 0, 1},
    {20, 20, 1},
    {0, 40, 0},
    {0, 20, 1},
    {0, 0, 1},
    {0, 20, 0}
};

uint8_t segmentOffset[3][3] = {
    {71, 8, 3},
    {31, 4, 3},
    {15, 3, 3}
};

uint8_t segmentMap[13][7] = {
    {0, 0, 0, 0, 0, 0, 0},      //  null
    {1, 1, 1, 1, 1, 1, 0},      //  0
    {0, 1, 1, 0, 0, 0, 0},      //  1
    {1, 1, 0, 1, 1, 0, 1},      //  2
    {1, 1, 1, 1, 0, 0, 1},      //  3
    {0, 1, 1, 0, 0, 1, 1},      //  4
    {1, 0, 1, 1, 0, 1, 1},      //  5
    {1, 0, 1, 1, 1, 1, 1},      //  6
    {1, 1, 1, 0, 0, 0, 0},      //  7
    {1, 1, 1, 1, 1, 1, 1},      //  8
    {1, 1, 1, 1, 0, 1, 1},      //  9
    {0, 0, 1, 0, 1, 0, 1},      //  n
    {1, 1, 1, 0, 1, 1, 1}       //  r
};

uint32_t chartPos[5][2] = {
    {222, COLOR_CHART_GREEN},
    {180, COLOR_CHART_GREEN},
    {138, COLOR_CHART_YELLOW},
    {96, COLOR_CHART_RED},
    {54, COLOR_CHART_RED}
};

int8_t segmentBuf[9] = {-1, -1, -1, -1, -1, -1, -1, -1, -1};

// titles
char* titles[] = {
    "Connecting...\0",
    "255.255.255.255\0",
    "Testing Game\0",
    "Assetto Corsa\0",
    "DiRT Rally\0",
    "DiRT 3\0",
    "F1 2015\0",
    "Live For Speed\0",
    "Project Cars\0"
};

// udp instance
EthernetUDP udpClient;
bool udpConnected;

// tft screen
UTFT LCD(ITDB43, 38, 39, 40, 41);
extern uint8_t TinyFont[];
extern uint8_t SixteenSegment16x24[];

// init
void setup() {
#ifdef DEBUG
    Serial.begin(9600);
    while (!Serial) {
        ;
    }
#endif

    // init LCD
    setupLCD();

    // init dhcp
    if (Ethernet.begin(mac) == 0) {
#ifdef DEBUG
            Serial.println("DHCP Failed");
#endif

        for (;;)
            ;
    }

    displayIPAddress();

#ifdef DEBUG
    Serial.println(Ethernet.localIP());
#endif

    memset(&carInfo, 0, sizeof(CarInfo));
    time = millis();

    // init udp
    udpClient.begin(UDP_PORT);
    udpConnected = false;
}


// main loop
void loop() {
    // auto detect network
    if (!detectIPAddress()) {
        return;
    }

    // read from udp
    int packetSize = udpClient.parsePacket();

    if (packetSize > 0) {
        udpClient.read((char *) &carInfo, sizeof(CarInfo));
        udpConnected = true;
        time = millis();

#ifdef DEBUG
        for (int i = 0; i < sizeof(CarInfo); i ++) {
          Serial.print(((char *) &carInfo)[i], HEX);
          Serial.print(" ");
        }

        Serial.println();
        Serial.print(" speed: ");
        Serial.println(carInfo.speed);
#endif
    }

    updateCarInfo();
}

// detect the ip address is available
bool detectIPAddress() {
    switch (Ethernet.maintain()) {
        case 1:
            //renewed fail
#ifdef DEBUG
            Serial.println("Error: renewed fail");
#endif

            return false;

        case 2:
#ifdef DEBUG
                Serial.println("Renewed success");
#endif

            return true;

        case 3:
#ifdef DEBUG
                Serial.println("Error: rebind fail");
#endif

            return false;

        case 4:
            //rebind success
#ifdef DEBUG
                Serial.println("Rebind success");
#endif

            return true;

        default:
            //nothing happened
            return true;
    }
}

// setup LCD
void setupLCD() {
    uint8_t step = 25, start = 25;

    // clear screen
    LCD.InitLCD(LANDSCAPE_ROTATE);
    LCD.clrScr();

    // print body
    LCD.setColor(COLOR_BODY_BACK);
    LCD.fillRect(0, 15, 479, 272);

    // print top
    LCD.setColor(COLOR_TOP_BACK);
    LCD.fillRect(0, 0, 479, 14);

    // init gear
    displayStr("GEAR", 145, 35, COLOR_BODY_DESC_FONT, COLOR_BODY_BACK, SixteenSegment16x24);
    displayGear(0);

    // init rpm
    displayStr("RPM", 415, 78, COLOR_BODY_DESC_FONT, COLOR_BODY_BACK, SixteenSegment16x24);
    displayRpm(0);

    // init chart
    for (uint8_t i = 0; i < 5; i ++) {
        String str = "";
        str += start;
        str += "%";
        
        LCD.setColor(COLOR_CHART);
        LCD.fillRect(40, chartPos[i][0], 56, chartPos[i][0] + 16);
        displayStr(str.c_str(), 10, chartPos[i][0] + 5, COLOR_BODY_DESC_FONT, COLOR_BODY_BACK, TinyFont);

        start += step;
        step -= 5;
    }

    // init speed
    displayStr("KPH", 415, 218, COLOR_BODY_DESC_FONT, COLOR_BODY_BACK, SixteenSegment16x24);
    displaySpeed(0);

    // init ip
    displayTitle(0);
}

// display car info
void updateCarInfo() {
    static CarInfo carInfoDisplayed;

    if (!udpConnected) {
        return;
    }

    if (millis() - time > 2000) {
        memset(&carInfo, 0, sizeof(CarInfo));
        udpConnected = false;
    }

    if (carInfoDisplayed.speed != carInfo.speed) {
        carInfoDisplayed.speed = carInfo.speed;
        displaySpeed(carInfo.speed);
    }

    if (carInfoDisplayed.rpm != carInfo.rpm ||
        carInfoDisplayed.maxRpm != carInfo.maxRpm) {
        carInfoDisplayed.maxRpm = carInfo.maxRpm;
        displayChart(carInfo.rpm, carInfo.maxRpm);
    }

    if (carInfoDisplayed.rpm != carInfo.rpm) {
        carInfoDisplayed.rpm = carInfo.rpm;
        displayRpm(carInfo.rpm);
    } 

    if (carInfoDisplayed.gear != carInfo.gear) {
        carInfoDisplayed.gear = carInfo.gear;
        displayGear(carInfo.gear);
    }

    if (carInfoDisplayed.game != carInfo.game) {
        carInfoDisplayed.game = carInfo.game;
        displayTitle(carInfo.game + 1);
    }
}

// display ip address (every segment)
void displayIPAddress() {
    String ip = ""; 

    // init ip
    for (uint8_t i = 0; i < 4; i ++) {
        if (i != 0) {
            ip += ".";
        }

        ip += Ethernet.localIP()[i];
    }

    memset(titles[1], 0, 15);
    memcpy(titles[1], ip.c_str(), ip.length());
    displayTitle(1);
}

// display string
void displayStr(const char* str, int x, int y, uint32_t color, uint32_t bgColor, uint8_t* font) {
    LCD.setColor(color);
    LCD.setBackColor(bgColor);
    LCD.setFont(font);
    LCD.print(str, x, y);
}

// display game
void displayTitle(uint8_t nowTitle) {
    static uint8_t title = 255;
    static char buf[25];
    uint8_t len;

    if (nowTitle != title) {
        len = strlen(titles[nowTitle]);
      
        memset(buf, ' ', 24);
        buf[24] = 0;
        memcpy(buf + (24 - len) / 2, titles[nowTitle], len);
        displayStr(buf, CENTER, 4, COLOR_TOP_FONT, COLOR_TOP_BACK, TinyFont);
        title = nowTitle;
    }
}

// display gear
void displayGear(uint8_t gear) {
    if (gear == 0) {
        gear = 10;
    } else if (gear == 255) {
        gear = 11;
    }

    drawNum(gear, 0, 0, 115, 71, COLOR_GEAR, COLOR_BODY_BACK);
}

// display chart
void displayChart(uint16_t rpm, uint16_t maxRpm) {
    const uint8_t r = 16, x = 40;
    static uint8_t size = 0;
    uint8_t nowSize = 0;
    float percent;

    if (maxRpm > 0) {
        percent = rpm <= maxRpm ? rpm / ((float) maxRpm) : 1;
    } else {
        percent = 0;
    }

    if (percent >= 0.95) {
        nowSize = 5;
    } else if (percent >= 0.85) {
        nowSize = 4;
    } else if (percent >= 0.7) {
        nowSize = 3;
    } else if (percent >= 0.5) {
        nowSize = 2;
    } else if (percent >= 0.25) {
        nowSize = 1;
    }
    
    if (nowSize != size) {
        if (nowSize > size) {
            for (uint8_t i = size; i < nowSize; i ++) {
                LCD.setColor(chartPos[i][1]);
                LCD.fillRect(x + 1, chartPos[i][0] + 1, x + r - 1, chartPos[i][0] + r - 1);
            }
        } else {
            for (uint8_t i = nowSize; i < size; i ++) {
                LCD.setColor(COLOR_CHART);
                LCD.fillRect(x + 1, chartPos[i][0] + 1, x + r - 1, chartPos[i][0] + r - 1);
            }
        }

        size = nowSize;
    }
}

// display rpm
void displayRpm(uint16_t rpm) {
    const int x = 250, y = 53, offset = 31;
    int div = 10000;
    uint8_t count;
    bool found = false;

    for (uint8_t i = 0; i < 5; i ++) {
        count = rpm / div;
        rpm = rpm % div;
        
        if (count > 0 || div == 1) {
            found = true;
        }

        count = count > 0 ? count : (found ? count : -1);
        drawNum(count, 1 + i, 2, x + i * 31, y, COLOR_RPM, COLOR_BODY_BACK);
        div = div / 10;
    }
}

// display speed
void displaySpeed(uint16_t speed) {
    const int x = 248, y = 158, offset = 31;
    int div = 100;
    uint8_t count;
    bool found = false;

    for (uint8_t i = 0; i < 3; i ++) {
        count = speed / div;
        speed = speed % div;
        
        if (count > 0 || div == 1) {
            found = true;
        }

        count = count > 0 ? count : (found ? count : -1);
        drawNum(count, 6 + i, 1, x + i * 53, y, COLOR_SPEED, COLOR_BODY_BACK);
        div = div / 10;
    }
}

// display ip
void displayIp(char* ip) {
    displayStr(ip, 5, 4, COLOR_TOP_FONT, COLOR_TOP_BACK, TinyFont);
}

// draw segment number
void drawNum(int8_t num, uint8_t offset, uint8_t type, int x, int y, uint32_t color, uint32_t bgColor) {
    for (uint8_t i = 0; i < 7; i ++) {
        if (segmentMap[segmentBuf[offset] + 1][i] == segmentMap[num + 1][i]) {
            continue;
        }

        drawSegment(i, type, x, y, segmentMap[segmentBuf[offset] + 1][i] > segmentMap[num + 1][i] 
            ? bgColor : color);
    }

    segmentBuf[offset] = num;
}

void drawSegment(uint8_t pos, uint8_t type, int x, int y, word color) {
    uint8_t offset = type * 7 + pos;
    LCD.setColor(color);

    x += segmentPos[offset][0];
    y += segmentPos[offset][1];

    if (segmentPos[offset][2]) {
        drawVSegment(x, y, type);
    } else {
        drawHSegment(x, y, type);
    }
}

void drawHSegment(int x, int y, uint8_t type) {
    x += segmentOffset[type][1] + segmentOffset[type][2];
    y += segmentOffset[type][1];

    for (uint8_t i = 0; i < segmentOffset[type][1]; i ++) {
        LCD.drawLine(x + i, y - i, x + segmentOffset[type][0] - i, y - i);
        LCD.drawLine(x + i, y + 1 + i, x + segmentOffset[type][0] - i, y + 1 + i);
    }
}

void drawVSegment(int x, int y, uint8_t type) {
    x += segmentOffset[type][1];
    y += segmentOffset[type][1] + segmentOffset[type][2];

    for (uint8_t i = 0; i < segmentOffset[type][1]; i ++) {
        LCD.drawLine(x - i, y + i, x - i, y + segmentOffset[type][0] - i);
        LCD.drawLine(x + 1 + i, y + i, x + 1 + i, y + segmentOffset[type][0] - i);
    }
}

