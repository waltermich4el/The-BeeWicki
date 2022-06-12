//The BEEWICKI is a MIDI controller based on the Wicki Layout.
//The notes to the left are a full tone up, notes to the right are a full tone lower.
//Upper left is a perfect fourth, upper right a perfect fifth and directly above would be an octave higher.

//The keyboard utilises a pushbutton matrix with diodes
//having the columns set as outputs and  the rows as inputs.
//As for the microcontroller, an ATMEGA32U4 microprocessor
//was used which has the properties of being MIDI USB compliant.
//No Further software is needed to "translate" the packages sent.


//The code was written by Walter Buchholtzer in May 2021.



//The pins have been connected as follows :

//R0 - D8 - 28
//R1 - D7 -  1
//R2 - D6 - 27
//R3 - D5 - 31
//R4 - D4 - 25
//R5 - D3 - 18
//R6 - D2 - 19
//R7 - D15 SCK - 9
//R8 - D14 MISO - 11
//
//C0 - A5 - 41
//C1 - D9 - 29
//C2 - D10 - 30
//C3 - D11 - 12
//C4 - D12 - 26
//C5 - A0 - 36
//C6 - A1 - 37
//C7 - A2 - 38
//C8 - A3 - 39
//C9 - A4 - 40

//*where C corresponds to columns and results in const byte outputs
//and R corresponds to rows and results in const byte inputs;
//
//The code has three main loops:
//1. scan () - which scans through the entire matrix and indexes buttons
//2. playNotes () - which detects if a change of state has appeared in the buttons ands sends the accordingly midi message and flushes after each one.
//3. noteON () , noteOff() - creates the midi event packet (midiEventPacket_t) which is understood by the VST program.

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< START OF CODE >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

//defining constants and variables
#include "MIDIUSB.h"  // this library allows any controller with native USB capabilities ( ATMEGA32U4 based boards or ARM boards) to 
  //appear as a MIDI peripherial over USB to a connected computer.

// Assigning pins to inputs in outputs
const byte input[] = {A5, 9, 10, 11, 12, A0, A1, A2, A3, A4}; //defining the Arduino pins as inputs - rows
const byte output[] = {8, 7, 6, 5, 4, 3, 2, MISO, SCK}; // defining the Arduino pins as outputs - coulumns

// Constant variables that build the array for indexing the switches
const byte size_input = sizeof(input); // the number of elements in input
const byte size_output = sizeof(output); // the number of elements in the output 

unsigned long readVal; // used for reading the pin state

const byte sensorVal = size_input * size_output ; // creating array for indexing and updating pin states

// Global time variables
unsigned long   currentTime;                                            // program loop consistent variable for time in milliseconds since power on
const byte      debounceTime = 80;                                      // global digital button debounce time in milliseconds

// Variables for holding digital button states and activation times
byte            activeButtons[sensorVal];                            // array to hold current note button states
byte            previousActiveButtons[sensorVal];                    // array to hold previous note button states for comparison
unsigned long   activeButtonsTime[sensorVal];                        // array to track last note button activation time for debounce

// Velocity levels
byte velocity = 127;

// Octave number
int octave = 0;

// The MIDI channel used for the packages
byte midiChannel = 0;


// Creating an array of the midi notes used in the Wicki-Hayden Layout and storring them in a constant variable
const byte wicki_layout[sensorVal] = {
  // '00' indicates unused node
  00,  78,  80,  82,  84,  86,  88,  90,  92,  94,
  71,  73,  75,  77,  79,  81,  83,  85,  87,  89,
  00,   66,  68,  70,  72,  74,  76,  78,  80,  82,
  59,  61,  63,  65,  67,  69,  71,  73,  75,  77,
  00,   54,  56,  58,  60,  62,  64,  66,  68,  70,
  47,  49,  51,  53,  55,  57,  59,  61,  63,  65,
  00,   42,  44,  46,  48,  50,  52,  54,  56,  58,
  35,  37,  39,  41,  43,  45,  47,  49,  51,  53,
  00,   30,  32,  34,  36,  38,  40,  42,  44,  46

};

//<<<<<<<<<< SETUP >>>>>>>>>>>>

void setup() {

  // setting pin modes
  for (int i = 0; i < size_input; i = i + 1) {
    pinMode(input[i], INPUT);         // setting the inpust as INPUT 0V
  }
  
  for (int ii = 0; ii < size_output; ii = ii + 1) {
    pinMode(output[ii], OUTPUT);     // setting the outputs as OUTPUT  0V
    digitalWrite(output[ii], HIGH);  // making them HIGH 3.3V
  }
  
// setting the baud rate for MIDI communication 
  Serial1.begin(31250);

}


//<<<<<<<<<<<<<<<< MAIN BODY >>>>>>>>>>>>>>>>>>>>>>>>>>>>

// the functions that run with each iteration
void loop()
{
  // obtaining and setting the current time for later use in debounce of switches
  currentTime = millis(); 

  //function that scans through the button matrix
  scan();

  //functions for playing the pressed buttons
  playNotes();

  //function to see the states of buttons in the serial monitor ( please change the baud rate to 9600 ) 
  printButtons();
}



// creating the functions
//-----------------------------------------------------
void printButtons()
{

  for (byte i = 0; i < sensorVal ; i++) { // running through all the buttons
    Serial.print(activeButtons[i]);       // obtaining the state for each individual button
  }
  Serial.println("");                      //and printing the state, 0 or 1
}


