///////////////////////////////////////////////STuff to transfer over////////////////////////////////////////////////////////
#include <SoftwareSerial.h>
/*-----( Declare Constants and Pin Numbers )-----*/
#define SSerialRX        19//a5 //12  //Serial Receive pin
#define SSerialTX        18//a4 //11  //Serial Transmit pin

#define SSerialTxControl 5//pd5 //10   //RS485 Direction control

#define RS485Transmit    HIGH
#define RS485Receive     LOW

word CRC_find(byte *msg, byte msglen);
bool error_check(byte *msg, byte msglen);
word getdata(byte ID, word *data);
byte get_CC_SOC_TempRemote(byte ID, word *SOC_TempRemote);
byte get_Date_Time(byte ID, word *Date_Time);
byte get_CC_TempBat_TempIE(byte ID, word *TempBat_TempIE);
byte get_CC_Load(byte ID, word *Load);
byte get_CC_PV(byte ID, word *PV);
byte get_CC_Bat(byte ID, word *Bat);

/*-----( Declare objects )-----*/
SoftwareSerial RS485Serial(SSerialRX, SSerialTX); // RX, TX

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*-----( Declare Variables )-----*/
int size_MsgReceived;
word data[20] = {0}; //This too as long as the size is 16 or greater it's fine. you might want to use 20 since we have the other 4 stuff too
int byteSendi;
byte byteSend[8] = {0};
int start = 0;


void setup()   /****** SETUP: RUNS ONCE ******/
{
  // Start the built-in serial port, probably to Serial Monitor
  Serial.begin(9600);
  Serial.println("Use Serial Monitor, type and press ENTER");
  
  ///////////////////////////////////////////////STuff to transfer over////////////////////////////////////////////////////////
  pinMode(SSerialTxControl, OUTPUT);    
  
  digitalWrite(SSerialTxControl, RS485Receive);  // Init Transceiver   
  
  // Start the software serial port, to another device
  RS485Serial.begin(115200);   // set the data rate 
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  Serial.println();
  Serial.print("Enter Message: ");
  
  
}//--(end setup )---


void loop()   /****** LOOP: RUNS CONSTANTLY ******/
{
  if (start>0)
  {
    Serial.println();
    Serial.print("Enter Message: ");
    start = 0;
  }
  
  int count = 0;
  if (Serial.available())
  {
    start = 1;
    
    char let = Serial.read();

    Serial.print("User pressed: ");
    Serial.println(let);
    
///////////////////////////////////////////////STuff maybe////////////////////////////////////////////////////////
//no need for the serial.print stuff but it shows you how to get the data and the error check stuff (prints it in binary to know th position of the error
    word err = getdata(0x01, data);
    Serial.print("error check: ");
    Serial.println(err,BIN);
    Serial.print("data received: ");
    for (int j = 0; j < 16;j++)
    {
      Serial.print(data[j],DEC);
      Serial.print(" ");
    }
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Serial.println();
    
    delay(1000); 
    Serial.flush();
  }

}


///////////////////////////////////////////////STuff to transfer over////////////////////////////////////////////////////////
//I will work on the commenting of the rest. the last function is what you are most interested in.
word CRC_find(byte *msg, byte msglen)
{
  
  word CRC = 0xFFFF;
      //010431040001
    for (int p = 0; p<msglen-2;p++)
    {
      CRC = CRC ^ msg[p];
      for (int c = 8; c!=0;c--)
      {
        if ((0x0001 & CRC)==0)
        {
          CRC = CRC >> 1;
        }
        else
        {
          CRC = CRC >> 1;
          CRC = CRC ^ 0xA001;
        }
      }
    }
    return CRC;
}

bool error_check(byte *msg, byte msglen)
{
  word CRC = CRC_find(msg,msglen);
  byte CRC_H = byte (CRC>>8);
  byte CRC_L = byte (0x00FF & CRC);

  if ((CRC_H == msg[msglen-1]) && (CRC_L == msg[msglen-2]))
    return 1;
  else
    return 0;
}


