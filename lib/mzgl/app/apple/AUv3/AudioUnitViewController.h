//
//  AudioUnitViewController.h
//  MZGLiOSEffectAU
//
//  Created by Marek Bereza on 05/04/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#if !TARGET_OS_SIMULATOR


#import <CoreAudioKit/CoreAudioKit.h>

#include "mzgl_platform.h"
#if MZGL_IOS
#	import "MZGLKitView.h"
#endif
@interface AudioUnitViewController : AUViewController <AUAudioUnitFactory>
#if MZGL_IOS
- (MZGLKitView *)getView;
#endif
@end

#endif
