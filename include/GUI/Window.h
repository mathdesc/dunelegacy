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

#ifndef WINDOW_H
#define WINDOW_H

#include "Widget.h"


#include <iostream>
#include <queue>

#include <SDL.h>

/// A class representing a window
class Window : public Widget {
public:
	/**
		Constructor for creating a window
		\param	x	x position of this window
		\param	y	y position of this window
		\param	w	width of this window
		\param	h	height of this window
	*/
	Window(Uint32 x, Uint32 y, Uint32 w, Uint32 h);

	/// destructor
	virtual ~Window();

	/**
		This method is called if a child widget is destroyed (see Widget::~Widget).
		If pChildWidget is the window widget (See setWindowWidget) then this widget
		is removed. If pChildWidget is the child window then the child window is closed.
		\param	pChildWidget	widget to remove
	*/
	virtual void removeChildWidget(Widget* pChildWidget) {
		if(pChildWidget == pWindowWidget) {
			pWindowWidget = NULL;
		} else if(pChildWidget == pChildWindow) {
			pChildWindow = NULL;
		}
	}

	/**
		Opens a child window. The new window is drawn above this window.
		\param	pChildWindow	new window
	*/
	virtual void openWindow(Window* pChildWindow);

	/**
		Closes the child window.
	*/
	virtual void closeChildWindow();

	/**
		This method is called, when the child window is about to be closed.
		This child window will be closed after this method returns.
		\param	pChildWindow	The child window that will be closed
	*/
	virtual void onChildWindowClose(Window* pChildWindow) { ; };


	/**
        This method checks whether this window has a child window.
        \return true if child window present, false otherwise
	*/
	bool hasChildWindow() const { return (pChildWindow != NULL); };

	/**
		Get the current position of this window.
		\return current position of this window
	*/
	inline const Point& getPosition() { return position; };

	/**
		Sets the current window position and size.
		\param	x	x position of this window
		\param	y	y position of this window
		\param	w	width of this window
		\param	h	height of this window
	*/
	virtual void setCurrentPosition(Uint32 x, Uint32 y, Uint32 w, Uint32 h);

	/**
		Handles the input recieved from SDL. Everytime a sdl event occures this method should
		be called.
		\param	event	SDL_Event that occures.
	*/
	virtual void handleInput(SDL_Event& event);

	/**
		Handles a mouse movement.
		\param	x x-coordinate (relative to the left top corner of the window)
		\param	y y-coordinate (relative to the left top corner of the window)
		\param  insideOverlay   true, if (x,y) is inside an overlay and this widget may be behind it, false otherwise
	*/
	virtual void handleMouseMovement(Sint32 x, Sint32 y, bool insideOverlay = false);

	/**
		Handles a left mouse click.
		\param	x x-coordinate (relative to the left top corner of the window)
		\param	y y-coordinate (relative to the left top corner of the window)
		\param	pressed	true = mouse button pressed, false = mouse button released
		\return	true = click was processed by the window, false = click was not processed by the window
	*/
	virtual bool handleMouseLeft(Sint32 x, Sint32 y, bool pressed);

	/**
		Handles a right mouse click.
		\param	x x-coordinate (relative to the left top corner of the window)
		\param	y y-coordinate (relative to the left top corner of the window)
		\param	pressed	true = mouse button pressed, false = mouse button released
		\return	true = click was processed by the window, false = click was not processed by the window
	*/
	virtual bool handleMouseRight(Sint32 x, Sint32 y, bool pressed);

	/**
		Handles mouse wheel scrolling.
		\param	x x-coordinate (relative to the left top corner of the widget)
		\param	y y-coordinate (relative to the left top corner of the widget)
		\param	up	true = mouse wheel up, false = mouse wheel down
		\return	true = the mouse wheel scrolling was processed by the widget, false = mouse wheel scrolling was not processed by the widget
	*/

	virtual bool handleMouseWheel(Sint32 x, Sint32 y, bool up);

	/**
		Handles a key stroke.
		\param	key the key that was pressed or released.
		\return	true = key stroke was processed by the window, false = key stroke was not processed by the window
	*/
	virtual bool handleKeyPress(SDL_KeyboardEvent& key);

	/**
		Draws this window to screen. This method should be called every frame.
		\param	screen	Surface to draw on
	*/
	virtual void draw(SDL_Surface* screen) { draw(screen,Point(0,0)); };

	/**
		Draws this window to screen. This method should be called every frame.
		\param	screen	Surface to draw on
		\param	position	Position to draw the window to. The position of the window is added to this.
	*/
	virtual void draw(SDL_Surface* screen, Point position);

	/**
		This method draws the parts of this window that must be drawn after all the other
		widgets are drawn (e.g. tooltips). This method is called after draw().
		\param	screen	Surface to draw on
	*/
	virtual void drawOverlay(SDL_Surface* screen) { drawOverlay(screen,Point(0,0)); };

	/**
		This method draws the parts of this window that must be drawn after all the other
		widgets are drawn (e.g. tooltips). This method is called after draw().
		\param	screen	Surface to draw on
		\param	Position	Position to draw the window to. The position of the window is added to this.
	*/
	virtual void drawOverlay(SDL_Surface* screen, Point position);

	/**
		That the current window widget. This is typically a container that hold all the widgets in this window.
		The window itself can contain only one widget.
		\param	pWindowWidget	The only widget that this window contains
	*/
	virtual inline void setWindowWidget(Widget* pWindowWidget) {
		this->pWindowWidget = pWindowWidget;
		if(this->pWindowWidget != NULL) {
			this->pWindowWidget->setParent(this);
			this->pWindowWidget->resize(getSize().x,getSize().y);
			this->pWindowWidget->setActive();
		}
	};

	/**
		Returns the current window widget.
		\return the current window widget
	*/
	virtual inline Widget* getWindowWidget() { return pWindowWidget; };

	/**
		This method resized the window to width and height.
		\param	width	the new width of this widget
		\param	height	the new height of this widget
	*/
	virtual void resize(Uint32 width, Uint32 height);

	/**
		This method is typically called by the child widget when the child widget
		requests to resizes its surrounding container. But windows do not resize if
		it's content changes.
	*/
	virtual void resizeAll() {
		// Windows do not get bigger if content changes
		resize(getSize().x,getSize().y);
	};

	/**
		Set the background of this window.
		\param	pBackground	the new background of this window. NULL=default background
		\param	bFreeBackground	should this background be automatically be freed
	*/
	virtual void setBackground(SDL_Surface* pBackground, bool bFreeBackground = true);

	/**
		This method sets a transparent background for this window.
		\param bTransparent	true = the background is transparent, false = the background is not transparent
	*/
	virtual void setTransparentBackground(bool bTransparent);

protected:

    bool processChildWindowOpenCloses();

    int    closeChildWindowCounter;             ///< Close the child window after processing all input?
	Window* pChildWindow;			            ///< The current child window
	bool pChildWindowAlreadyClosed;             /// Is the child window already closed?
	std::queue<Window*> queuedChildWindows;     ///< We cannot close child windows while they are processed. Queue any newly opened windows here until we close the current child window
	Widget* pWindowWidget;			            ///< The current window widget
	Point position;					            ///< The left top corner of this window
	bool bTransparentBackground;	            ///< true = no background is drawn
	bool bSelfGeneratedBackground;	            ///< true = background is created by this window, false = created by someone else
	bool bFreeBackground;			            ///< true = background should be automatically be freed
	SDL_Surface* pBackground;		            ///< background surface
};

#endif //WINDOW_H
