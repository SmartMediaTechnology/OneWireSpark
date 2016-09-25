/*

Particle Verison of OneWire Libary

Hotaman 2/1/2016
Bit and Byte write functions have been changed to only drive the bus high at the end of a byte when requested.
They no longer drive the bus for High bits when outputting to avoid a holy war.
Some folks just can't accept that a 10K resistor works just fine when the calculation calls for 10,042.769 ohms.
Bit and Byte writes are now 100% compliant with specs and app notes.

Support for P1 and Electron added by Hotaman 11/30/2015

Support for Photon added by Brendan Albano and cdrodriguez
- Brendan Albano 2015-06-10

I made monor tweeks to allow use in the web builder and created this repository for
use in the contributed libs list.

6/2014 - Hotaman

I've taken the code that Spark Forum user tidwelltimj posted
split it back into separte code and header files and put back in the
credits and comments and got it compiling on the command line within SparkCore core-firmware


Justin Maynard 2013

Original Comments follow

Copyright (c) 2007, Jim Studt  (original old version - many contributors since)

The latest version of this library may be found at:
  http://www.pjrc.com/teensy/td_libs_OneWire.html

OneWire has been maintained by Paul Stoffregen (paul@pjrc.com) since
January 2010.  At the time, it was in need of many bug fixes, but had
been abandoned the original author (Jim Studt).  None of the known
contributors were interested in maintaining OneWire.  Paul typically
works on OneWire every 6 to 12 months.  Patches usually wait that
long.  If anyone is interested in more actively maintaining OneWire,
please contact Paul.

Version 2.2:
  Teensy 3.0 compatibility, Paul Stoffregen, paul@pjrc.com
  Arduino Due compatibility, http://arduino.cc/forum/index.php?topic=141030
  Fix DS18B20 example negative temperature
  Fix DS18B20 example's low res modes, Ken Butcher
  Improve reset timing, Mark Tillotson
  Add const qualifiers, Bertrik Sikken
  Add initial value input to crc16, Bertrik Sikken
  Add target_search() function, Scott Roberts

Version 2.1:
  Arduino 1.0 compatibility, Paul Stoffregen
  Improve temperature example, Paul Stoffregen
  DS250x_PROM example, Guillermo Lovato
  PIC32 (chipKit) compatibility, Jason Dangel, dangel.jason AT gmail.com
  Improvements from Glenn Trewitt:
  - crc16() now works
  - check_crc16() does all of calculation/checking work.
  - Added read_bytes() and write_bytes(), to reduce tedious loops.
  - Added ds2408 example.
  Delete very old, out-of-date readme file (info is here)

Version 2.0: Modifications by Paul Stoffregen, January 2010:
http://www.pjrc.com/teensy/td_libs_OneWire.html
  Search fix from Robin James
    http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1238032295/27#27
  Use direct optimized I/O in all cases
  Disable interrupts during timing critical sections
    (this solves many random communication errors)
  Disable interrupts during read-modify-write I/O
  Reduce RAM consumption by eliminating unnecessary
    variables and trimming many to 8 bits
  Optimize both crc8 - table version moved to flash

Modified to work with larger numbers of devices - avoids loop.
Tested in Arduino 11 alpha with 12 sensors.
26 Sept 2008 -- Robin James
http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1238032295/27#27

Updated to work with arduino-0008 and to include skip() as of
2007/07/06. --RJL20

Modified to calculate the 8-bit CRC directly, avoiding the need for
the 256-byte lookup table to be loaded in RAM.  Tested in arduino-0010
-- Tom Pollard, Jan 23, 2008

Jim Studt's original library was modified by Josh Larios.

Tom Pollard, pollard@alum.mit.edu, contributed around May 20, 2008

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Much of the code was inspired by Derek Yerger's code, though I don't
think much of that remains.  In any event that was..
    (copyleft) 2006 by Derek Yerger - Free to distribute freely.

The CRC code was excerpted and inspired by the Dallas Semiconductor
sample code bearing this copyright.
//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//--------------------------------------------------------------------------
*/

#include "OneWire.h"
#include "application.h"

