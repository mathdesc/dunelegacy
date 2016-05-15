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

#include <units/Carryall.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/SFXManager.h>
#include <House.h>
#include <Map.h>
#include <Game.h>
#include <SoundPlayer.h>

#include <structures/RepairYard.h>
#include <structures/Refinery.h>
#include <structures/ConstructionYard.h>
#include <units/Harvester.h>

#include <misc/strictmath.h>

Carryall::Carryall(House* newOwner) : AirUnit(newOwner)
{
    Carryall::init();

    setHealth(getMaxHealth());

	booked = false;
    idle = true;
	firstRun = true;
    owned = true;

    aDropOfferer = false;
    droppedOffCargo = false;
    respondable = true;

    currentMaxSpeed = 2.0f;


	curFlyPoint = 0;
	for(int i=0; i < 8; i++) {
		flyPoints[i].invalidate();
	}
	constYardPoint.invalidate();
}

Carryall::Carryall(InputStream& stream) : AirUnit(stream)
{
    Carryall::init();

	pickedUpUnitList = stream.readUint32List();
	if(!pickedUpUnitList.empty()) {
		drawnFrame = 1;
	}

    stream.readBools(&booked, &idle, &firstRun, &owned, &aDropOfferer, &droppedOffCargo);

	currentMaxSpeed = stream.readFloat();

	curFlyPoint = stream.readUint8();
	for(int i=0; i < 8; i++) {
		flyPoints[i].x = stream.readSint32();
		flyPoints[i].y = stream.readSint32();
	}
	constYardPoint.x = stream.readSint32();
	constYardPoint.y = stream.readSint32();
}

void Carryall::init()
{
	itemID = Unit_Carryall;
	owner->incrementUnits(itemID);

	canAttackStuff = false;
	respondable = true;

	graphicID = ObjPic_Carryall;
	graphic = pGFXManager->getObjPic(graphicID,getOwner()->getHouseID());
	shadowGraphic = pGFXManager->getObjPic(ObjPic_CarryallShadow,getOwner()->getHouseID());

	numImagesX = NUM_ANGLES;
	numImagesY = 2;
}

Carryall::~Carryall()
{
	;
}

void Carryall::save(OutputStream& stream) const
{
	AirUnit::save(stream);

	stream.writeUint32List(pickedUpUnitList);

    stream.writeBools(booked, idle, firstRun, owned, aDropOfferer, droppedOffCargo);

	stream.writeFloat(currentMaxSpeed);

	stream.writeUint8(curFlyPoint);
	for(int i=0; i < 8; i++) {
		stream.writeSint32(flyPoints[i].x);
		stream.writeSint32(flyPoints[i].y);
	}
	stream.writeSint32(constYardPoint.x);
	stream.writeSint32(constYardPoint.y);
}

bool Carryall::update() {

    if(AirUnit::update() == false) {
        return false;
    }


    setSpeeds();

   // dbg_relax_print(" >Carryall::deployUnit  speed:%.1f(%.1f) trydeploy=%d \n", currentMaxSpeed,getRegulatedSpeed(),tryDeploy);

	// check if this carryall has to be removed because it has just brought something
	// to the map (e.g. new harvester)
	if (active)	{
		if(aDropOfferer && droppedOffCargo && (hasCargo() == false)
            && (    (location.x == 0) || (location.x == currentGameMap->getSizeX()-1)
                    || (location.y == 0) || (location.y == currentGameMap->getSizeY()-1))
            && !moving)	{

            setVisible(VIS_ALL, false);
            destroy();
            return false;
		}
	}
	return true;
}

