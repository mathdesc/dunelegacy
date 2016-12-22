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

#include <GUI/dune/NewsTicker.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/FontManager.h>

#define MESSAGETIME 440
#define SLOWDOWN timer/55

NewsTicker::NewsTicker() : Widget() {
	enableResizing(true,false);

 	timer = -MESSAGETIME/2;
	pBackground = copySurface(pGFXManager->getUIGraphic(UI_MessageBox));

	resize(pBackground->w,pBackground->h);
}

NewsTicker::~NewsTicker() {

	if (pBackground != NULL) {
		SDL_FreeSurface(pBackground);
	}

}

void NewsTicker::addMessage(const std::string& msg)
{
	bool found = false;

	/*if message is already there, do nothing*/
	std::queue<std::string> msgcpy(messages);
	while(!msgcpy.empty()) {
		if(msgcpy.front() == msg) {
			found = true;
		}
	    msgcpy.pop();
	}

	if(!found && messages.size() < 3) {
        messages.push(msg);
    }
}

void NewsTicker::addUrgentMessage(const std::string& msg)
{
	while(!messages.empty()) {
		messages.pop();
	}

	messages.push(msg);
}

void NewsTicker::resize(Uint32 _width, Uint32 _height) {
	int width  = (int) _width;
	int height = (int) _height;

	double ratiox = (double)width/pBackground->w;
	double ratioy = (double)height/pBackground->h;


	SDL_Surface * temp;

    if (pBackground->w*ratiox >= getMinimumSize().x && pBackground->h*ratioy >= getMinimumSize().y && (ratiox != 1.0 || ratioy != 1.0))
	{
    	//printf("resize : %i;%i (%i,%i)\t",(int)_width,(int)_height,pBackground->w,pBackground->h);
    	temp = scaleSurface(pBackground,ratiox, ratioy,false);
    	/*if (temp == NULL ) printf("ATT! pBack is null !\n");*/
    	SDL_FreeSurface(pBackground);
    	/*else */ pBackground = temp;
    	Widget::resize(_width,_height);
	} else	Widget::resize(pBackground->w,pBackground->h);
}

void NewsTicker::draw(SDL_Surface* screen, Point position) {
	if(isVisible() == false) {
		return;
	}

	//draw background
	if(pBackground == NULL) {
			return;
	}

	SDL_Rect dest = { position.x, position.y, pBackground->w, pBackground->h };
	SDL_BlitSurface(pBackground,NULL,screen,&dest);

	// draw message
	if(!messages.empty()) {
		if(timer++ == (MESSAGETIME/3)) {
			timer = -MESSAGETIME/2;
			// delete first message
            messages.pop();

			// if no more messages leave
			if(messages.empty()) {
				timer = -MESSAGETIME/2;
				return;
			};
		};

		//draw text
		SDL_Rect textLocation = { position.x +  10, position.y + 5, 0, 0 };
		if(timer>0) {
			textLocation.y -= SLOWDOWN;
		}


		SDL_Surface *surface = pFontManager->createSurfaceWithText(messages.front(), COLOR_BLACK, FONT_STD10);
		SDL_Rect cut = { 0, 0, 0, 0 };
		if(timer>0) {
			cut.y = 3*SLOWDOWN;
		}

		if (surface->w > pBackground->w ) {
			cut.w = pBackground->w - 10;
		} else {
			cut.w = surface->w;
		}
		cut.h = surface->h - cut.y;

		SDL_BlitSurface(surface, &cut, screen, &textLocation);
		SDL_FreeSurface(surface);
	};
}
