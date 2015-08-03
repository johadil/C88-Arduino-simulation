#define SPI_MOSI 2
#define SPI_CLK 3
#define SPI_CS 4

#define OP_DECODEMODE 9
#define OP_INTENSITY 10
#define OP_SCANLIMIT 11
#define OP_SHUTDOWN 12
#define OP_DISPLAYTEST 15

#define POTI A0

#define ROW1 13
#define ROW2 12
#define ROW3 11
#define ROW4 10
#define COL1 A1
#define COL2 A2
#define COL3 A3
#define COL4 A4

byte spidata[2];
byte status[8];

byte ram[8];
byte accumulator;
byte programCounter;

byte whatToPrint=1; //1=Ram 2=Accumulator 3=ProgramCounter
byte whatIsPrinted=0; //0=Nothing 1=Ram 2=Accumulator 3=ProgramCounter
boolean run=false;

void setup()
{
  C88Initialize();
  pinMode(ROW1,OUTPUT);
  pinMode(ROW2,OUTPUT);
  pinMode(ROW3,OUTPUT);
  pinMode(ROW4,OUTPUT);
  digitalWrite(ROW1,HIGH);
  digitalWrite(ROW2,HIGH);
  digitalWrite(ROW3,HIGH);
  digitalWrite(ROW4,HIGH);
  pinMode(COL1,INPUT_PULLUP);
  pinMode(COL2,INPUT_PULLUP);
  pinMode(COL3,INPUT_PULLUP);
  pinMode(COL4,INPUT_PULLUP);
  Serial.begin(9600);
}

void loop()
{
  byte input=keyMatrix();
  if(input!=0)
  {
    switch(input)
   {
      case 4:
       whatToPrint=1;
        break;
      case 8:
        whatToPrint=2;
        break;
      case 12:
        whatToPrint=3;
        break;
      case 16:
        run=!run;
        break;
      default:
        break;
    }
  }
  switch(whatToPrint)
  {
    case 1:
      C88PrintRam();
      whatIsPrinted=whatToPrint;
      break;
    case 2:
      C88PrintAccumulator();
      whatIsPrinted=whatToPrint;
      break;
    case 3:
      C88PrintProgramCounter();
      whatIsPrinted=whatToPrint;
      break;
  }
  if(run)
    C88Step();
}

void C88Initialize()
{
  initializeLedMatrix();
  C88ClearRam();
  accumulator=0;
  programCounter=0;
  C88SetRam();
}

void C88Step()
{
  byte opcode= ram[programCounter] & B11111000;
  byte adress= ram[programCounter] & B00000111;
  byte cache;
  switch(opcode)
  {
    //LOAD Load an address into the register
    case B00000000:
      accumulator=ram[adress];
      programCounter++;
      break;
    //SWAP Swap the register value and the value at some address
    case B00001000:
      cache=ram[adress];
      ram[adress]=accumulator;
      accumulator=cache;
      programCounter++;
      break;
    //STORE Store the register into an address
    case B00010000:
      ram[adress]=accumulator;
      programCounter++;
      break;
    //STOP Stop the program
    case B00011000:
      break;
    //TSG Test, skip if greater
    case B00100000:
      if(ram[adress]>accumulator)
      {
        programCounter+=2;
      }
      else
      {
        programCounter++;
      }
      break;
    //TSL Test, skip if less
    case B00101000:
      if(ram[adress]<accumulator)
      {
        programCounter+=2;
      }
      else
      {
        programCounter++;
      }
      break;
    //TSE Test, skip if equal
    case B00110000:
      if(ram[adress]==accumulator)
      {
        programCounter+=2;
      }
      else
      {
        programCounter++;
      }
      break;
    //TSI Test, skip if inequal
    case B00111000:
      if(ram[adress]!=accumulator)
      {
        programCounter+=2;
      }
      else
      {
        programCounter++;
      }
      break;
    //JMP Jump to specified address
    case B01000000:
      programCounter=adress;
      break;
    //JMA Jump to the address stored at the specified address
    case B01001000:
      programCounter= ram[adress] & B00000111;
      break;
    //ADD Add value at address to register, result in register
    case B10000000:
      accumulator=(char)accumulator+(char)ram[adress];
      programCounter++;
      break;
    //SUB Subtract value at address from register, result in register
    case B10001000:
      accumulator=(char)accumulator-(char)ram[adress];
      programCounter++;
      break;
    //MUL Multiply value at address by register, result in register
    case B10010000:
      accumulator=(char)accumulator*(char)ram[adress];
      programCounter++;
      break;
    //DIV Divide register by value at address, result in register
    case B10011000:
      accumulator=(char)accumulator/(char)ram[adress];
      programCounter++;
      break;
    //DOUBLE Double the value of the register
    case B11110000:
    //SHL Shift register left by amount specified (as address)
    case B10100000:
      accumulator= accumulator<<adress;
      programCounter++;
      break;
    //HALF Half the value of register
    case B11111000:
    //SHR Shift register right by amount specified (as address)
    case B10101000:
      accumulator= accumulator>>adress;
      programCounter++;
      break;
    //ROL Rotate register left by amount specified (as address)
    case B10110000:
      cache=adress%8;
      accumulator= (accumulator<<cache) | (accumulator>>(8-cache));
      programCounter++;
      break;
    //ROR Rotate register right by amount specified (as address)
    case B10111000:
      cache=adress%8;
      accumulator= (accumulator>>cache) | (accumulator<<(8-cache));
      programCounter++;
      break;
    //11000 ADDU Same as ADD but unsigned (same as ADD, double encoded)
    case B11000000:
      accumulator+=ram[adress];
      programCounter++;
      break;
    //11001 SUBU Same as SUB but unsigned (same as SUB, double encoded)
    case B11001000:
      accumulator-=ram[adress];
      programCounter++;
      break;
    //MULU Same as MUL but unsigned
    case B11010000:
      accumulator*=ram[adress];
      programCounter++;
      break;
    //DIVU Same as DIV but unsigned
    case B11011000:
      accumulator/=ram[adress];
      programCounter++;
      break;
    //INC Increment register by one
    case B11100000:
      accumulator++;
      programCounter++;
      break;
    //DEC Decrement register by one
    case B11101000:
      accumulator--;
      programCounter++;
      break;
  }
  if(programCounter>7)
  {
    programCounter=0;
  }
  int temp=analogRead(POTI);
  if(temp<500)
    delayMicroseconds(map(temp,0,499,1,16000));
  else
    delay(map(temp,500,1023,16,250));
}

