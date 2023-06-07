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
		for(int i = 0; i < listeners.size(); i++) {
			if(listeners[i]==listener) {
				return;
			}
		}
		listeners.push_back(listener);
	}
	
	void removeListener(T *listener) {
		for(int i = 0; i < listeners.size(); i++) {
			if(listeners[i]==listener) {
				listeners.erase(listeners.begin() + i);
				return;
			}
		}
	}
protected:
	std::vector<T*> listeners;
};

