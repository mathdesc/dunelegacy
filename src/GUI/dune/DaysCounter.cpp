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




#include <GUI/dune/DaysCounter.h>
#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <misc/draw_util.h>

extern GFXManager* pGFXManager;

#define CYCLE_BENDBLITT_NBSTEPS 3000

DaysCounter::DaysCounter(): Widget() {
		enableResizing(true,true);

		count = 0;
		timer = 0;
		phase = PHASE_NONE;
		bPressed = false;
		bHover = false;
		bToggleState = false;
		tooltipText = "";
		tooltipVarText = "";
		tooltipSurface = NULL;


		planet = pGFXManager->getUIGraphic(UI_PlanetDay);
		back = planet;

		currentLocation = {0,0,planet->w,planet->h};
		if (!bToggleState) {
			isIcon = false;
			iconify();
		}
		else {
			isIcon = true;
			enlarge();
		}



		next = NULL;
		dbb = NULL;
		setPhaseMode(Day);

}

DaysCounter::~DaysCounter() {
		delete dbb;
		dbb = NULL;

		if(tooltipSurface != NULL) {
			SDL_FreeSurface(tooltipSurface);
			tooltipSurface = NULL;
		};
	}


void DaysCounter::update() {
	setTooltipText(tooltipText);
	updateLocation(currentLocation);
	updateBlitter(screen);
	drawSpecificStuff(screen);
}


void DaysCounter::setTooltipText(std::string text) {
	std::string qualif,sepa, daycal;
	if (count == 1) {
		qualif = (_("st"));
	} else if (count == 2) {
		qualif = (_("nd"));
	} else {
		qualif = (_("th"));;
	}
	if (text != "" ) {
		sepa=" : ";
		tooltipText = text;
	} else {
		sepa="";
	}

	daycal= (_("on the"));

	tooltipVarText = getDayPhaseString()+" "+daycal+" "+ std::to_string(count)+ qualif+sepa+text;

	if(tooltipSurface != NULL) {
		SDL_FreeSurface(tooltipSurface);
		tooltipSurface = NULL;
	}

	if(tooltipVarText != "") {
		tooltipSurface = GUIStyle::getInstance().createToolTip(tooltipVarText);

	}
}


void DaysCounter::drawOverlay(SDL_Surface* screen, Point Pos) {
#if 1
	if(isVisible() && isEnabled()) {
		int x,y;
		SDL_Rect dest;
		SDL_GetMouseState(&x,&y);

		if (bPressed ||  bToggleState) {
			dest = {(screen->w-planet->w)/2,(screen->h-planet->h)/2,planet->w,planet->h};
			updateLocation(dest);
			drawSpecificStuff(screen);


			if(tooltipSurface != NULL) {
				tooltipLastMouseMotion = SDL_GetTicks();
				dest = { screen->w/2-tooltipSurface->w/2,
						 screen->h/2-planet->h/2,
						 tooltipSurface->w,
						 tooltipSurface->h };
				SDL_BlitSurface(tooltipSurface,NULL,screen,&dest);



			}
		} else if (!bPressed && bHover == true && !bToggleState) {
			if(tooltipSurface != NULL) {
				dest = { x, y - tooltipSurface->h, tooltipSurface->w, tooltipSurface->h };
				if(dest.x + dest.w >= screen->w) {
					// do not draw tooltip outside screen
					dest.x = screen->w - dest.w;
				}

				if(dest.y < 0) {
					// do not draw tooltip outside screen
					dest.y = 0;
				} else if(dest.y + dest.h >= screen->h) {
					// do not draw tooltip outside screen
					dest.y = screen->h - dest.h;
				}

					if((SDL_GetTicks() - tooltipLastMouseMotion) > 750) {
							SDL_BlitSurface(tooltipSurface,NULL,screen,&dest);

					}
			}
		}
	}
#endif
}

void DaysCounter::setCounterMode(Phase phase) {
	this->phase = phase;
	/*int p = ((int)phase+1)%sizeof(Phase);
	if (p == 0) p++;*/
	setPhaseMode((Phase)phase);
}

