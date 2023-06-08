//
//  GainMeter.h
//  mzgl
//
//  Created by Marek Bereza on 08/06/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#pragma once


class GainMeter : public Layer {
public:
	float maxDB = 6;
	float minDB = -90;
	
	GainMeter(Graphics &g, UIResources &R, LevelMeter &meter) : Layer(g), R(R), meter(meter) {
	}
	
	void draw() override {
		
		g.fill();
		
		Rectf r = *this;
		
		const float meterWidth = round(width * 0.30);
		const float meterSpacing = round(width * 0.08);
		// draw blue meter bars
		
		const float dbL = rms2db(meter.getLevelL());
		const float dbR = rms2db(meter.getLevelR());
		const float maxDbL = rms2db(meter.getMaxL());
		const float maxDbR = rms2db(meter.getMaxR());
		const float maxL = db2y(maxDbL);
		const float maxR = db2y(maxDbR);
		
		
		{

			

			// left channel
			Rectf barRect = r;
			
			barRect.x = r.x + meterSpacing;
			barRect.width = meterWidth;
			
			barRect.y = db2y(dbL);
			barRect.setBottomEdge(r.bottom());
		
			g.setColor(dbL>0?R.pink:R.blue);
			g.draw(barRect);
			
			if(maxL<r.bottom()-2) {
				g.setColor(maxDbL>0?R.pink:R.blue);
				g.drawRect(barRect.x-4, maxL-2, barRect.width+8, 4);
			}
			
			// right channel
			barRect.x = barRect.right() + meterSpacing;
			
			barRect.y = db2y(dbR);
			barRect.setBottomEdge(r.bottom());
			
			g.setColor(dbR>0?R.pink:R.blue);
			g.draw(barRect);
			
			if(maxR<r.bottom()-2) {
				g.setColor(maxDbR>0?R.pink:R.blue);
				g.drawRect(barRect.x-4, maxR-2, barRect.width+8, 4);
			}
		}

		g.setColor(1);

		drawDBMarking(0);
//		drawDBMarking(3);
//		drawDBMarking(6);
//		drawDBMarking(-6);
		drawDBMarking(-12);
//		drawDBMarking(-18);
		drawDBMarking(-24);
//		drawDBMarking(-30);
		drawDBMarking(-36);
//		drawDBMarking(-42);
//		drawDBMarking(-48);
		drawDBMarking(-54);
//		drawDBMarking(-60);
		
		
		g.fill();
		ScopedAlphaBlend bl(g, true);
		g.setColor(1);
		fg->draw(g);
		
		
		auto dbStr = to_string(std::max(maxDbL, maxDbR), 2) + " dB";
		R.smallFont.draw(g, dbStr, vec2(x + width*0.5, y + height - R.buttonHeight/2), HTextAlign::Centre, VTextAlign::Centre);
	}
	
	void doLayout() override {
		
		Rectf r = *this;
		Drawer d;
		d.strokeWeight = R.strokeWeight;
		
		g.noFill();
		g.setColor(1);
		d.drawRect(r);
		d.setColor(1, 1, 1, 0.5);
		float db0 = db2y(0);
		d.drawLine(r.x, db0, r.right(), db0);
		fg = d.createVbo();
	}
	
	VboRef fg;
	
	
	
	
	
	
private:
	
	float db2y(float db) {
		// could be optimized - double mapf wasteful
		float norm = mapf(db, maxDB, minDB, 1, 0, true);
		norm = pow(norm, 2.f);
		return mapf(norm, 1, 0, y-R.offset.y, bottom()-R.offset.y);
	}
	
	float y2db(float yy) {
		// could be optimized - double mapf wasteful
		float norm =  mapf(yy-R.offset.y, y, bottom(), 1, 0, true);
		norm = pow(norm, 1.f/2.f);
		return mapf(norm, 1, 0, maxDB, minDB);
	}
	
	void drawDBMarking(float db) {
		
		// todo: draw these all at once like eq
		float yy = db2y(db);
		auto str = to_string(std::abs(db), 0);
//		if(db>0) str = "+" + str;
		g.setColor(1, 1, 1, 0.5);
		R.smallFont.setScale(0.75);
		R.smallFont.draw(g, str, vec2(width*0.75, yy), HTextAlign::Left, VTextAlign::Centre);
		R.smallFont.setScale(1);
	}
	
	UIResources &R;
	
	float startY = 0;
	float startVal = 0.f;
	LevelMeter &meter;
};

