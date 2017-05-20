#include <DS3231.h> // RTC用ライブラリ
#include <Wire.h>

////////////////INA226/////////////////////////

#define NELEMS(arg) (sizeof(arg) / sizeof((arg)[0]))

const int   INA226_ADDR         = 0x40;                 // INA226 I2C Address (A0=A1=GND)
const word  INA226_CAL_VALUE    = 0x0A00;               // INA226 Calibration Register Value
const int   INA226_CAL_VALUE2   = 50;                   // シャント抵抗が大きい場合の計算値

// INA226 Registers
#define INA226_REG_CONGIGURATION_REG            0x00    // Configuration Register (R/W)
#define INA226_REG_SHUNT_VOLTAGE                0x01    // Shunt Voltage (R)
#define INA226_REG_BUS_VOLTAGE                  0x02    // Bus Voltage (R)
#define INA226_REG_POWER                        0x03    // Power (R)
#define INA226_REG_CURRENT                      0x04    // Current (R)
#define INA226_REG_CALIBRATION                  0x05    // Calibration (R/W)
#define INA226_REG_MASK_ENABLE                  0x06    // Mask/Enable (R/W)
#define INA226_REG_ALERT_LIMIT                  0x07    // Alert Limit (R/W)
#define INA226_REG_DIE_ID                       0xFF    // Die ID (R)

// Operating Mode (Mode Settings [2:0])
#define INA226_CONF_MODE_POWER_DOWN             (0<<0)  // Power-Down
#define INA226_CONF_MODE_TRIG_SHUNT_VOLTAGE     (1<<0)  // Shunt Voltage, Triggered
#define INA226_CONF_MODE_TRIG_BUS_VOLTAGE       (2<<0)  // Bus Voltage, Triggered
#define INA226_CONF_MODE_TRIG_SHUNT_AND_BUS     (3<<0)  // Shunt and Bus, Triggered
#define INA226_CONF_MODE_POWER_DOWN2            (4<<0)  // Power-Down
#define INA226_CONF_MODE_CONT_SHUNT_VOLTAGE     (5<<0)  // Shunt Voltage, Continuous
#define INA226_CONF_MODE_CONT_BUS_VOLTAGE       (6<<0)  // Bus Voltage, Continuous
#define INA226_CONF_MODE_CONT_SHUNT_AND_BUS     (7<<0)  // Shunt and Bus, Continuous (default)

// Shunt Voltage Conversion Time (VSH CT Bit Settings [5:3])
#define INA226_CONF_VSH_140uS                   (0<<3)  // 140us
#define INA226_CONF_VSH_204uS                   (1<<3)  // 204us
#define INA226_CONF_VSH_332uS                   (2<<3)  // 332us
#define INA226_CONF_VSH_588uS                   (3<<3)  // 588us
#define INA226_CONF_VSH_1100uS                  (4<<3)  // 1.1ms (default)
#define INA226_CONF_VSH_2116uS                  (5<<3)  // 2.116ms
#define INA226_CONF_VSH_4156uS                  (6<<3)  // 4.156ms
#define INA226_CONF_VSH_8244uS                  (7<<3)  // 8.244ms

// Bus Voltage Conversion Time (VBUS CT Bit Settings [8:6])
#define INA226_CONF_VBUS_140uS                  (0<<6)  // 140us
#define INA226_CONF_VBUS_204uS                  (1<<6)  // 204us
#define INA226_CONF_VBUS_332uS                  (2<<6)  // 332us
#define INA226_CONF_VBUS_588uS                  (3<<6)  // 588us
#define INA226_CONF_VBUS_1100uS                 (4<<6)  // 1.1ms (default)
#define INA226_CONF_VBUS_2116uS                 (5<<6)  // 2.116ms
#define INA226_CONF_VBUS_4156uS                 (6<<6)  // 4.156ms
#define INA226_CONF_VBUS_8244uS                 (7<<6)  // 8.244ms

// Averaging Mode (AVG Bit Settings[11:9])
#define INA226_CONF_AVG_1                       (0<<9)  // 1 (default)
#define INA226_CONF_AVG_4                       (1<<9)  // 4
#define INA226_CONF_AVG_16                      (2<<9)  // 16
#define INA226_CONF_AVG_64                      (3<<9)  // 64
#define INA226_CONF_AVG_128                     (4<<9)  // 128
#define INA226_CONF_AVG_256                     (5<<9)  // 256
#define INA226_CONF_AVG_512                     (6<<9)  // 512
#define INA226_CONF_AVG_1024                    (7<<9)  // 1024

