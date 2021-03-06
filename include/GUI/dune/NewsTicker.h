/*
 *  This file is part of Dune Legacy.
 *
 *  Dune Legacy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dune Legacy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dune Legacy.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NEWSTICKER_H
#define NEWSTICKER_H

#include <GUI/Widget.h>

#include <string>
#include <queue>
#include <math.h>
#include <misc/draw_util.h>

#include <SDL.h>

class NewsTicker : public Widget {
public:
	NewsTicker();
	virtual ~NewsTicker();

    bool hasMessage() const { return !messages.empty(); };
	void addMessage(const std::string& msg);
	void addUrgentMessage(const std::string& msg);

	/**
		Draws this button to screen. This method is called before drawOverlay().
		\param	screen	Surface to draw on
		\param	position	Position to draw the button to
	*/
	void draw(SDL_Surface* screen, Point position);

	/**
		Returns the minimum size of this widget. The widget should not
		be resized to a size smaller than this.
		\return the minimum size of this widget
	*/
	virtual Point getMinimumSize() const {
		if(pBackground != NULL) {
			return Point((Sint32) 180, (Sint32) pBackground->h);
		} else {
			return Point(0,0);
		}
	}


	virtual void resize(Uint32 _width, Uint32 _height) ;

private:
	SDL_Surface* pBackground;
	std::queue<std::string> messages;
	int timer;
};

#endif //NEWSTICKER_H