float Carryall::getMaxSpeed()  {

    float dist = distanceFrom(location.x*TILESIZE + TILESIZE/2, location.y*TILESIZE + TILESIZE/2,
                                destination.x*TILESIZE + TILESIZE/2, destination.y*TILESIZE + TILESIZE/2);

    if((target || hasCargo()) && dist < 256.0f) {
		currentMaxSpeed = (((2.0f - currentGame->objectData.data[itemID][originalHouseID].maxspeed)/256.0f) * (256.0f - dist)) + currentGame->objectData.data[itemID][originalHouseID].maxspeed;
        currentMaxSpeed /= (float)tryDeploy+1 ;

    } else {
        currentMaxSpeed = std::min(currentMaxSpeed + 0.2f, currentGame->objectData.data[itemID][originalHouseID].maxspeed);

    }


	float maxrelativespeed = currentMaxSpeed;
	bool inList = false;

	if (currentGame->getSelectedList().size() > 1 ) {
		std::list<Uint32> *list = &currentGame->getSelectedList();
		std::list<Uint32>::iterator test;
		for(test = list->begin() ; test != list->end(); ++test) {
			ObjectBase *obj = currentGame->getObjectManager().getObject(*test);
			UnitBase *unit = dynamic_cast<UnitBase*>(obj);
			if(obj->isAUnit() && unit->getRegulatedSpeed() != 0) {
				if (unit->getItemID() == Unit_Carryall)
					maxrelativespeed = std::min(maxrelativespeed,((Carryall*) unit)->currentMaxSpeed);
				else if (unit->getItemID() == Unit_Harvester)
					maxrelativespeed = std::min(maxrelativespeed,((Harvester*) unit)->currentMaxSpeed);
				else
					maxrelativespeed = std::min(maxrelativespeed, currentGame->objectData.data[itemID][originalHouseID].maxspeed);
				maxrelativespeed = std::min(maxrelativespeed,currentGame->objectData.data[obj->getItemID()][obj->getOriginalHouseID()].maxspeed);
				if (this  == unit) {
					unit->setRegulatedSpeed(maxrelativespeed);
					inList = true;
				}
			}
		}
	}

	if (inList) {
		return maxrelativespeed;
	}
	else if (regulatedSpeed != 0 && currentGame->getSelectedList().size() > 1 ) {
		return regulatedSpeed;
	}
	else {
		return currentMaxSpeed;
	}


}


void Carryall::setSpeeds() {


	float speed = getMaxSpeed();

	if(isBadlyDamaged()) {
        speed *= HEAVILYDAMAGEDSPEEDMULTIPLIER;
	}

	if(isSelected() && owner == pLocalHouse) {
		// Selected Carryall enable player control but fly a lower speed
        speed *= CARRYALL_MANUAL_SPEED;
	}


	switch(drawnAngle){
        case LEFT:      xSpeed = -speed;                    ySpeed = 0;         break;
        case LEFTUP:    xSpeed = -speed*DIAGONALSPEEDCONST; ySpeed = xSpeed;    break;
        case UP:        xSpeed = 0;                         ySpeed = -speed;    break;
        case RIGHTUP:   xSpeed = speed*DIAGONALSPEEDCONST;  ySpeed = -xSpeed;   break;
        case RIGHT:     xSpeed = speed;                     ySpeed = 0;         break;
        case RIGHTDOWN: xSpeed = speed*DIAGONALSPEEDCONST;  ySpeed = xSpeed;    break;
        case DOWN:      xSpeed = 0;                         ySpeed = speed;     break;
        case LEFTDOWN:  xSpeed = -speed*DIAGONALSPEEDCONST; ySpeed = -xSpeed;   break;
	}
}


void Carryall::deploy(const Coord& newLocation) {
	AirUnit::deploy(newLocation, true);

	respondable = true;
}

void Carryall::flyAround() {

	//fly around set flyPoints
	Coord point = this->getClosestPoint(location);

	if(point == guardPoint) {
		//arrived at point, move to next
		curFlyPoint++;

		if(curFlyPoint >= 8) {
			curFlyPoint = 0;
		}

        int looped = 0;
		while(!(currentGameMap->tileExists(flyPoints[curFlyPoint].x, flyPoints[curFlyPoint].y)) && looped <= 2) {
			curFlyPoint++;

			if(curFlyPoint >= 8) {
				curFlyPoint = 0;
				looped++;
			}
		}

		setGuardPoint(flyPoints[curFlyPoint]);
		setDestination(guardPoint);

	}
}

