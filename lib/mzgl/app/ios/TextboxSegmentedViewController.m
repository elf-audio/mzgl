#import "TextboxSegmentedViewController.h"

@interface TextboxSegmentedViewController ()

@property(nonatomic, strong) UILabel *titleLabel;
@property(nonatomic, strong) UITextField *textField;
@property(nonatomic, strong) UISegmentedControl *segmentedControl;
@property(nonatomic, strong) UIButton *saveButton;
@property(nonatomic, strong) UIButton *cancelButton;
@property(nonatomic, strong) NSArray<NSString *> *options;
@property(nonatomic, copy) NSString *alertTitle;
@property(nonatomic, copy) NSString *alertMessage;
@property(nonatomic, copy) NSString *defaultFieldText;
@property(nonatomic, assign) NSInteger selectedItem;
@end

@implementation TextboxSegmentedViewController

- (instancetype)initWithTitle:(NSString *)title
					  message:(NSString *)message
						 text:(NSString *)text
					  options:(NSArray<NSString *> *)options
					 selected:(NSInteger)selected {
	self = [super init];
	if (self) {
		self.alertTitle		  = title;
		self.alertMessage	  = message;
		self.defaultFieldText = text;
		self.options		  = options;
		self.selectedItem	  = selected;
	}
	return self;
}

