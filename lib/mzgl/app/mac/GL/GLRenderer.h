#pragma once

#import <Cocoa/Cocoa.h>
#import <CoreVideo/CVDisplayLink.h>
#include <memory>
#include <mutex>
#include <functional>

class EventDispatcher;

@class MZGLView;

class GLRenderer {
public:
	using RenderCallback = std::function<void()>;

	virtual ~GLRenderer() = default;

	virtual void start() = 0;
	virtual void stop()	 = 0;

	virtual bool isDrawing() const		  = 0;
	virtual void setDrawing(bool enabled) = 0;

	virtual void lock() {}
	virtual void unlock() {}

protected:
	RenderCallback renderCallback;
	bool drawing = false;
};

class CVDisplayLinkRenderer : public GLRenderer {
public:
	explicit CVDisplayLinkRenderer(MZGLView *view);
	~CVDisplayLinkRenderer() override;

	void start() override;
	void stop() override;
	bool isDrawing() const override { return drawing; }
	void setDrawing(bool enabled) override { drawing = enabled; }
	void lock() override { mutex.lock(); }
	void unlock() override { mutex.unlock(); }

	void render();

	void clearView() { view = nil; }

private:
	MZGLView *view				 = nil;
	CVDisplayLinkRef displayLink = nullptr;
	std::mutex mutex;
};

@class TimerRendererHelper;

class TimerRenderer : public GLRenderer {
public:
	explicit TimerRenderer(MZGLView *view);
	~TimerRenderer() override;

	void start() override;
	void stop() override;
	bool isDrawing() const override { return drawing; }
	void setDrawing(bool enabled) override { drawing = enabled; }

	void render();

	void clearView() { view = nil; }

private:
	MZGLView *view					 = nil;
	NSTimer *renderTimer			 = nil;
	TimerRendererHelper *timerHelper = nil;
};
