//
//  AudioUnitViewController.h
//  MZGLiOSEffectAU
//
//  Created by Marek Bereza on 05/04/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#import <CoreAudioKit/CoreAudioKit.h>

#include "mzgl_platform.h"
#if MZGL_IOS
#	import "MZGLKitView.h"
#endif
@interface AudioUnitViewController : AUViewController <AUAudioUnitFactory>
#if MZGL_IOS
- (EventsView *)getView;
#endif
@end

