#import "NSBlockButton.h"

@implementation NSBlockButton

- (void)setActionBlock:(ButtonActionBlock)actionBlock {
	_actionBlock = [actionBlock copy];
	[self setTarget:self];
	[self setAction:@selector(executeActionBlock)];
}

- (void)executeActionBlock {
	if (self.actionBlock) {
		self.actionBlock();
	}
}

@end