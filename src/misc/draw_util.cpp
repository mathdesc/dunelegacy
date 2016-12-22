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

#include <misc/draw_util.h>

#include <globals.h>

#include <stdexcept>
#include <math.h>
#include <misc/strictmath.h>


Uint32 getPixel(SDL_Surface *surface, int x, int y) {
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + (y * surface->pitch) + (x * bpp);

    switch(bpp) {
    case 1:
        return *p;

    case 2:
        return *(Uint16 *)p;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
        return *(Uint32 *)p;

    default:
        throw std::runtime_error("getPixel(): Invalid bpp value!");
    }
}


void putPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	if(x >= 0 && x < surface->w && y >=0 && y < surface->h) {
		int bpp = surface->format->BytesPerPixel;
		/* Here p is the address to the pixel want to set */
		Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

		switch(bpp) {
		case 1:
			*p = color;
			break;

		case 2:
			*(Uint16 *)p = color;
			break;

		case 3:
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
				p[0] = (color>> 16) & 0xff;
				p[1] = (color>> 8) & 0xff;
				p[2] = color& 0xff;
			} else {
				p[0] = color& 0xff;
				p[1] = (color>> 8) & 0xff;
				p[2] = (color>> 16) & 0xff;
			}
			break;

		case 4:
			*(Uint32 *)p = color;
			break;
		}
	}
}

void drawLineNoLock(SDL_Surface *surface, int x0, int y0, int x1, int y1, Uint32 color) {

#if 0
		int i;
		double x = x1 - x0;
		double y = y1 - y0;
		double length = sqrt( x*x + y*y );
		double addx = x / length;
		double addy = y / length;
		x = x0;
		y = y0;

		for ( i = 0; i < length; i += 1) {
			putPixel(surface, x, y, color);
			x += addx;
			y += addy;

		}
#else
		int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
		int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
		int err = dx+dy, e2; /* error value e_xy */

		for(;;){  /* loop */
		  putPixel(surface,x0,y0,color);
		  if (x0==x1 && y0==y1) break;
		  e2 = 2*err;
		  if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
		  if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
		}
#endif
}