void Carryall::doMove2Pos(int xPos, int yPos, bool bForced) {


	UnitBase::doMove2Pos(xPos,yPos,bForced);
	 dbg_print(" Carryall::doMove2Pos %d,%d %d,%d  \n", xPos,yPos, guardPoint.x, guardPoint.y);
	this->guardPoint = Coord(xPos,yPos);
	findConstYard();
	 dbg_print(" Carryall::doMove2Pos %d,%d %d,%d  \n", xPos,yPos, guardPoint.x, guardPoint.y);
}


void Carryall::checkPos()
{
	AirUnit::checkPos();

	if (active)	{
		if (hasCargo() && (location == destination) && (distanceFrom(realX, realY, destination.x * TILESIZE + (TILESIZE/2), destination.y * TILESIZE + (TILESIZE/2)) < TILESIZE/8) ) {

		    // drop up to 3 infantry units at once or one other unit
            int droppedUnits = 0;
            do {
                Uint32 unitID = pickedUpUnitList.front();
                UnitBase* pUnit = (UnitBase*) (currentGame->getObjectManager().getObject(unitID));

                if(pUnit == NULL) {
                    return;
                }

                if((pUnit != NULL) && (pUnit->isInfantry() == false) && (droppedUnits > 0)) {
                    // we already dropped infantry and this is no infantry
                    // => do not drop this here
                    break;
                }
                //dbg_print(" Carryall::checkPos %d,%d %d,%d  \n", destination.x,destination.y, guardPoint.x, guardPoint.y);
                deployUnit(unitID);
                droppedUnits++;

                if((pUnit != NULL) && (pUnit->isInfantry() == false)) {
                    // we dropped a non infantry unit
                    // => do not drop another unit
                    break;
                }
            } while(hasCargo() && (droppedUnits < 3));

            if(pickedUpUnitList.empty() == false) {
                // find next place to drop
                for(int i=8;i<18;i++) {
                    int r = currentGame->randomGen.rand(3,i/2);
                    float angle = 2.0f * strictmath::pi * currentGame->randomGen.randFloat();

                    Coord dropCoord = location + Coord( (int) (r*strictmath::sin(angle)), (int) (-r*strictmath::cos(angle)));
                    if(currentGameMap->tileExists(dropCoord) && currentGameMap->getTile(dropCoord)->hasAGroundObject() == false) {
                        setDestination(dropCoord);
                        break;
                    }
                }
            } else {
                setTarget(NULL);
                setDestination(guardPoint);
                idle = true;
            }
		} else if( (isBooked() == false) && idle && !firstRun &&  !(isSelected() && owner == pLocalHouse)) {
				flyAround();
		} else if(firstRun && owned) {
			findConstYard();
			setGuardPoint(constYardPoint);
			setDestination(guardPoint);
			firstRun = false;
		} else if (isSelected()) {
            //dbg_relax_print(" Carryall::checkPos %d,%d %d,%d  \n", destination.x,destination.y, guardPoint.x, guardPoint.y);
		}
	}

}

