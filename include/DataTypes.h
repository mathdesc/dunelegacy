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

#ifndef DATATYPES_H
#define DATATYPES_H

#include <Definitions.h>

// Libraries
#include <SDL.h>
#include <string>


class Coord {
public:
	Coord() {
		x = y = 0;
	}

	Coord(int x,int y) {
		this->x = x;
		this->y = y;
	}

	/** Copy constructor */
	Coord(const Coord &c)  {
			this->x = c.x;
			this->y = c.y;
	}

#if 0
	 /** Move constructor */
	Coord (Coord&& c) noexcept : x(c.x), y(c.y) {
		this->x = c.x;
		this->y = c.y;
        c.x = 0;
        c.y = 0;
    }

	/** Move assignment operator */
	Coord& operator= (Coord&& other) noexcept {
		if (this != &other) {
			this->x = 0;
			this->y = 0;
			this->x = other.x;
			this->y = other.y;
			other.x = 0;
			other.y = 0;
		}
		return *this;
	}

	/** Copy assignment operator */
	Coord& operator= (const Coord& c) noexcept
	{
		Coord tmp(c);         // re-use copy-constructor
		*this = std::move_if_noexcept(tmp); // re-use move-assignment
		return *this;
	}

#else

	Coord(Coord&&) = default;
	Coord& operator=(Coord&&) = default;
	Coord& operator=(const Coord& c) = default;

#endif

	inline bool operator==(const Coord& c) const {
        return (x == c.x && y == c.y);
	}

	inline bool operator!=(const Coord& c) const {
        return !operator==(c);
	}

	inline Coord& operator+=(const Coord& c) {
        x += c.x;
        y += c.y;
        return *this;
	}

    inline Coord operator+(const Coord& c) const {
        Coord ret = *this;
        ret += c;
        return ret;
	}

    inline Coord& operator-=(const Coord& c) {
        x -= c.x;
        y -= c.y;
        return *this;
	}

    inline Coord operator-(const Coord& c) const {
        Coord ret = *this;
        ret -= c;
        return ret;
	}

    inline Coord& operator*=(int c) {
        x *= c;
        y *= c;
        return *this;
	}

    inline Coord operator*(int c) const {
        Coord ret = *this;
        ret *= c;
        return ret;
	}

    inline Coord& operator/=(int c) {
        x /= c;
        y /= c;
        return *this;
	}

    inline Coord operator/(int c) const {
        Coord ret = *this;
        ret /= c;
        return ret;
	}

	inline void invalidate() {
        x = INVALID_POS;
        y = INVALID_POS;
	}

	inline bool isValid() const {
        return ((x != INVALID_POS) && (y != INVALID_POS));
	}

    inline bool isInvalid() const {
        return ((x == INVALID_POS) || (y == INVALID_POS));
	}

	static inline const Coord Invalid() {
	    return Coord(INVALID_POS, INVALID_POS);
	}

    inline operator bool() const {
		return isValid();
	};



    inline Coord swapCoord() {
    	//Coord ret = *this;
    	int tmp = x;
    	x = y;
    	y = tmp;
		return *this;
	};

public:
	int	x;
	int y;
};



class Frame {
public:
	Frame() {
		numImagesX = numImagesY = firstAnimFrame = lastAnimFrame = 0;
	}

	Frame(int x,int y, int f, int l) {
		this->numImagesX = x;
		this->numImagesY = y;
		this->firstAnimFrame = f;
		this->lastAnimFrame = l;
	}

	inline bool operator==(const Frame& f) const {
        return (numImagesX == f.numImagesX && numImagesY == f.numImagesY && firstAnimFrame == f.firstAnimFrame && lastAnimFrame == f.lastAnimFrame);
	}

	inline bool operator!=(const Frame& f) const {
        return !operator==(f);
	}

	inline bool isValid() const {
		return (numImagesX != 0 && numImagesY != 0 && firstAnimFrame <= numImagesX * numImagesY  && lastAnimFrame <= numImagesX * numImagesY && firstAnimFrame <= lastAnimFrame );
	}
public:
	int	numImagesX;
	int numImagesY;
	int firstAnimFrame;
	int lastAnimFrame;
};



typedef enum {
    ATTACKMODE_INVALID = -1,
    GUARD = 0,      ///< The unit will attack enemy units but will not move or follow enemy units.
    AREAGUARD = 1,  ///< Area Guard is the most common command for pre-placed AI units. They will scan for targets in a relatively large radius, and return to their original position after their target was either destroyed or left the immediate area.
    AMBUSH = 2,     ///< Ambush means a unit will remain in position until sighted by the enemy, and then proceed to attack any enemy units it might find on the map.
    HUNT = 3,       ///< Hunt makes a unit start from its position towards enemy units, even if the player has not sighted the AI (normally the AI will not attack until there has been a contact between the player's and the AI's units). Also works for human units, they'll go towards any enemy units on the map just as the mission starts.
    HARVEST = 4,    ///< Only used by the map editor
    SABOTAGE = 5,   ///< Only used by the map editor
    STOP = 6,
    CAPTURE = 7,    ///< Capture is only used for infantry units when ordered to capture a building
    ATTACKMODE_MAX
} ATTACKMODE;