byte get_CC_PV(byte ID, word *PV)
{
    bool err = 0;
    int false_count = 0;
    byte msgR[60] = {0};
    int iterations = 5;

    byte byteSend[] = {ID, 0x04, 0x31, 0x00, 0x00, 0x02, 0x00, 0x00};
    word CRC = 0x0000;
    CRC = CRC_find(byteSend,8);
    byteSend[7] = byte (CRC>>8);
    byteSend[6] = byte (0x00FF & CRC);

    while (false_count < iterations)
    {
      digitalWrite(SSerialTxControl, RS485Transmit);  // Enable RS485 Transmit  
      delay(1);
      
      RS485Serial.write(byteSend,sizeof(byteSend));// Send byte to Remote Arduino 
      delay(1);
      digitalWrite(SSerialTxControl, RS485Receive);  // Disable RS485 Transmit
      byte size_data = RS485Serial.readBytes(msgR,60);    // Read received byte
      err = error_check(msgR,size_data);
      
      if (err && (msgR[2]==(size_data-5)))
      {
        PV[0] = ((word (msgR[3]<<8))|(word (0x00FF & msgR[4])));
        PV[1] = ((word (msgR[5]<<8))|(word (0x00FF & msgR[6])));
        break;
      }
      else
      {
        PV[0] = ((word (msgR[1]<<8))|(word (0x00FF & msgR[2])));
        PV[1] = ((word (msgR[1]<<8))|(word (0x00FF & msgR[2])));
        false_count = false_count +1;
      }
    }
    
    if (err)
    {
      return msgR[2]/2;
    }
    else
      return 0;    
}

byte get_CC_Bat(byte ID, word *Bat)
{
    bool err = 0;
    int false_count = 0;
    byte msgR[60] = {0};
    int iterations = 5;

    byte byteSend[] = {ID, 0x04, 0x31, 0x04, 0x00, 0x02, 0x00, 0x00};
    word CRC = 0x0000;
    CRC = CRC_find(byteSend,8);
    byteSend[7] = byte (CRC>>8);
    byteSend[6] = byte (0x00FF & CRC);

    while (false_count < iterations)
    {
      digitalWrite(SSerialTxControl, RS485Transmit);  // Enable RS485 Transmit  
      delay(1);
      
      RS485Serial.write(byteSend,sizeof(byteSend));// Send byte to Remote Arduino  
      delay(1);
      digitalWrite(SSerialTxControl, RS485Receive);  // Disable RS485 Transmit
      
      byte size_data = RS485Serial.readBytes(msgR,60);    // Read received byte
      err = error_check(msgR,size_data);
      if (err && (msgR[2]==(size_data-5)))
      {
        Bat[0] = ((word (msgR[3]<<8))|(word (0x00FF & msgR[4])));
        Bat[1] = ((word (msgR[5]<<8))|(word (0x00FF & msgR[6])));
        break;
      }
      else
      {
        Bat[0] = ((word (msgR[1]<<8))|(word (0x00FF & msgR[2])));
        Bat[1] = ((word (msgR[1]<<8))|(word (0x00FF & msgR[2])));
        false_count = false_count +1;
      }
    }
    
    if (err)
    {
      return msgR[2]/2;
    }
    else
      return 0;  
}

byte get_CC_Load(byte ID, word *Load)
{
    bool err = 0;
    int false_count = 0;
    byte msgR[60] = {0};
    int iterations = 5;

    byte byteSend[] = {ID, 0x04, 0x31, 0x0C, 0x00, 0x02, 0x00, 0x00};
    word CRC = 0x0000;
    CRC = CRC_find(byteSend,8);
    byteSend[7] = byte (CRC>>8);
    byteSend[6] = byte (0x00FF & CRC);

    while (false_count < iterations)
    {
      digitalWrite(SSerialTxControl, RS485Transmit);  // Enable RS485 Transmit  
      delay(1);
      
      RS485Serial.write(byteSend,sizeof(byteSend));// Send byte to Remote Arduino   
      delay(1);
      digitalWrite(SSerialTxControl, RS485Receive);  // Disable RS485 Transmit
      
      byte size_data = RS485Serial.readBytes(msgR,60);    // Read received byte
      err = error_check(msgR,size_data);
      if (err && (msgR[2]==(size_data-5)))
      {
        Load[0] = ((word (msgR[3]<<8))|(word (0x00FF & msgR[4])));
        Load[1] = ((word (msgR[5]<<8))|(word (0x00FF & msgR[6])));
        break;
      }
      else
      {
        Load[0] = ((word (msgR[1]<<8))|(word (0x00FF & msgR[2])));
        Load[1] = ((word (msgR[1]<<8))|(word (0x00FF & msgR[2])));
        false_count = false_count +1;
      }
        
    }
    
    if (err)
    {
      return msgR[2]/2;
    }
    else
      return 0;    
}

