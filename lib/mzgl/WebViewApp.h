//
//  WebViewApp.h
//  mzgl
//
//  Created by Marek Bereza on 16/07/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once

#include "App.h"

class WebViewApp : public App {
public:
    WebViewApp(Graphics &g) : App(g) {}
    bool isWebView() const override { return true; }
    
    virtual void receivedJSMessage(const std::string &key, const std::string &value) {}
    std::function<void(const std::string&)> callJS;

};