void drawLine(SDL_Surface *surface, int x0, int y0, int x1, int y1, Uint32 color) {

#if 0
	if(!SDL_MUSTLOCK(surface) || (SDL_LockSurface(surface) == 0)) {
		int i;
		double x = x1 - x0;
		double y = y1 - y0;
		double length = sqrt( x*x + y*y );
		double addx = x / length;
		double addy = y / length;
		x = x0;
		y = y0;

		for ( i = 0; i < length; i += 1) {
			putPixel(surface, x, y, color);
			x += addx;
			y += addy;

		}

		if (SDL_MUSTLOCK(surface)) {
					SDL_UnlockSurface(surface);
		}
	}
#else
	if(!SDL_MUSTLOCK(surface) || (SDL_LockSurface(surface) == 0)) {


		int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
		int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
		int err = dx+dy, e2; /* error value e_xy */

		for(;;){  /* loop */
		  putPixel(surface,x0,y0,color);
		  if (x0==x1 && y0==y1) break;
		  e2 = 2*err;
		  if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
		  if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
		}


		if (SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
	}
#endif
}


void drawArrowLine(SDL_Surface *surface, int x0, int y0, int x1, int y1, Uint32 colorline,Uint32 colorarrow, int arrowsize) {

	 drawLine(surface,  x0,  y0,  x1,  y1,  colorline);
	 drawArrow(screen,  x0,  y0,  x1,  y1,  arrowsize, colorarrow);

}


void drawHLineNoLock(SDL_Surface *surface, int x1, int y, int x2, Uint32 color) {
	int	min = x1;
	int	max = x2;

	if(min > max) {
		int temp = max;
		max = min;
		min = temp;
	}

	for(int i = min; i <= max; i++) {
		putPixel(surface, i, y, color);
	}
}


void drawVLineNoLock(SDL_Surface *surface, int x, int y1, int y2, Uint32 color) {
	int	min = y1;
	int	max = y2;

	if(min > max) {
		int temp = max;
		max = min;
		min = temp;
	}

	for(int i = min; i <= max; i++) {
		putPixel(surface, x, i, color);
	}
}


void drawHLine(SDL_Surface *surface, int x1, int y, int x2, Uint32 color) {
	if(!SDL_MUSTLOCK(surface) || (SDL_LockSurface(surface) == 0)) {
		drawHLineNoLock(surface, x1, y, x2, color);

		if(SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
	}
}


void drawVLine(SDL_Surface *surface, int x, int y1, int y2, Uint32 color) {
	if(!SDL_MUSTLOCK(surface) || (SDL_LockSurface(surface) == 0)) {
		drawVLineNoLock(surface, x, y1, y2, color);

		if (SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
	}
}


void drawRect(SDL_Surface *surface, int x1, int y1, int x2, int y2, Uint32 color) {
	if(!SDL_MUSTLOCK(surface) || (SDL_LockSurface(surface) == 0)) {
		int	min = x1;
		int max = x2;

		if(min > max) {
			int temp = max;
			max = min;
			min = temp;
		}

		for(int i = min; i <= max; i++) {
			putPixel(surface, i, y1, color);
			putPixel(surface, i, y2, color);
		}

		min = y1+1;
		max = y2;
		if(min > max) {
			int temp = max;
			max = min;
			min = temp;
		}

		for(int j = min; j < max; j++) {
			putPixel(surface, x1, j, color);
			putPixel(surface, x2, j, color);
		}

		if(SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
	}
}


void drawCircle (SDL_Surface *surface, int x, int y ,int radius, Uint32 color, bool fill) {

#if 0
	if(!SDL_MUSTLOCK(surface) || (SDL_LockSurface(surface) == 0)) {

		float x2, y2;
		float angle = 0.0;
		float angle_stepsize = 0.025;

		// go through all angles from 0 to 2 * PI radians
		while (angle < 2 * strictmath::pi)
		{
			// calculate x, y from a vector with known length and angle
			x2 = radius * cos (angle);
			y2 = radius * sin (angle);

			putPixel (surface, ceilf(((float)x)+x2) , ceilf(((float)y)+y2) , color);
			angle += angle_stepsize;
		}

		if(SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
	}
#else
	if(!SDL_MUSTLOCK(surface) || (SDL_LockSurface(surface) == 0)) {
		int _x = radius;
		int _y = 0;
		int err = 0;

		while (_x >= _y)
		{
			putPixel(surface, x + _x, y + _y, color);
			putPixel(surface, x + _y, y + _x, color);
			putPixel(surface, x - _y, y + _x, color);
			putPixel(surface, x - _x, y + _y, color);
			putPixel(surface, x - _x, y - _y, color);
			putPixel(surface, x - _y, y - _x, color);
			putPixel(surface, x + _y, y - _x, color);
			putPixel(surface, x + _x, y - _y, color);

			_y += 1;
			err += 1 + 2*_y;
			if (2*(err-_x) + 1 > 0)
			{
				_x -= 1;
				err += 1 - 2*_x;
			}
		}


		if (fill) {

			int r2 = radius * radius;
			int area = r2 << 2;
			int rr = radius << 1;

			for (int i = 0; i < area; i++)
			{
			    int tx = (i % rr) - radius;
			    int ty = (i / rr) - radius;

			    if (tx * tx + ty * ty <= r2)
			        putPixel(surface,x + tx, y + ty, color);
			}
		}

		if(SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
	}
#endif
}

void drawArrow(SDL_Surface *surface,int x0,int y0, int x1, int y1,int z, Uint32 color) {

	int a = ceilf((2*z) / sqrt(3));


	// downrightward arrow
	if (x0 < x1 && y0 < y1)
	drawTrigon(surface, x1 ,y1-a, x1, y1, x1-a, y1, color);
	// downleftward arrow
	if (x0 > x1 && y0 < y1)
	drawTrigon(surface, x1 ,y1-a, x1, y1, x1+a, y1, color);
	// uprightward arrow
	if (x0 < x1 && y0 > y1)
	drawTrigon(surface, x1-a ,y1, x1, y1, x1, y1+a, color);
	// upleftward arrow
	if (x0 > x1 && y0 > y1)
	drawTrigon(surface, x1+a ,y1, x1, y1, x1, y1+a, color);

	// rightward arrow -->
	if (x0 < x1 && y0 == y1)
		drawTrigon(surface, x1-a ,y1-a, x1, y1, x1-a, y1+a, color);
	// leftward arrow <--
	if (x0 > x1 && y0 == y1)
		drawTrigon(surface, x1+a ,y1-a, x1, y1, x1+a, y1+a, color);
	// downward arrow
	if (x0 == x1 && y0 < y1)
		drawTrigon(surface, x1+a ,y1-a, x1, y1, x1-a, y1-a, color);
	// upward arrow
	if (x0 == x1 && y0 > y1)
		drawTrigon(surface, x1+a ,y1+a, x1, y1, x1-a, y1+a, color);
}


void drawTrigon(SDL_Surface *surface, int x1, int y1, int x2, int y2, int x3, int y3, Uint32 color) {

	if (x1 == x2) {
		drawVLine(surface, x1, y1, y2, color);
	} else if (y1 == y2) {
		drawHLine(surface, x1, y1, x2, color);
	} else {
		drawLine(surface, x1, y1, x2, y2, color);
	}

	if (x2 == x3) {
		drawVLine(surface, x2, y2, y3, color);
	} else if (y2 == y3) {
		drawHLine(surface, x2, y2, x3, color);
	} else {
		drawLine(surface, x2, y2, x3, y3, color);
	}

	if (x3 == x1) {
		drawVLine(surface, x3, y3, y1, color);
	} else if (y3 == y1) {
		drawHLine(surface, x3, y3, x1, color);
	} else {
		drawLine(surface, x3, y3, x1, y1, color);
	}


}



SDL_Color inverse(const SDL_Color& color) {
  SDL_Color inverse;
  inverse.r = sizeof(Uint8)*255 - color.r;
  inverse.g = sizeof(Uint8)*255 - color.g;
  inverse.b = sizeof(Uint8)*255 - color.b;
  return inverse;
}


void invertColors (SDL_Surface* surface) {
	if(!SDL_MUSTLOCK(surface) || (SDL_LockSurface(surface) == 0)) {

		Uint8 ck_r,ck_g,ck_b;

		SDL_GetRGB(surface->format->colorkey,screen->format, &ck_r,&ck_g,&ck_b);
        for(int y = 0; y < surface->h; y++) {
            Uint8 *p = (Uint8 *)surface->pixels + (y * surface->pitch);

            for(int x = 0; x < surface->w; x++, ++p) {
            	if (*p != 0 && (*p != ck_r && *p+1 != ck_g && *p+2 != ck_b))
                    *p = sizeof(Uint8)*255 - *p;

            }
        }

        if(SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
	}
}

void replaceNotColor(SDL_Surface *surface, Uint32 oldColor, Uint32 newColor) {
	if(!SDL_MUSTLOCK(surface) || (SDL_LockSurface(surface) == 0)) {
        for(int y = 0; y < surface->h; y++) {
            Uint8 *p = (Uint8 *)surface->pixels + (y * surface->pitch);

            for(int x = 0; x < surface->w; x++, ++p) {
                if(*p != oldColor) {
                    *p = newColor;
                }
            }
        }

        if(SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
	}
}


void replaceColor(SDL_Surface *surface, Uint32 oldColor, Uint32 newColor) {
	if(!SDL_MUSTLOCK(surface) || (SDL_LockSurface(surface) == 0)) {
        for(int y = 0; y < surface->h; y++) {
            Uint8 *p = (Uint8 *)surface->pixels + (y * surface->pitch);

            for(int x = 0; x < surface->w; x++, ++p) {
                if(*p == oldColor) {
                    *p = newColor;
                }
            }
        }

        if(SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
	}
}

void mapColor(SDL_Surface *surface, Uint8 colorMap[256]) {
	if(!SDL_MUSTLOCK(surface) || (SDL_LockSurface(surface) == 0)) {
        for(int y = 0; y < surface->h; y++) {
            Uint8 *p = (Uint8 *)surface->pixels + (y * surface->pitch);

            for(int x = 0; x < surface->w; x++, ++p) {
                *p = colorMap[*p];
            }
        }

        if(SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
	}
}


SDL_Surface* copySurface(SDL_Surface* inSurface) {
	//return SDL_DisplayFormat(inSurface);
	return SDL_ConvertSurface(inSurface, inSurface->format, inSurface->flags);
}


SDL_Surface* scaleSurface(SDL_Surface *surf, double ratio, bool freeSrcSurface) {
	SDL_Surface *scaled = SDL_CreateRGBSurface(SDL_HWSURFACE, (int) (surf->w * ratio),(int) (surf->h * ratio),8,0,0,0,0);
    if(scaled == NULL) {
        if(freeSrcSurface) {
            SDL_FreeSurface(surf);
        }

        return NULL;
    }
    SDL_SetColors(scaled, surf->format->palette->colors, 0, surf->format->palette->ncolors);
    SDL_SetColorKey(scaled, surf->flags & (SDL_SRCCOLORKEY | SDL_RLEACCEL), surf->format->colorkey);

	SDL_LockSurface(scaled);
	SDL_LockSurface(surf);

	int X2 = (int)(surf->w * ratio);
	int Y2 = (int)(surf->h * ratio);

	for(int x = 0;x < X2;++x)
		for(int y = 0;y < Y2;++y)
			putPixel(scaled,x,y,getPixel(surf,(int) (x/ratio), (int) (y/ratio)));

	SDL_UnlockSurface(scaled);
	SDL_UnlockSurface(surf);

	if(freeSrcSurface) {
		SDL_FreeSurface(surf);
	}

	return scaled;
}

SDL_Surface* scaleSurface(SDL_Surface *surf, double ratiox,double ratioy, bool freeSrcSurface) {
	SDL_Surface *scaled = SDL_CreateRGBSurface(SDL_HWSURFACE, (int)ceil((surf->w * ratiox)),(int)ceil((surf->h * ratioy)),8,0,0,0,0);
    if(scaled == NULL) {
        if(freeSrcSurface) {
            SDL_FreeSurface(surf);
        }

        return NULL;
    }
    SDL_SetColors(scaled, surf->format->palette->colors, 0, surf->format->palette->ncolors);
    SDL_SetColorKey(scaled, surf->flags & (SDL_SRCCOLORKEY | SDL_RLEACCEL), surf->format->colorkey);

	SDL_LockSurface(scaled);
	SDL_LockSurface(surf);

	int X2 = (int)ceil((surf->w * ratiox));
	int Y2 = (int)ceil((surf->h * ratioy));

	for(int x = 0;x < X2;++x) {
		for(int y = 0;y < Y2;++y) {
			if (x >=0 && x <= ceil(x*ratiox))  {
				if (y >=0 && y < 40)
					putPixel(scaled,x,y,getPixel(surf,(int)floor((x/ratiox)), (int)floor((y/ratioy))));
				else
					putPixel(scaled,x,y,getPixel(surf,(int)floor((x/ratiox)), (int)ceil((y/ratioy))));
			}
			else {
				if (y >=0 && y <= ceil(y*ratioy))
					putPixel(scaled,x,y,getPixel(surf,(int)ceil((x/ratiox)), (int)floor((y/ratioy))));
				else
					putPixel(scaled,x,y,getPixel(surf,(int)ceil((x/ratiox)), (int)ceil((y/ratioy))));
			}

		}
	}

	SDL_UnlockSurface(scaled);
	SDL_UnlockSurface(surf);

	if(freeSrcSurface) {
		SDL_FreeSurface(surf);
	}

	return scaled;
}



SDL_Surface* getSubPicture(SDL_Surface* Pic, int left, int top,
											unsigned int width, unsigned int height) {
	if(Pic == NULL) {
	    throw std::invalid_argument("getSubPicture(): Pic == NULL!");
	}
/*
	if(((int) (left+width) > Pic->w) || ((int) (top+height) > Pic->h)) {
		throw std::invalid_argument("getSubPicture(): left+width > Pic->w || top+height > Pic->h!");
	}
*/
	SDL_Surface *returnPic;

	// create new picture surface
	if((returnPic = SDL_CreateRGBSurface(SDL_HWSURFACE,width,height,8,0,0,0,0))== NULL) {
		throw std::runtime_error("getSubPicture(): Cannot create new Picture!");
	}

	SDL_SetColors(returnPic, Pic->format->palette->colors, 0, Pic->format->palette->ncolors);
	SDL_SetColorKey(returnPic, Pic->flags & (SDL_SRCCOLORKEY | SDL_RLEACCEL), Pic->format->colorkey);

	SDL_Rect srcRect = {left,top,width,height};
	SDL_BlitSurface(Pic,&srcRect,returnPic,NULL);

	return returnPic;
}

SDL_Surface* getSubFrame(SDL_Surface* Pic, int i, int j, int numX, int numY) {
	if(Pic == NULL) {
	    throw std::invalid_argument("getSubFrame(): Pic == NULL!");
	}

	int frameWidth = Pic->w/numX;
	int frameHeight = Pic->h/numY;

	return getSubPicture(Pic, frameWidth*i, frameHeight*j, frameWidth, frameHeight);
}

SDL_Surface* combinePictures(SDL_Surface* basePicture, SDL_Surface* topPicture, int x, int y, bool bFreeBasePicture, bool bFreeTopPicture) {

    if((basePicture == NULL) || (topPicture == NULL)) {
        if(bFreeBasePicture) SDL_FreeSurface(basePicture);
        if(bFreeTopPicture) SDL_FreeSurface(topPicture);
        return NULL;
    }

    SDL_Surface* dest = copySurface(basePicture);
    if(dest == NULL) {
        if(bFreeBasePicture) SDL_FreeSurface(basePicture);
        if(bFreeTopPicture) SDL_FreeSurface(topPicture);
        return NULL;
    }

    SDL_Rect destRect = {x, y, topPicture->w, topPicture->h};
	SDL_BlitSurface(topPicture, NULL, dest, &destRect);

	if(bFreeBasePicture) SDL_FreeSurface(basePicture);
    if(bFreeTopPicture) SDL_FreeSurface(topPicture);

	return dest;
}


SDL_Surface* rotateSurfaceLeft(SDL_Surface* inputPic, bool bFreeInputPic) {
	if(inputPic == NULL) {
		throw std::invalid_argument("rotateSurfaceLeft(): inputPic == NULL!");
	}

	SDL_Surface *returnPic;

	// create new picture surface
	if((returnPic = SDL_CreateRGBSurface(SDL_HWSURFACE,inputPic->h,inputPic->w,8,0,0,0,0))== NULL) {
	    if(bFreeInputPic) SDL_FreeSurface(inputPic);
		throw std::runtime_error("rotateSurfaceLeft(): Cannot create new Picture!");
	}

	SDL_SetColors(returnPic, inputPic->format->palette->colors, 0, inputPic->format->palette->ncolors);
	SDL_SetColorKey(returnPic, inputPic->flags & (SDL_SRCCOLORKEY | SDL_RLEACCEL), inputPic->format->colorkey);

	SDL_LockSurface(returnPic);
	SDL_LockSurface(inputPic);

	//Now we can copy pixel by pixel
	for(int y = 0; y < inputPic->h;y++) {
		for(int x = 0; x < inputPic->w; x++) {
			char val = *( ((char*) (inputPic->pixels)) + y*inputPic->pitch + x);
			*( ((char*) (returnPic->pixels)) + (returnPic->h - x - 1)*returnPic->pitch + y) = val;
		}
	}

	SDL_UnlockSurface(inputPic);
	SDL_UnlockSurface(returnPic);

    if(bFreeInputPic) SDL_FreeSurface(inputPic);

	return returnPic;
}


SDL_Surface* rotateSurfaceRight(SDL_Surface* inputPic, bool bFreeInputPic) {
	if(inputPic == NULL) {
		throw std::invalid_argument("rotateSurfaceRight(): inputPic == NULL!");
	}

	SDL_Surface *returnPic;

	// create new picture surface
	if((returnPic = SDL_CreateRGBSurface(SDL_HWSURFACE,inputPic->h,inputPic->w,8,0,0,0,0))== NULL) {
	    if(bFreeInputPic) SDL_FreeSurface(inputPic);
		throw std::runtime_error("rotateSurfaceRight(): Cannot create new Picture!");
	}

	SDL_SetColors(returnPic, inputPic->format->palette->colors, 0, inputPic->format->palette->ncolors);
	SDL_SetColorKey(returnPic, inputPic->flags & (SDL_SRCCOLORKEY | SDL_RLEACCEL), inputPic->format->colorkey);

	SDL_LockSurface(returnPic);
	SDL_LockSurface(inputPic);

	//Now we can copy pixel by pixel
	for(int y = 0; y < inputPic->h;y++) {
		for(int x = 0; x < inputPic->w; x++) {
			char val = *( ((char*) (inputPic->pixels)) + y*inputPic->pitch + x);
			*( ((char*) (returnPic->pixels)) + x*returnPic->pitch + (returnPic->w - y - 1)) = val;
		}
	}

	SDL_UnlockSurface(inputPic);
	SDL_UnlockSurface(returnPic);

    if(bFreeInputPic) SDL_FreeSurface(inputPic);

	return returnPic;
}


SDL_Surface* flipHSurface(SDL_Surface* inputPic, bool bFreeInputPic) {
	if(inputPic == NULL) {
		throw std::invalid_argument("flipHSurface(): inputPic == NULL!");
	}

	SDL_Surface *returnPic;

	// create new picture surface
	if((returnPic = SDL_CreateRGBSurface(SDL_HWSURFACE,inputPic->w,inputPic->h,8,0,0,0,0))== NULL) {
	    if(bFreeInputPic) SDL_FreeSurface(inputPic);
		throw std::runtime_error("flipHSurface(): Cannot create new Picture!");
	}

	SDL_SetColors(returnPic, inputPic->format->palette->colors, 0, inputPic->format->palette->ncolors);
	SDL_SetColorKey(returnPic, inputPic->flags & (SDL_SRCCOLORKEY | SDL_RLEACCEL), inputPic->format->colorkey);

	SDL_LockSurface(returnPic);
	SDL_LockSurface(inputPic);

	//Now we can copy pixel by pixel
	for(int y = 0; y < inputPic->h;y++) {
		for(int x = 0; x < inputPic->w; x++) {
			char val = *( ((char*) (inputPic->pixels)) + y*inputPic->pitch + x);
			*( ((char*) (returnPic->pixels)) + (returnPic->h - y - 1)*returnPic->pitch + x) = val;
		}
	}

	SDL_UnlockSurface(inputPic);
	SDL_UnlockSurface(returnPic);

    if(bFreeInputPic) SDL_FreeSurface(inputPic);

	return returnPic;
}


SDL_Surface* flipVSurface(SDL_Surface* inputPic, bool bFreeInputPic) {
	if(inputPic == NULL) {
		throw std::invalid_argument("flipVSurface(): inputPic == NULL!");
	}

	SDL_Surface *returnPic;

	// create new picture surface
	if((returnPic = SDL_CreateRGBSurface(SDL_HWSURFACE,inputPic->w,inputPic->h,8,0,0,0,0))== NULL) {
	    if(bFreeInputPic) SDL_FreeSurface(inputPic);
		throw std::runtime_error("flipVSurface(): Cannot create new Picture!");
	}

	SDL_SetColors(returnPic, inputPic->format->palette->colors, 0, inputPic->format->palette->ncolors);
	SDL_SetColorKey(returnPic, inputPic->flags & (SDL_SRCCOLORKEY | SDL_RLEACCEL), inputPic->format->colorkey);

	SDL_LockSurface(returnPic);
	SDL_LockSurface(inputPic);

	//Now we can copy pixel by pixel
	for(int y = 0; y < inputPic->h;y++) {
		for(int x = 0; x < inputPic->w; x++) {
			char val = *( ((char*) (inputPic->pixels)) + y*inputPic->pitch + x);
			*( ((char*) (returnPic->pixels)) + y*returnPic->pitch + (inputPic->w - x - 1)) = val;
		}
	}

	SDL_UnlockSurface(inputPic);
	SDL_UnlockSurface(returnPic);

    if(bFreeInputPic) SDL_FreeSurface(inputPic);

	return returnPic;
}





SDL_Surface* createShadowSurface(SDL_Surface* source) {
    if(source == NULL) {
	    throw std::invalid_argument("createShadowSurface(): source == NULL!");
	}

	SDL_Surface *retPic;

	if((retPic = SDL_ConvertSurface(source,source->format,SDL_HWSURFACE)) == NULL) {
	    throw std::runtime_error("createShadowSurface(): Cannot copy image!");
	}

	if(SDL_LockSurface(retPic) != 0) {
	    SDL_FreeSurface(retPic);
	    throw std::runtime_error("createShadowSurface(): Cannot lock image!");
	}

	for(int i = 0; i < retPic->w; i++) {
		for(int j = 0; j < retPic->h; j++) {
			Uint8* pixel = &((Uint8*)retPic->pixels)[j * retPic->pitch + i];
			if(*pixel != 0) {
                *pixel = 12;
			}
		}
	}
	SDL_UnlockSurface(retPic);

	return retPic;
}





SDL_Surface* mapSurfaceColorRange(SDL_Surface* source, int srcColor, int destColor, bool bFreeSource) {
	SDL_Surface *retPic;

    if(bFreeSource) {
        retPic = source;
    } else {
        if((retPic = SDL_ConvertSurface(source,source->format,SDL_HWSURFACE)) == NULL) {
            throw std::runtime_error("mapSurfaceColorRange(): Cannot copy image!");
        }
    }

	if(SDL_LockSurface(retPic) != 0) {
	    SDL_FreeSurface(retPic);
	    throw std::runtime_error("mapSurfaceColorRange(): Cannot lock image!");
	}

    for(int y = 0; y < retPic->h; ++y) {
        Uint8* p = (Uint8*) retPic->pixels + y * retPic->pitch;
        for(int x = 0; x < retPic->w; ++x, ++p) {
			if ((*p >= srcColor) && (*p < srcColor + 7))
				*p = *p - srcColor + destColor;
		}
	}
	SDL_UnlockSurface(retPic);

	return retPic;
}

SDL_Surface* mapSurfaceNotColorRange(SDL_Surface* source, int srcColor, int destColor, int excludeColor, bool bFreeSource) {
	SDL_Surface *retPic;

    if(bFreeSource) {
        retPic = source;
    } else {
        if((retPic = SDL_ConvertSurface(source,source->format,SDL_HWSURFACE)) == NULL) {
            throw std::runtime_error("mapSurfaceColorRange(): Cannot copy image!");
        }
    }

	if(SDL_LockSurface(retPic) != 0) {
	    SDL_FreeSurface(retPic);
	    throw std::runtime_error("mapSurfaceColorRange(): Cannot lock image!");
	}

    for(int y = 0; y < retPic->h; ++y) {
        Uint8* p = (Uint8*) retPic->pixels + y * retPic->pitch;
        for(int x = 0; x < retPic->w; ++x, ++p) {
			if ((*p >= srcColor) && (*p < srcColor + 7) && (*p < excludeColor) &&  (*p >= excludeColor + 7) )
				*p = *p - srcColor + destColor;
		}
	}
	SDL_UnlockSurface(retPic);

	return retPic;
}
