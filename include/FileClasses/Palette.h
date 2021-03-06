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

#ifndef PALETTE_H
#define PALETTE_H

#include <SDL.h>
#include <string.h>
#include <stdexcept>

class Palette
{
    public:
        Palette() : pSDLPalette(NULL), pSDLPaletteBck(NULL) {

        }

        Palette(int numPaletteEntries) : pSDLPalette(NULL),pSDLPaletteBck(NULL) {
            try {
                pSDLPalette = new SDL_Palette;
                pSDLPalette->ncolors = numPaletteEntries;
                pSDLPalette->colors = NULL;
                pSDLPalette->colors = new SDL_Color[pSDLPalette->ncolors];

                memset(pSDLPalette->colors, 0, sizeof(SDL_Color) * pSDLPalette->ncolors);

                pSDLPaletteBck = new SDL_Palette;
                pSDLPaletteBck->ncolors = numPaletteEntries;
                pSDLPaletteBck->colors = NULL;
                pSDLPaletteBck->colors = new SDL_Color[pSDLPaletteBck->ncolors];

                memset(pSDLPaletteBck->colors, 0, sizeof(SDL_Color) * pSDLPaletteBck->ncolors);

            } catch (...) {
                if(pSDLPalette != NULL) {
                    delete [] pSDLPalette->colors;
                }
                if(pSDLPaletteBck != NULL) {
                    delete [] pSDLPaletteBck->colors;
                }
                delete pSDLPalette;
                delete pSDLPaletteBck;
                throw;
            }
        }

        Palette(const SDL_Palette* pSDLPalette) : pSDLPalette(NULL), pSDLPaletteBck(NULL) {
            setSDLPalette(pSDLPalette);
        }

        Palette(const Palette& palette) : pSDLPalette(NULL), pSDLPaletteBck(NULL) {
            *this = palette;
        }

        virtual ~Palette() {
            deleteSDLPalette();
        }

        Palette& operator=(const Palette& palette) {
            if(this == &palette) {
                return *this;
            }

            this->setSDLPalette(palette.pSDLPalette);

            return *this;
        }

        inline SDL_Color& operator[](const int i) {
            if(pSDLPalette == NULL || i < 0 || i >= pSDLPalette->ncolors) {
                throw std::runtime_error("Palette::operator[]: Invalid index!");
            }

            return pSDLPalette->colors[i];
        }

        inline const SDL_Color& operator[](const int i) const {
            if(pSDLPalette == NULL || i < 0 || i >= pSDLPalette->ncolors) {
                throw std::runtime_error("Palette::operator[]: Invalid index!");
            }

            return pSDLPalette->colors[i];
        }

        inline void invertPalette() {
        	 SDL_Palette* pTemp = pSDLPaletteBck;
        	 pSDLPaletteBck = pSDLPalette;
        	 pSDLPaletteBck = pTemp;
        }

        inline SDL_Palette* getSDLPalette() const {
            return pSDLPalette;
        }
        inline SDL_Palette* getSDLPaletteBackup() const {
            return pSDLPaletteBck;
        }

        void setSDLPalette(const SDL_Palette* pSDLPalette) {
            SDL_Palette* pNewSDLPalette = NULL;
            SDL_Palette* pNewSDLPaletteBck = NULL;

            try {
                pNewSDLPalette = new SDL_Palette;
                pNewSDLPalette->ncolors = pSDLPalette->ncolors;
                pNewSDLPalette->colors = NULL;
                pNewSDLPalette->colors = new SDL_Color[pNewSDLPalette->ncolors];

                memcpy(pNewSDLPalette->colors, pSDLPalette->colors, pSDLPalette->ncolors * sizeof(SDL_Color));

                pNewSDLPaletteBck = new SDL_Palette;
                pNewSDLPaletteBck->ncolors = pSDLPalette->ncolors;
                pNewSDLPaletteBck->colors = NULL;
                pNewSDLPaletteBck->colors = new SDL_Color[pNewSDLPaletteBck->ncolors];
                memcpy(pNewSDLPaletteBck->colors, pSDLPalette->colors, pSDLPalette->ncolors * sizeof(SDL_Color));

            } catch (...) {
                if(pNewSDLPalette != NULL) {
                    delete [] pNewSDLPalette->colors;
                }
                if(pNewSDLPaletteBck != NULL) {
                    delete [] pNewSDLPaletteBck->colors;
                }
                delete pNewSDLPalette;
                delete pNewSDLPaletteBck;
                throw;
            }

            deleteSDLPalette();
            this->pSDLPalette = pNewSDLPalette;
            this->pSDLPaletteBck = pNewSDLPaletteBck;
        }

        inline int getNumColors() const {
            if(pSDLPalette == NULL) {
                return 0;
            }

            return pSDLPalette->ncolors;
        }

        void applyToSurface(SDL_Surface* pSurface, int flags = SDL_LOGPAL | SDL_PHYSPAL, int firstColor = 0, int endColor = -1) const {
            if(pSDLPalette == NULL) {
                throw std::runtime_error("Palette::applyToSurface(): Palette not initialized yet!");
            }

            if(pSurface == NULL) {
                throw std::runtime_error("Palette::applyToSurface(): pSurface == NULL!");
            }

            int nColors = (endColor != -1) ? (endColor - firstColor + 1) : (pSDLPalette->ncolors - firstColor);
            SDL_SetPalette(pSurface, flags, pSDLPalette->colors + firstColor, firstColor, nColors);
        }

    private:

        void deleteSDLPalette() {
            if(pSDLPalette != NULL) {
                delete [] pSDLPalette->colors;
            }
            if(pSDLPaletteBck != NULL) {
            	delete [] pSDLPaletteBck->colors;
            }
            delete pSDLPalette;
            delete pSDLPaletteBck;
            pSDLPalette = NULL;
            pSDLPaletteBck = NULL;
        }

        SDL_Palette* pSDLPalette;
        SDL_Palette* pSDLPaletteBck;
};

#endif // PALETTE_H
