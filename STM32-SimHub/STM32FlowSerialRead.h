
#ifndef __FLOW_SERIAL_READ_H__
#define __FLOW_SERIAL_READ_H__

//STM32 version compatible with STM32ArqSerial.h
#include "STM32ArqSerial.h" 
STM32ARQSerial arqserial;

// =======================================================
// SERIAL CORE WRAPPERS
// =======================================================
// NOTE: Only for compatibility with legacy code
#define FlowSerialBegin Serial.begin
#define FlowSerialFlush Serial.flush

// IMPORTANT: MUST be called every loop
#define FlowSerialUpdate() arqserial.update()

// =======================================================
// DATA ACCESS
// =======================================================
#define FlowSerialAvailable() arqserial.Available()
#define FlowSerialTimedRead() arqserial.read()
#define FlowSerialWrite(data) arqserial.write((uint8_t)(data))

// =======================================================
// STRING READ HELPERS
// =======================================================
inline String FlowSerialReadStringUntil(char terminator)
{
    String ret;
    int c = FlowSerialTimedRead();

    while (c >= 0 && c != terminator)
    {
        ret += (char)c;
        c = FlowSerialTimedRead();
    }

    return ret;
}

inline String FlowSerialReadStringUntil(char terminator1, char terminator2)
{
    String ret;
    int c = FlowSerialTimedRead();

    while (c >= 0 && c != terminator1 && c != terminator2)
    {
        ret += (char)c;
        c = FlowSerialTimedRead();
    }

    return ret;
}

inline void FlowSerialReadStringUntil(char buffer[], char terminator)
{
    int pos = 0;
    int c = FlowSerialTimedRead();

    while (c >= 0 && c != terminator)
    {
        buffer[pos++] = (char)c;
        c = FlowSerialTimedRead();
    }

    buffer[pos] = '\0';
}

// =======================================================
// PRINT FUNCTIONS
// =======================================================
inline void FlowSerialPrint(String& data) { arqserial.print(data.c_str()); }
inline void FlowSerialPrint(char data) { arqserial.print(data); }
inline void FlowSerialPrint(const char str[]) { arqserial.print(str); }

inline void FlowSerialPrintLn(String& data) { arqserial.print(data.c_str()); arqserial.print('\n'); }
inline void FlowSerialPrintLn(const char str[]) { arqserial.print(str); arqserial.print('\n'); }
inline void FlowSerialPrintLn() { arqserial.print('\n'); }

// Debug (kept for compatibility)
inline void FlowSerialDebugPrintLn(String& data) { arqserial.print(data.c_str()); arqserial.print('\n'); }
inline void FlowSerialDebugPrintLn(const char str[]) { arqserial.print(str); arqserial.print('\n'); }


// =======================================================
// BAUDRATE COMMAND HANDLER (UNCHANGED LOGIC)
// =======================================================
inline void SetBaudrate()
{
    int br = FlowSerialTimedRead();

    delay(200);

    switch (br)
    {
    case 1:  FlowSerialBegin(300); break;
    case 2:  FlowSerialBegin(1200); break;
    case 3:  FlowSerialBegin(2400); break;
    case 4:  FlowSerialBegin(4800); break;
    case 5:  FlowSerialBegin(9600); break;
    case 6:  FlowSerialBegin(14400); break;
    case 7:  FlowSerialBegin(19200); break;
    case 8:  FlowSerialBegin(28800); break;
    case 9:  FlowSerialBegin(38400); break;
    case 10: FlowSerialBegin(57600); break;
    case 11: FlowSerialBegin(115200); break;
    case 12: FlowSerialBegin(230400); break;
    case 13: FlowSerialBegin(250000); break;
    case 14: FlowSerialBegin(1000000); break;
    case 15: FlowSerialBegin(2000000); break;
    case 16: FlowSerialBegin(200000); break;
    case 17: FlowSerialBegin(500000); break;
    default: break;
    }
}

#endif