OneWire::OneWire(uint16_t pin)
{
    pinMode(pin, INPUT);
    _pin = pin;
}
// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return a 0;
//
// Returns 1 if a device asserted a presence pulse, 0 otherwise.
//
uint8_t OneWire::reset(void)
{
    uint8_t r;
    uint8_t retries = 125;

    noInterrupts();
    pinModeFastInput();
    interrupts();
    // wait until the wire is high... just in case
    do {
        if (--retries == 0) return 0;

        delayMicroseconds(2);
    } while ( !digitalReadFast());

    noInterrupts();

    digitalWriteFastLow();
    pinModeFastOutput();   // drive output low

    interrupts();
    delayMicroseconds(480);
    noInterrupts();

    pinModeFastInput();    // allow it to float

    delayMicroseconds(70);

    r =! digitalReadFast();

    interrupts();

    delayMicroseconds(410);

    return r;
}

void OneWire::write_bit(uint8_t v)
{
    if (v & 1) {
        noInterrupts();

        digitalWriteFastLow();
        pinModeFastOutput();   // drive output low

        delayMicroseconds(10);

        pinModeFastInput();    // float high

        interrupts();

        delayMicroseconds(55);
    } else {
        noInterrupts();

        digitalWriteFastLow();
        pinModeFastOutput();   // drive output low

        delayMicroseconds(65);

        pinModeFastInput();    // float high

        interrupts();

        delayMicroseconds(5);
    }
}

//
// Read a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
uint8_t OneWire::read_bit(void)
{
    uint8_t r;

    noInterrupts();

    digitalWriteFastLow();
    pinModeFastOutput();

    delayMicroseconds(3);

    pinModeFastInput();    // let pin float, pull up will raise

    delayMicroseconds(10);

    r = digitalReadFast();

    interrupts();
    delayMicroseconds(53);

    return r;
}

//
// Write a byte. The writing code uses the active drivers to raise the
// pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set 'power' to 1, otherwise the pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.
//
void OneWire::write(uint8_t v, uint8_t power /* = 0 */)
{
    uint8_t bitMask;

    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
        OneWire::write_bit( (bitMask & v)?1:0);
    }

    if ( power) {
        noInterrupts();

        digitalWriteFastHigh();
        pinModeFastOutput();        // Drive pin High when power is True

        interrupts();
    }
}

void OneWire::write_bytes(const uint8_t *buf, uint16_t count, bool power /* = 0 */)
{
    for (uint16_t i = 0 ; i < count ; i++)
        write(buf[i]);

    if (power) {
        noInterrupts();

        digitalWriteFastHigh();
        pinModeFastOutput();        // Drive pin High when power is True

        interrupts();
    }
}

//
// Read a byte
//
uint8_t OneWire::read()
{
    uint8_t bitMask;
    uint8_t r = 0;

    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
        if ( OneWire::read_bit()) r |= bitMask;
    }

    return r;
}

void OneWire::read_bytes(uint8_t *buf, uint16_t count)
{
    for (uint16_t i = 0 ; i < count ; i++)
        buf[i] = read();
}

//
// Do a ROM select
//
void OneWire::select(const uint8_t rom[8])
{
    uint8_t i;

    write(0x55);           // Choose ROM
    for (i = 0; i < 8; i++) {
        write(rom[i]);
        selectedRom[i] = rom[i];
    }
}

//
// Do a ROM skip
//
void OneWire::skip()
{
    write(0xCC);           // Skip ROM
}

void OneWire::depower()
{
    noInterrupts();

    pinModeFastInput();

    interrupts();
}

#if ONEWIRE_SEARCH

//
// You need to use this function to start a search again from the beginning.
// You do not need to do it for the first search, though you could.
//
void OneWire::reset_search()
{
    // reset the search state
    LastDiscrepancy = 0;
    LastDeviceFlag = FALSE;
    LastFamilyDiscrepancy = 0;

    for(int i = 7; ; i--) {
        ROM_NO[i] = 0;
        if ( i == 0) break;
    }
}

// Setup the search to find the device type 'family_code' on the next call
// to search(*newAddr) if it is present.
//
void OneWire::target_search(uint8_t family_code)
{
   // set the search state to find SearchFamily type devices

   ROM_NO[0] = family_code;

   for (uint8_t i = 1; i < 8; i++)
      ROM_NO[i] = 0;

   LastDiscrepancy = 64;
   LastFamilyDiscrepancy = 0;
   LastDeviceFlag = FALSE;
}