// Reset Bit (RST bit [15])
#define INA226_CONF_RESET_ACTIVE                (1<<15)
#define INA226_CONF_RESET_INACTIVE              (0<<15)


static void writeRegister(byte reg, word value)
{
  Wire.beginTransmission(INA226_ADDR);
  Wire.write(reg);
  Wire.write((value >> 8) & 0xFF);
  Wire.write(value & 0xFF);
  Wire.endTransmission();
}

static void setupINA226Register(void)
{
  writeRegister(INA226_REG_CONGIGURATION_REG, 
        INA226_CONF_RESET_INACTIVE
      | INA226_CONF_MODE_CONT_SHUNT_AND_BUS
      | INA226_CONF_VSH_1100uS
      | INA226_CONF_VBUS_1100uS
      | INA226_CONF_AVG_64
      );

  writeRegister(INA226_REG_CALIBRATION, INA226_CAL_VALUE);
}

static word readRegister(byte reg)
{
  word res = 0x0000;
  
  Wire.beginTransmission(INA226_ADDR);
  Wire.write(reg);
  
  if(Wire.endTransmission() == 0) {
    if(Wire.requestFrom(INA226_ADDR, 2) >= 2) {
      res = Wire.read() * 256;
      res += Wire.read();
    }
  }
  
  return res;
}

typedef struct tagREG_INFO {
  byte  reg;
  const char* name;
}REG_INFO;

const static REG_INFO   st_aRegs[] = {
  { INA226_REG_CONGIGURATION_REG,   "Configuration Register"    },
  { INA226_REG_SHUNT_VOLTAGE,       "Shunt Voltage"             },
  { INA226_REG_BUS_VOLTAGE,         "Bus Voltage"               },
  { INA226_REG_POWER,               "Power"                     },
  { INA226_REG_CURRENT,             "Current"                   },
  { INA226_REG_CALIBRATION,         "Calibration"               },
  { INA226_REG_MASK_ENABLE,         "Mask/Enable"               },
  { INA226_REG_ALERT_LIMIT,         "Alert Limit"               },
  { INA226_REG_DIE_ID,              "Die ID"                    },
};

static void dumpRegisters(void)
{ 
  int i;
  const REG_INFO* pInfo;
  static word REGS[NELEMS(st_aRegs)];
  static char  buf[64];
  
  for(i = 0; i < NELEMS(REGS); i++) {
    pInfo = &st_aRegs[i];
    REGS[i] = readRegister(pInfo->reg);
  }
  
  Serial.println("---INA226 Registers ---");
  
  for(i = 0; i < NELEMS(REGS); i++) {
    pInfo = &st_aRegs[i];
    snprintf(buf, NELEMS(buf), "%24s (%02Xh) : %04Xh (%u)", pInfo->name, pInfo->reg, REGS[i], REGS[i]);
    Serial.println(buf);
  }
}

/////////////////INA226 end //////////////////////////////


DS3231 RTC;    // RTC用構造体
char dispChar[70];  //  表示用文字列バッファ
char input[70]; //  文字列格納
int  comd_i = 0; //  コマンド文字列カウンタ

//  ヘルプ　コマンド一覧表示関数
void helpDisp() {
  Serial.println("***********************************");
  Serial.println("help      This Help Command List");
  Serial.println("setdate   YYMMDD   Date set command");
  Serial.println("getdate            Date get command");
  Serial.println("get226             INA226 INFO get command");
  Serial.println("relay on | off     Relay on/off command");
  Serial.println("***********************************");
}

// 日付設定関数
void setDateDisp(char *date) {
  char year[3];
  char month[3];
  char day[3];
  if (strlen(date) != 6) {
    Serial.println("Date chaek fail not YYMMDD format");
    return;
  }
  strncpy(year, date, 2);
  strncpy(month, date+2, 2);
  strncpy(day, date+4, 2);
  Serial.print("Set Date:"); Serial.println(date);
//   Serial.println(year);  Serial.println(month);Serial.println(day);  //Debug
  RTC.setYear(atoi(year));
  RTC.setMonth(atoi(month));
  RTC.setDate(atoi(day));
}

// 日付取得関数
void getDateDisp() {
  bool century = false;
  sprintf(dispChar, "Get Date 20%02d/%02d/%02d\n", RTC.getYear(), RTC.getMonth(century), RTC.getDate());
  Serial.println(dispChar);
}

