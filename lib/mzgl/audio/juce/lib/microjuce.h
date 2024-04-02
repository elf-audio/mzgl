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
#define JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(a)
#define JUCE_DECLARE_NON_COPYABLE(a)
#define JUCE_CALLTYPE

#define jmin std::min
#define jmax std::max
#include <stdint.h>
using uint32 = uint32_t;
using uint64 = uint64_t;
using int32	 = int32_t;
using int64	 = int64_t;
using uint8	 = uint8_t;
#define exactlyEqual(A, B) ((A) == (B))

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
};

template <typename T>
using Atomic = std::atomic<T>;

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

class AudioSampleBuffer : public std::vector<float> {
public:
	void setSize(int numCh, int numFrames) {}
	int getNumSamples() {
		//		return size();
		return 0;
	}
	int getNumChannels() { return 0; }
	float *getReadPointer(int channel, int pos) { return nullptr; }
	float *getWritePointer(int channel, int pos) { return nullptr; }
	float *const *getArrayOfWritePointers() noexcept { return nullptr; }
};

#define jlimit(A, B, C) std::clamp(C, A, B)
class AudioWorkgroup {};
template <typename T>
class HeapBlock {
public:
	T *data		= nullptr;
	HeapBlock() = default;
	void calloc(int sz) { data = static_cast<T *>(std::calloc(sz, sizeof(T))); }
	void malloc(int sz) { data = static_cast<T *>(std::malloc(sz * sizeof(T))); }
	void realloc(int sz) { data = static_cast<T *>(std::realloc(data, sz * sizeof(T))); }

	~HeapBlock() { std::free(data); }
	// subscript operator
	T &operator[](int i) { return data[i]; }
	T *get() const { return data; }
	void swapWith(HeapBlock<T> &other) noexcept { std::swap(data, other.data); }
	void free() noexcept {
		std::free(data);
		data = nullptr;
	}
	inline bool operator!=(const T *otherPointer) const noexcept { return otherPointer != data; }
	inline bool operator==(const T *otherPointer) const noexcept { return otherPointer == data; }
};
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
class StringArray : public Array<std::string> {
public:
	StringArray() = default;
	StringArray(std::initializer_list<std::string> il)
		: Array<std::string>(il) {}
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
class Timer {
public:
	virtual ~Timer() = default;
	virtual void timerCallback() {}
	void stopTimer() noexcept {}
	void startTimer(int ms) noexcept {}
};

class AsyncUpdater {
public:
	virtual ~AsyncUpdater() = default;
	virtual void handleAsyncUpdate() {}
	void cancelPendingUpdate() {}
	void triggerAsyncUpdate() { handleAsyncUpdate(); }
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
template <typename T>
using WeakReference = std::weak_ptr<T>;
