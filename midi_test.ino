#include <MIDI.h>

// Create and bind the MIDI interface to the default hardware Serial port
MIDI_CREATE_DEFAULT_INSTANCE();

const int analogInPin = A0;
const int bankPin = A1;
const int bpmPin = A2;

// pins to control TC4051 (or CD74HC4051)
const int s0Pin = 2;
const int s1Pin = 3;
const int s2Pin = 4;

// temporal variable to store sensor value
int sensorValue = 0;

float bpm = 0.0;
float delayBeforeToneOff = 0.0;
float delayAfterToneOff = 0.0;

// bank number to be specified on sendProgramChange()
int mainBank = 0;
int subBank = 0;

// base tone number and whether the chord is major or minor
int baseTone = 0;
int major = 0;

// length of the tone and the steps to consume
float length = 0.0;
int steps = 0;
int stepsLeft = 0;
const int totalSteps = 8;

// (currently unused)
int velocity = 0;

// Probability and number of notes
int prob = 0;
int notes = 0;

// These steps are for minor chords.
// If it's major, adds one step to 2nd and 4th notes.
int chord[5] = {
    0,
    3,
    7,
    10,
    -13, // meant to add one-octave-lower tone? I forgot why I chose this number...
};

// number of channel of TC4051 (or CD74HC4051)
int i = 0;

void setup()
{
    MIDI.begin(MIDI_CHANNEL_OMNI); // Listen to all incoming messages

    pinMode(s0Pin, OUTPUT);
    pinMode(s1Pin, OUTPUT);
    pinMode(s2Pin, OUTPUT);

    stepsLeft = totalSteps;
}

int readSensorValue(int i, int pin, int map_max)
{
    PORTD = (PORTD & B00000010) | (i << 2);
    sensorValue = analogRead(pin);
    return map(sensorValue, 0, 1023, 0, map_max);

    randomSeed(analogRead(5));
}

void loop()
{
    // Program change
    sensorValue = analogRead(bankPin);
    int bank = map(sensorValue, 0, 1023, 0, 127);
    // if a new bank is chosen, choose another bank randomly
    if (bank != mainBank)
    {
        mainBank = bank;
        subBank = random(128);
    }

    // sometimes use subBank to provide various sounds
    if (random(10) > 1)
    {

        MIDI.sendProgramChange(mainBank, 1);
    }
    else
    {
        MIDI.sendProgramChange(subBank, 1);
    }

    // Speed
    sensorValue = analogRead(bpmPin);
    bpm = map(sensorValue, 0, 1023, 50.0, 200.0);

    // tone
    baseTone = readSensorValue(i, analogInPin, 255);
    major = baseTone & 1; // major or minor
    baseTone = baseTone >> 1;

    i = (i + 1) % 8; // proceed to the next pod

    if (i < 6)
    {
        length = map(readSensorValue(i, analogInPin, 1027), 0, 1027, 0, 4);
        steps = ceil(length); // at maximum, 4 steps

        velocity = 127; // TODO
        // velocity = map(velocity, 0, 127, 127, 127);

        stepsLeft -= steps;

        i = (i + 1) % 8;
    }
    else
    {
        steps = stepsLeft;
        length = steps;
        stepsLeft = totalSteps;
        velocity = 127; // TODO
    }

    delayBeforeToneOff = 1000.0 * 60.0 / bpm / totalSteps * length;
    delayAfterToneOff = 1000.0 * 60.0 / bpm / totalSteps * (totalSteps - length);

    prob = readSensorValue(i, analogInPin, 1027);
    if (prob < 256)
    {
        notes = 1;
        prob = 1027;
    }
    else
    {
        notes = map(prob, 256, 1027, 1, 5);
    }
    i = (i + 1) % 8;

    if (steps <= 0)
        return;

    for (int n = 0; n < notes; n++)
    {
        if (random(1027) > 1027 - prob)
        {
            MIDI.sendNoteOn(baseTone + chord[n] + n % 2 * major, velocity, 1);
        }
    }

    delay(delayBeforeToneOff);

    for (int n = 0; n < notes; n++)
    {
        if (random(1027) > 1027 - prob)
        {

            MIDI.sendNoteOff(baseTone + chord[n] + n % 2 * major, 0, 1);
        }
    }

    delay(delayAfterToneOff);
}
