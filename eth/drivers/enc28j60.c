/***************************************************************************
 *    www.braintechnology.de                                               *
 *    Advairtizer V1.0                                                     *
 *                                                                         *
 *    (c) Copyright Braintechnology 2007                                   *
 *    File name   : sw_main.c                                              *
 *    Description : Demo Software for Advairtizer Development Modules      *
 *    History     :                                                        *
 *    Date        : Oktober, 2007                                          *
 *    Author      : Kai Klein                                              *
 *    $Revision: 1.0 $                                                     *
 ***************************************************************************/
#include "enc28j60.h"

#include "spi.h"

#include <lpc2000/io.h>
#include <lpc2000/lpc214x.h>

#include "log.h"

#define debug_printf(...)

uint8_t now = 0;
static volatile uint32_t g_dummy;

bus_ptr_t iodir_ptr;
bus_ptr_t iopin_ptr;

#define IODIR (*iodir_ptr)
#define IOPIN (*iopin_ptr)


//#define CS_ETHERNET         BIT13
#define SPI0_IOSET_MASK 0x00001500 // SCK,MISO and MOSI for SPI0 usage

//#define TRACE DBG
//#define TRACE(X, REG...) DEBUG_PRINT_HIGH(X)
#define TRACE(X, REG...)
//#define TRACE(X,Y) {TRACE(X); TRACE(Y);}
/*
 * Copyright (c) 2009, Manish Shakya,Real Time Solutions Pvt. Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 */
/**

Based upon

Advairtizer V1.0
www.braintechnology.de
*/

//
//#include <global.h>
//#include <uip/uip.h>
//#include <uip/uip_arp.h>
//#include <uip/timer.h>

//#include "peripherals.h"
//#include <stdio.h>

//#include "irq.h"

uint8_t Enc28j60Bank;
uint16_t NextPacketPtr;

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])
#define CS_ETHERNET	    13
#define RESET_ETHERNET	    12
#define INTR_ETHERNET	    14

#define ESTAT_UNIMP         (1<<3)

#define ETH_CS_INIT()		        \
    {					\
	IODIR |= (1 << CS_ETHERNET);	\
    }

// Like ENC28J60_Deselect()
#define ETH_CS_HIGH()		            \
    {					    \
     IOPIN |= (1 << CS_ETHERNET);	    \
	{				    \
	    unsigned i;			    \
	    for(i = 0; i < 100; i++);	    \
	}				    \
    }

// Like ENC28J60_Select()
#define ETH_CS_LOW()		\
    {					\
	IOPIN &= ~(1 << CS_ETHERNET);	\
	{				\
	    unsigned i;			\
	    for(i=0; i<100; i++);	\
	}				\
    }

#define ETH_RESET_INIT()		\
    {					\
	IODIR |= ( 1 << RESET_ETHERNET);	\
    }


#define ETH_RESET_HIGH()			\
    {						\
	IOPIN |= (1 << RESET_ETHERNET);        \
	{					\
	    unsigned i;				\
	    for(i = 0; i < 100; i++);		\
	}					\
    }


#define ETH_RESET_LOW()		    \
    {					    \
	IOPIN &= ~(1 << RESET_ETHERNET);   \
	{				    \
	    unsigned i;			    \
	    for(i = 0; i < 100; i++);	    \
	}				    \
    }


#define ETH_INT_INIT()		    \
    {					    \
	IODIR &= ~(1 << INTR_ETHERNET);   \
    }


#define ETH_INT_IN(X)		    \
    {					    \
	X = IOPIN;			    \
    }


#define ETH_SPI_INIT()		 initSPI()
#define ETH_SPI(charVar)  spiPut((charVar))

#define MS2MICROSEC	1000

uint8_t  enc28j60_read_op(uint8_t op, uint8_t address);
void enc28j60_write_op(uint8_t op, uint8_t address, uint8_t data);
uint8_t enc28j60_read(uint8_t address);
void enc28j60_set_bank(uint8_t address);
void enc28j60_write(uint8_t address, uint8_t data);
void enc28j60_set_mac_address(uint8_t *macaddr);
void enc28j60_phy_write(uint8_t address, uint16_t data);
uint16_t enc28j60_phy_read(uint8_t address);


