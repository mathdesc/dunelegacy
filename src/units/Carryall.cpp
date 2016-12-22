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
#include <sand.h>
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

	for(int i=0; i < 8; i++) {
		flyPoints.push_back(Coord::Invalid());
	}
	curFlyPoint = 0;
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

	flyPoints = stream.readUint32CoordVector();
	/*for (auto &fp : flyPoints) {
			fp.x = stream.readSint32();
			fp.y = stream.readSint32();
	}*/


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
	deployPos = Coord::Invalid();
	fallBackPos = Coord::Invalid();

	for(int i=0; i < 8; i++) {
		flyPoints.push_back(Coord::Invalid());
	}
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
	stream.writeUint32CoordVector(flyPoints);
	/*
	for (auto &fp : flyPoints) {
		stream.writeSint32(fp.x);
		stream.writeSint32(fp.y);
	}*/

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

    if((fellow || hasCargo()) && dist < 256.0f) {
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

	if (hasCargo()) {
		float weight = 0;
		UnitBase* pUnit ;

		std::list<Uint32>::iterator iter;
		for(iter = pickedUpUnitList.begin() ; iter != pickedUpUnitList.end(); ++iter) {
			pUnit = (UnitBase*) (currentGame->getObjectManager().getObject(*iter));
			if (!pUnit) continue;
			weight += pUnit->getWeight();
		}

		speed *= (1.0f-(CARRYALL_WEIGHTMULT * (weight/10.0f)));
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

	AirUnit::deploy(newLocation, getOwned());

	respondable = true;
}

void Carryall::flyAround() {

	//fly around set flyPoints
	Coord point = this->getClosestPoint(location);

	if(point == guardPoint  && attackMode != STOP) {
		//arrived at point, move to next
		curFlyPoint++;
		curFlyPoint %= 8;



      /*  int looped = 0;
		while(!(currentGameMap->tileExists(flyPoints[curFlyPoint].x, flyPoints[curFlyPoint].y)) && looped <= 2) {
			curFlyPoint++;

			if(curFlyPoint >= 8) {
				curFlyPoint = 0;
				looped++;
			}
		}*/

		setGuardPoint(flyPoints[curFlyPoint]);
		setDestination(guardPoint);

	}
}


void Carryall::handleDamage(int damage, Uint32 damagerID, House* damagerOwner) {
	UnitBase::handleDamage(damage, damagerID, damagerOwner);
	// Emergency drop
	if (getHealth() < getMaxHealth()/3 && hasCargo() ) {
		doCancel();
		if (fallBackPos.isValid()) {
			setDestination(fallBackPos);
		} else {
			setDestination(guardPoint);
		}
	}
}


void Carryall::doMove2Pos(int xPos, int yPos, bool bForced) {

	if (canPass(xPos,yPos)) {

		if (!hasCargo()) {
			Coord gp = this->guardPoint ;
			this->guardPoint = Coord(xPos,yPos);
			 if (!getFlyPlan()) {
				 this->guardPoint = gp;
				 soundPlayer->playSound(InvalidAction);
			 } else {
				 soundPlayer->playSound(Affirmative);
				 UnitBase::doMove2Pos(xPos,yPos,bForced);
			 }
		} else {
			std::list<Uint32>::iterator iter;
			UnitBase* pUnit = NULL;
			for(iter = pickedUpUnitList.begin() ; iter != pickedUpUnitList.end(); ++iter) {
				pUnit = (UnitBase*) (currentGame->getObjectManager().getObject(*iter));
				if (!pUnit) continue;
				pUnit->setDestination(xPos,yPos);
				pUnit->setGuardPoint(xPos,yPos);
				pUnit->setForced(true);
			}
			if (pUnit != NULL) {
				giveDeliveryOrders(pUnit,pUnit->getDestination(),pUnit->getDestination(),pUnit->getLocation());
			}
			setForced(bForced);
		}
		dbg_print(" Carryall::doMove2Pos %d,%d %d,%d  \n", xPos,yPos, guardPoint.x, guardPoint.y);

	}
}


void Carryall::checkPos()
{
	AirUnit::checkPos();

	if (active)	{
		if (hasCargo() && (location == destination) && (distanceFrom(realX, realY, destination.x * TILESIZE + (TILESIZE/2), destination.y * TILESIZE + (TILESIZE/2)) < TILESIZE/4) ) {
#if 1
		    // drop up to 5 infantry units at once or one other unit
            int droppedUnits = 0;
            do {
            	// manual drop
            	if (attackMode == STOP) break;

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
                dbg_print(" Carryall::checkPos %d,%d %d,%d  Deploying %d(%s)\n", destination.x,destination.y, guardPoint.x, guardPoint.y,pUnit->getObjectID(),resolveItemName(pUnit->getItemID()).c_str());
                deployUnit(unitID);
                droppedUnits++;

                if((pUnit != NULL) && (pUnit->isInfantry() == false)) {
                    // we dropped a non infantry unit
                    // => do not drop another unit
                    break;
                }
            } while(hasCargo() && (droppedUnits < 6) );

            if(hasCargo()) {
            	/*
                // find next place to drop
                for(int i=8;i<18;i++) {
                    int r = currentGame->randomGen.rand(3,i/2);
                    float angle = 2.0f * strictmath::pi * currentGame->randomGen.randFloat();

                    Coord dropCoord = location + Coord( (int) (r*strictmath::sin(angle)), (int) (-r*strictmath::cos(angle)));
                    if(currentGameMap->tileExists(dropCoord) && currentGameMap->getTile(dropCoord)->hasAGroundObject() == false) {
                        setDestination(dropCoord);
                        break;
                    }
                }*/
            	setDestination(deployPos);
            } else {
                setFellow(NULL);
                setDestination(guardPoint);
                idle = true;
            }
#endif
		} else if( (isBooked() == false) && idle && !firstRun /*&&  !(isSelected() && owner == pLocalHouse)*/) {
				flyAround();
		} else if(firstRun && owned) {
			if (getFlyPlan()) {
				setGuardPoint(constYardPoint);
				setDestination(guardPoint);
			}
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
	    // XXX : This code should be reviewed shoudn't it go into checkPos instead ?
	    if (currentGameMap->getTile(location)->hasANonInfantryGroundObject()) {
			ObjectBase* object = currentGameMap->getTile(location)->getNonInfantryGroundObject();
			ObjectBase* newobject = object;
			if (object->getOwner() == getOwner()) {
				if (object->getItemID() == Structure_RepairYard) {
					if (((RepairYard*)object)->isFree()) {
						pUnit->setFellow(object);
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
						((Harvester*)pUnit)->setFellow(object);
						((Harvester*)pUnit)->setReturned();

						pUnit = NULL;
						goingToRepairYard = false;
					} else {
						dbg_print(" Carryall::deployUnit _Awaiting on refinery, find a free one for %d\n",((Harvester*)pUnit)->getObjectID());
						newobject=((Harvester*)pUnit)->findRefinery();
						if (newobject != NULL) {
							((Harvester*)pUnit)->setFellow(newobject);
							setFellow(newobject);
							deployPos = (newobject->getLocation());
							setDestination(newobject->getLocation());
						}

					}
				}
			}
		}

		if(pUnit != NULL) {
			pUnit->setAngle(drawnAngle);

			// In case not delivery orders has been give we are charged to find a deploy zone
			if (deployPos.isInvalid()) {
				dbg_print(" Carryall::deployUnit deployPos (%d,%d) was invalid, fallback (%d,%d) new deployPos for %d(%s)\n",
						deployPos.x,deployPos.y,fallBackPos.x,fallBackPos.y,pUnit->getObjectID(),resolveItemName(pUnit->getItemID()).c_str());
				if (fallBackPos.isInvalid() &&  ((currentGame->getGameCycleCount() + getObjectID()*1337) % 5) == 0) {
					fallBackPos = currentGameMap->findDeploySpot(pUnit, location, pUnit->getDestination().isValid() ? pUnit->getDestination() : Coord::Invalid());
					if (fallBackPos.isInvalid()) {
						dbg_print(" Carryall::deployUnit _deployPos was invalid, can't get a deployPos ! release fellow %d(%s)\n",pUnit->getObjectID(),resolveItemName(pUnit->getItemID()).c_str());
						releaseFellow();
						return;
					}
				}
				deployPos = fallBackPos;
				if (deployPos.isValid())
					setDestination(deployPos);
				return;
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

			dbg_print(" Carryall::deployUnit UnitDeploying to destination(%d,%d %d,%d) pos=%d,%d [#%d] \n",
					pUnit->getDestination().x,pUnit->getDestination().y,deployPos.x,deployPos.y,location.x,location.y,tryDeploy);

			if (deployPos.isValid() && (distanceFrom(realX, realY, deployPos.x * TILESIZE + (TILESIZE/2), deployPos.y * TILESIZE + (TILESIZE/2)) <= TILESIZE/2) && pUnit->canPass(deployPos.x,deployPos.y)) {
				// arrive at in the drop zone
				dbg_print(" Carryall::deployUnit UnitDeploying arrived at drop zone @(%d,%d) for a deploy @(%d,%d) [#%d] \n",location.x, location.y,deployPos.x,deployPos.y,tryDeploy);
				pUnit->deploy(deployPos, sound);
				deployPos.invalidate();
				tryDeploy = 0;
				pUnit = NULL;
			}
			else {
				{
					// It look like we can deploy the unit at deploy pos
					if (deployPos.isValid() ) {
						/*
						std::pair<std::vector<Coord>,int> p_v_i;
						p_v_i = currentGameMap->getTile(location)->getFreeTile();
						float shorter = std::numeric_limits<float>::infinity(), distance;
						Coord best = Coord(0,0);
						for(Coord n : p_v_i.first) {
							distance = distanceFrom(deployPos+n,pUnit->getDestination());
							if (distance < shorter) {
								shorter = distance;
								best = n;
							}
						}
						deployPos +=best;
						dbg_print(" Carryall::deployUnit ReDeploying to free spot %d,%d\n",deployPos.x,deployPos.y);
						setDestination(deployPos);
						*/
						float shorter = std::numeric_limits<float>::infinity(), distance;
						Coord best = Coord(0,0);
						Coord t;
					     for(int i=8;i<18;i++) {
							int r = currentGame->randomGen.rand(3,i/2);
							float angle = 2.0f * strictmath::pi * currentGame->randomGen.randFloat();
							t =  Coord( (int) (r*strictmath::sin(angle)), (int) (-r*strictmath::cos(angle)));
							distance = distanceFrom(deployPos+t,pUnit->getDestination());
							Coord dropCoord = location + t;
							if(currentGameMap->tileExists(dropCoord) && canPass(dropCoord.x,dropCoord.y) && currentGameMap->getTile(dropCoord)->hasAGroundObject() == false) {
								if (distance < shorter) {
									shorter = distance;
									best = t;
								}
							}
						 }
					     deployPos +=best;
						 dbg_print(" Carryall::deployUnit ReDeploying to free spot %d,%d\n",deployPos.x,deployPos.y);
						 setDestination(deployPos);
					}
					return;
				}
			}
		}

		if (true) {
			pickedUpUnitList.remove(unitID);
			soundPlayer->playSoundAt(Sound_Drop, location);
		}

		if (pickedUpUnitList.empty())
		{
			setFallbackPos(Coord::Invalid());
			setDeployPos(Coord::Invalid());
			if(!aDropOfferer) {
				booked = false;
                idle = true;
			}
			droppedOffCargo = true;
			drawnFrame = 0;

			clearPath();
		}
	    currentMaxSpeed = 0.0f;
	    setSpeeds();
	}
}

void Carryall::destroy()
{
    // destroy cargo
	std::list<Uint32>::const_iterator iter;
	for(iter = pickedUpUnitList.begin() ; iter != pickedUpUnitList.end(); ++iter) {
		UnitBase* pUnit = (UnitBase*) (currentGame->getObjectManager().getObject(*iter));
		if(pUnit != NULL) {
			//TODO add a chance for cargo to survive the crash, damaged
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



void Carryall::releaseFellow() {

    dbg_print(" Carryall::releaseFellow  %d \n", fellow.getObjectID());
    setFellow(NULL);
    if(!hasCargo()) {
        booked = false;
        idle = true;
        setDestination(guardPoint);
    }
}

void Carryall::engageTarget() {
	engageFollow();

}


void Carryall::engageFollow() {

    if(fellow && (fellow.getObjPointer() == NULL)) {
        // the target does not exist anymore
    	dbg_print(" Carryall::engageTarget target doesnt exist any more  \n");
    	releaseFellow();
        return;
    }

    // TODO : change the logic to be able to pickup & keep cargo of forced carryall
    if(fellow && (fellow.getObjPointer()->isActive() == false)) {
        // the target changed its state to inactive
    	dbg_print(" Carryall::engageTarget target is not active any more  \n");
    	releaseFellow();
        return;
    }

    // TODO : change the logic to be able to pickup & keep cargo of forced carryall
    if(fellow && fellow.getObjPointer()->isAGroundUnit() && !((GroundUnit*)fellow.getObjPointer())->isAwaitingPickup() /* && !wasForced() */) {
        // the target changed its state to not awaiting pickup anymore
    	dbg_print(" Carryall::engageTarget target is not more awaiting pickup or carryall is not forced to pick it up (!awaiting:%s,!forced:%s)  \n",!((GroundUnit*)fellow.getObjPointer())->isAwaitingPickup() ? "y" : "n", !wasForced() ? "y" : "n");
    	releaseFellow();
        return;
    }

    if(fellow && (fellow.getObjPointer()->getOwner()->getTeam() != owner->getTeam())) {
        // the target changed its owner (e.g. was deviated)
    	releaseFellow();
        return;
    }

    Coord targetLocation;
    if (fellow.getObjPointer()->getItemID() == Structure_Refinery) {
        targetLocation = fellow.getObjPointer()->getLocation() + Coord(2,0);
    } else {
        targetLocation = fellow.getObjPointer()->getClosestPoint(location);
    }

    Coord realLocation = Coord(lround(realX), lround(realY));
    Coord realDestination = targetLocation * TILESIZE + Coord(TILESIZE/2,TILESIZE/2);

    targetAngle = lround((float)NUM_ANGLES*destinationAngle(location, destination)/256.0f);
    if (targetAngle == 8) {
        targetAngle = 0;
    }

    targetDistance = distanceFrom(realLocation, realDestination);

    if (targetDistance <= TILESIZE/8) {
        if (fellow.getObjPointer()->isAUnit()) {
            targetAngle = ((GroundUnit*)fellow.getObjPointer())->getAngle();
        }

        if(hasCargo()) {
            if(fellow.getObjPointer()->isAStructure()) {
            	if (((StructureBase*)(fellow.getObjPointer()))->getItemID() == Structure_RepairYard && ((RepairYard*)(fellow.getObjPointer()))->isFree()) {
					while(pickedUpUnitList.begin() != pickedUpUnitList.end()) {
						deployUnit(*(pickedUpUnitList.begin()) );
					}
					setFellow(NULL);
					setDestination(guardPoint);

            	} else if (((StructureBase*)(fellow.getObjPointer()))->getItemID() == Structure_Refinery && ((RepairYard*)(fellow.getObjPointer()))->isFree()) {
					while(pickedUpUnitList.begin() != pickedUpUnitList.end()) {
						deployUnit(*(pickedUpUnitList.begin()) );
					}
					setFellow(NULL);
					setDestination(guardPoint);

            	}  else {
            		flyAround();
            	}

            } else if(fellow.getObjPointer()->isAUnit() && fellow.getObjPointer()->isInfantry()) {
            	tryDeploy=0;
                pickupTarget();
            }
        } else {
        	tryDeploy=0;
            pickupTarget();
        }
    } else {
        setDestination(targetLocation);
    }
}
void Carryall::doCancel() {

	clearPath();
	findTargetTimer = 0;
	oldTargetTimer = 0;
	//dump here
	int size = pickedUpUnitList.size();
	int tries = 0;
	while(pickedUpUnitList.begin() != pickedUpUnitList.end() && ++tries < 8) {
		deployUnit(*(pickedUpUnitList.begin()) );
	}


	// cannot deploy, still has a cargo, return to pickup location !
	if (hasCargo()) {
		UnitBase* pUnit = (UnitBase*) (currentGame->getObjectManager().getObject(pickedUpUnitList.front()));
		if (pUnit->getGuardPoint().isValid()) {
			setDestination(pUnit->getGuardPoint());
		} else {
			UnitBase::doMove2Pos(guardPoint.x,guardPoint.y,true);
		}
	} else {
		deployPos = Coord::Invalid();
		fallBackPos = Coord::Invalid();
	}
	releaseFellow();

}

void Carryall::giveCargo(UnitBase* newUnit)
{
	if(newUnit == NULL) {
		return;
    }
	dbg_print(" Carryall::giveCargo now has a cargo %d(%s)   \n", newUnit->getObjectID(),getItemNameByID(newUnit->getItemID()).c_str());
	booked = true;
	pickedUpUnitList.push_back(newUnit->getObjectID());
	tryDeploy=0;
	deployPos.invalidate();
	firstRun = false;
	newUnit->setPickedUp(this);

	if (getItemID() != Unit_Frigate)
		drawnFrame = 1;

	droppedOffCargo = false;

}

void Carryall::giveDeliveryOrders(UnitBase* newUnit, Coord unitDest, Coord deploy, Coord fallback) {

	if (hasCargo()) {
		setDestination(unitDest);
		setDeployPos(deploy);
		setFallbackPos(fallback);
	} else {
		dbg_print("Carryall::giveDeliveryOrders with no cargo ! \n");
	}
}

void Carryall::pickupTarget()
{


	// TODO multiple load for infantry (max5)
	if (hasCargo()) {
		UnitBase* pUnit = (UnitBase*) (currentGame->getObjectManager().getObject(pickedUpUnitList.front()));
		if ((pickedUpUnitList.size() >= 6 || !pUnit->isInfantry())) {
			soundPlayer->playSound(InvalidAction);
			return;
		}
	}



    ObjectBase* pTarget = fellow.getObjPointer();

	if(pTarget->isAGroundUnit()) {
        GroundUnit* pGroundUnitTarget = dynamic_cast<GroundUnit*>(pTarget);

        if(pTarget->getHealth() <= 0.0f) {
            // unit died just in the moment we tried to pick it up => carryall also crashes
            setHealth(0.0f);
            return;
        }

		if (  pTarget->hasAFellow()
			|| ( pGroundUnitTarget->getGuardPoint() != pTarget->getLocation())
			|| pGroundUnitTarget->isBadlyDamaged())
		{


			if(pGroundUnitTarget->isBadlyDamaged() && !pGroundUnitTarget->isInfantry())	{
				dbg_print(" Carryall::pickupTarget %d(%s) do repair  \n", pGroundUnitTarget->getObjectID(),getItemNameByID(pGroundUnitTarget->getItemID()).c_str());
				pGroundUnitTarget->doRepair();
			}

			ObjectBase* newTarget = pGroundUnitTarget->hasAFellow() ? pGroundUnitTarget->getFellow() : NULL;

			pickedUpUnitList.push_back(pTarget->getObjectID());
			pGroundUnitTarget->setPickedUp(this);

			drawnFrame = 1;
			booked = true;
			soundPlayer->playSoundAt(Sound_AirLift,location);
			setFallbackPos(location);

            if(newTarget && ((newTarget->getItemID() == Structure_Refinery)
                              || (newTarget->getItemID() == Structure_RepairYard)))
            {
                setFellow(newTarget);
                if(newTarget->getItemID() == Structure_Refinery) {
                    setDestination(fellow.getObjPointer()->getLocation() + Coord(2,0));
                } else {
                    setDestination(fellow.getObjPointer()->getClosestPoint(location));
                }
            } else if (pGroundUnitTarget->getDestination().isValid()) {
            	dbg_print( " Carryall::pickupTarget %d(%s) go get it !  \n", pGroundUnitTarget->getObjectID(),getItemNameByID(pGroundUnitTarget->getItemID()).c_str());
                setDestination(pGroundUnitTarget->getDestination());
                giveDeliveryOrders(pGroundUnitTarget,pGroundUnitTarget->getDestination(),pGroundUnitTarget->getDestination(),pGroundUnitTarget->getLocation());
            }

            clearPath();

		} else {
			dbg_print( " Carryall::pickupTarget %d(%s) that is going to be pickup BUGGY : don't seem to be fellowing this carrier \n", pGroundUnitTarget->getObjectID(),getItemNameByID(pGroundUnitTarget->getItemID()).c_str());
			pGroundUnitTarget->setAwaitingPickup(true);
			pGroundUnitTarget->setFellow(this);
			dbg_print( " Carryall::pickupTarget %d(%s) destination : %d,%d\n",
					pGroundUnitTarget->getObjectID(),getItemNameByID(pGroundUnitTarget->getItemID()).c_str(),pGroundUnitTarget->getDestination().x,pGroundUnitTarget->getDestination().y);
			giveDeliveryOrders(pGroundUnitTarget,pGroundUnitTarget->getDestination(),pGroundUnitTarget->getDestination(),pGroundUnitTarget->getLocation());

			//releaseFellow();
		}
	} else {
        // get unit from structure
        ObjectBase* pObject = fellow.getObjPointer();
        bool deployed = false;
        if(pObject->getItemID() == Structure_Refinery) {
            // get harvester
        	dbg_print("Carryall::pickupTarget %d deployHavesterUnit\n", this->getObjectID());
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
        	releaseFellow();
        }
        clearPath();
	}

    currentMaxSpeed = 0.0f;
    setSpeeds();

}


void Carryall::setFellow(const ObjectBase* newTarget) {


	if(fellow.getObjPointer() != NULL && fellow.getObjPointer()->isAGroundUnit() && (((GroundUnit*)fellow.getObjPointer())->getCarrier() == this)) {
		((GroundUnit*)fellow.getObjPointer())->bookCarrier(NULL);
	}

	UnitBase::setFellow(newTarget);

	if(fellow && fellow.getObjPointer()->isAGroundUnit()) {
		((GroundUnit*)fellow.getObjPointer())->setAwaitingPickup(true);
	}

	booked = fellow;
}


void Carryall::setTarget(const ObjectBase* newTarget) {


	UnitBase::setTarget(newTarget);

}

void Carryall::targeting() {


	if (this->isSelected()) err_relax_print("UnitBase::targeting ID:%s(%d) Destination:[%d,%d] AreaGuardRange:%d salv:%s nofollow:%s(%d) notarget:%s(%d)  nooldtarget:%s(%d) noattackpos:%s notmoving:%s notjuststopped:%s notforced:%s guardpoint:[%d,%d] isidle:%s attackmode=%s timer=%d\n",
			getItemNameByID(itemID).c_str(),objectID, destination.x, destination.y, getAreaGuardRange(), salving ? "yes" : "no", isFollowing() ? "y" : "n", fellow.getObjectID(),!target ? "y" : "n", target.getObjectID(),!oldtarget ? "y" : "n", oldtarget.getObjectID(),
			!attackPos ? "y" : "n", !moving ? "y" : "n", !justStoppedMoving ? "y" : "n", !forced ? "y" : "n", guardPoint.x ,guardPoint.y, isIdle() ? "y" : "n", getAttackModeNameByMode(attackMode).c_str(), findTargetTimer);

	if(fellow) {
		engageTarget();
	}


	/*if (target) {
		engageTarget();
	}*/
}

bool Carryall::canPass(int xPos, int yPos) const
{
	bool tile_airunit = currentGameMap->tileExists(xPos, yPos) && (currentGameMap->getTile(xPos, yPos)->isExplored(owner->getHouseID()) || owned == false )
			&&  (!(currentGameMap->getTile(xPos, yPos)->hasAnAirUnit()) ||
				  ((currentGameMap->getTile(xPos, yPos)->getAirUnit()->getOwner() == owner && !isSelected()) &&
				   !(attackMode == STOP && currentGameMap->getTile(xPos, yPos)->getAirUnit()->getAttackMode() == STOP)
				  )
				) ;


	return tile_airunit;
}

ConstructionYard* Carryall::findConstYard() {
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

	return bestYard;
}

bool Carryall::getFlyPlan() {

    ConstructionYard* bestYard = findConstYard();

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
                                        { Coord(-3,-8), Coord(-8,-3), Coord(-8,4), Coord(-3,9), Coord(4,9), Coord(9,4), Coord(9,-3), Coord(4,-8) },
										{ Coord::Invalid()}
                                      };

    Coord c;
    bool impassable;
    int i;

    std::vector<int> v ;


    for (i=0; circles[i][0].isValid(); i++) {
    	impassable = false;
    	for (int j=0; j<8 ; j++) {
    		c = Coord(constYardPoint.x+circles[i][j].x , constYardPoint.y+circles[i][j].y);
    		if (!canPass(c.x,c.y) ) {
    			impassable = true;
    		}
    	}
    	if (!impassable) {
    		v.push_back(i);
    	}
    }

    if (v.size() == 0) {
    	err_print("Carryall::findConstYard can not get a fly plan !   constyard %d,%d\n", constYardPoint.x,constYardPoint.y);
    	return false;
    }

   const Coord* pUsedCircle = circles[currentGame->randomGen.rand(0,v.size()-1)];


	for(int j=0;j<8; j++) {
		flyPoints[j] = constYardPoint + pUsedCircle[j];
	}

	err_print("Carryall::findConstYard get fly plan %d ! ",v.size());
	int h=0;
	for (auto &fp : flyPoints) {
		err_print("%d [%d,%d] - ",h++,fp.x,fp.y);
	}
	err_print("\n");

	return true;
}



