//
//  Listenable.h
//  mzgl
//
//  Created by Marek Bereza on 07/06/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#pragma once

template <class T>
class Listenable {
public:
	void addListener(T *listener) {
		if (std::find(listeners.begin(), listeners.end(), listener) != listeners.end()) {
			return;
		}

		listeners.push_back(listener);
	}

	void removeListener(T *listener) {
		listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
	}
	[[nodiscard]] int getNumListeners() const { return listeners.size(); }

protected:
	std::vector<T *> listeners {};
};

template <class T>
class ScopedListener {
public:
	ScopedListener(Listenable<T> &listenable, T *listener)
		: listenable(listenable)
		, listener(listener) {
		listenable.addListener(listener);
	}
	~ScopedListener() { listenable.removeListener(listener); }

private:
	Listenable<T> &listenable;
	T *listener;
};