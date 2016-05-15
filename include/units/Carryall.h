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

#ifndef CARRYALL_H
#define CARRYALL_H

#include <units/AirUnit.h>

#include <list>

class Carryall : public AirUnit
{
public:
	Carryall(House* newOwner);
	Carryall(InputStream& stream);
	void init();
	virtual ~Carryall();

	void flyAround();
	void doMove2Pos(int xPos, int yPos, bool bForced);
	void checkPos();

    /**
        Updates this carryall.
        \return true if this object still exists, false if it was destroyed
	*/
	bool update();

    virtual float getMaxSpeed() ;
    virtual void setSpeeds();

	virtual void deploy(const Coord& newLocation);

	void destroy();

	void deployUnit(Uint32 unitID);

	void giveCargo(UnitBase* newUnit);

	void save(OutputStream& stream) const;

	void setFellow(const ObjectBase* newTarget);
	void setTarget(const ObjectBase* newTarget);

	bool hasCargo() const {
        return !pickedUpUnitList.empty();
    }

	inline void book() { booked = true; }

	inline void setOwned(bool b) { owned = b; }

	inline void setDropOfferer(bool status) {
	    aDropOfferer = status;
	    if(aDropOfferer) {
	        booked = true;
        }
    }

	inline bool isBooked() const { return (booked || hasCargo()); }

	inline Coord getDeployPos() const { return deployPos; }
	inline void setDeployPos(Coord coord) {  deployPos = coord; }

	inline Coord getFallbackPos() const { return fallBackPos; }
	inline void setFallbackPos(Coord coord) { fallBackPos = coord; }

	bool canPass(int xPos, int yPos) const;

	float   currentMaxSpeed;    ///< The current maximum allowed speed

protected:
	void findConstYard();
    void releaseFellow();
    void engageFollow();
	void engageTarget();
	void pickupTarget();
	void targeting();

    // unit state/properties
    std::list<Uint32>   pickedUpUnitList;   ///< What units does this carryall carry?


    bool    booked;             ///< Is this carryall currently booked?
    bool    idle;               ///< Is this carryall currently idle?
	bool    firstRun;           ///< Is this carryall new?
	bool    owned;              ///< Is this carryall owned or is it just here to drop something off
	Uint32	tryDeploy = 0;		///< The current deployment tries

	bool	aDropOfferer;       ///< This carryall just drops some units and vanishes afterwards
	bool    droppedOffCargo;    ///< Is the cargo already dropped off?

	Coord 	fallBackPos;	///< The fallback spot in case of retreat

	Uint8   curFlyPoint;        ///< The current flyPoint
	Coord	flyPoints[8];       ///< Array of flight points
	Coord	constYardPoint;     ///< The position of the construction yard to fly around
	Coord	deployPos = Coord::Invalid();			///< The last deployed unit position
};

#endif // CARRYALL_H
