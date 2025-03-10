//
//  CustomAlertViewController.h
//  buttonDeletme
//
//  Created by Marek Bereza on 16/07/2024.
//
#import <UIKit/UIKit.h>

@interface TextboxSegmentedViewController : UIViewController

@property(nonatomic, copy) void (^completionHandler)(NSString *filename, NSInteger selectedSegment);

- (instancetype)initWithTitle:(NSString *)title
					  message:(NSString *)message
						 text:(NSString *)text
					  options:(NSArray<NSString *> *)options
					 selected:(NSInteger)selected;

@end