- (void)viewDidLoad {
	[super viewDidLoad];

	self.view.backgroundColor = [UIColor colorWithWhite:0.0 alpha:0.5];

	UIView *alertView = [[UIView alloc] initWithFrame:CGRectZero];

	alertView.layer.cornerRadius						= 12.0;
	alertView.translatesAutoresizingMaskIntoConstraints = NO;
	alertView.layer.shadowColor							= [UIColor blackColor].CGColor;
	alertView.layer.shadowOpacity						= 0.5;
	alertView.layer.shadowOffset						= CGSizeMake(0, 2);
	alertView.layer.shadowRadius						= 10.0;
	alertView.layer.masksToBounds						= NO;

	[self.view addSubview:alertView];

	[NSLayoutConstraint activateConstraints:@[
		[alertView.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
		[alertView.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
		[alertView.widthAnchor constraintEqualToConstant:300],
		[alertView.heightAnchor constraintGreaterThanOrEqualToConstant:200]
	]];

	self.titleLabel											  = [[UILabel alloc] init];
	self.titleLabel.text									  = self.alertTitle;
	self.titleLabel.textAlignment							  = NSTextAlignmentCenter;
	self.titleLabel.font									  = [UIFont boldSystemFontOfSize:18];
	self.titleLabel.translatesAutoresizingMaskIntoConstraints = NO;

	[alertView addSubview:self.titleLabel];

	if (@available(iOS 13.0, *)) {
		// dark mode compatibility
		alertView.backgroundColor = [UIColor secondarySystemBackgroundColor];
		self.titleLabel.textColor = [UIColor labelColor];
		//		self.messageLabel.textColor = [UIColor labelColor];
	} else {
		alertView.backgroundColor = [UIColor whiteColor];
		self.titleLabel.textColor = [UIColor blackColor];
	}

	self.textField											 = [[UITextField alloc] init];
	self.textField.placeholder								 = @"Enter filename";
	self.textField.borderStyle								 = UITextBorderStyleRoundedRect;
	self.textField.translatesAutoresizingMaskIntoConstraints = NO;
	self.textField.text 									 = self.defaultFieldText;
	[alertView addSubview:self.textField];

	self.segmentedControl					   = [[UISegmentedControl alloc] initWithItems:self.options];
	self.segmentedControl.selectedSegmentIndex = self.selectedItem;
	self.segmentedControl.translatesAutoresizingMaskIntoConstraints = NO;
	[alertView addSubview:self.segmentedControl];

	self.saveButton = [UIButton buttonWithType:UIButtonTypeSystem];
	[self.saveButton setTitle:@"Save" forState:UIControlStateNormal];
	[self.saveButton addTarget:self
						action:@selector(saveButtonTapped)
			  forControlEvents:UIControlEventTouchUpInside];
	self.saveButton.translatesAutoresizingMaskIntoConstraints = NO;
	[alertView addSubview:self.saveButton];

	self.cancelButton = [UIButton buttonWithType:UIButtonTypeSystem];
	[self.cancelButton setTitle:@"Cancel" forState:UIControlStateNormal];

	[self.cancelButton addTarget:self
						  action:@selector(cancelButtonTapped)
				forControlEvents:UIControlEventTouchUpInside];
	self.cancelButton.translatesAutoresizingMaskIntoConstraints = NO;
	[alertView addSubview:self.cancelButton];

	self.cancelButton.backgroundColor	 = [UIColor colorWithWhite:1 alpha:0.0625];
	self.cancelButton.layer.cornerRadius = 8.0;

	self.saveButton.layer.cornerRadius = self.cancelButton.layer.cornerRadius;
	self.saveButton.backgroundColor	   = self.cancelButton.backgroundColor;

	[NSLayoutConstraint activateConstraints:@[
		[self.titleLabel.topAnchor constraintEqualToAnchor:alertView.topAnchor constant:20],
		[self.titleLabel.leadingAnchor constraintEqualToAnchor:alertView.leadingAnchor constant:20],
		[self.titleLabel.trailingAnchor constraintEqualToAnchor:alertView.trailingAnchor constant:-20],

		[self.textField.topAnchor constraintEqualToAnchor:self.titleLabel.bottomAnchor constant:20],

		[self.textField.leadingAnchor constraintEqualToAnchor:alertView.leadingAnchor constant:20],
		[self.textField.trailingAnchor constraintEqualToAnchor:alertView.trailingAnchor constant:-20],

		[self.segmentedControl.topAnchor constraintEqualToAnchor:self.textField.bottomAnchor constant:20],
		[self.segmentedControl.leadingAnchor constraintEqualToAnchor:alertView.leadingAnchor constant:20],
		[self.segmentedControl.trailingAnchor constraintEqualToAnchor:alertView.trailingAnchor constant:-20],

		[self.cancelButton.topAnchor constraintEqualToAnchor:self.segmentedControl.bottomAnchor constant:20],
		[self.cancelButton.leadingAnchor constraintEqualToAnchor:alertView.leadingAnchor constant:20],
		[self.cancelButton.bottomAnchor constraintEqualToAnchor:alertView.bottomAnchor constant:-20],

		[self.saveButton.topAnchor constraintEqualToAnchor:self.segmentedControl.bottomAnchor constant:20],
		[self.saveButton.trailingAnchor constraintEqualToAnchor:alertView.trailingAnchor constant:-20],
		[self.saveButton.bottomAnchor constraintEqualToAnchor:alertView.bottomAnchor constant:-20],

		[self.cancelButton.trailingAnchor constraintEqualToAnchor:self.saveButton.leadingAnchor constant:-20],
		[self.cancelButton.widthAnchor constraintEqualToAnchor:self.saveButton.widthAnchor]
	]];

	[self addKeyboardObservers];
	[self.textField becomeFirstResponder];
}

- (void)addKeyboardObservers {
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(keyboardWillShow:)
												 name:UIKeyboardWillShowNotification
											   object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(keyboardWillHide:)
												 name:UIKeyboardWillHideNotification
											   object:nil];
}
- (void)keyboardWillShow:(NSNotification *)notification {
	NSDictionary *userInfo			 = notification.userInfo;
	CGRect keyboardFrame			 = [userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
	NSTimeInterval animationDuration = [userInfo[UIKeyboardAnimationDurationUserInfoKey] doubleValue];

	[UIView animateWithDuration:animationDuration
					 animations:^{
					   // Adjust the bottom constraint of the alertView
					   self.view.transform = CGAffineTransformMakeTranslation(0, -keyboardFrame.size.height / 2);
					 }];
}

- (void)keyboardWillHide:(NSNotification *)notification {
	NSDictionary *userInfo			 = notification.userInfo;
	NSTimeInterval animationDuration = [userInfo[UIKeyboardAnimationDurationUserInfoKey] doubleValue];

	[UIView animateWithDuration:animationDuration
					 animations:^{
					   // Reset the view's transform
					   self.view.transform = CGAffineTransformIdentity;
					 }];
}
- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}
- (void)saveButtonTapped {
	if (self.completionHandler) {
		self.completionHandler(self.textField.text, self.segmentedControl.selectedSegmentIndex);
	}
	[self dismissViewControllerAnimated:YES completion:nil];
}

- (void)cancelButtonTapped {
	[self dismissViewControllerAnimated:YES completion:nil];
}

@end