byte get_CC_TempBat_TempIE(byte ID, word *TempBat_TempIE)
{
    bool err = 0;
    int false_count = 0;
    byte msgR[60] = {0};
    int iterations = 5;

    byte byteSend[] = {ID, 0x04, 0x31, 0x10, 0x00, 0x02, 0x00, 0x00};
    word CRC = 0x0000;
    CRC = CRC_find(byteSend,8);
    byteSend[7] = byte (CRC>>8);
    byteSend[6] = byte (0x00FF & CRC);
      
    while (false_count < iterations)
    {
      digitalWrite(SSerialTxControl, RS485Transmit);  // Enable RS485 Transmit  
      delay(1);
      
      RS485Serial.write(byteSend,sizeof(byteSend));// Send byte to Remote Arduino  
      delay(1);
      digitalWrite(SSerialTxControl, RS485Receive);  // Disable RS485 Transmit
      
      byte size_data = RS485Serial.readBytes(msgR,60);    // Read received byte
      err = error_check(msgR,size_data);
      if (err && (msgR[2]==(size_data-5)))
      {
        TempBat_TempIE[0] = ((word (msgR[3]<<8))|(word (0x00FF & msgR[4])));
        TempBat_TempIE[1] = ((word (msgR[5]<<8))|(word (0x00FF & msgR[6])));
        break;
      }
      else
        false_count = false_count +1;
    }
    
    if (err)
    {
      return msgR[2]/2;
    }
    else
    {
      TempBat_TempIE[0] = ((word (msgR[1]<<8))|(word (0x00FF & msgR[2])));
      TempBat_TempIE[1] = ((word (msgR[1]<<8))|(word (0x00FF & msgR[2])));
      return 0;    
    }
}

byte get_CC_SOC_TempRemote(byte ID, word *SOC_TempRemote)
{
    bool err = 0;
    int false_count = 0;
    byte msgR[60] = {0};
    int iterations = 5;
    
    byte byteSend[] = {ID, 0x04, 0x31, 0x1A, 0x00, 0x02, 0x00, 0x00};
    word CRC = 0x0000;
    CRC = CRC_find(byteSend,8);
    byteSend[7] = byte (CRC>>8);
    byteSend[6] = byte (0x00FF & CRC);

    while (false_count < iterations)
    {
      digitalWrite(SSerialTxControl, RS485Transmit);  // Enable RS485 Transmit  
      delay(1);
      
      RS485Serial.write(byteSend,sizeof(byteSend));// Send byte to Remote Arduino 
      delay(1);
      digitalWrite(SSerialTxControl, RS485Receive);  // Disable RS485 Transmit
      
      byte size_data = RS485Serial.readBytes(msgR,60);    // Read received byte
      err = error_check(msgR,size_data);
      if (err && (msgR[2]==(size_data-5)))
      {
        SOC_TempRemote[0] = ((word (msgR[3]<<8))|(word (0x00FF & msgR[4])));
        SOC_TempRemote[1] = ((word (msgR[5]<<8))|(word (0x00FF & msgR[6])));
        break;
      }
      else
        false_count = false_count +1;
    }
    
    if (err)
    {
      return msgR[2]/2;
    }
    else
    {
      SOC_TempRemote[0] = ((word (msgR[1]<<8))|(word (0x00FF & msgR[2])));
      SOC_TempRemote[1] = ((word (msgR[1]<<8))|(word (0x00FF & msgR[2])));
      return 0;    
    }
}