//
// Perform a search. If this function returns a '1' then it has
// enumerated the next device and you may retrieve the ROM from the
// OneWire::address variable. If there are no devices, no further
// devices, or something horrible happens in the middle of the
// enumeration then a 0 is returned.  If a new device is found then
// its address is copied to newAddr.  Use OneWire::reset_search() to
// start over.
//
// --- Replaced by the one from the Dallas Semiconductor web site ---
//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
uint8_t OneWire::search(uint8_t *newAddr)
{
    uint8_t id_bit_number;
    uint8_t last_zero, rom_byte_number, search_result;
    uint8_t id_bit, cmp_id_bit;

    unsigned char rom_byte_mask, search_direction;

    // initialize for search
    id_bit_number = 1;
    last_zero = 0;
    rom_byte_number = 0;
    rom_byte_mask = 1;
    search_result = 0;

    // if the last call was not the last one
    if (!LastDeviceFlag)
    {
        // 1-Wire reset
        if (!reset()){
            // reset the search
            LastDiscrepancy = 0;
            LastDeviceFlag = FALSE;
            LastFamilyDiscrepancy = 0;

            return FALSE;
        }

        // issue the search command
        write(0xF0);

        // loop to do the search
        do
        {
            // read a bit and its complement
            id_bit = read_bit();
            cmp_id_bit = read_bit();

            // check for no devices on 1-wire
            if ((id_bit == 1) && (cmp_id_bit == 1)){
                break;
            }
            else
            {
                // all devices coupled have 0 or 1
                if (id_bit != cmp_id_bit){
                    search_direction = id_bit;  // bit write value for search
                }
                else{
                    // if this discrepancy if before the Last Discrepancy
                    // on a previous next then pick the same as last time
                    if (id_bit_number < LastDiscrepancy)
                        search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
                    else
                        // if equal to last pick 1, if not then pick 0
                        search_direction = (id_bit_number == LastDiscrepancy);

                    // if 0 was picked then record its position in LastZero
                    if (search_direction == 0){
                        last_zero = id_bit_number;

                        // check for Last discrepancy in family
                        if (last_zero < 9)
                            LastFamilyDiscrepancy = last_zero;
                    }
                }

                // set or clear the bit in the ROM byte rom_byte_number
                // with mask rom_byte_mask
                if (search_direction == 1)
                  ROM_NO[rom_byte_number] |= rom_byte_mask;
                else
                  ROM_NO[rom_byte_number] &= ~rom_byte_mask;

                // serial number search direction write bit
                write_bit(search_direction);

                // increment the byte counter id_bit_number
                // and shift the mask rom_byte_mask
                id_bit_number++;
                rom_byte_mask <<= 1;

                // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
                if (rom_byte_mask == 0)
                {
                    rom_byte_number++;
                    rom_byte_mask = 1;
                }
            }
        }while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

        // if the search was successful then
        if (!(id_bit_number < 65))
        {
            // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
            LastDiscrepancy = last_zero;

            // check for last device
            if (LastDiscrepancy == 0)
                LastDeviceFlag = TRUE;

            search_result = TRUE;
        }
    }

    // if no device found then reset counters so next 'search' will be like a first
    if (!search_result || !ROM_NO[0]){
        LastDiscrepancy = 0;
        LastDeviceFlag = FALSE;
        LastFamilyDiscrepancy = 0;
        search_result = FALSE;
    }

    for (int i = 0; i < 8; i++) newAddr[i] = ROM_NO[i];

    return search_result;
}

#endif

#if ONEWIRE_CRC
// The 1-Wire CRC scheme is described in Maxim Application Note 27:
// "Understanding and Using Cyclic Redundancy Checks with Maxim iButton Products"
//


//
// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but much smaller, than the lookup table.
//
uint8_t OneWire::crc8( uint8_t *addr, uint8_t len)
{
    uint8_t crc = 0;

    while (len--) {
        uint8_t inbyte = *addr++;
        for (uint8_t i = 8; i; i--) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
                inbyte >>= 1;
        }
    }

    return crc;
}
#endif

#if ONEWIRE_CRC16
bool OneWire::check_crc16(const uint8_t* input, uint16_t len, const uint8_t* inverted_crc, uint16_t crc)
{
    crc = ~crc16(input, len, crc);

    return (crc & 0xFF) == inverted_crc[0] && (crc >> 8) == inverted_crc[1];
}

