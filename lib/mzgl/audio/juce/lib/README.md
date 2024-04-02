# JUCE audio system

This dir contains some stuff pulled out of JUCE and slightly massaged to build without incorporating all of juce.

Pretty much all the actual juce_*.h files have been slightly modified to reduce dependencies - mostly just taking out
stuff that isn't needed by the audio system stuff.

The more I've worked on it the more I've realized that there's actually quite a lot of juce that is modular enough to be
used directly.

One day will probs come back to this and remove more of my own implementations of juce objects and replace with actual
unmodified juce code

microjuce.h contains most of this glue.