void Carryall::deployUnit(Uint32 unitID)
{
	bool found = false;

	std::list<Uint32>::iterator iter;
	for(iter = pickedUpUnitList.begin() ; iter != pickedUpUnitList.end(); ++iter) {
		if(*iter == unitID) {
			found = true;
			break;
		}
	}

	if(found == false) {
        return;
	}




	UnitBase* pUnit = (UnitBase*) (currentGame->getObjectManager().getObject(unitID));

	if(pUnit == NULL) {
		return;
    }


	if (found) {
	    currentMaxSpeed = 0.0f;
	    setSpeeds();

	    if (currentGameMap->getTile(location)->hasANonInfantryGroundObject()) {
			ObjectBase* object = currentGameMap->getTile(location)->getNonInfantryGroundObject();
			if (object->getOwner() == getOwner()) {
				if (object->getItemID() == Structure_RepairYard) {
					if (((RepairYard*)object)->isFree()) {
						pUnit->setTarget(object);
						pUnit->setGettingRepaired();
						pUnit = NULL;

					} else {
					    // carryall has booked this repair yard but now will not go there => unbook
						((RepairYard*)object)->unBook();

						// unit is still going to repair yard but was unbooked from repair yard at pickup => book now
						((RepairYard*)object)->book();
					}
				} else if ((object->getItemID() == Structure_Refinery) && (pUnit->getItemID() == Unit_Harvester)) {
					if (((Refinery*)object)->isFree()) {
						((Harvester*)pUnit)->setTarget(object);
						((Harvester*)pUnit)->setReturned();

						pUnit = NULL;
						goingToRepairYard = false;
					}
				}
			}
		}

		if(pUnit != NULL) {
			pUnit->setAngle(drawnAngle);
			if (deployPos.isInvalid()) {
				dbg_print(" Carryall::deployUnit _deployPos was invalid, new deployPos\n");
				if (fallBackPos.isInvalid()) {
					fallBackPos = currentGameMap->findDeploySpot(pUnit, location, pUnit->getDestination().isValid() ? pUnit->getDestination() : Coord::Invalid());
				}
				deployPos = fallBackPos;
			}

			bool sound ;

			if (pUnit->isAFlyingUnit()) {
				sound = true;
			} else if (pUnit->isInfantry()) {
				sound = true;
			} else if (pUnit->isAUnit() && pUnit->getItemID() != Unit_Harvester ) {
				sound = true;
			} else  {
				sound = false;
			}

			dbg_print(" Carryall::deployUnit UnitDeploying to destination(%d,%d-%d,%d) [#%d] \n", pUnit->getDestination().x,pUnit->getDestination().y,deployPos.x,deployPos.y,tryDeploy);

			if ((abs(location.x - deployPos.x) <= 1 && abs(location.y - deployPos.y) <= 1) && pUnit->canPass(deployPos.x,deployPos.y)) {
				pUnit->deploy(deployPos, sound);
				deployPos.invalidate();
				tryDeploy = 0;
				pUnit = NULL;
			}
			else {
				tryDeploy++;
				if (tryDeploy > 3) {
					tryDeploy = 0;
					if (pUnit->canPass(location.x,location.y)) {
						pUnit->deploy(location, sound);
						dbg_print(" Carryall::deployUnit UnitDeploying for too long, DUMPING cargo @(%d,%d) instead of destination(%d,%d-%d,%d) [#%d] \n",location.x, location.y, pUnit->getDestination().x,pUnit->getDestination().y,deployPos.x,deployPos.y,tryDeploy);
						deployPos.invalidate();
						pUnit = NULL;
					} else {
						dbg_print(" Carryall::deployUnit UnitDeploying for too long, CAN NOT DUMP cargo @(%d,%d) instead of destination(%d,%d-%d,%d) [#%d] \n",location.x, location.y, pUnit->getDestination().x,pUnit->getDestination().y,deployPos.x,deployPos.y,tryDeploy);
						if (fallBackPos.isInvalid() || deployPos == fallBackPos)
							fallBackPos = currentGameMap->findDeploySpot(pUnit, location, pUnit->getDestination().isValid() ? pUnit->getDestination() : Coord::Invalid());
						deployPos = fallBackPos;
						return;
					}
				} else {
					setDestination(deployPos);
					if(pUnit->getOwner() == pLocalHouse) {
						//soundPlayer->playVoiceAt(DropImpossible,pLocalHouse->getHouseID(),deployPos);
					}
					return;
				}
			}
		}

		if (true) {
			pickedUpUnitList.remove(unitID);
			soundPlayer->playSoundAt(Sound_Drop, location);
			setFallbackPos(Coord::Invalid());
		}

		if (pickedUpUnitList.empty())
		{
			if(!aDropOfferer) {
				booked = false;
                idle = true;
			}
			droppedOffCargo = true;
			drawnFrame = 0;

			clearPath();
		}
	}
}

