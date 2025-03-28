//
//  Haptics.h
//  mzgl
//
//  Created by Marek Bereza on 16/06/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#include "Haptics.h"
#ifdef __APPLE__
#	include <TargetConditionals.h>
#endif
using namespace std;
class HapticsImpl {
public:
	virtual ~HapticsImpl() {}
	virtual void checkInit() {}
	virtual void lightTap() {}
};

#if TARGET_OS_IOS
#	import <Foundation/Foundation.h>
#	import <UIKit/UIKit.h>
#	include <AVFAudio/AVFAudio.h>
@interface Happer : NSObject
@end

@implementation Happer {
	UISelectionFeedbackGenerator *feedbackGenerator;
	// use UIImpactFeedbackGenerator for more different types of haptics
}

- (id)init {
	self = [super init];

	AVAudioSession *audioSession = [AVAudioSession sharedInstance];

	// in order to have haptics on iOS...
	// you have to do something like this in your audio system - make sure you do
	// it after all your other AVAudioSession configuration or it may not work.
	//	NSError * err = nil;
	//	if (@available(iOS 13.0, *)) {
	//		[audioSession setAllowHapticsAndSystemSoundsDuringRecording:YES error:&err];
	//		if(err != nil) {
	//			NSLog(@"Haptics couldn't be enabled because %@", err);
	//			err = nil;
	//		}
	//	}
	//

	feedbackGenerator = [[UISelectionFeedbackGenerator alloc] init]; //WithStyle:UIImpactFeedbackStyleLight];
	[feedbackGenerator prepare];
	return self;
}

- (void)lightTap {
	// Trigger selection feedback.
	[feedbackGenerator selectionChanged];
	//	printf("light tap\n");
	// Keep the generator in a prepared state.
	[feedbackGenerator prepare];
}

- (void)dealloc {
	feedbackGenerator = nil; // dunno if I need this line
}
@end

class HapticsIOS : public HapticsImpl {
public:
	Happer *happer;
	HapticsIOS() { happer = [[Happer alloc] init]; }
	virtual ~HapticsIOS() { happer = nil; }
	void checkInit() override {
		//		feedbackGenerator = [[UISelectionFeedbackGenerator alloc] init];
	}

	void lightTap() override { [happer lightTap]; }
};
#elif TARGET_OS_MAC==1
#	include <AppKit/AppKit.h>

class HapticsMac : public HapticsImpl {
	void lightTap() {
		[[NSHapticFeedbackManager defaultPerformer] performFeedbackPattern:NSHapticFeedbackPatternAlignment
														   performanceTime:NSHapticFeedbackPerformanceTimeNow];
	}
};
#endif
Haptics::Haptics() {
#if TARGET_OS_IOS
	impl = std::make_shared<HapticsIOS>();
#elif TARGET_OS_MAC
	impl = std::make_shared<HapticsMac>();
#else
	impl = make_shared<HapticsImpl>();
#endif
}

Haptics::~Haptics() {
}

void Haptics::lightTap() {
	impl->lightTap();
}
