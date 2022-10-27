//
//  MZGLiOSEffectAUAudioUnit.h
//  MZGLiOSEffectAU
//
//  Created by Marek Bereza on 05/04/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#import <AudioToolbox/AudioToolbox.h>
#include <memory>
#include "Plugin.h"
// Define parameter addresses.
extern const AudioUnitParameterID myParam1;

@interface MZGLEffectAU : AUAudioUnit
-(std::shared_ptr<Plugin>) getPlugin;

@end