void Carryall::destroy()
{
    // destroy cargo
	std::list<Uint32>::const_iterator iter;
	for(iter = pickedUpUnitList.begin() ; iter != pickedUpUnitList.end(); ++iter) {
		UnitBase* pUnit = (UnitBase*) (currentGame->getObjectManager().getObject(*iter));
		if(pUnit != NULL) {
			pUnit->destroy();
		}
	}
	pickedUpUnitList.clear();

	// place wreck
    if(isVisible() && currentGameMap->tileExists(location)) {
        Tile* pTile = currentGameMap->getTile(location);
        pTile->assignDeadUnit(DeadUnit_Carrall, owner->getHouseID(), Coord((Sint32) realX, (Sint32) realY));
    }

	AirUnit::destroy();
}

void Carryall::releaseTarget() {
    setTarget(NULL);
    dbg_print(" Carryall::releaseTarget   \n");
    if(!hasCargo()) {
        booked = false;
        idle = true;
        setDestination(guardPoint);
    }
}

void Carryall::engageTarget()
{
    if(target && (target.getObjPointer() == NULL)) {
        // the target does not exist anymore
    	dbg_print(" Carryall::engageTarget target doesnt exist any more  \n");
        releaseTarget();
        return;
    }

    // TODO : change the logic to be able to pickup & keep cargo of forced carryall
    if(target && (target.getObjPointer()->isActive() == false)) {
        // the target changed its state to inactive
    	dbg_print(" Carryall::engageTarget target is not active any more  \n");
        releaseTarget();
        return;
    }

    // TODO : change the logic to be able to pickup & keep cargo of forced carryall
    if(target && target.getObjPointer()->isAGroundUnit() && !((GroundUnit*)target.getObjPointer())->isAwaitingPickup() /* && !wasForced() */) {
        // the target changed its state to not awaiting pickup anymore
    	dbg_print(" Carryall::engageTarget target is not more awaiting pickup or carryall is not forced to pick it up (!awaiting:%s,!forced:%s)  \n",!((GroundUnit*)target.getObjPointer())->isAwaitingPickup() ? "y" : "n", !wasForced() ? "y" : "n");
        releaseTarget();
        return;
    }

    if(target && (target.getObjPointer()->getOwner()->getTeam() != owner->getTeam())) {
        // the target changed its owner (e.g. was deviated)
        releaseTarget();
        return;
    }

    Coord targetLocation;
    if (target.getObjPointer()->getItemID() == Structure_Refinery) {
        targetLocation = target.getObjPointer()->getLocation() + Coord(2,0);
    } else {
        targetLocation = target.getObjPointer()->getClosestPoint(location);
    }

    Coord realLocation = Coord(lround(realX), lround(realY));
    Coord realDestination = targetLocation * TILESIZE + Coord(TILESIZE/2,TILESIZE/2);

    targetAngle = lround((float)NUM_ANGLES*destinationAngle(location, destination)/256.0f);
    if (targetAngle == 8) {
        targetAngle = 0;
    }

    targetDistance = distanceFrom(realLocation, realDestination);

    if (targetDistance <= TILESIZE/8) {
        if (target.getObjPointer()->isAUnit()) {
            targetAngle = ((GroundUnit*)target.getObjPointer())->getAngle();
        }

        if(hasCargo()) {
            if(target.getObjPointer()->isAStructure()) {
            	if (((StructureBase*)(target.getObjPointer()))->getItemID() == Structure_RepairYard && ((RepairYard*)(target.getObjPointer()))->isFree()) {
					while(pickedUpUnitList.begin() != pickedUpUnitList.end()) {
						deployUnit(*(pickedUpUnitList.begin()) );
					}
					setTarget(NULL);
					setDestination(guardPoint);

            	} else {
            		flyAround();
            	}

            }
        } else {
        	tryDeploy=0;
            pickupTarget();
        }
    } else {
        setDestination(targetLocation);
    }
}