byte get_Date_Time(byte ID, word *Date_Time)
{
    bool err = 0;
    int false_count = 0;
    byte msgR[60] = {0};
    int iterations = 5;

    byte byteSend[] = {ID, 0x03, 0x90, 0x13, 0x00, 0x03, 0x00, 0x00};
    word CRC = 0x0000;
    CRC = CRC_find(byteSend,8);
    byteSend[7] = byte (CRC>>8);
    byteSend[6] = byte (0x00FF & CRC);

    while (false_count < iterations)
    {
      digitalWrite(SSerialTxControl, RS485Transmit);  // Enable RS485 Transmit  
      delay(1);
      
      RS485Serial.write(byteSend,sizeof(byteSend));// Send byte to Remote Arduino   
      delay(1);
      digitalWrite(SSerialTxControl, RS485Receive);  // Disable RS485 Transmit
      
      byte size_data = RS485Serial.readBytes(msgR,60);    // Read received byte
      err = error_check(msgR,size_data);
      if (err && (msgR[2]==(size_data-5)))
      {
        Date_Time[0] = word (0x00FF & msgR[7]);
        Date_Time[1] = word (0x00FF & msgR[8]);
        Date_Time[2] = word (0x00FF & msgR[5]);
        Date_Time[3] = word (0x00FF & msgR[6]);
        Date_Time[4] = word (0x00FF & msgR[3]);
        Date_Time[5] = word (0x00FF & msgR[4]);
        break;
      }
      else
      {
        Date_Time[0] = ((word (msgR[1]<<8))|(word (0x00FF & msgR[2])));
        false_count = false_count +1;
      }
    }
    
    if (err)
    {
      return msgR[2]/2;
    }
    else
    {
      Date_Time[1] = Date_Time[0];
      Date_Time[2] = Date_Time[0];
      Date_Time[3] = Date_Time[0];
      Date_Time[4] = Date_Time[0];
      Date_Time[5] = Date_Time[0];
      return 0;
    }
}


word getdata(byte ID, word *data)//ID is in options default is 1 check later.
{
    word error = 0;
    word Date_Time[6] = {0};
    word PV[2] = {0};
    word Bat[2] = {0};
    word Load[2] = {0};
    word SOC_TempRemote[2] = {0};
    word TempBat_TempIE[2] = {0};
   
    byte err_dt = get_Date_Time(ID, Date_Time);
    byte err_pv = get_CC_PV(ID, PV);
    byte err_bat = get_CC_Bat(ID, Bat);
    byte err_load = get_CC_Load(ID, Load);
    byte err_temp_bat_ie = get_CC_TempBat_TempIE(ID, TempBat_TempIE);
    byte err_soc_tempext = get_CC_SOC_TempRemote(ID, SOC_TempRemote);

  //How the data array is structured:
    data[0] = Date_Time[0];
    data[1] = Date_Time[1];
    data[2] = Date_Time[2];
    data[3] = Date_Time[3];
    data[4] = Date_Time[4];
    data[5] = Date_Time[5];
    data[6] = PV[0];
    data[7] = PV[1];
    data[8] = Bat[0];
    data[9] = Bat[1];
    data[10] = Load[0];
    data[11] = Load[1];
    data[12] = TempBat_TempIE[0];
    data[13] = TempBat_TempIE[1];
    data[14] = SOC_TempRemote[0];
    data[15] = SOC_TempRemote[1];

    if (err_dt == 0)
      error = error | 0x003F;
    if (err_pv == 0)
      error = error | 0x00C0;
    if (err_bat ==0)
      error = error | 0x0300;
    if (err_load == 0)
      error = error | 0x0C00;
    if (err_temp_bat_ie ==0)
      error = error | 0x3000;
    if (err_soc_tempext == 0)
      error = error | 0xC000;
      
    return error;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

