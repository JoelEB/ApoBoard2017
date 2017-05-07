

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "Arduino.h"
#include "IRSerial-2014.h"

#define bittime 32
const uint8_t TX_zerotime = bittime >> 1;
const uint8_t TX_onetime = bittime * 3;
const uint8_t TX_gaptime = bittime * 1;
const uint8_t TX_starttime = bittime * 4;



//
// Statics
//
IRSerial *IRSerial::active_object = 0;
char IRSerial::_receive_buffer[_SS_MAX_RX_BUFF];
volatile uint8_t IRSerial::_receive_buffer_tail = 0;
volatile uint8_t IRSerial::_receive_buffer_head = 0;
char IRSerial::_transmit_buffer[_SS_MAX_RX_BUFF];
volatile uint8_t IRSerial::_transmit_buffer_tail = 0;
volatile uint8_t IRSerial::_transmit_buffer_head = 0;

//
// Debugging
//
// This function generates a brief pulse
// for debugging or measuring on an oscilloscope.
inline void DebugPulse(uint8_t pin, uint8_t count)
{
#if _DEBUG
  volatile uint8_t *pport = portOutputRegister(digitalPinToPort(pin));

  uint8_t val = *pport;
  while (count--)
  {
    *pport = val | digitalPinToBitMask(pin);
    *pport = val;
  }
#endif
}

//
// Private methods
//

/* static */
inline void IRSerial::tunedDelay(uint16_t delay) {
  uint8_t tmp = 0;

  asm volatile("sbiw    %0, 0x01 \n\t"
               "ldi %1, 0xFF \n\t"
               "cpi %A0, 0xFF \n\t"
               "cpc %B0, %1 \n\t"
               "brne .-10 \n\t"
               : "+r" (delay), "+a" (tmp)
               : "0" (delay)
              );
}

// This function sets the current object as the "listening"
// one and returns true if it replaces another
bool IRSerial::listen()
{
  if (active_object != this)
  {
    _RX_buffer_overflow = false;
    _TX_buffer_overflow = false;
    uint8_t oldSREG = SREG;
    cli();
    _receive_buffer_head = _receive_buffer_tail = 0;
    _transmit_buffer_head = _transmit_buffer_tail = 0;
    active_object = this;
    SREG = oldSREG;
    return true;
  }

  return false;
}