void Carryall::giveCargo(UnitBase* newUnit)
{
	if(newUnit == NULL) {
		return;
    }
	dbg_print(" Carryall::giveCargo %d(%d)   \n", newUnit->getObjectID(),newUnit->getItemID());
	booked = true;
	pickedUpUnitList.push_back(newUnit->getObjectID());
	tryDeploy=0;
	deployPos.invalidate();

	newUnit->setPickedUp(this);

	if (getItemID() != Unit_Frigate)
		drawnFrame = 1;

	droppedOffCargo = false;

}

void Carryall::pickupTarget()
{
	if (hasCargo()) return;

    currentMaxSpeed = 0.0f;
    setSpeeds();

    ObjectBase* pTarget = target.getObjPointer();

	if(pTarget->isAGroundUnit()) {
        GroundUnit* pGroundUnitTarget = dynamic_cast<GroundUnit*>(pTarget);

        if(pTarget->getHealth() <= 0.0f) {
            // unit died just in the moment we tried to pick it up => carryall also crashes
            setHealth(0.0f);
            return;
        }

		if (  pTarget->hasATarget()
			|| ( pGroundUnitTarget->getGuardPoint() != pTarget->getLocation())
			|| pGroundUnitTarget->isBadlyDamaged())
		{


			if(pGroundUnitTarget->isBadlyDamaged() /*|| (pTarget->hasATarget() == false && pTarget->getItemID() != Unit_Harvester)*/)	{
				dbg_print(" Carryall::pickupTarget %d(%d) do repair  \n", pTarget->getObjectID(),pTarget->getItemID());
				pGroundUnitTarget->doRepair();
			}

			ObjectBase* newTarget = pGroundUnitTarget->hasATarget() ? pGroundUnitTarget->getTarget() : NULL;

			pickedUpUnitList.push_back(target.getObjectID());
			pGroundUnitTarget->setPickedUp(this);

			drawnFrame = 1;
			booked = true;
			soundPlayer->playSoundAt(Sound_AirBrakes,location);
			setFallbackPos(location);

            if(newTarget && ((newTarget->getItemID() == Structure_Refinery)
                              || (newTarget->getItemID() == Structure_RepairYard)))
            {
                setTarget(newTarget);
                if(newTarget->getItemID() == Structure_Refinery) {
                    setDestination(target.getObjPointer()->getLocation() + Coord(2,0));
                } else {
                    setDestination(target.getObjPointer()->getClosestPoint(location));
                }
            } else if (pGroundUnitTarget->getDestination().isValid()) {
            	dbg_print( " Carryall::pickupTarget %d(%d) go get it !  \n", pGroundUnitTarget->getObjectID(),pGroundUnitTarget->getItemID());
                setDestination(pGroundUnitTarget->getDestination());
            }

            clearPath();

		} else {
			dbg_print( " Carryall::pickupTarget %d(%d) that is going to be pickup \n", pGroundUnitTarget->getObjectID(),pGroundUnitTarget->getItemID());
			pGroundUnitTarget->setAwaitingPickup(false);
			//releaseTarget();
		}
	} else {
        // get unit from structure
        ObjectBase* pObject = target.getObjPointer();
        bool deployed = false;
        if(pObject->getItemID() == Structure_Refinery) {
            // get harvester
        	deployed = ((Refinery*) pObject)->deployHarvester(this);
        } else if(pObject->getItemID() == Structure_RepairYard) {
            // get repaired unit
        	dbg_print("Carryall::pickupTarget %d deployRepairUnit\n", this->getObjectID());
        	deployed = ((RepairYard*) pObject)->deployRepairUnit(this);
        } else if (pObject->getItemID() == Structure_StarPort) {
            // get orderer unit
        	dbg_print("Carryall::pickupTarget %d deployOrderedUnit\n", this->getObjectID());
        	deployed = ((StarPort*) pObject)->deployOrderedUnit(this);
        }

        if (!deployed) {
        	releaseTarget();
        }
        clearPath();
	}
}

