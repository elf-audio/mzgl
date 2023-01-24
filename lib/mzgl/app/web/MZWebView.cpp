//
//  MZWebView.cpp
//  mzgl
//
//  Created by Marek Bereza on 24/01/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#include "MZWebView.h"
#ifdef __APPLE__
#	include <TargetConditionals.h>
#	if TARGET_OS_IOS
#		include "iOSWebView.h"
#	else
#		include "MacWebView.h"
#	endif
#elif defined(__ANDROID__)
#	include "AndroidWebView.h"
#endif

MZWebView::MZWebView(App *app) {
#ifdef __APPLE__
#	if TARGET_OS_IOS
	impl = std::make_shared<iOSWebView>(app);
#	else
	impl = std::make_shared<MacWebView>(app);
#	endif
#elif defined(__ANDROID__)
	impl = std::make_shared<AndroidWebView>(app);
#endif
}

void MZWebView::show(const std::string &path, std::function<void()> callbacks) {
	impl->show(path, callbacks);
}

void MZWebView::callJS(const std::string &js) {
	impl->callJS(js);
}
