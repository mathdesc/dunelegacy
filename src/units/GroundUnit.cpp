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

#include <units/GroundUnit.h>

#include <globals.h>

#include <Game.h>
#include <House.h>
#include <Map.h>
#include <SoundPlayer.h>

#include <structures/RepairYard.h>
#include <units/Carryall.h>
#include <players/HumanPlayer.h>

GroundUnit::GroundUnit(House* newOwner) : UnitBase(newOwner) {

    GroundUnit::init();

    awaitingPickup = false;
	bookedCarrier = NONE;
}

GroundUnit::GroundUnit(InputStream& stream) : UnitBase(stream) {

    GroundUnit::init();

    awaitingPickup = stream.readBool();
    bookedCarrier = stream.readUint32();
}

void GroundUnit::init() {
    aGroundUnit = true;
}

GroundUnit::~GroundUnit() {
}

void GroundUnit::save(OutputStream& stream) const {
	UnitBase::save(stream);

	stream.writeBool(awaitingPickup);
	stream.writeUint32(bookedCarrier);
}

void GroundUnit::assignToMap(const Coord& pos) {
	if (currentGameMap->tileExists(pos)) {
		currentGameMap->getTile(pos)->assignNonInfantryGroundObject(getObjectID());
		currentGameMap->viewMap(owner->getTeam(), location, getViewRange());
	}
}

void GroundUnit::checkPos() {
    if(!moving && !justStoppedMoving && !isInfantry()) {
        currentGameMap->getTile(location.x,location.y)->setTrack(drawnAngle);
    }

	if(justStoppedMoving)
	{
		realX = location.x*TILESIZE + TILESIZE/2;
		realY = location.y*TILESIZE + TILESIZE/2;
		//findTargetTimer = 0;	//allow a scan for new targets now

		if(currentGameMap->getTile(location)->isSpiceBloom()) {
		    setHealth(0.0f);
		    setVisible(VIS_ALL, false);
			currentGameMap->getTile(location)->triggerSpiceBloom(getOwner());
		} else if(currentGameMap->getTile(location)->isSpecialBloom()){
            currentGameMap->getTile(location)->triggerSpecialBloom(getOwner());
		}
	}

	if(!isInfantry() && goingToRepairYard) {
	    if(fellow.getObjPointer() == NULL) {
            goingToRepairYard = false;
            awaitingPickup = false;
            bookedCarrier = NONE;

            clearPath();
	    } else {
            Coord closestPoint = fellow.getObjPointer()->getClosestPoint(location);
            if (!moving && !justStoppedMoving && (blockDistance(location, closestPoint) <= 1.5f)
                && ((RepairYard*)fellow.getObjPointer())->isFree())
            {
                if (getHealth()*100.0f/getMaxHealth() < 100.0f) {
                    setGettingRepaired();
                } else {
                    setFellow(NULL);
                    setDestination(guardPoint);
                }
            }
	    }
	}
}

void GroundUnit::playConfirmSound() {
	soundPlayer->playSound((Sound_enum) getRandomOf(3,Acknowledged,Affirmative,YesSir));
}

void GroundUnit::playSelectSound() {
	soundPlayer->playSound(Reporting);
}

bool GroundUnit::requestCarryall() {

	if (itemID == Unit_MCV || itemID == Unit_Sandworm ) {
		return false;
	}

	if (getOwner()->hasCarryalls())	{
		Carryall* carryall = NULL;
		float distance = std::numeric_limits<float>::infinity();

		RobustList<UnitBase*>::const_iterator iter;
		UnitBase* unit, *bestunit = NULL;
	    for(iter = unitList.begin(); iter != unitList.end(); ++iter) {
			unit = *iter;
			if ((unit->getOwner() == owner) && (unit->getItemID() == Unit_Carryall)) {
				if(!((Carryall*)unit)->isBooked() && ((Carryall*)unit)->getAttackMode() != STOP ) {
					if (distance >  std::min(distance,blockDistance(this->location, unit->getLocation())) ) {
						bestunit=unit;
						distance = std::min(distance,blockDistance(this->location, unit->getLocation()));
					}
				}
			}
		}


	    if (bestunit != NULL) {
			carryall = (Carryall*)bestunit;
			carryall->setFellow(this);
			carryall->clearPath();
			bookCarrier(carryall);
			return true;
	    }

	}
	return false;
}

void GroundUnit::setPickedUp(UnitBase* newCarrier) {
	UnitBase::setPickedUp(newCarrier);
	awaitingPickup = false;
	bookedCarrier = NONE;

	clearPath();
}

void GroundUnit::bookCarrier(UnitBase* newCarrier) {
    if(newCarrier == NULL) {
        bookedCarrier = NONE;
        awaitingPickup = false;
    } else {
        bookedCarrier = newCarrier->getObjectID();
        awaitingPickup = true;
    }
}

bool GroundUnit::hasBookedCarrier() const {
    if(bookedCarrier == NONE) {
        return false;
    } else {
        return (currentGame->getObjectManager().getObject(bookedCarrier) != NULL);
    }
}

const UnitBase* GroundUnit::getCarrier() const {
    return (UnitBase*) currentGame->getObjectManager().getObject(bookedCarrier);
}

void GroundUnit::navigate() {
	if(!awaitingPickup) {
		UnitBase::navigate();
    }
}

void GroundUnit::handleRepairClick() {
	currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_UNIT_REPAIR,objectID));
}


void GroundUnit::doRepair() {
	if( !isInfantry() && getHealth() < getMaxHealth()) {
		//find a repair yard to return to

		float	closestLeastBookedRepairYardDistance = std::numeric_limits<float>::infinity();
        RepairYard* bestRepairYard = NULL;

        RobustList<StructureBase*>::const_iterator iter;
        for(iter = structureList.begin(); iter != structureList.end(); ++iter) {
            StructureBase* tempStructure = *iter;

			if ((tempStructure->getItemID() == Structure_RepairYard) && (tempStructure->getOwner() == owner)) {
                RepairYard* tempRepairYard = ((RepairYard*)tempStructure);

                if(tempRepairYard->getNumBookings() == 0) {
                    float tempDistance = distanceFrom(location, tempRepairYard->getClosestPoint(location));
					if(tempDistance < closestLeastBookedRepairYardDistance) {
                        closestLeastBookedRepairYardDistance = tempDistance;
                        bestRepairYard = tempRepairYard;
                    }
                }
            }
        }

        if(bestRepairYard) {
            if(requestCarryall()) {
                doMove2Object(bestRepairYard,target.getObjPointer());
            } else if(owner->isAI()) {
                doMove2Object(bestRepairYard,target.getObjPointer());
            }
        }
	}
}
