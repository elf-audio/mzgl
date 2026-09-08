//
//  Listenable.h
//  mzgl
//
//  Created by Marek Bereza on 07/06/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#pragma once

#include <algorithm>
#include <functional>
#include <vector>

template <class T>
class Listenable {
public:
	using Listener = T;

	void addListener(T *listener) {
		if (std::find(listeners.begin(), listeners.end(), listener) != listeners.end()) {
			return;
		}

		listeners.push_back(listener);
	}

	virtual ~Listenable() = default;

	void removeListener(T *listener) {
		listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
	}
	[[nodiscard]] int getNumListeners() const { return listeners.size(); }

	void notify(std::function<void(T *)> f) {
		for (auto *l: listeners)
			f(l);
	}

protected:
	std::vector<T *> listeners;
};

// RAII registration: adds `listener` to `owner` on construction and removes it
// on destruction. `Owner` is any type exposing a nested `Listener` type plus
// addListener(Listener*)/removeListener(Listener*) - Listenable<T> qualifies,
// as do hand-rolled listener lists (SequenceManager, NoteRepeater...).
// Declare it as a member *after* anything it references so it is torn down
// before the listener object itself goes away.
template <class Owner>
class ScopedListener {
public:
	using T = typename Owner::Listener;

	ScopedListener(Owner &listenable, T *listener)
		: listenable(listenable)
		, listener(listener) {
		listenable.addListener(listener);
	}
	~ScopedListener() { listenable.removeListener(listener); }

	ScopedListener(const ScopedListener &)			  = delete;
	ScopedListener &operator=(const ScopedListener &) = delete;
	ScopedListener(ScopedListener &&)				  = delete;
	ScopedListener &operator=(ScopedListener &&)	  = delete;

private:
	Owner &listenable;
	T *listener;
};