//
// The receive routine called by the interrupt handler
//
void IRSerial::recv()
{

#if GCC_VERSION < 40302
  // Work-around for avr-gcc 4.3.0 OSX version bug
  // Preserve the registers that the compiler misses
  // (courtesy of Arduino forum user *etracer*)
  asm volatile(
    "push r18 \n\t"
    "push r19 \n\t"
    "push r20 \n\t"
    "push r21 \n\t"
    "push r22 \n\t"
    "push r23 \n\t"
    "push r26 \n\t"
    "push r27 \n\t"
    ::);
#endif

  uint8_t d = 0;

  // If RX line is high, then we don't see any start bit
  // so interrupt is probably not for us
  uint8_t rxPin = rx_pin_read();

  if (_inverse_logic_rx ? rxPin : !rxPin)
  {
    //DBUG Serial.write('.');
    // Wait approximately 1/2 of a bit width to "center" the sample
    tunedDelay(_rx_delay_centering);

    // Read each of the 8 bits
    for (uint8_t i = 0x1; i; i <<= 1)
    {
      tunedDelay(_rx_delay_intrabit);  //Totally blocks Timer0 interupt!
      //DebugPulse(_DEBUG_PIN2, 1);
      uint8_t noti = ~i;
      if (rx_pin_read())
        d |= i;
      else // else clause added to ensure function timing is ~balanced
        d &= noti;
    }

    // skip the stop bit
    tunedDelay(_rx_delay_stopbit);
    //DebugPulse(_DEBUG_PIN2, 1);

    if (_inverse_logic_rx)
      d = ~d;
    //darknet dbug line. Serial.write(d);

    // if buffer full, set the overflow flag and return
    if ((_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF != _receive_buffer_head)
    {
      // save new data in buffer: tail points to where byte goes
      _receive_buffer[_receive_buffer_tail] = d; // save new byte
      _receive_buffer_tail = (_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF;
    }
    else
    {
#if _DEBUG // for scope: pulse pin as overflow indictator
      DebugPulse(_DEBUG_PIN1, 1);
#endif
      _RX_buffer_overflow = true;
    }
  }

#if GCC_VERSION < 40302
  // Work-around for avr-gcc 4.3.0 OSX version bug
  // Restore the registers that the compiler misses
  asm volatile(
    "pop r27 \n\t"
    "pop r26 \n\t"
    "pop r23 \n\t"
    "pop r22 \n\t"
    "pop r21 \n\t"
    "pop r20 \n\t"
    "pop r19 \n\t"
    "pop r18 \n\t"
    ::);
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//
// The receive routine called by the interrupt handler
//

const uint8_t buflen = 16;
int recv_valbuf[buflen];
int recv_timebuf[buflen];
uint8_t recv_buf_head = 0;
uint32_t recv_last_micros;
volatile uint8_t
shift_in = 0,
shift_count = 0,
rxdata = 0;
volatile bool rxdatavalid = false;
volatile bool TX_pending;
volatile uint16_t RX_dynabittime;

void IRSerial::recv_SPECTER()
{
  //uint8_t rx = *_receivePortRegister & _receiveBitMask;
  uint32_t now = micros();
  uint16_t elapsed = now - recv_last_micros ;
  if (TX_pending) {
    shift_count = 0;
    return;
  }

  //uint8_t oldSREG = SREG;
  //cli();  // turn off interrupts
  if (*_receivePortRegister & _receiveBitMask) {
    if (!shift_count) {
      if (elapsed > 500) {
        RX_dynabittime = elapsed >> 1;
        shift_in = 0;
        shift_count = 8;
        rxdatavalid = false;
      }
    }
    else {
      shift_in <<= 1;
      shift_count --;
      if (elapsed > RX_dynabittime) shift_in |= 1;
      if (!shift_count) {
        rxdata = shift_in;
        rxdatavalid = true;
        // if buffer full, set the overflow flag and return
        if ((_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF != _receive_buffer_head)
        {
          // save new data in buffer: tail points to where byte goes
          _receive_buffer[_receive_buffer_tail] = shift_in; // save new byte
          _receive_buffer_tail = (_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF;
        }
        else
        {
          _RX_buffer_overflow = true;
        }

      }

    }

    //Serial.println(elapsed);

  }
  recv_last_micros = now;
}

uint8_t IRSerial::rx_pin_read()
{
  return *_receivePortRegister & _receiveBitMask;
}



//
// Interrupt handling
//

/* static */
inline void IRSerial::handle_interrupt()
{
  if (active_object)
  {
    active_object->recv_SPECTER();
  }
}

#if defined(PCINT0_vect)
ISR(PCINT0_vect)
{
  IRSerial::handle_interrupt();
}
#endif

#if defined(PCINT1_vect)
ISR(PCINT1_vect)
{
  IRSerial::handle_interrupt();
}
#endif

#if defined(PCINT2_vect)
ISR(PCINT2_vect)
{
  IRSerial::handle_interrupt();
}
#endif

#if defined(PCINT3_vect)
ISR(PCINT3_vect)
{
  IRSerial::handle_interrupt();
}
#endif

//
// Constructor
//
IRSerial::IRSerial(uint8_t receivePin, uint8_t transmitPin, bool inverse_logic_rx /* = false */, bool inverse_logic_tx /* = false */, uint16_t modulation_frequency /* = 0 */) :
  _rx_delay_centering(0),
  _rx_delay_intrabit(0),
  _rx_delay_stopbit(0),
  _tx_delay(0),
  _RX_buffer_overflow(false),
  _TX_buffer_overflow(false),
  _inverse_logic_rx(inverse_logic_rx),
  _inverse_logic_tx(inverse_logic_tx),
  _modulation_frequency(modulation_frequency)
{
  setTX(transmitPin);
  setRX(receivePin);
}

//
// Destructor
//
IRSerial::~IRSerial()
{
  end();
}

void IRSerial::setTX(uint8_t tx)
{
  pinMode(tx, OUTPUT);
  digitalWrite(tx, HIGH);
  _transmitBitMask = digitalPinToBitMask(tx);
  uint8_t port = digitalPinToPort(tx);
  _transmitPortRegister = portOutputRegister(port);
}

void IRSerial::setRX(uint8_t rx)
{
  pinMode(rx, INPUT);
  //if (!_inverse_logic_rx)
  //  digitalWrite(rx, HIGH);  // pullup for normal logic!
  _receivePin = rx;
  _receiveBitMask = digitalPinToBitMask(rx);
  uint8_t port = digitalPinToPort(rx);
  _receivePortRegister = portInputRegister(port);
}

//
// Public methods
//

void IRSerial::begin(long speed)
{
  if (digitalPinToPCICR(_receivePin))
  {
    *digitalPinToPCICR(_receivePin) |= _BV(digitalPinToPCICRbit(_receivePin));
    *digitalPinToPCMSK(_receivePin) |= _BV(digitalPinToPCMSKbit(_receivePin));
  }
  tunedDelay(_tx_delay); // if we were low this establishes the end

#if _DEBUG
  pinMode(_DEBUG_PIN1, OUTPUT);
  pinMode(_DEBUG_PIN2, OUTPUT);
#endif

  listen();
}

void IRSerial::end()
{
  if (digitalPinToPCMSK(_receivePin))
    *digitalPinToPCMSK(_receivePin) &= ~_BV(digitalPinToPCMSKbit(_receivePin));
}

// Write data to buffer
size_t IRSerial::write(uint8_t d)
{
  if (!isListening())
    return -1;

  // Empty buffer?
  if (_transmit_buffer_head == _transmit_buffer_tail)
    return -1;

  // Write to "tail"
  // if buffer full, set the overflow flag and return
  if ((_transmit_buffer_tail + 1) % _SS_MAX_RX_BUFF != _receive_buffer_head)
  {
    // save new data in buffer: tail points to where byte goes
    _transmit_buffer[_transmit_buffer_tail ] = d; // save new byte
    _transmit_buffer_tail = (_transmit_buffer_tail + 1) % _SS_MAX_RX_BUFF;
    write_SPECTER(d);
  }
  else
  {
    _TX_buffer_overflow = true;
  }



  return 0;
}


// Read data from buffer
int IRSerial::read()
{
  if (!isListening())
    return -1;

  // Empty buffer?
  if (_receive_buffer_head == _receive_buffer_tail)
    return -1;

  // Read from "head"
  uint8_t d = _receive_buffer[_receive_buffer_head]; // grab next byte
  _receive_buffer_head = (_receive_buffer_head + 1) % _SS_MAX_RX_BUFF;
  return d;
}


int IRSerial::available()
{
  if (!isListening())
    return 0;

  return (_receive_buffer_tail + _SS_MAX_RX_BUFF - _receive_buffer_head) % _SS_MAX_RX_BUFF;
}


/* I'll be modifying this code to use timers to output a 38kHz
   square wave instead of just turning on.  To do that, we'll
   be monkeying with Timer1 registers.  When we're done, we
   need to put them back.

   This is what they look like before we start, with no analog
   output:
   TCCR1A 1
     COM1[AB][01]: 00, Normal operation OC1[AB] disconnected
     WGM1[3210]: 0001, PWM, Phase correct, 8-bit TOP=0xFF
       (WGM1[32] are from TCCR1B below)
   TCCR1B 3
     ICNC1: 0 No noise filtering
     ICES1: 0 Trigger on falling edge.
     CS1[210]: 011, clk/64 (from prescalar)
   TCCR1C 0 (not used)
   TCNT1 53
     Actual timer value. Don't care.
   OCR1A 0
     Output compare register
   ICR1 0
     In put Capture Register. Not used.
   TIMSK1 0
     ICIE1: 0  No input capture interrupt
     ICIE1[BA]: 00 No output compare match interrupt
     TOIE1: 0 No overflow interrupt
   TIFR1 7  Set by hardware. Don't care.
     ICF1: 0  input capture flag.
     OCF1[BA]: 11  Output Compare Match flag.
     TOV1: 1  Timer overflow flag

   This is what they are with analogWrite(OC1A, 128):
   TCCR1A 129 (+128)
     COM1A[10]: 10  Clear OC1A on Compare Match (set output to low)
   TCCR1B 3
   TCCR1C 0 (not used)
   TCNT1 don't care
   OCR1A 128 (+128, the analogWrite() value)
   ICR1 don't care
   TIMSK1 0
   TIFR1 39 don't care
     ICF1: 1  (set by hardware, not us)

   What I need the timer to do.
   TCCR1A &= 00111100(0x3B) |= 10000010 (0x82)
     COM1[AB][01]: 10 (Clear on match)
     WGM1[3210]: 1110 (Fast PWM, TOP = ICR1, TOV1 at TOP)
   TCCR1B &= 11100000(0xE0) |= 00011001 (0x19)
     ICNC1: 0
     ICES1: 0
     CS1[210]: 001 (no prescaler)
   TCCR1C (not used)
   TCNT1 Actual timer
   OCR1A: 205 (half ICR1)
   ICR1: 421 (16MHz/38kHz)
   TIMSK1:
     ICIE1: 0 (no Input Capture Interrupt)
     OCIE1[BA]: 00 (no Output Compare Match Interrupts)
     TOIE: 0 (no Overflow Interrupt)
   TIFR1:
     Various flags set when conditions happen.

   So, if I'm reading my own notes correctly,
   we need to read and store TCCR1A, TCCR1B, TCNT1,
   OCR1A, and ICR1.  Then set those values to what
   we need for serial.  When we're done, we just
   restore the original registers.
   While doing serial, set TCCR1A[76] to 00 for
   no output, and 10 for 38kHz output.
*/


//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------


ISR(TIMER1_OVF_vect) {
};

volatile uint8_t TX_shift_counter;
volatile uint8_t TX_shift_out;
volatile uint8_t TX_original;
volatile uint8_t TX_ontime;
volatile uint8_t TX_offtime;
volatile uint8_t oldTCCR1A;
volatile uint8_t oldTCCR1B;
volatile uint16_t oldTCNT1;
volatile uint16_t oldOCR1A;
volatile uint16_t oldICR1;
volatile uint8_t oldTIMSK1;
volatile uint8_t out_bittimes [19]; //precalcd by IRSerial::write_SPECTER written backwards
volatile uint8_t out_bittime_counter;



ISR(TIMER1_COMPB_vect) {
  cli();
  if (out_bittime_counter) {
    out_bittimes[out_bittime_counter] --;
    if (out_bittimes[out_bittime_counter] == 0) {
      //TCCR1A = (TCCR1A & 0x3F) ^ 0x80;
      TCCR1A ^= 0x80;
      out_bittime_counter --;
      //Serial.println(TCCR1A,HEX);
    }
  }
  else if (TX_pending) {
    TIMSK1 = oldTIMSK1;
    TCCR1A = oldTCCR1A;
    TCCR1B = oldTCCR1B;
    TCNT1 = oldTCNT1;
    OCR1A = oldOCR1A;
    ICR1 = oldICR1;
    //TX_shift_counter = 0;
    TX_pending = false;
  }
};
/* OLD ISR
  if (TX_ontime) {
    TX_ontime --;
    TCCR1A = (TCCR1A & 0x3F) | 0x80;  //p.170 Clear OC1A/OC1B on Compare Match (Set output to low level).
  }
  else if (TX_offtime) {
    TX_offtime --;
    TCCR1A &= 0x3F;  // p.170 Normal port operation, OC1A/OC1B disconnected
  }
  else if (TX_shift_counter == 9) {
    TX_ontime = TX_starttime;
    TX_offtime = TX_gaptime;
    TX_shift_counter --;
  }
  else if (TX_shift_counter > 0) {
    if (TX_shift_out & 0x80) {
      TX_ontime = TX_onetime;
    }
    else {
      TX_ontime = TX_zerotime;
    }
    TX_offtime = TX_gaptime;
    TX_shift_out += TX_shift_out; //(TX_shift_out << 1);
    TX_shift_counter --;
  }
  else if (TX_pending) {
    TIMSK1 = oldTIMSK1;
    TCCR1A = oldTCCR1A;
    TCCR1B = oldTCCR1B;
    TCNT1 = oldTCNT1;
    OCR1A = oldOCR1A;
    ICR1 = oldICR1;
    //TX_shift_counter = 0;
    TX_pending = false;
  }
  };
*/

size_t IRSerial::write_SPECTER(uint8_t b)
{
  if (TX_pending) {
    return 0; //still shifting out data so exit
  }
  TX_shift_counter = 0;

  out_bittimes[18] = TX_starttime;
  out_bittimes[17] = TX_gaptime;
  uint8_t i = 16;
  while (i > 0) {
    if (b & 0x80) {
      out_bittimes[i] = TX_onetime;
      i --;
      out_bittimes[i] = TX_gaptime;
      i --;
    }
    else {
      out_bittimes[i] = TX_zerotime;
      i --;
      out_bittimes[i] = TX_gaptime;
      i --;
    }
    b <<= 1;
  }
  out_bittime_counter = 18;

  // Save current Timer1 values.
  oldTCCR1A = TCCR1A;
  oldTCCR1B = TCCR1B;
  oldTCNT1 = TCNT1;
  oldOCR1A = OCR1A;
  oldICR1 = ICR1;
  oldTIMSK1 = TIMSK1;

  // Setup Timer1 for 38kHz. See comments for
  // definitions of magic values.
  TCCR1A = (TCCR1A & 0x38) | 0x82;
  TCCR1B = (TCCR1B & 0xE0) | 0x19;
  TCNT1 = 0; // 16bit counter
  OCR1A = 210;
  ICR1 = 421;

  TX_shift_out = b;
  TX_original = b;
  TX_shift_counter = 9;
  TX_pending = true;
  TIMSK1 |= (1 << OCIE1B);
  //SREG = oldSREG; // turn interrupts back on

  return 1;
}

void IRSerial::tx_pin_write(uint8_t pin_state)
{
  if (pin_state == (_inverse_logic_tx ? HIGH : LOW))
    //*_transmitPortRegister &= ~_transmitBitMask;
    TCCR1A &= 0x3F;  // p.170 Normal port operation, OC1A/OC1B disconnected
  else
    //*_transmitPortRegister |= _transmitBitMask;
    TCCR1A = (TCCR1A & 0x3F) | 0x80;  //p.170 Clear OC1A/OC1B on Compare Match (Set output to low level).
}

void IRSerial::flush()
{
  if (!isListening())
    return;

  uint8_t oldSREG = SREG;
  cli();
  _receive_buffer_head = _receive_buffer_tail = 0;
  SREG = oldSREG;
}

int IRSerial::peek()
{
  if (!isListening())
    return -1;

  // Empty buffer?
  if (_receive_buffer_head == _receive_buffer_tail)
    return -1;

  // Read from "head"
  return _receive_buffer[_receive_buffer_head];
}