void enc28j60_write16 (uint8_t address, uint16_t data)
{
    enc28j60_write(address, (uint8_t) (data & 0xff));
    enc28j60_write(address+1, (uint8_t) (data >> 8));
}

uint16_t enc28j60_read16(uint8_t address)
{
    uint16_t res;
    res = enc28j60_read(address) << 8;
    res |= enc28j60_read(address+1) & 0xff;
    return res;
}

void enc28j60_init(bus_ptr_t iodir, bus_ptr_t iopin, uint8_t macaddr[])
{
    iodir_ptr = iodir;
    iopin_ptr = iopin;
    
    uint32_t waits; //, timeout;
    unsigned char i;

    // Init busywait mechanism
    //busywaitInit();

    IODIR |= (1 << CS_ETHERNET);
    TRACE("Chip select pin dir init\n");

    IODIR |= ( 1 << RESET_ETHERNET);
    TRACE("Reset pin dir init\n");

    //  Set up the reset line.  This isn't done in the SPI code, because it
    //  doesn't need to be aware of the ENC28J60 reset.  Do it first so we
    //  can hold the part in reset before SPI initialization.
    //
    ETH_RESET_LOW();
    TRACE("RESET_LOW\n");

    IODIR &= ~(1 << INTR_ETHERNET);
    TRACE("Interrupt pin dir init\n");

    ETH_SPI_INIT();
    TRACE("SPI init\n");
    //fflush(stdout);
    //
    //  Now hold part in reset for 100ms
    //

//    TRACE("Holding in reset for ~100ms");
//    ETH_CS_HIGH();
//    TRACE("CS HIGH\n");

//  ETH_RESET_LOW();
//    TRACE("RESET_LOW");

//    busywait(100 * MS2MICROSEC);
//    ETH_RESET_HIGH();
//    busywait(100 * MS2MICROSEC);
//    fflush(stdout);
//    TRACE("POR (Power-On Reset) Delay...");
//    fflush(stdout);

    ETH_RESET_HIGH();
    ETH_CS_HIGH();
    //
    //  Give the part 1 second for the PHY to become ready (CLKRDY == 1).  If it
    //  doesn't, return an error to the user. 
    //
    //  Note that we also check that bit 3 is 0.  The data sheet says this is
    //  unimplemented and will return 0.  We use this as a sanity check for the
    //  ENC28J60 actually being present, because the MISO line typically floats
    //  high.  If we only checked the CLKRDY, it will likely return 1 for when no
    //  ENC28J60 is present.
    //
    waits = 0;
    while ( ((enc28j60_read(ESTAT) & (ESTAT_UNIMP | ESTAT_CLKRDY)) != ESTAT_CLKRDY) )
    {
	if(waits >= 10)
	{
	    TRACE("PHY ERROR !");
	    return;
	}
	waits++;
	BusyWait(500);
	TRACE("reading ESTAT...");
    }
    waits = 0;
    //fflush(stdout);
    TRACE("Ended\n");
    //fflush(stdout);

    // Soft reset
    //ETH_CS_LOW();
    //ETH_SPI(0xff);
    //ETH_CS_HIGH();

	BusyWait(20);

    // Perform system reset
    enc28j60_write_op(ENC28J60_BIT_FIELD_CLR, ECON2, ECON2_PWRSV);//wake up
    //enc28j60_write_op(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);

    // According to Errata workaround #2, CLKRDY check is unreliable, delay 20ms instead
    BusyWait(20);
    TRACE("System Soft Reset\n");
#if 0
    for(i = 0; i < 10; i++)
    {
	TRACE("1 sec...");
        busywait(1000 * MS2MICROSEC);
	TRACE("ended");
    }
#endif
    // Do Bank 0 stuff
    TRACE("Initializing Bank 0\n");

    // initialize receive buffer
    // 16-bit transfers, must write low byte first

    // Set receive buffer start address
    NextPacketPtr = RXSTART_INIT;
    enc28j60_write16(ERXSTL, RXSTART_INIT);

    // Set receive pointer address
    enc28j60_write16(ERXRDPTL, RXSTART_INIT);

    //	Set receive buffer end
    enc28j60_write16(ERXNDL, RXSTOP_INIT);

    // Set transmit buffer start
    enc28j60_write16(ETXSTL, TXSTART_INIT);

    // Do Bank 2 stuff
    TRACE("Initializing Bank 2\n");

    // Bring MAC out of reset
    enc28j60_write(MACON2, 0x00);

    // Enable MAC receive, TX Pause, RX Pause, Rx (Full Duplex)
    enc28j60_write(MACON1,
	    MACON1_MARXEN | MACON1_TXPAUS | MACON1_RXPAUS);

    // Enable automatic padding and CRC operations
    enc28j60_write_op(ENC28J60_BIT_FIELD_SET,
	    MACON3,
	    MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN);

    // Set full-duplex mode
    enc28j60_write_op(ENC28J60_BIT_FIELD_SET,
	    MACON3,
	    MACON3_FULDPX);

    // Set inter-packet gap to 9.6us
    enc28j60_write16(MAIPGL, 0x012);

    // Set inter-frame gap (back-to-back)
    enc28j60_write(MABBIPG, 0x15);

    // When the medium is occupied, the MAC will wait indefinitely
    // for it to become free when attempting to transmit
    // (use this setting for IEEE 802.3? compliance)
#define MACON4_DEFER BIT6
    enc28j60_write(MACON4, MACON4_DEFER);

    // Late collisions occur beyond 63+8 bytes (8 bytes for preamble/start of frame delimiter)
    // 55 is all that is needed for IEEE 802.3, but ENC28J60 B5 errata for improper link pulse 
    // collisions will occur less often with a larger number.
    //enc28j60_write((BYTE)MACLCON2, 63);

    // Set the maximum packet size which the controller will accept
    enc28j60_write16(MAMXFLL, MAX_FRAMELEN);

    // Do Bank 3 stuff
    TRACE("Initializing Bank 3\n");

    // Set Mac Addr, MAC address in ENC28J60 is byte-backward
    {
		//uint8_t mymac[6] = MAC_ADDRESS;
		enc28j60_set_mac_address(macaddr);
    }

    // Disable the CLKOUT output to reduce EMI generation
    //enc28j60_write(ECOCON, 0x00);	// Output off (0V)

    // Set full duplex mode (must be the same as MACON3.FULDPX)
    enc28j60_phy_write(PHCON1, PHCON1_PDPXMD);

    // No loopback of transmitted frames
    //enc28j60_phy_write(PHCON2, PHCON2_HDLDIS);

    // Read a PHY register
    TRACE("PHHID1==%x\n", enc28j60_phy_read(PHHID1));

#if 1
    // Flash leds
//    int i;
	TRACE("FLASHING LEDS\n");
    for(i = 0; i < 1; i++)
    {
	// OFF
	enc28j60_phy_write(PHLCON, 0x3990);
	BusyWait(500);
	TRACE("FLASHING LEDS\n");

	// ON
	enc28j60_phy_write(PHLCON, 0x3880);
	BusyWait(500);
	TRACE("FLASHING LEDS\n");
    }
    enc28j60_phy_write(PHLCON, 0x472);//Display link status and transmit/receive activity (always stretched)
#endif

    // Enable interrutps
    //   EIE: ETHERNET INTERRUPT ENABLE REGISTER
    //	    PKTIE: Receive Packet Pending Interrupt Enable bit
    //	    INTIE: Global INT Interrupt Enable bit
    enc28j60_write_op(ENC28J60_BIT_FIELD_SET,
	    EIE,
	    EIE_INTIE | EIE_PKTIE);

    // Enable packet reception
    enc28j60_write_op(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);

    // Enternet receive filter set
    //	CRCEN: Post-Filter CRC Check Enable bit
    //  ANDOR: AND/OR Filter Select bit
    enc28j60_write(ERXFCON,0);//(CRCEN|ANDOR)); // Bank 1
}