uint16_t OneWire::crc16(const uint8_t* input, uint16_t len, uint16_t crc)
{
    static const uint8_t oddparity[16] =
        { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

    for (uint16_t i = 0 ; i < len ; i++) {
        // Even though we're just copying a byte from the input,
        // we'll be doing 16-bit computation with it.
        uint16_t cdata = input[i];
        cdata = (cdata ^ crc) & 0xff;
        crc >>= 8;

        if (oddparity[cdata & 0x0F] ^ oddparity[cdata >> 4])
            crc ^= 0xC001;

        cdata <<= 6;
        crc ^= cdata;
        cdata <<= 1;
        crc ^= cdata;
    }

    return crc;
}

#endif


byte OneWire::getType(uint8_t *addr) {
  switch (addr[0]) {
    case 0x10:
      // Serial.println("  Chip = DS1820/DS18S20");
      return DS1820;
      break;
    case 0x28:
      // Serial.println("  Chip = DS18B20");
      return DS1822;
      break;
    case 0x22:
      // Serial.println("  Chip = DS1822");
      return DS1822;
      break;
    case 0x26:
      // Serial.println("  Chip = DS2438");
      return DS2438;
      break;
    default:
      Serial.printlnf("No or unknown temperature chip: 0x%02X", addr[0]);
      printRomID(addr);
      return 0;
  }
};

bool OneWire::isTemp(uint8_t *addr)
{
  byte type = OneWire::getType(addr);
  return (type == DS1820 || type == DS1822 || type == DS2438);
};

String OneWire::printRomID()
{
  return OneWire::printRomID(selectedRom);
};

String OneWire::printRomID(uint8_t *addr)
{
  String str = "";
  for (uint8_t i=0; i<8; i++) {
    if (i > 0) str.concat(":");
    str.concat(String::format("%02X", addr[i]));
  }
  return str;
}

float OneWire::readTemperature(uint8_t *addr)
{
  return OneWire::readTemperature(addr, 5);
}

float OneWire::readTemperature(uint8_t *addr, uint8_t tries)
{
  byte data[12];
  float celsius = -9999;
  // first make sure current values are in the scratch pad
  OneWire::reset();
  OneWire::select(addr);
  OneWire::write(0xB8,0);         // Recall Memory 0
  OneWire::write(0x00,0);         // Recall Memory 0

  // now read the scratch pad
  OneWire::reset();
  OneWire::select(addr);
  OneWire::write(0xBE,0);         // Read Scratchpad
  //if (OneWireDevice->getType() == DS2438) {
  //  OneWire::write(0x00,0);       // The DS2438 needs a page# to read
  //}

  // transfer and print the values
  // Serial.print("  Data = ");
  // Serial.print(present, HEX);
  // Serial.print(" ");
  for ( uint8_t i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = OneWire::read();
    // Serial.print(data[i], HEX);
    // Serial.print(" ");
  }

  if (OneWire::crc8(data, 8) != data[8])
  {
    tries--;
    Serial.printf("Invalid CRC8: %02X != %02X, tries left: %d", OneWire::crc8(data, 8), data[8], tries);
    if (tries > 0)
    {
      Serial.println(",.. retrying");
      return OneWire::readTemperature(addr, tries);
    } else {
        Serial.println(",.. no more tries FAILED");
    }
  } else {
    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (OneWire::getType(addr) == DS2438) raw = (data[2] << 8) | data[1];
    byte cfg = (data[4] & 0x60);

    switch (OneWire::getType(addr)) {
      case DS1820:
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10) {
          // "count remain" gives full 12 bit resolution
          raw = (raw & 0xFFF0) + 12 - data[6];
        }
        celsius = (float)raw * 0.0625;
        break;
      case DS1822:
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
        if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
        // default is 12 bit resolution, 750 ms conversion time
        celsius = (float)raw * 0.0625;
        break;

      case DS2438:
        data[1] = (data[1] >> 3) & 0x1f;
        if (data[2] > 127) {
          celsius = (float)data[2] - ((float)data[1] * .03125);
        }else{
          celsius = (float)data[2] + ((float)data[1] * .03125);
        }
    }

    OneWire::write(0xB8,0);         // Recall Memory 0
    OneWire::write(0x00,0);         // Recall Memory 0
  }
  return celsius;
}
