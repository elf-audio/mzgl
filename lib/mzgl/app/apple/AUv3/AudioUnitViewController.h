//
//  AudioUnitViewController.h
//  MZGLiOSEffectAU
//
//  Created by Marek Bereza on 05/04/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#import <CoreAudioKit/CoreAudioKit.h>

#include <TargetConditionals.h>
#if TARGET_OS_IOS
#	import "MZGLKitView.h"
#endif
@interface AudioUnitViewController : AUViewController <AUAudioUnitFactory>
#if TARGET_OS_IOS
- (MZGLKitView *)getView;
#endif
@end