uint8_t  enc28j60_read_op(uint8_t op, uint8_t address)
{

    uint8_t data;
    /* assert CS*/
    ETH_CS_LOW();

    ETH_SPI( op | (address & ADDR_MASK));
    data = ETH_SPI(0);
    if(address & 0x80)
    {
	data=ETH_SPI(0);
    }

    ETH_CS_HIGH();
    return data;
}

void enc28j60_write_op(uint8_t op, uint8_t address, uint8_t data)
{
    /* assert CS*/
    ETH_CS_LOW();

    // issue write command
    ETH_SPI( op | (address & ADDR_MASK));

    // write data
    ETH_SPI(data);

    // release CS
    ETH_CS_HIGH();//CS auf High
}

void enc28j60_set_bank(uint8_t address)
{
    // set the bank (if needed)
    if((address & BANK_MASK) != Enc28j60Bank)
    {
	// set the bank
	enc28j60_write_op(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
	enc28j60_write_op(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
	Enc28j60Bank = (address & BANK_MASK);
    }
}

uint8_t enc28j60_read(uint8_t address)
{
    // set the bank
    enc28j60_set_bank(address);
    // do the read
    return enc28j60_read_op(ENC28J60_READ_CTRL_REG, address);
}

void enc28j60_write(uint8_t address, uint8_t data)
{
    // set the bank
    enc28j60_set_bank(address);
    // do the write
    enc28j60_write_op(ENC28J60_WRITE_CTRL_REG, address, data);
}

void enc28j60_set_mac_address(uint8_t *macaddr)
{
    // write MAC address
    // NOTE: MAC address in ENC28J60 is byte-backward
    enc28j60_write(MAADR5, *macaddr++);
    enc28j60_write(MAADR4, *macaddr++);
    enc28j60_write(MAADR3, *macaddr++);
    enc28j60_write(MAADR2, *macaddr++);
    enc28j60_write(MAADR1, *macaddr++);
    enc28j60_write(MAADR0, *macaddr++);
}


void enc28j60_phy_write(uint8_t address, uint16_t data)
{
    uint8_t r;
    // set the PHY register address
    enc28j60_write(MIREGADR, address);

    // write the PHY data
    enc28j60_write16(MIWRL, data);
    //enc28j60_write(MIWRH, data>>8);
    TRACE("wait phy write... ");
    // wait until the PHY write completes
    r = enc28j60_read(MISTAT);
    //TRACE("r=%x", r);
    while(r & MISTAT_BUSY);
    TRACE("complete\n");	
}

uint16_t enc28j60_phy_read(uint8_t address) {

    /* write address to MIREGADR */
    enc28j60_write(MIREGADR, address);

    /* set MICMD.MIIRD */
    enc28j60_write(MICMD, MICMD_MIIRD);

    /* poll MISTAT.BUSY bit until operation is complete */
    while(enc28j60_read(MISTAT) & MISTAT_BUSY);

    /* clear MICMD.MIIRD */
    //enc28j60_write(MICMD, 0);
    enc28j60_write_op(ENC28J60_BIT_FIELD_CLR, MICMD, MICMD_MIIRD);
    return enc28j60_read16(MIRDH);
}


void enc28j60_get_mac_address(uint8_t *macaddr)
{
    // read MAC address registers
    // NOTE: MAC address in ENC28J60 is byte-backward
    *macaddr++ = enc28j60_read(MAADR5);
    *macaddr++ = enc28j60_read(MAADR4);
    *macaddr++ = enc28j60_read(MAADR3);
    *macaddr++ = enc28j60_read(MAADR2);
    *macaddr++ = enc28j60_read(MAADR1);
    *macaddr++ = enc28j60_read(MAADR0);
}

void enc28j60_write_buffer(uint32_t len, uint8_t * data)
{
    // assert CS
    ETH_CS_LOW(); //CS auf Low

    //   	pause(ONE_US*100);
    //	// issue write command
    ETH_SPI(ENC28J60_WRITE_BUF_MEM);
    while(len--)
    {
	// write data
	ETH_SPI(*data++);
    }
    // release CS
    ETH_CS_HIGH();
}

void enc28j60_read_buffer(uint32_t len, uint8_t *data)
{
    // assert CS
    ETH_CS_LOW(); //CS auf Low

    // pause(ONE_US*100);
    // issue read command
    ETH_SPI(ENC28J60_READ_BUF_MEM);

    while(len--)
    {
	// read data
	*data++ =ETH_SPI(0);
    }

    // release CS
    ETH_CS_HIGH();

}


void enc28j60_packet_send(uint32_t len, uint8_t *packet)
{
    // Errata sheet
    enc28j60_write_op(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
    enc28j60_write_op(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);
    enc28j60_write_op(ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXERIF | EIR_TXIF);
    //BFCReg(EIR, EIR_TXERIF | EIR_TXIF);

    // Set the write pointer to start of transmit buffer area
    enc28j60_write16(EWRPTL, (unsigned char  )TXSTART_INIT);
    //enc28j60_write(EWRPTH, TXSTART_INIT>>8);

    // Set the TXND pointer to correspond to the packet size given
    enc28j60_write(ETXNDL, (TXSTART_INIT+len));
    enc28j60_write(ETXNDH, (TXSTART_INIT+len)>>8);

    // write per-packet control byte
    enc28j60_write_op(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

    // copy the packet into the transmit buffer
    enc28j60_write_buffer(len, packet);

    // send the contents of the transmit buffer onto the network
    enc28j60_write_op(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

unsigned int enc28j60_packet_receive(uint32_t maxlen, uint8_t *packet)
{
    uint16_t rxstat;
    uint16_t len;
    
    uint8_t packets_count = enc28j60_read(EPKTCNT);
    if (packets_count == 0)
        return 0;
    
    // Set the read pointer to the start of the received packet
    enc28j60_write16(ERDPTL, NextPacketPtr);
    //enc28j60_write(ERDPTH, (NextPacketPtr)>>8);

    // read the next packet pointer
    NextPacketPtr  = enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0);
    NextPacketPtr |= enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0)<<8;

    // read the packet length
    len  = enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0);
    len |= enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0)<<8;

    // read the receive status
    rxstat  = enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0);
    rxstat |= enc28j60_read_op(ENC28J60_READ_BUF_MEM, 0)<<8;

    if ((len - 4) > maxlen) {
        LOG_WARNING("Received frame larger than the given buffer: %d", (int)(len - 4));
    }
    
    // limit retrieve length
    // (we reduce the MAC-reported length by 4 to remove the CRC)
    len = IFMIN(len - 4, maxlen);

    // copy the packet from the receive buffer
    enc28j60_read_buffer(len, packet);

    // Move the RX read pointer to the start of the next received packet
    // This frees the memory we just read out
    enc28j60_write16(ERXRDPTL, (NextPacketPtr));
    //enc28j60_write(ERXRDPTH, (NextPacketPtr)>>8);

    // Errata workaround #13. Make sure ERXRDPT is odd
//     {
// 	uint16_t rs,re;
// 	rs = enc28j60_read16(ERXSTL);
// 	re = enc28j60_read(ERXNDL);
// 
// 	if (NextPacketPtr - 1 < rs || NextPacketPtr - 1 > re) {
// 	    enc28j60_write16(ERXRDPTL, (re));
// 	}
// 	else {
// 	    enc28j60_write16(ERXRDPTL, (NextPacketPtr-1));
// 	}
//     }

    // decrement the packet counter indicate we are done with this packet
    // clear the PKTIF: Receive Packet Pending Interrupt Flag bit
    enc28j60_write_op(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);

    return len;
}
