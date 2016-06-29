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

#ifndef DAYSCOUNTER_H
#define DAYSCOUNTER_H

#include <GUI/Widget.h>

#include <FileClasses/GFXManager.h>
#include <misc/functional.h>
#include <SoundPlayer.h>
#include <misc/DedicatedBlendBlitter.h>
#include <GUI/GUIStyle.h>
#include <FileClasses/TextManager.h>


extern GFXManager* pGFXManager;

#define MAX_DAYS_BUFFER 4
typedef enum {
		Morning = 1,
		Day = 2,
        PHASE_NONE = 0 ,	// symmetry line
        Eve = 3,
        Night = 4,
		PHASE_END = 5
} Phase;
#define PHASES_NB (PHASE_END-1)

/// A widget for showing digits (like the credits in dune are shown)
class DaysCounter : public Widget
{
public:
	void enlarge() ;

	void iconify() ;

	/// default constructor
	DaysCounter() ;
	/// destructor
	 ~DaysCounter() ;

	/**
		Get the current count of this digits counter
		\return	the number that this digits counter currently shows
	*/
	inline unsigned int getCount() { return count; }

	/**
		Set the count of this digits counter
		\param	newCount	the new number to show
	*/
	inline void setCount(unsigned int newCount) {
		count = newCount;
		timer=9000;
	}


	void handleMouseMovement(Sint32 x, Sint32 y, bool insideOverlay) {

			if((x < 0) || (x >= getSize().x) || (y < 0) || (y >= getSize().y)) {
				bPressed = false;
				bHover = false;
			} else if(isEnabled() && !insideOverlay) {
				bHover = true;
				tooltipLastMouseMotion = SDL_GetTicks();
			}


			setToggleState(false);
			iconify();

	}

	void drawOverlay(SDL_Surface* screen, Point Pos) ;

    /**
        This method sets the counter mode directly.
        \param phase Phase of the day
	*/
	void setCounterMode(Phase phase);
	void setPhaseMode(Phase phase);


	void updateLocation(SDL_Rect dest) ;
	virtual void drawSpecificStuff(SDL_Surface* screen) ;

	/**
		Draws this widget to screen. This method is called before drawOverlay().
		\param	screen	Surface to draw on
		\param	position	Position to draw the widget to
	*/
	virtual void draw(SDL_Surface* screen, Point position);

	/**
		Returns the minimum size of this digits counter. The widget should not
		be resized to a size smaller than this.
		\return the minimum size of this digits counter
	*/
	virtual Point getMinimumSize() const {
		if(planet != NULL) {
			return Point((Sint32) planet->w, (Sint32) planet->h);
		} else {
			return Point(0,0);
		}
	}

	/**
		Sets a tooltip text. This text is shown when the mouse remains a short time over this widget.
		\param	text	The text for this tooltip
	*/
	inline void setTooltipText(std::string text);


	/**
		Returns the current tooltip text.
		\return	the current tooltip text
	*/
	inline std::string getTooltipText() {
		return tooltipVarText;
	}


	/**
		Sets the function that should be called when this button is clicked on.
		\param	pOnClick	A function to call on click
	*/
	inline void setOnClick(std::function<void ()> pOnClick) {
		this->pOnClick = pOnClick;
	}

	bool handleMouseLeft(Sint32 x, Sint32 y, bool pressed) {
		if((x < 0) || (x >= getSize().x) || (y < 0) || (y >= getSize().y)) {
			return false;
		}

		if((isEnabled() == false) || (isVisible() == false)) {
			return true;
		}

		if(pressed == true) {
			// button pressed
			bPressed = true;

				//soundPlayer->playSound(ButtonClick);
			enlarge();

		} else {
			// button released
			if(bPressed == true) {
				bPressed = false;
				bool oldState = getToggleState();
				setToggleState(!bToggleState);
				if(getToggleState() != oldState) {
					//soundPlayer->playSound(ButtonClick);
				}
				 if (!bToggleState) {
					 				iconify();

				 }
			}
		}
		return true;
	}


	/**
		This method sets the current toggle state. If this button is no
		toggle button this method has no effect.
		\param bToggleState	true = toggled, false = untoggled
	*/
	virtual void setToggleState(bool bToggleState) {
			this->bToggleState = bToggleState;
	}

	/**
		This method returns whether this button is currently toggled or not.
		\return	true = toggled, false = untoggled
	*/
	inline bool getToggleState() const {
		return bToggleState;
	}

	void updateBlitter(SDL_Surface* screen) ;

	virtual void update ();



private:

	unsigned int count;
	int timer;
	Phase phase;
	SDL_Surface *back,*planet, *next;
	double currentscaling;
	DedicatedBlendBlitter *dbb;
	SDL_Rect currentLocation;

	bool isIcon;


protected :
	std::string tooltipVarText;			///< the tooltip variable text
	std::string tooltipText;			///< the tooltip text
	SDL_Surface* tooltipSurface;		///< the tooltip surface
	Uint32 tooltipLastMouseMotion;		///< the last time the mouse was moved

	bool bToggleState;					///< true = currently toggled, false = currently not toggled
	bool bPressed;						///< true = currently pressed, false = currently unpressed
	bool bHover;						///< true = currently mouse hover, false = currently no mouse hover
	std::function<void ()> pOnClick;	///< function that is called when this button is clicked

	// FIXME : Already declared in Game.h (a mess of forward declaration)
    inline std::string getDayPhaseString() {
    	switch (phase) {
    		case	Morning:
    			return (_("Morning"));
    			break;
    		case	Day:
    			return (_("Day"));
    			break;
    		case	Eve:
    			return (_("Eve"));
    			break;
    		case	Night:
    			return (_("Night"));
    			break;
    		case	PHASE_NONE:
    		default:
    			return (_("None"));
    			break;

    	}
    };




};

#endif // DIGITSCOUNTER_H
