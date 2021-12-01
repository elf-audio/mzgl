//
//  AudioUnitViewController.h
//  MZGLiOSEffectAU
//
//  Created by Marek Bereza on 05/04/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#import <CoreAudioKit/CoreAudioKit.h>
#import "MZGLKitView.h"

@interface AudioUnitViewController : AUViewController <AUAudioUnitFactory>
-(MZGLKitView*) getView;
@end