void DaysCounter::setPhaseMode(Phase phase) {
	SDL_Surface *temp;
	switch (phase) {
		case	Morning:
			next = pGFXManager->getUIGraphic(UI_PlanetMorning);
			back = next;
			next = scaleSurface(next,currentscaling,false);

			break;
		case	Day:
			next = pGFXManager->getUIGraphic(UI_PlanetDay);
			back = next;
			next = scaleSurface(next,currentscaling,false);

			break;
		case	Eve:
			next = pGFXManager->getUIGraphic(UI_PlanetEve);
			back = next;
			next = scaleSurface(next,currentscaling,false);

			break;
		case	Night:
			next = pGFXManager->getUIGraphic(UI_PlanetNight);
			back = next;
			next = scaleSurface(next,currentscaling,false);
			;
			break;
		case	PHASE_NONE:
		default:
			next = pGFXManager->getUIGraphic(UI_PlanetDay);
			back = next;
			next = scaleSurface(next,currentscaling,false);

			break;

	}

		  dbb = new DedicatedBlendBlitter (next, planet, currentLocation, CYCLE_BENDBLITT_NBSTEPS, DedicatedBlendBlitter::constructorInit::Builtin ,
						DedicatedBlendBlitter::constructorInit::Builtin, DedicatedBlendBlitter::constructorInit::Random, 256);

}

void DaysCounter::updateLocation(SDL_Rect dest) {
	//SDL_Rect dest = { currentLocation.x, currentLocation.y , planet->w, planet->h } ;
	currentLocation = dest;
	SDL_Rect blit = {0,0,planet->w,planet->h};
	if (dbb!=NULL) {
		dbb->setBlitterDestination(blit);
	}
}
void DaysCounter::drawSpecificStuff(SDL_Surface* screen) {
		SDL_BlitSurface(planet, NULL, screen, &currentLocation);
}


void DaysCounter::draw(SDL_Surface* screen, Point position) {
	int x,y;

	SDL_GetMouseState(&x,&y);

	if (!bPressed && !bToggleState)	{
		if (planet !=NULL) {
			SDL_Rect dest = { position.x, position.y-5 , planet->w, planet->h } ;
			updateLocation(dest);
			drawSpecificStuff(screen);
		}

		SDL_Surface* digitsSurface;
		if (timer != 0 ) {
				digitsSurface= pGFXManager->getUIGraphic(UI_DaysFlashingDigits);
		}
			else {
				digitsSurface= pGFXManager->getUIGraphic(UI_DaysDigits);
		}


		char daybuff[MAX_DAYS_BUFFER];
		if (count > 999) {
			sprintf(daybuff, "%d", 999);
		} else {
			sprintf(daybuff, "%d", count);
		}
		int digits = strlen(daybuff);

		for(int i=digits-1; i>=0; i--) {
			SDL_Rect source = { (daybuff[i] - '0')*(digitsSurface->w/10), 0, digitsSurface->w/10, digitsSurface->h };
			SDL_Rect dest2 = { position.x + (MAX_DAYS_BUFFER - digits + i)*10, position.y+5, digitsSurface->w/10, digitsSurface->h } ;
			SDL_BlitSurface(digitsSurface, &source, screen, &dest2);
		}
		if (timer >0) timer--;

	}
}


void DaysCounter::enlarge() {
	if (isIcon) {
		currentscaling = 1;
		planet = scaleSurface(back,currentscaling,false);
		currentLocation = {(screen->w-planet->w)/2,(screen->h-planet->h)/2,planet->w,planet->h};
		isIcon = false;
	}
}

void DaysCounter::iconify() {
	if (! isIcon) {
		currentscaling = 0.15;
		planet = scaleSurface(back,currentscaling,false);
		currentLocation.w  = planet->w;
		currentLocation.h = planet->h;
		isIcon = true;
	}
}

void DaysCounter::updateBlitter(SDL_Surface* screen) {
		if (dbb != NULL) {
			SDL_Rect r = dbb->getBlitterDestination();
			if (dbb->nextStep() == 0) {
				 delete dbb;
				 dbb = NULL;
				// printf("UpdateBlitter end of blendblitting\n");
				 //planet= next;
			}
		}
		//printf("UpdateBlitter [%d,%d,%d,%d]\n",currentLocation.x,currentLocation.y,currentLocation.w,currentLocation.h);
}


