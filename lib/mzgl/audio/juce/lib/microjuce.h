#pragma once
#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <memory>
#include <cstddef>
#include <optional>
#include <vector>
#include "juce_CoreAudioTimeConversions_mac.h"
#include "mzAssert.h"
#include <mutex>
#include <atomic>
#include <cstdlib>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <thread>

#define jassert		 mzAssert
#define jassertfalse mzAssert(false)
#define JUCE_API
#define JUCE_END_IGNORE_WARNINGS_GCC_LIKE
#define TRANS(A) A
#define JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(a)
#define JUCE_DECLARE_NON_COPYABLE(className)                                                                      \
	className(const className &)			= delete;                                                             \
	className &operator=(const className &) = delete;

/** This is a shorthand macro for deleting a class's move constructor and
    move assignment operator.
*/
#define JUCE_DECLARE_NON_MOVEABLE(className)                                                                      \
	className(className &&)			   = delete;                                                                  \
	className &operator=(className &&) = delete;

#define JUCE_CALLTYPE
#define JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE(A)
//#define JUCE_DECLARE_WEAK_REFERENCEABLE(A)

/** Returns the larger of two values. */
template <typename Type>
constexpr Type jmax(Type a, Type b) {
	return a < b ? b : a;
}

/** Returns the larger of three values. */
template <typename Type>
constexpr Type jmax(Type a, Type b, Type c) {
	return a < b ? (b < c ? c : b) : (a < c ? c : a);
}

/** Returns the larger of four values. */
template <typename Type>
constexpr Type jmax(Type a, Type b, Type c, Type d) {
	return jmax(a, jmax(b, c, d));
}

/** Returns the smaller of two values. */
template <typename Type>
constexpr Type jmin(Type a, Type b) {
	return b < a ? b : a;
}

/** Returns the smaller of three values. */
template <typename Type>
constexpr Type jmin(Type a, Type b, Type c) {
	return b < a ? (c < b ? c : b) : (c < a ? c : a);
}

/** Returns the smaller of four values. */
template <typename Type>
constexpr Type jmin(Type a, Type b, Type c, Type d) {
	return jmin(a, jmin(b, c, d));
}

#include <stdint.h>

using uint32 = uint32_t;
using uint64 = uint64_t;
using int32	 = int32_t;
using int64	 = int64_t;
using uint8	 = uint8_t;
#define exactlyEqual(A, B) ((A) == (B))
template <typename T>
std::unique_ptr<T> rawToUniquePtr(T *ptr) {
	return std::unique_ptr<T>(ptr);
}

class String : public std::string {
public:
	String(double d) { *this = std::to_string(d); }
	// all necessary std::string constructors
	String() = default;
	String(const char *s)
		: std::string(s) {}
	String(const std::string &s)
		: std::string(s) {}
	String(const String &s)
		: std::string(s) {}
	String(String &&s) noexcept
		: std::string(std::move(s)) {}
	String &operator=(const String &s) {
		std::string::operator=(s);
		return *this;
	}

	// << operator
	String &operator<<(const std::string &s) {
		append(s);
		return *this;
	}
	String &operator<<(const char *s) {
		append(s);
		return *this;
	}
	// << with double
	String &operator<<(double d) {
		append(std::to_string(d));
		return *this;
	}
	String &operator=(String &&s) noexcept {
		std::string::operator=(std::move(s));
		return *this;
	}
	bool isNotEmpty() const { return !empty(); }
	bool isEmpty() const { return empty(); }
	//	static String fromCFString(CFStringRef cfString) {
	//		auto *str = CFStringGetCStringPtr(cfString, kCFStringEncodingUTF8);
	//		if (str == nullptr) return "<null - maybe has emoji>";
	//		return String(str);
	//	}
	static String toHexString(int v) {
		char buf[16];
		snprintf(buf, sizeof(buf), "%x", v);
		return buf;
	}

	static String fromCFString(CFStringRef cfString);
};

template <typename T>
//using Atomic = std::atomic<T>;
class Atomic : public std::atomic<T> {
public:
	T get() const { return std::atomic<T>::load(); }
	// assign bool operator
	void operator=(const bool &newValue) { std::atomic<T>::store(newValue); }
};
template <typename T>
class Array : public std::vector<T> {
public:
	T getFirst() {
		if (this->empty()) return T();
		return this->front();
	}

