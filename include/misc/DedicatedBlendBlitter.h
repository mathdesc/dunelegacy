#ifndef DEDICATEDBLENDBLITTER_H
#define DEDICATEDBLENDBLITTER_H

#include <SDL.h>
#include <mmath.h>




// Step by step one more pixel of the source image is blitted to the destination image
class DedicatedBlendBlitter  {



public:
	typedef enum {Builtin,Custom,Zero,One,Random} constructorInit;

	DedicatedBlendBlitter (SDL_Surface* SrcPic, SDL_Surface* DestPic, SDL_Rect DestPicRect = {0,0,0,0}, int numSteps = 50,
			constructorInit aInit = Builtin ,constructorInit cInit = Builtin, constructorInit currentValueInit = Random,
			int nextValueOffsetDivisor = 256 )
	{
        src = SrcPic;
        dest = DestPic;
        destRect = DestPicRect;
        this->numSteps = numSteps;

        N = src->w*src->h;


        // compute next greater 2^x value
        m = N;
        m |= (m >> 1);
        m |= (m >> 2);
        m |= (m >> 4);
        m |= (m >> 8);
        m |= (m >> 16);
        m |= (m >> 32);
        m++;


        if (currentValueInit == Random)
        	this->currentValue = getRandomInt(0,m-1);
        else if (currentValueInit == Zero)
        	this->currentValue = 0;

        if  (cInit == Custom ) {
			if (nextValueOffsetDivisor != 0) {
				c = m / nextValueOffsetDivisor;
				if (c%2 !=0) c++;
			}
			else {
				throw std::logic_error("DedicatedBlendBlitter : Divide by zero");
			}
        } else if (cInit == Builtin ) {
        	c = getRandomInt(0,(m/2)-1) * 2 + 1;  // c is any odd number from [0;m]
        }

        if  (aInit == One ) {
        	 a = 1;
        } else if (aInit == Builtin ) {
        	 a = getRandomInt(0,(m/4)-1) * 4 + 1;  // (a-1) is divisible by all prime factors of log_2(m), and 4
        }

        StepsLeft = numSteps;
    }

    ~DedicatedBlendBlitter() {
    }

    Uint64 getNextValue() {
        do {
            currentValue = (a*currentValue + c) % m;
        } while (currentValue >= N);

        return currentValue;
    }

   void setBlitterDestination(SDL_Rect rect) {
        destRect = rect;
    }

   SDL_Rect getBlitterDestination() {
        return destRect ;
    }


   int getStepCount() {
	   return StepsLeft;
   }
    /**
        Blits the next pixel to the destination surface
        \return The number of steps to do
    */
    int nextStep() {

    	if (destRect.x == 0 && destRect.y == 0 && destRect.w == 0 && destRect.h == 0) {
    		return 0;
    	}

        if(StepsLeft <= 0) {
            return 0;
        }
        StepsLeft--;

        if(SDL_LockSurface(dest) != 0) {
            fprintf(stderr,"BlendBlitter::nextStep(): Cannot lock image!\n");
            exit(EXIT_FAILURE);
        }

        if(SDL_LockSurface(src) != 0) {
            fprintf(stderr,"BlendBlitter::nextStep(): Cannot lock image!\n");
            exit(EXIT_FAILURE);
        }

        Uint32 numPixelsPerStep = (N / numSteps) + 1;
        for(Uint32 i=0;i<numPixelsPerStep;i++) {
            Uint64 cur = getNextValue();

            int x = (cur % src->w);
            int y = (cur / src->w);

            if( ((Uint8*)src->pixels)[y * src->pitch + x] != 0) {

                if(	(destRect.x + x < dest->w) && (destRect.x + x >= 0) &&
                    (destRect.x + x <= destRect.x + destRect.w) &&
                    (destRect.y + y < dest->h) && (destRect.y + y >= 0) &&
                    (destRect.y + y <= destRect.y + destRect.h) ) {

                    // is inside destRect and the destination surface
                    Uint8* pSrcPixel = &((Uint8*)src->pixels)[y * src->pitch + x];
                    Uint8* pDestPixel = &((Uint8*)dest->pixels)[(destRect.y + y) * dest->pitch + (destRect.x + x)];

                    *pDestPixel = *pSrcPixel;
                }
            }
        }

        SDL_UnlockSurface(src);
        SDL_UnlockSurface(dest);
        return StepsLeft;
    }


private:
    SDL_Surface* src;
    SDL_Surface* dest;
    SDL_Rect	destRect;
    int numSteps;
    int StepsLeft;

    Uint64 N;
    Uint64 m;
    Uint64 a;
    Uint64 c;
    Uint64 currentValue;
};

#endif //DEDICATEDBLENDBLITTER_H