//-----------------------------------------------------------------------------------------------------------------------
void scan()
{
  for (byte i = 0; i < size_output; i++) {     // indexing through all the outputs from 0 to 8 
    digitalWrite(output[i], LOW);              // activating the current column by making it LOW/ 0V and
    for (byte ii = 0; ii < size_input; ii++) { // now indexing through the rows 
      digitalWrite(input[ii], INPUT_PULLUP);   // and activating the built-in pull-up resistors making the inputs 3.3V
      readVal = digitalRead(input[ii]);        // reading the state of the current node ( intersection of row and column = button location )

      byte button = ii + i * size_input;       // addressing the button and assigning this location in the matrix a unique number.
      delayMicroseconds(50);                   // adding a delay to give the pin modes time to change state, and eliminate ghost inputs
      if (readVal == HIGH && (millis() - activeButtonsTime[button]) > debounceTime) { // If button passes debounce and a input pin is HIGH
       
        activeButtons[button] = 0;                     //write a 0 meaning that the button is not pressed 

      }
      else                                             //is the input is LOW means buttons is pressed and current is flowing 
      {  
        activeButtons[button] = 1;                     // write a 1 meaning the button is pressed
    
      }
      digitalWrite(input[ii], INPUT);                 //deactivate the current row by making it 0V
    }
    digitalWrite(output[i], HIGH);                    //deactivate current column by makng it 3.3V
  }
}



//-----------------------------------------------------------------------------------------------------------------------
void playNotes()
{
  for (int i = 0; i < sensorVal; i++)                                                  // For all buttons in the deck
  {
    if (activeButtons[i] != previousActiveButtons[i])                                   // If a change is detected
    {
      if (activeButtons[i] == 1)                                                      // If the button is active
      {

        {
          noteOn(midiChannel, wicki_layout[i] + octave, velocity);           // Send a noteOn using the Wicki-Hayden layout
          MidiUSB.flush();                                                          
        }
        previousActiveButtons[i] = 1;                                               // Update the "previous" variable for comparison on next loop
      }

      if (activeButtons[i] == 0)                                                      // If the button is inactive
      {

        noteOn(midiChannel, wicki_layout[i] + octave, 0);                 // Send a noteOff using the Wicki-Hayden layout
        MidiUSB.flush();
        previousActiveButtons[i] = 0;                                               // Update the "previous" variable for comparison on next loop
      }
    }
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------
// MIDI PACKET FUNCTIONS

//Send MIDI Note On
// 1st byte = Event type (0x09 = note on, 0x08 = note off).
// 2nd byte = Event type bitwise ORed with MIDI channel.
// 3rd byte = MIDI note number.
// 4th byte = Velocity (7-bit range 0-127)
void noteOn(byte channel, byte pitch, byte velocity)
{

  channel = 0x90 | channel;                                                   // Bitwise OR outside of the struct to prevent compiler warnings
  midiEventPacket_t noteOn = {0x09, channel, pitch, velocity};                // noteOn Build a struct containing all of our information in a single packet
  MidiUSB.sendMIDI(noteOn);                                                   // Send packet to the MIDI USB bus
  Serial1.write(0x90 | channel);                                              // Send event type/channel to the MIDI serial bus
  Serial1.write(pitch);                                                       // Send note number to the MIDI serial bus
  Serial1.write(velocity);                                                    // Send velocity value to the MIDI serial bus
}


// Send MIDI Note Off
// 1st byte = Event type (0x09 = note on, 0x08 = note off).
// 2nd byte = Event type bitwise ORed with MIDI channel.
// 3rd byte = MIDI note number.
// 4th byte = Velocity (7-bit range 0-127)
void noteOff(byte channel, byte pitch, byte velocity)
{

  //
  channel = 0x80 | channel;                                                   // Bitwise OR outside of the struct to prevent compiler warnings
  midiEventPacket_t noteOff = {0x08, channel, pitch, velocity};               // Build a struct containing all of our information in a single packet
  MidiUSB.sendMIDI(noteOff);                                                  // Send packet to the MIDI USB bus
  Serial1.write(0x80 | channel);                                              // Send event type/channel to the MIDI serial bus
  Serial1.write(pitch);                                                       // Send note number to the MIDI serial bus
  Serial1.write(velocity);                                                    // Send velocity value to the MIDI serial bus
}


//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< END FUNCTIONS SECTION >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//     <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< THE END >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//
// ______   __  __     ______        ______     ______     ______     __     __     __     ______     __  __     __  __    
///\__  _\ /\ \_\ \   /\  ___\      /\  == \   /\  ___\   /\  ___\   /\ \  _ \ \   /\ \   /\  ___\   /\ \/ /    /\ \_\ \   
//\/_/\ \/ \ \  __ \  \ \  __\      \ \  __<   \ \  __\   \ \  __\   \ \ \/ ".\ \  \ \ \  \ \ \____  \ \  _"-.  \ \____ \  
//   \ \_\  \ \_\ \_\  \ \_____\     \ \_____\  \ \_____\  \ \_____\  \ \__/".~\_\  \ \_\  \ \_____\  \ \_\ \_\  \/\_____\ 
//    \/_/   \/_/\/_/   \/_____/      \/_____/   \/_____/   \/_____/   \/_/   \/_/   \/_/   \/_____/   \/_/\/_/   \/_____/ 
                                                                                                                         

                                                                                                                                  
