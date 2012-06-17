#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include "rf12.h"

#include <util/delay.h>

#ifndef cbi
#define cbi(sfr, bit)     (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit)     (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define RF_PORT		PORTB
#define RF_DDR		DDRB
#define RF_PIN		PINB

#define FSK   		4
#define SDO			3
#define SDI			2
#define SCK			1
#define CS			0

char airid[4];

#define noinline __attribute__((noinline))

unsigned short noinline rf12_trans(unsigned short wert)
{
	unsigned short werti = 0;
	unsigned char i;

	cbi(RF_PORT, CS);
	for (i = 0; i < 16; i++)
	{
		if (wert & 32768)
			sbi(RF_PORT, SDI);
		else
			cbi(RF_PORT, SDI);
		werti <<= 1;
		if (RF_PIN & (1 << SDO))
			werti |= 1;
		sbi(RF_PORT, SCK);
		wert <<= 1;
		_delay_us(0.3);
		cbi(RF_PORT, SCK);
	}
	sbi(RF_PORT, CS);
	return werti;
}

unsigned short noinline rf12_readytrans(unsigned short wert)
{
	rf12_ready();
	return rf12_trans(wert);
}

void rf12_preinit(const char *AirId)
{
  memcpy(airid, AirId, 4);
  RF_DDR = (1 << SDI) | (1 << SCK) | (1 << CS) | (1 << FSK);
  RF_PORT = (1 << CS);

  sbi(RF_PORT, FSK);

  for (unsigned int i = 0; i < 100; i++)
    _delay_ms(10); // wait until POR done
}

void rf12_init(void)
{
	rf12_trans(0xC0E0); // AVR CLK: 10MHz
	rf12_trans(0x80E7); // Enable FIFO
	rf12_trans(0xC2AB); // Data Filter: internal
	rf12_trans(0xCA81); // Set FIFO mode
	rf12_trans(0xE000); // disable wakeuptimer
	rf12_trans(0xC800); // disable low duty cycle
	rf12_trans(0xC4F7); // AFC settings: autotuning: -10kHz...+7,5kHz
}

void rf12_setbandwidth(unsigned char bandwidth, unsigned char gain, unsigned char drssi)
{
	rf12_trans(0x9400 | ((bandwidth & 7) << 5) | ((gain & 3) << 3)
			| (drssi & 7));
}

void rf12_setfreq(unsigned short freq)
{
	if (freq < 96) // 430,2400MHz
		freq = 96;
	else if (freq > 3903) // 439,7575MHz
		freq = 3903;
	rf12_trans(0xA000 | freq);
}

void rf12_setbaud(unsigned short baud)
{
//	if (baud < 663)
//		return;
//	if (baud < 5400) // Baudrate= 344827,58621/(R+1)/(1+CS*7)
		rf12_trans(0xC680 | ((43104 / baud) - 1));
//	else
//		rf12_trans(0xC600 | ((344828UL / baud) - 1));
}

void rf12_setpower(unsigned char power, unsigned char mod)
{
	rf12_trans(0x9800 | (power & 7) | ((mod & 15) << 4));
}

void noinline rf12_ready(void)
{
	//  cbi(RF_PORT, SDI);
	//  cbi(RF_PORT, CS);
	//  asm( "nop" );
	//  while (!(RF_PIN & (1 << SDO)))
	//    ; // wait until FIFO ready
	int timeout = 100;

	cbi(RF_PORT, SDI);
	cbi(RF_PORT, CS);
	asm( "nop" );
	asm( "nop" );
	while (!(RF_PIN & (1 << SDO)) && timeout)
	{
		timeout--;
		_delay_ms(1); // wait until FIFO ready
	}
	sbi(PORTB, CS);
	//        return 0;
	//     else
	//         return 1;
}

void rf12_txdataa(char *airid1, char *data, unsigned char number)
{
  rf12_txdata_start();
  rf12_txdata_send(airid1, 4);
  rf12_txdata_send(data, number);
  rf12_txdata_end();
}

void rf12_txdata(char *data, unsigned char number)
{
  rf12_txdata_start();
  rf12_txdata_send(airid, 4);
  rf12_txdata_send(data, number);
  rf12_txdata_end();
}

void rf12_txdata_start()
{
	rf12_trans(0x0000);
	rf12_readytrans(0x8238); // TX on
	rf12_readytrans(0x0000);

	for(int xc = 0;xc<5;xc++)
	{
		rf12_readytrans(0xB8AA);
	}
	rf12_readytrans(0xB82D);
	rf12_readytrans(0xB8D4);
	rf12_readytrans(0x0000);
}

void rf12_txdata_send(const char *data, unsigned char number)
{
	unsigned char i;
	for (i = 0; i < number; i++)
	{
		rf12_readytrans(0xB800 | (*data++));
	}
}

void rf12_txdata_end()
{
	rf12_readytrans(0x8208); // TX off
}



void rf12_rxdata(unsigned char *data, unsigned char number)
{
	unsigned char i;
	rf12_trans(0x82C8); // RX on
	rf12_trans(0xCA81); // set FIFO mode
	rf12_trans(0xCA83); // enable FIFO
	for (i = 0; i < number; i++)
	{
		*data++ = rf12_readytrans(0xB000);
	}
	rf12_trans(0x8208); // RX off
}

