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

#ifndef HARVESTER_H
#define HARVESTER_H

#include <units/TrackedUnit.h>
#include <structures/Refinery.h>

class Harvester : public TrackedUnit
{
public:

	Harvester(House* newOwner);
	Harvester(InputStream& stream);
	void init();
	virtual ~Harvester();

	virtual void save(OutputStream& stream) const;

	void blitToScreen();

	Refinery* findRefinery();
	void checkPos();
	virtual void deploy(const Coord& newLocation);
	void destroy();
	virtual void drawSelectionBox();
	void handleDamage(int damage, Uint32 damagerID, House* damagerOwner);

	void handleReturnClick();

    /**
        Order this harvester to return to a refinery.
    */
	void doReturn();

    /**
        Order this harvester to re-deploy.
    */
	void doDeploy(Coord location);

	void move();
	void setAmountOfSpice(float newSpice);
	void setReturned();

	void setDestination(int newX, int newY);
    inline void setDestination(const Coord& location) { setDestination(location.x, location.y); }

    void setFellow(const ObjectBase* newTarget);
	void setTarget(const ObjectBase* newTarget);

	bool canAttack(const ObjectBase* object) const;

	float extractSpice(float extractionSpeed);
	virtual float getMaxSpeed() ;

	inline float getAmountOfSpice() const { return spice; }
	inline bool isReturning() const { return returningToRefinery; }
	bool isHarvesting() const;
	bool isIdle() const;

	float   currentMaxSpeed;    ///< The current maximum allowed speed

private:

    virtual void setSpeeds();

    // harvester state
	bool	harvestingMode;         ///< currently harvesting
    bool    returningToRefinery;    ///< currently on the way back to the refinery
	float   spice;                  ///< loaded spice
	Uint32  spiceCheckCounter;      ///< Check for available spice on map to harvest
};

#endif // HARVESTER_H