	T getLast() {
		if (this->empty()) return T();
		return this->back();
	}
	// initializer list constructor
	Array(std::initializer_list<T> il)
		: std::vector<T>(il) {}
	Array() = default;
	bool isEmpty() const { return this->empty(); }

	void addUsingDefaultSort(const T &newElement) {
		this->emplace_back(newElement);
		sort();
	}
	void sort() { std::sort(this->begin(), this->end()); }
	void add(const T &newElement) { std::vector<T>::emplace_back(newElement); }
	T getUnchecked(int index) const { return (*this)[index]; }
	void add(T &&newElement) { std::vector<T>::emplace_back(std::move(newElement)); }

	void addIfNotAlreadyThere(const T &newElement) {
		if (!contains(newElement)) add(newElement);
	}

	bool contains(const T &elementToLookFor) const {
		auto e		= this->begin();
		auto endPtr = this->end();

		for (; e != endPtr; ++e)
			if (elementToLookFor == *e) return true;

		return false;
	}
	T &getReference(int i) { return (*this)[i]; }
	const T &getReference(int i) const { return (*this)[i]; }

	void swapWith(Array<T> &other) noexcept { std::swap(*this, other); }

	template <class OtherArrayType>
	void removeValuesNotIn(const OtherArrayType &otherArray) {
		if (this != &otherArray) {
			if (otherArray.size() <= 0) {
				this->clear();
			} else {
				for (int i = this->size(); --i >= 0;)
					if (!otherArray.contains((*this)[i])) this->erase(this->begin() + i);
			}
		}
	}
};
inline void zeromem(void *memory, size_t numBytes) noexcept {
	memset(memory, 0, numBytes);
}

namespace FloatVectorOperations {
	void copy(float *dest, const float *src, int num);
	void clear(float *dest, int num);
	void multiply(float *dest, const float amt, int num);
} // namespace FloatVectorOperations
//==============================================================================
/** Casts a pointer to another type via `void*`, which suppresses the cast-align
    warning which sometimes arises when casting pointers to types with different
    alignment.
    You should only use this when you know for a fact that the input pointer points
    to a region that has suitable alignment for `Type`, e.g. regions returned from
    malloc/calloc that should be suitable for any non-over-aligned type.
*/
template <typename Type>
inline Type unalignedPointerCast(void *ptr) noexcept {
	static_assert(std::is_pointer_v<Type>);
	return reinterpret_cast<Type>(ptr);
}

#define jlimit(A, B, C) std::clamp(C, A, B)
class AudioWorkgroup {};

/** This macro can be added to class definitions to disable the use of new/delete to
    allocate the object on the heap, forcing it to only be used as a stack or member variable.
*/
#define JUCE_PREVENT_HEAP_ALLOCATION                                                                              \
private:                                                                                                          \
	static void *operator new(size_t)	= delete;                                                                 \
	static void operator delete(void *) = delete;
#define JUCE_BEGIN_IGNORE_WARNINGS_MSVC(A)
#define JUCE_END_IGNORE_WARNINGS_MSVC
#include "juce_HeapBlock.h"
using ScopedLock = std::lock_guard<std::mutex>;

class ScopedUnlock {
public:
	ScopedUnlock(std::mutex &m)
		: mutex(m) {
		mutex.unlock();
	}
	~ScopedUnlock() { mutex.lock(); }

private:
	std::mutex &mutex;
};

using namespace juce;
//class CriticalSection : public std::mutex {
//public:
//};
using CriticalSection = std::mutex;
class StringArray : public Array<String> {
public:
	StringArray() = default;
	StringArray(std::initializer_list<String> il)
		: Array<String>(il) {}
	StringArray(const StringArray &other)
		: Array<String>(other) {}
	StringArray(const Array<String> &other)
		: Array<String>(other) {}

	void add(const std::string &newElement) { emplace_back(newElement); }
	void add(const char *newElement) { emplace_back(newElement); }
	void add(const String &newElement) { emplace_back(newElement); }
	void addIfNotAlreadyThere(const std::string &newElement) {
		if (!contains(newElement)) add(newElement);
	}
	void addIfNotAlreadyThere(const char *newElement) {
		if (!contains(newElement)) add(newElement);
	}
	void addIfNotAlreadyThere(const String &newElement) {
		if (!contains(newElement)) add(newElement);
	}
	String joinIntoString(const String &separator) {
		String out;
		for (auto &s: *this)
			out += s + separator;
		return out;
	}
	// indexOf
	int indexOf(const std::string &stringToLookFor, int startIndex = 0) const {
		auto e		= this->begin() + startIndex;
		auto endPtr = this->end();
		int i		= startIndex;

		for (; e != endPtr; ++e, ++i)
			if (*e == stringToLookFor) return i;

		return -1;
	}