void Carryall::setTarget(const ObjectBase* newTarget) {
	if(target.getObjPointer() != NULL
		&& targetFriendly
		&& target.getObjPointer()->isAGroundUnit()
		&& (((GroundUnit*)target.getObjPointer())->getCarrier() == this))
	{
		((GroundUnit*)target.getObjPointer())->bookCarrier(NULL);
	}

	UnitBase::setTarget(newTarget);

	if(target && targetFriendly && target.getObjPointer()->isAGroundUnit()) {
		((GroundUnit*)target.getObjPointer())->setAwaitingPickup(true);
	}

	booked = target;
}

void Carryall::targeting() {
	if(target) {
		engageTarget();
	}
}

bool Carryall::canPass(int xPos, int yPos) const
{
	// When selected units fly at lower altitude and then cannot pass above other flying unit
	return ((isSelected() && owner == pLocalHouse) /* TODO || evasiveTimer >0*/) ? AirUnit::canPass(xPos, yPos) && !currentGameMap->getTile(xPos, yPos)->hasAnAirUnit() : AirUnit::canPass(xPos, yPos) ;
}




void Carryall::findConstYard() {
    float	closestYardDistance = std::numeric_limits<float>::infinity();;
    ConstructionYard* bestYard = NULL;

    RobustList<StructureBase*>::const_iterator iter;
    for(iter = structureList.begin(); iter != structureList.end(); ++iter) {
        StructureBase* tempStructure = *iter;

        if((tempStructure->getItemID() == Structure_ConstructionYard) && (tempStructure->getOwner() == owner)) {
            ConstructionYard* tempYard = ((ConstructionYard*) tempStructure);
            Coord closestPoint = tempYard->getClosestPoint(location);
            float tempDistance = distanceFrom(location, closestPoint);

            if(tempDistance < closestYardDistance) {
                closestYardDistance = tempDistance;
                bestYard = tempYard;
            }
        }
    }

    if(bestYard && guardPoint.isInvalid()) {
        constYardPoint = bestYard->getClosestPoint(location);
        dbg_print("Carryall::findConstYard getting bestYard %d,%d\n", constYardPoint.x,constYardPoint.y);
    } else {
        constYardPoint = guardPoint;
        dbg_print("Carryall::findConstYard getting specified guardPoint %d,%d\n", constYardPoint.x,constYardPoint.y);
    }



    // make all circles a bit different
    constYardPoint.x += (getObjectID() % 5) - 2;
    constYardPoint.y += ((getObjectID() >> 2) % 5) - 2;

    // stay on map
    constYardPoint.x = std::min(currentGameMap->getSizeX()-11, std::max(constYardPoint.x, 9));
    constYardPoint.y = std::min(currentGameMap->getSizeY()-11, std::max(constYardPoint.y, 9));

    static const Coord circles[][8] = {
                                        { Coord(-2,-6), Coord(3,-6), Coord(7,-2), Coord(7,3), Coord(3,7), Coord(-2,7), Coord(-6,3), Coord(-6,-2) },
                                        { Coord(-2,-6), Coord(-6,-2), Coord(-6,3), Coord(-2,7), Coord(3,7), Coord(7,3), Coord(7,-2), Coord(3,-6) },
                                        { Coord(-3,-8), Coord(4,-8), Coord(9,-3), Coord(9,4), Coord(4,9), Coord(-3,9), Coord(-8,4), Coord(-8,-3) },
                                        { Coord(-3,-8), Coord(-8,-3), Coord(-8,4), Coord(-3,9), Coord(4,9), Coord(9,4), Coord(9,-3), Coord(4,-8) }
                                      };

    const Coord* pUsedCircle = circles[currentGame->randomGen.rand(0,3)];


    for(int i=0;i<8; i++) {
        flyPoints[i] = constYardPoint + pUsedCircle[i];
    }
}



