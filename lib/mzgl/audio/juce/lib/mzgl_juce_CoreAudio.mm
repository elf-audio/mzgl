// cannibalized from juce_CoreAudio_mac.cpp
#import <Foundation/Foundation.h>

#include "juce_BigInteger.h"
#include "juce_SystemAudioVolume.h"
#include "juce_SampleRateHelpers.cpp"

std::string CFStringToASCIIStringIgnoringEmoji(CFStringRef cfString) {
	// First, cast CFStringRef to NSString*
	NSString *nsString = (__bridge NSString *) cfString;

	// Create an empty std::string to hold the ASCII characters
	std::string asciiString;

	// Iterate over each character in the NSString
	for (NSUInteger i = 0; i < [nsString length]; ++i) {
		// Get the character at the current index
		unichar c = [nsString characterAtIndex:i];

		// Check if the character is ASCII by checking its code point
		// ASCII characters have code points in the range 0-127
		if (c <= 127) {
			// If it's an ASCII character, append it to the asciiString
			asciiString += static_cast<char>(c);
		}
		// Non-ASCII characters (including emojis) will be ignored
	}

	return asciiString;
}

String String::fromCFString(CFStringRef cfString) {
	return CFStringToASCIIStringIgnoringEmoji(cfString);

	//	return std::string(CFStringGetCStringPtr(cfString, kCFStringEncodingUTF8));
}
namespace Thread {
	void sleep(int ms) {
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	}
}; // namespace Thread
template <typename CFType>
struct CFObjectDeleter {
	void operator()(CFType object) const noexcept {
		if (object != nullptr) CFRelease(object);
	}
};

namespace FloatVectorOperations {
	void copy(float *dest, const float *src, int num) {
		memcpy(dest, src, num * sizeof(float));
	}

	void clear(float *dest, int num) {
		memset(dest, 0, num * sizeof(float));
	}
	void multiply(float *dest, const float amt, int num) {
		for (int i = 0; i < num; ++i) {
			dest[i] *= amt;
		}
	}
} // namespace FloatVectorOperations

template <typename CFType>
using CFUniquePtr = std::unique_ptr<std::remove_pointer_t<CFType>, CFObjectDeleter<CFType>>;

String nsStringToJuce(NSString *str) {
	return str.UTF8String;
}

/** Returns true if a value is at least zero, and also below a specified upper limit.
    This is basically a quicker way to write:
    @code valueToTest >= 0 && valueToTest < upperLimit
    @endcode
*/
template <typename Type1, typename Type2>
bool isPositiveAndBelow(Type1 valueToTest, Type2 upperLimit) noexcept {
	jassert(Type1() <= static_cast<Type1>(
				upperLimit)); // makes no sense to call this if the upper limit is itself below zero..
	return Type1() <= valueToTest && valueToTest < static_cast<Type1>(upperLimit);
}

template <typename Type>
bool isPositiveAndBelow(int valueToTest, Type upperLimit) noexcept {
	jassert(upperLimit >= 0); // makes no sense to call this if the upper limit is itself below zero..
	return static_cast<unsigned int>(valueToTest) < static_cast<unsigned int>(upperLimit);
}

#include "juce_AudioIODevice.h"
#include "juce_AudioIODeviceType.h"
#include "juce_AudioIODeviceType.cpp"

/////////////////////////////////

#if JUCE_COREAUDIO_LOGGING_ENABLED
#	define JUCE_COREAUDIOLOG(a)                                                                                  \
		{                                                                                                         \
			String camsg("CoreAudio: ");                                                                          \
			camsg << a;                                                                                           \
			Logger::writeToLog(camsg);                                                                            \
		}
#else
#	define JUCE_COREAUDIOLOG(a)
#endif

constexpr auto juceAudioObjectPropertyElementMain =
#if defined(MAC_OS_VERSION_12_0)
	kAudioObjectPropertyElementMain;
#else
	kAudioObjectPropertyElementMaster;
#endif

#include "juce_CoreAudio_mac.cpp"

std::shared_ptr<AudioIODeviceType> createJuceIODeviceType() {
	return std::make_shared<CoreAudioClasses::CoreAudioIODeviceType>();
}