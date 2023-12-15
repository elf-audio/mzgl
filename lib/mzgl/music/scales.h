/**     ___           ___           ___                         ___           ___     
 *     /__/\         /  /\         /  /\         _____         /  /\         /__/|    
 *    |  |::\       /  /::\       /  /::|       /  /::\       /  /::\       |  |:|    
 *    |  |:|:\     /  /:/\:\     /  /:/:|      /  /:/\:\     /  /:/\:\      |  |:|    
 *  __|__|:|\:\   /  /:/~/::\   /  /:/|:|__   /  /:/~/::\   /  /:/  \:\   __|__|:|    
 * /__/::::| \:\ /__/:/ /:/\:\ /__/:/ |:| /\ /__/:/ /:/\:| /__/:/ \__\:\ /__/::::\____
 * \  \:\~~\__\/ \  \:\/:/__\/ \__\/  |:|/:/ \  \:\/:/~/:/ \  \:\ /  /:/    ~\~~\::::/
 *  \  \:\        \  \::/          |  |:/:/   \  \::/ /:/   \  \:\  /:/      |~~|:|~~ 
 *   \  \:\        \  \:\          |  |::/     \  \:\/:/     \  \:\/:/       |  |:|   
 *    \  \:\        \  \:\         |  |:/       \  \::/       \  \::/        |  |:|   
 *     \__\/         \__\/         |__|/         \__\/         \__\/         |__|/   
 *
 *  Description: 
 *				 
 *  scales.h, created by Marek Bereza on 05/02/2013.
 */

#define PENTATONIC 2
#define CHROMATIC  1
#define WHOLE	   3
#define MAJOR	   4
#define MINOR	   0
#include <string>

float mtof(float f);
float ftom(float f);

int getScaled(int pos, int scale);

// this gives you a speed at which to play back a sample
// given the midi note you want to pitch it at, and its
// original pitch as a midi note.
float midiNoteToSpeed(int note, int originalNote);

// just the name given 0 - 11
// C = 0, C# = 1, D = 2 etc.
std::string noteNumToName(int note);
int noteNameToNum(const std::string &noteName);
// includes the octave too
std::string midiNoteNumToString(int note);

bool isSharp(int i);
int qwertyToMidi(int k);