// 時間設定関数
void setTimeDisp(char *timeval) {
  char hour[3];
  char minute[3];
  char second[3];
  if (strlen(timeval) != 6) {
    Serial.println("Date chaek fail not HHMMSS format");
    return;
  }
  strncpy(hour, timeval, 2);
  strncpy(minute, timeval+2, 2);
  strncpy(second, timeval+4, 2);
  Serial.print("Set Time:"); Serial.println(timeval);
  //Serial.println(hour);  Serial.println(minute);Serial.println(second);  //Debug
  RTC.setHour(atoi(hour));
  RTC.setMinute(atoi(minute));
  RTC.setSecond(atoi(second));
}

// 時間取得関数
void getTimeDisp() {
  bool h12 = false;
  bool PM = false;
  sprintf(dispChar, "Get Time %02d:%02d:%02d\n", RTC.getHour(h12, PM), RTC.getMinute(), RTC.getSecond());
  Serial.println(dispChar);
}

// INA226 電圧電流計測関数
void getINA226Disp() {
  char  buf[64];
  long  voltage;   // Bus Voltage (mV)
  float current;   // Current (mA)
  float  power;     // Power (uW)
  char curflt[20];
  char powflt[20];

  voltage  = (long)((short)readRegister(INA226_REG_BUS_VOLTAGE)) * 1250L;    // LSB=1.25mV
  current  = (float)readRegister(INA226_REG_CURRENT);
  power    = (float)readRegister(INA226_REG_POWER) * 25000L;                  // LSB=25mW
  
  Serial.println();
  snprintf(buf, NELEMS(buf)
    , "V:%5ldmV, I:%smA, P:%smW"
    , (voltage + (1000/2)) / 1000
    , dtostrf((current / INA226_CAL_VALUE2), 5, 2, curflt)
    , dtostrf((power + (1000/2)) / 1000 / INA226_CAL_VALUE2, 5, 2, powflt)
    );    
  Serial.println(buf);

  dumpRegisters();
}

// リレーON/OFF設定関数
void setRelay(char *onOffFlg) {

  if(strcmp(onOffFlg, "on")==0) {
    digitalWrite(10, 1);
  } else if(strcmp(onOffFlg, "off")==0) {
    digitalWrite(10, 0);
  } else {
    sprintf(dispChar, "recv Relay not command; %s\n", onOffFlg); // 受信パラメータ表示
    Serial.print(dispChar);
  }

}

void setup() {
  Wire.begin(); // I2C通信用
  Serial.begin(9600); // シリアル通信速度設定

  setupINA226Register();  // INA226用初期設定

  pinMode(10, OUTPUT);    // リレー接続PIN
}


void loop() {
  
  char *comm_buf[2];  // コマンド処理用バッファ

  // シリアルコマンド受信部 データが来ているときは読み込む。改行コード\nがあればコマンドを処理
  while(Serial.available() > 0) {
    input[comd_i] = Serial.read();
    //sprintf(dispChar, "avail:%d, readChar:%c, comd_i;%d\n", Serial.available(), input[comd_i], comd_i); // 受信コマンドを一文字ずつ処理
    //Serial.print(dispChar);
    comd_i++;
  }

  // コマンド処理部 最後が改行コードなら文字が着ていると判断
  if (input[comd_i-1] == '\n') {
    
    input[comd_i-2] = '\0'; //CR+LFで文字列が来ているので最後に終端文字を挿入
    sprintf(dispChar, "recv command; %s\n", input); // 受信コマンド表示
    Serial.print(dispChar);
    
    // コマンドの先頭をスペースを区切りに切り出す
    comm_buf[0] = strtok(input, " ");
    comm_buf[1] = strtok(NULL, " ");

    //sprintf(dispChar, "recv buf1 command; %s\n", comm_buf[0]); // 受信コマンド表示
    //Serial.print(dispChar);
    //sprintf(dispChar, "recv buf2 command; %s\n", comm_buf[1]); // 受信コマンド表示
    //Serial.print(dispChar);
    
    // コマンド一覧 以下にif分で区切って処理を書いていく
    if(strcmp(comm_buf[0], "help")==0) {
      helpDisp();
    } else if(strcmp(comm_buf[0], "setdate")==0) {
      setDateDisp(comm_buf[1]);
    } else if(strcmp(comm_buf[0], "getdate")==0) {
      getDateDisp();
    } else if(strcmp(comm_buf[0], "settime")==0) {
      setTimeDisp(comm_buf[1]);
    } else if(strcmp(comm_buf[0], "gettime")==0) {
      getTimeDisp();
    } else if(strcmp(comm_buf[0], "get226")==0) {
      getINA226Disp();
    } else if(strcmp(comm_buf[0], "relay")==0) {
      setRelay(comm_buf[1]);
    } else {
      Serial.println("Recv String is not command\n");
    }
    input[0] = '\0';
    comd_i = 0;
  }
  
}
