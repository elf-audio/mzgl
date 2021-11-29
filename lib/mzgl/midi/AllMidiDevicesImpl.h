//
//  AllMidiDevicesImpl.h
//  mzgl
//
//  Created by Marek Bereza on 13/11/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once
class MidiListener;
class AllMidiDevicesImpl {
public:
    virtual void setup() {}
    virtual void addListener(MidiListener *l) = 0;
};