	void appendNumbersToDuplicates(bool ignoreCase, bool appendNumberToFirstInstance) {
		String preNumberString = " (";

		String postNumberString = ")";

		for (int i = 0; i < size() - 1; ++i) {
			auto &s		   = getReference(i);
			auto nextIndex = indexOf(s, i + 1);

			if (nextIndex >= 0) {
				auto original = s;
				int number	  = 0;

				if (appendNumberToFirstInstance)
					s = original + String(preNumberString) + String(++number) + String(postNumberString);

				while (nextIndex >= 0) {
					(*this)[nextIndex] =
						(*this)[nextIndex] + String(preNumberString) + String(++number) + String(postNumberString);
					nextIndex = indexOf(original, nextIndex + 1);
				}
			}
		}
	}
};

#include <dispatch/dispatch.h>
class Timer {
public:
	Timer()
		: dispatchSource(nullptr) {}

	virtual ~Timer() { stopTimer(); }

	virtual void timerCallback() = 0;

	void stopTimer() noexcept {
		if (dispatchSource != nullptr) {
			dispatch_source_cancel(dispatchSource);
			dispatch_release(dispatchSource);
			dispatchSource = nullptr;
		}
	}

	void startTimer(int ms) noexcept {
		if (dispatchSource != nullptr) {
			stopTimer(); // Ensure we don't create multiple timers
		}

		// Create a new dispatch source timer
		dispatchSource = dispatch_source_create(
			DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));

		if (dispatchSource == nullptr) {
			return;
		}

		dispatch_source_set_timer(dispatchSource,
								  dispatch_time(DISPATCH_TIME_NOW, ms * NSEC_PER_MSEC),
								  ms * NSEC_PER_MSEC,
								  (1ull * NSEC_PER_MSEC) / 10);
		dispatch_source_set_event_handler(dispatchSource, ^{ this->timerCallback(); });

		dispatch_resume(dispatchSource);
	}

private:
	dispatch_source_t dispatchSource;
};

class AsyncUpdater {
public:
	AsyncUpdater()
		: isPending(false) {}

	virtual ~AsyncUpdater() = default;

	virtual void handleAsyncUpdate() {
		// Override this method to perform your asynchronous operation on the main thread
	}

	void cancelPendingUpdate() {
		// Atomic flag check and set to ensure thread safety
		__sync_bool_compare_and_swap(&isPending, true, false);
	}

	void triggerAsyncUpdate() {
		if (!__sync_bool_compare_and_swap(&isPending, false, true)) {
			// If an update is already pending, don't queue another
			return;
		}

		dispatch_async(dispatch_get_main_queue(), ^{
		  if (this->isPending) {
			  this->handleAsyncUpdate();
			  // Reset the pending flag after handling
			  this->isPending = false;
		  }
		});
	}

private:
	volatile bool isPending;
};
template <class T>
class ListenerList : public Array<T *> {
public:
	void remove(const T *newElement) {
		for (int i = 0; i < this->size(); i++) {
			if ((*this)[i] == newElement) {
				this->erase(this->begin() + i);
				break;
			}
		}
	}
	void call(std::function<void(T &)> f) {
		for (auto &l: *this)
			f(*l);
	}
};
template <typename Fn>
struct ScopeGuard : Fn {
	~ScopeGuard() { Fn::operator()(); }
};
template <typename Fn>
ScopeGuard(Fn) -> ScopeGuard<Fn>;

template <typename Type>
constexpr bool approximatelyEqual(Type a, Type b) {
	if (!(std::isfinite(a) && std::isfinite(b))) return exactlyEqual(a, b);

	const auto diff = std::abs(a - b);

	return diff <= std::numeric_limits<Type>::min()
		   || diff <= std::numeric_limits<Type>::epsilon() * std::max(std::abs(a), std::abs(b));
}

// rename weak_ptr to WeakReference
//template <typename T>
//using WeakReference = std::weak_ptr<T>;
#include "juce_ContainerDeletePolicy.h"
#include "juce_ReferenceCountedObject.h"
#include "juce_WeakReference.h"
#include "juce_AudioSampleBuffer.h"