void C88PrintRam()
{
  for(byte i=0;i<8;i++)
  {
    setRow(i,ram[i]);
  }
}

void C88PrintAccumulator()
{
  for(byte i=0;i<8;i++)
  {
    setRow(i,accumulator);
  }
}

void C88PrintProgramCounter()
{
  for(byte i=0;i<8;i++)
  {
    setRow(i,programCounter);
  }
}

void C88ClearRam()
{
  for(byte i=0;i<8;i++)
  {
    ram[i]=0;
  }
}

void C88SetRam()
{
  ram[0]=B01000001;
  ram[1]=B00000001;
  ram[2]=B10110001;
  ram[3]=B00101000;
  ram[4]=B01000010;
  ram[5]=B10111001;
  ram[6]=B00110001;
  ram[7]=B01000101;
}

byte keyMatrix()
{
  byte temp=readKeyMatrix();
  if(temp>0 && temp<5)
    digitalWrite(ROW1,HIGH);
  else if(temp>4 && temp<9)
    digitalWrite(ROW2,HIGH);
  else if(temp>8 && temp<13)
    digitalWrite(ROW3,HIGH);
  else if(temp>12 && temp<17)
    digitalWrite(ROW4,HIGH);
  return temp;
}

byte readKeyMatrix()
{     
  digitalWrite(ROW1,LOW);
  if(!digitalRead(COL1))
    return 1;
  else if(!digitalRead(COL2))
    return 2;
  else if(!digitalRead(COL3))
    return 3;
  else if(!digitalRead(COL4))
    return 4;
  digitalWrite(ROW1,HIGH);
  digitalWrite(ROW2,LOW);
  if(!digitalRead(COL1))
    return 5;
  else if(!digitalRead(COL2))
    return 6;
  else if(!digitalRead(COL3))
    return 7;
  else if(!digitalRead(COL4))
    return 8;
  digitalWrite(ROW2,HIGH);
  digitalWrite(ROW3,LOW);
  if(!digitalRead(COL1))
    return 9;
  else if(!digitalRead(COL2))
    return 10;
  else if(!digitalRead(COL3))
    return 11;
  else if(!digitalRead(COL4))
    return 12;
  digitalWrite(ROW3,HIGH);
  digitalWrite(ROW4,LOW);
  if(!digitalRead(COL1))
    return 13;
  else if(!digitalRead(COL2))
    return 14;
  else if(!digitalRead(COL3))
    return 15;
  else if(!digitalRead(COL4))
    return 16;
  digitalWrite(ROW4,HIGH);
    return 0;
}

void initializeLedMatrix()
{
    DDRD = DDRD | (1<<SPI_MOSI) | (1<<SPI_CLK) | (1<<SPI_CS);
    PORTD = PORTD | (1<<SPI_CS);
    for(byte i=0;i<8;i++)
    { 
      status[i]=0x00;
    }
    spiTransfer(OP_DISPLAYTEST,0);
    spiTransfer(OP_DECODEMODE,0);
    setScanLimit(7);
    setIntensity(8);
    clearDisplay();
    shutdown(false);
}

void setRow(int row, byte value)
{
    status[row]=value;
    spiTransfer(row+1,status[row]);
}

void setScanLimit(int limit)
{
    if(limit>=0 && limit<8)
    {
      spiTransfer(OP_SCANLIMIT,limit);
    }
}

void setIntensity(int intensity) {
    if(intensity>=0 && intensity<16)
    {	
      spiTransfer(OP_INTENSITY,intensity);
    }
}

void clearDisplay()
{
    for(byte i=0;i<8;i++)
    {
        status[i]=0;
        spiTransfer(i+1,status[i]);
    }
}

void shutdown(bool b)
{
    if(b)
    {
      spiTransfer(OP_SHUTDOWN,0);
    }
    else
    {
      spiTransfer(OP_SHUTDOWN,1);
    }
}

void spiTransfer(volatile byte opcode, volatile byte data)
{
    for(int i=0;i<2;i++)
    {
      spidata[i]=(byte)0;
    }
    spidata[1]=opcode;
    spidata[0]=data;
    PORTD = PORTD & !(1<<SPI_CS);
    for(byte i=2;i>0;i--)
    {
      shiftOut(SPI_MOSI,SPI_CLK,MSBFIRST,spidata[i-1]);
    }
    PORTD = PORTD | (1<<SPI_CS);
}
