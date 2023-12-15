//
//  UIBlockButton.m
//  mzgl
//
//  Created by Marek Bereza on 22/11/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#import "UIBlockButton.h"

@implementation UIBlockButton

- (void)handleControlEvent:(UIControlEvents)event withBlock:(ActionBlock)action {
	_actionBlock = action;
	[self addTarget:self action:@selector(callActionBlock:) forControlEvents:event];
}

- (void)callActionBlock:(id)sender {
	_actionBlock();
}
@end
