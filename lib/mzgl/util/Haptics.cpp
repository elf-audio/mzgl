//
//  Haptics.h
//  mzgl
//
//  Created by Marek Bereza on 16/06/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#include "Haptics.h"
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
using namespace std;
class HapticsImpl {
	
public:
	virtual ~HapticsImpl() {}
	virtual void checkInit() {}
	virtual void lightTap() {}
};

#if TARGET_OS_IOS
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface Happer : NSObject
@end

@implementation Happer {
	UIImpactFeedbackGenerator *feedbackGenerator;
}


- (id) init {
	self = [super init];
	feedbackGenerator = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium];
	[feedbackGenerator prepare];
	return self;
}

- (void) lightTap {
	// Trigger selection feedback.
	[feedbackGenerator impactOccurred];
	printf("light tap\n");
	// Keep the generator in a prepared state.
	[feedbackGenerator prepare];
}

-(void) dealloc {
	feedbackGenerator = nil; // dunno if I need this line
}
@end

class HapticsIOS : public HapticsImpl {
public:
	Happer *happer;
	HapticsIOS() {
		happer = [[Happer alloc] init];
	}
	virtual ~HapticsIOS() {
		happer = nil;
	}
	void checkInit() override {
//		feedbackGenerator = [[UISelectionFeedbackGenerator alloc] init];


	}
	
	void lightTap() override {
		[happer lightTap];
	}
};
#endif

Haptics::Haptics() {
#if TARGET_OS_IOS
	impl = std::make_shared<HapticsIOS>();
#else
	impl = make_shared<HapticsImpl>();
#endif
}

Haptics::~Haptics() {
	
}

void Haptics::lightTap() {
	impl->lightTap();
}
