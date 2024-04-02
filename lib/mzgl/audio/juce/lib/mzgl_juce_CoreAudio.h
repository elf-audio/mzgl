#pragma once
#include "microjuce.h"
#include "juce_BigInteger.h"
#include "juce_AudioIODevice.h"
#include "juce_AudioIODeviceType.h"
#include <memory>

std::shared_ptr<juce::AudioIODeviceType> createJuceIODeviceType();