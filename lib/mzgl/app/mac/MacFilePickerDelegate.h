//
//  MacFilePickerDelegate.h
//  mzgl-macOS
//
//  Created by Marek Bereza on 22/06/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

@interface FilePickerDelegate : NSObject <NSOpenSavePanelDelegate> {
}
- (void)setAllowedExtensions:(NSArray *)extensions;
- (void)enableFoldersOnly:(BOOL)_allowFoldersOnly;
@end