typedef enum {START, LOADING, BEGUN, DEINITIALIZE} GAMESTATETYPE;

typedef enum {
    GAMETYPE_INVALID            = -1,
    GAMETYPE_LOAD_SAVEGAME      = 0,
    GAMETYPE_CAMPAIGN           = 1,
    GAMETYPE_CUSTOM             = 2,
    GAMETYPE_SKIRMISH           = 3,
    GAMETYPE_CUSTOM_MULTIPLAYER = 4,
    GAMETYPE_LOAD_MULTIPLAYER   = 5
} GAMETYPE;


class SettingsClass
{
public:
	class GeneralClass {
	public:
		bool		      playIntro;
		std::string     playerName;
		std::string     language;
	} general;

	class VideoClass {
	public:
		bool	    doubleBuffering;
		bool	    fullscreen;
		int		    width;
		int		    height;
        bool	    frameLimit;
        int         preferredZoomLevel;
        std::string scaler;
	} video;

	class AudioClass {
	public:
        bool        playSFX;
        bool        playMusic;
		std::string musicType;
		int         frequency;
	} audio;

	class NetworkClass {
	public:
		int		    serverPort;
        std::string metaServer;
        bool        debugNetwork;
	} network;

	class AIClass {
    public:
        std::string campaignAI;
	} ai;

	class GameOptionsClass {
    public:
        GameOptionsClass()
         : gameSpeed(GAMESPEED_DEFAULT), concreteRequired(true), structuresDegradeOnConcrete(true), fogOfWar(false),
           startWithExploredMap(false), instantBuild(false), onlyOnePalace(false), rocketTurretsNeedPower(false),
           sandwormsRespawn(false), killedSandwormsDropSpice(false), daynight(false), dayscale(GAMEDAYSCALE_DEFAULT) {
        }


        bool operator==(const GameOptionsClass& goc) const {
            return (gameSpeed == goc.gameSpeed)
                    && (concreteRequired == goc.concreteRequired)
                    && (structuresDegradeOnConcrete == goc.structuresDegradeOnConcrete)
                    && (fogOfWar == goc.fogOfWar)
                    && (startWithExploredMap == goc.startWithExploredMap)
                    && (instantBuild == goc.instantBuild)
                    && (onlyOnePalace == goc.onlyOnePalace)
                    && (rocketTurretsNeedPower == goc.rocketTurretsNeedPower)
                    && (sandwormsRespawn == goc.sandwormsRespawn)
                    && (killedSandwormsDropSpice == goc.killedSandwormsDropSpice)
					&& (daynight == goc.daynight)
					&& (dayscale == goc.dayscale);
        }

        bool operator!=(const GameOptionsClass& goc) const {
            return !this->operator==(goc);
        }

        int         gameSpeed;
        bool		concreteRequired;
		bool        structuresDegradeOnConcrete;
		bool        fogOfWar;
		bool        startWithExploredMap;
		bool        instantBuild;
		bool        onlyOnePalace;
		bool        rocketTurretsNeedPower;
        bool        sandwormsRespawn;
		bool        killedSandwormsDropSpice;
		bool 		daynight;
		Uint8		dayscale;
	} gameOptions;
};

typedef enum
{
    HOUSE_UNUSED    = -2,
    HOUSE_INVALID   = -1,
	HOUSE_HARKONNEN =  0,
	HOUSE_ATREIDES  =  1,
	HOUSE_ORDOS     =  2,
	HOUSE_FREMEN    =  3,
	HOUSE_SARDAUKAR =  4,
	HOUSE_MERCENARY =  5,
	NUM_HOUSES
} HOUSETYPE;

typedef enum {
    RIGHT,
    RIGHTUP,
    UP,
    LEFTUP,
    LEFT,
    LEFTDOWN,
    DOWN,
    RIGHTDOWN,
    NUM_ANGLES
} ANGLETYPE;

typedef enum  {
    Drop_Invalid = -1,
    Drop_North,         ///< unit will appear at a random position at the top of the map
    Drop_East,          ///< unit will appear at a random position on the right side of the map
    Drop_South,         ///< unit will appear at a random position at the bottom of the map
    Drop_West,          ///< unit will appear at a random position on the left side of the map
    Drop_Air,           ///< unit will be dropped at a random position
    Drop_Visible,       ///< unit will be dropped at a random position in the middle of the map
    Drop_Enemybase,     ///< unit will be dropped near the enemy base
    Drop_Homebase       ///< unit will be dropped near the base of the owner of the new unit
} DropLocation;

typedef enum {
    TeamBehavior_Invalid = -1,
    TeamBehavior_Normal,            ///< Attack units and/or structures when building up the team is complete
    TeamBehavior_Guard,             ///< Same as TeamBehavior_Normal
    TeamBehavior_Kamikaze,          ///< Directly attack structures when building up the team is complete
    TeamBehavior_Staging,           ///< A team in the process of being built up
    TeamBehavior_Flee               ///< Do nothing (Unimplemented in Dune II?)
} TeamBehavior;

typedef enum {
    TeamType_Invalid = -1,
    TeamType_Foot,
    TeamType_Wheeled,
    TeamType_Tracked,
    TeamType_Winged,
    TeamType_Slither,
    TeamType_Harvester
} TeamType;

#endif //DATATYPES_H
