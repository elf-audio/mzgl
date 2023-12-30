//
//  UIBlockButton.h
//  mzgl
//
//  Created by Marek Bereza on 22/11/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//
#pragma once

#import <UIKit/UIKit.h>
typedef void (^ActionBlock)();

@interface UIBlockButton : UIButton {
	ActionBlock _actionBlock;
}

- (void)handleControlEvent:(UIControlEvents)event withBlock:(ActionBlock)action;
@end
