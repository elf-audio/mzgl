#import <Cocoa/Cocoa.h>

typedef void (^ButtonActionBlock)(void);

@interface NSBlockButton : NSButton

@property(nonatomic, copy) ButtonActionBlock actionBlock;

@end