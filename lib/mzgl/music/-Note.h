//
//  Note.h
//  emptyExample
//
//  Created by Marek Bereza on 28/09/2015.
//
//

#pragma once

#include <stdio.h>

class Note {
public:
	int pitch;
	int vel;

	// Length must never be non-negative or bad things will happen.
	int length;

	Note(int pitch = 0, int vel = 0, int length = 1) {
		this->pitch	 = pitch;
		this->vel	 = vel;
		this->length = length;

		if (length < 1) {
			printf("ERROR: annot have length < 1!!\n");
		}
	}
	bool operator==(const Note &other) const {
		return pitch == other.pitch && vel == other.vel && length == other.length;
	}

	bool operator!=(const Note &other) const { return !((*this) == other); }
};
