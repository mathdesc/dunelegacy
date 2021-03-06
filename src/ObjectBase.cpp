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

#include <ObjectBase.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/music/MusicPlayer.h>

#include <Game.h>
#include <House.h>
#include <SoundPlayer.h>
#include <Map.h>
#include <ScreenBorder.h>
#include <players/HumanPlayer.h>
#include <GUI/ObjectInterfaces/DefaultObjectInterface.h>

//structures
#include <structures/Barracks.h>
#include <structures/ConstructionYard.h>
#include <structures/GunTurret.h>
#include <structures/HeavyFactory.h>
#include <structures/HighTechFactory.h>
#include <structures/IX.h>
#include <structures/LightFactory.h>
#include <structures/Palace.h>
#include <structures/Radar.h>
#include <structures/Refinery.h>
#include <structures/RepairYard.h>
#include <structures/RocketTurret.h>
#include <structures/Silo.h>
#include <structures/StarPort.h>
#include <structures/Wall.h>
#include <structures/WindTrap.h>
#include <structures/WOR.h>

//units
#include <units/Carryall.h>
#include <units/Devastator.h>
#include <units/Deviator.h>
#include <units/Frigate.h>
#include <units/Harvester.h>
#include <units/Launcher.h>
#include <units/MCV.h>
#include <units/Ornithopter.h>
#include <units/Quad.h>
#include <units/RaiderTrike.h>
#include <units/Saboteur.h>
#include <units/SandWorm.h>
#include <units/SiegeTank.h>
#include <units/Soldier.h>
#include <units/SonicTank.h>
#include <units/Tank.h>
#include <units/Trike.h>
#include <units/Trooper.h>

ObjectBase::ObjectBase(House* newOwner) : originalHouseID(newOwner->getHouseID()), owner(newOwner) {
    ObjectBase::init();

	objectID = NONE;

    health = 0.0f;
    badlyDamaged = false;

	location = Coord::Invalid();
    oldLocation = Coord::Invalid();
	destination = Coord::Invalid();
	realX = 0.0f;
	realY = 0.0f;

    drawnAngle = 0;
	angle = (float) drawnAngle;

	active = false;
    respondable = true;
	selected = false;
	selectedByOtherPlayer = false;

	forced = false;
    setTarget(NULL);
    setFellow(NULL);
    setOldTarget(NULL);
    setOldFellow(NULL);
	targetFriendly = false;
	attackMode = GUARD;

	setVisible(VIS_ALL, false);
}

ObjectBase::ObjectBase(InputStream& stream) {
    originalHouseID = stream.readUint32();
    owner = currentGame->getHouse(stream.readUint32());

    ObjectBase::init();

    health = stream.readFloat();
    badlyDamaged = stream.readBool();

	location.x = stream.readSint32();
	location.y = stream.readSint32();
    oldLocation.x = stream.readSint32();
	oldLocation.y = stream.readSint32();
	destination.x = stream.readSint32();
	destination.y = stream.readSint32();
	realX = stream.readFloat();
	realY = stream.readFloat();

    angle = stream.readFloat();
    drawnAngle = stream.readSint8();

    active = stream.readBool();
    respondable = stream.readBool();

    if(currentGame->getGameInitSettings().getGameType() != GAMETYPE_CUSTOM_MULTIPLAYER) {
        selected = stream.readBool();
        selectedByOtherPlayer = stream.readBool();
    } else {
        selected = false;
        selectedByOtherPlayer = false;
    }

	forced = stream.readBool();
	target.load(stream);
	fellow.load(stream);
	targetFriendly = stream.readBool();
	attackMode = (ATTACKMODE) stream.readUint32();

    stream.readBools(&visible[0], &visible[1], &visible[2], &visible[3], &visible[4], &visible[5]);
}

void ObjectBase::init() {
	itemID = ItemID_Invalid;

	aFlyingUnit = false;
	aGroundUnit = false;
	aStructure = false;
	aUnit = false;
	infantry = false;
	aBuilder = false;

	canAttackStuff = false;
	canSalveAttackStuff = false;

	radius = TILESIZE/2;

	graphicID = -1;
    graphic = NULL;
    numImagesX = 0;
    numImagesY = 0;

}

ObjectBase::~ObjectBase() {
}

void ObjectBase::save(OutputStream& stream) const {
	stream.writeUint32(originalHouseID);
    stream.writeUint32(owner->getHouseID());

    stream.writeFloat(health);
    stream.writeBool(badlyDamaged);

	stream.writeSint32(location.x);
	stream.writeSint32(location.y);
    stream.writeSint32(oldLocation.x);
	stream.writeSint32(oldLocation.y);
	stream.writeSint32(destination.x);
	stream.writeSint32(destination.y);
	stream.writeFloat(realX);
	stream.writeFloat(realY);

    stream.writeFloat(angle);
    stream.writeSint8(drawnAngle);

	stream.writeBool(active);
	stream.writeBool(respondable);

	if(currentGame->getGameInitSettings().getGameType() != GAMETYPE_CUSTOM_MULTIPLAYER) {
        stream.writeBool(selected);
        stream.writeBool(selectedByOtherPlayer);
	}

	stream.writeBool(forced);
    target.save(stream);
    fellow.save(stream);
	stream.writeBool(targetFriendly);
    stream.writeUint32(attackMode);

    stream.writeBools(visible[0], visible[1], visible[2], visible[3], visible[4], visible[5]);
}


/**
    Returns the center point of this object
    \return the center point in world coordinates
*/
Coord ObjectBase::getCenterPoint() const {
    return Coord(lround(realX), lround(realY));
}

Coord ObjectBase::getClosestCenterPoint(const Coord& objectLocation) const {
	return getCenterPoint();
}


int ObjectBase::getMaxHealth() const {
    return currentGame->objectData.data[itemID][originalHouseID].hitpoints;
}

void ObjectBase::handleDamage(int damage, Uint32 damagerID, House* damagerOwner) {
    if(damage >= 0) {
        float newHealth = getHealth();

        newHealth -= damage;

        if(newHealth <= 0.0f) {
            setHealth(0.0f);

            if(damagerOwner != NULL) {
                damagerOwner->informHasKilled(itemID);
            }
        } else {
            setHealth(newHealth);
        }
    }
    if(getOwner() == pLocalHouse && damagerOwner != pLocalHouse) {
        musicPlayer->changeMusic(MUSIC_ATTACK);
    }

    getOwner()->noteDamageLocation(this, damage, damagerID,damagerOwner);
}

void ObjectBase::handleInterfaceEvent(SDL_Event* event) {
	;
}

ObjectInterface* ObjectBase::getInterfaceContainer() {
	return DefaultObjectInterface::create(objectID);
}

void ObjectBase::removeFromSelectionLists() {



	// FIXME : insure remove coordList when its a player controlled unit
	currentGame->getSelectedList().remove(getObjectID());

	if (isAUnit() && owner ==  pLocalHouse && currentGame->getSelectedListCoord().size() >0) {
		if ((currentGame->getGroupLeader()) != NULL)
			err_print("ObjectBase::removeFromSelectionLists groupLeader(%s)#%d\n", (currentGame->getGroupLeader()) == NULL ? "y" : "n",currentGame->getGroupLeader()->getObjectID());
		else {
			err_print("ObjectBase::removeFromSelectionLists nogroupLeader\n");
		}
		std::remove_if(currentGame->getSelectedListCoord().begin(), currentGame->getSelectedListCoord().end(), [=](std::pair<Uint32,Coord> & item) { return item.first == getObjectID() ;} );
		currentGame->getSelectedListCoord().pop_back();
	}

    currentGame->selectionChanged();
    currentGame->getSelectedByOtherPlayerList().remove(getObjectID());



    for(int i=0; i < NUMSELECTEDLISTS; i++) {
        pLocalPlayer->getGroupList(i).remove(getObjectID());
    }


    // GroupLeader has been destroyed
    if (isAUnit() && owner ==  pLocalHouse && (currentGame->getGroupLeader()) != NULL && isSelected()) {
    	if ((currentGame->getGroupLeader())->getObjectID() == objectID && currentGame->getSelectedList().size() >= 1) {
			ObjectBase * newLead = currentGame->getObjectManager().getObject(currentGame->getSelectedList().front());
			err_print("ObjectBase::removeFromSelectionLists new lead is %d \n",newLead->getObjectID());
			currentGame->setGroupLeader(newLead);
			currentGameMap->recalutateCoordinates(newLead,true);
    	}
    }
    selected = false;
}

void ObjectBase::setDestination(int newX, int newY) {
	if(currentGameMap->tileExists(newX, newY) || ((newX == INVALID_POS) && (newY == INVALID_POS))) {
		destination.x = newX;
		destination.y = newY;
	}
}

void ObjectBase::setHealth(float newHealth) {
	if((newHealth >= 0.0f) && (newHealth <= getMaxHealth())) {
		health = newHealth;
        badlyDamaged = (health/(float)getMaxHealth() < BADLYDAMAGEDRATIO);
	}
}

void ObjectBase::setLocation(int xPos, int yPos) {
	if((xPos == INVALID_POS) && (yPos == INVALID_POS)) {
		location.invalidate();
	} else if (currentGameMap->tileExists(xPos, yPos))	{
		location.x = xPos;
		location.y = yPos;
		realX = location.x*TILESIZE;
		realY = location.y*TILESIZE;

		assignToMap(location);
	}
}

void ObjectBase::setObjectID(int newObjectID) {
	if(newObjectID >= 0) {
		objectID = newObjectID;
	}
}

void ObjectBase::setVisible(int team, bool status) {
	if(team == VIS_ALL) {
		for(int i = 0; i < NUM_HOUSES; i++) {
			visible[i] = status;
		}
	} else if ((team >= 0) && (team < NUM_HOUSES)) {
		visible[team] = status;
	}
}

void ObjectBase::setFellow(const ObjectBase* newFellow) {

	if (newFellow == NULL)
		fellow.pointTo(NONE);
	else
		fellow.pointTo(const_cast<ObjectBase*>(newFellow));
}

void ObjectBase::setOldFellow(const ObjectBase* newFellow) {

	if (newFellow == NULL)
		oldfellow.pointTo(NONE);
	else
		oldfellow.pointTo(const_cast<ObjectBase*>(newFellow));
}

void ObjectBase::setOldTarget(const ObjectBase* oldTarget) {
	oldtarget.pointTo(const_cast<ObjectBase*>(oldTarget));
	//targetFriendly = (target && (target.getObjPointer()->getOwner()->getTeam() == owner->getTeam()) && (getItemID() != Unit_Sandworm) && (target.getObjPointer()->getItemID() != Unit_Sandworm));
}


void ObjectBase::setTarget(const ObjectBase* newTarget) {
	target.pointTo(const_cast<ObjectBase*>(newTarget));
	targetFriendly = (target && (target.getObjPointer()->getOwner()->getTeam() == owner->getTeam()) && (getItemID() != Unit_Sandworm) && (target.getObjPointer()->getItemID() != Unit_Sandworm));
}

void ObjectBase::unassignFromMap(const Coord& location) {
	if(currentGameMap->tileExists(location)) {
		currentGameMap->getTile(location)->unassignObject(getObjectID());
	}
}

bool ObjectBase::canAttack(const ObjectBase* object) const {
	if( canAttack()
        && (object != NULL)
		&& (object->isAStructure()
			|| !object->isAFlyingUnit())
		&& ((object->getOwner()->getTeam() != owner->getTeam())
			|| (object->getItemID() == Unit_Sandworm && getOwner()->getHouseID()!= HOUSE_FREMEN))
		&& object->isVisible(getOwner()->getTeam())) {
		return true;
	} else {
		return false;
	}
}

bool ObjectBase::isOnScreen() const {
    Coord position = Coord((int) getRealX(), (int) getRealY());
    Coord size = Coord(graphic[currentZoomlevel]->w/numImagesX, graphic[currentZoomlevel]->h/numImagesY);

	if(screenborder->isInsideScreen(position,size) == true){
		return true;
	} else {
		return false;
	}
}

bool ObjectBase::isVisible(int team) const {
	if((team >= 0) && (team < NUM_HOUSES)) {
		return visible[team];
	} else {
		return false;
	}
}

bool ObjectBase::isVisible() const {
    for(int i=0;i<NUM_HOUSES;i++) {
        if(visible[i]) {
            return true;
        }
    }

    return false;
}

int ObjectBase::getHealthColor() const {
	float healthPercent = (float)health/(float)getMaxHealth();

	if(healthPercent >= BADLYDAMAGEDRATIO) {
		return COLOR_LIGHTGREEN;
	} else if(healthPercent >= HEAVILYDAMAGEDRATIO) {
		return COLOR_YELLOW;
	} else {
		return COLOR_RED;
	}
}

Coord ObjectBase::getClosestPoint(const Coord& point) const {
	return location;
}

const StructureBase* ObjectBase::findClosestTargetStructure() const {

	StructureBase	*closestStructure = NULL;
	float			closestDistance = INFINITY;

    RobustList<StructureBase*>::const_iterator iter;
    for(iter = structureList.begin(); iter != structureList.end(); ++iter) {
        StructureBase* tempStructure = *iter;

        if(canAttack(tempStructure)) {
			Coord closestPoint = tempStructure->getClosestPoint(getLocation());
			float structureDistance = blockDistance(getLocation(), closestPoint);
			bool attacker_mask = tempStructure->isActive() && tempStructure->hasATarget() && tempStructure->getTarget() != NULL
					&& tempStructure->getTarget() ==  this && tempStructure->canAttack(this) && tempStructure->targetInWeaponRange() ;
			bool attacker_mask2 = tempStructure->isActive() && tempStructure->hasAOldTarget() && tempStructure->getOldTarget() != NULL
					&& tempStructure->getOldTarget() ==  this && tempStructure->canAttack(this) && tempStructure->oldtargetInWeaponRange() ;

			if (attacker_mask || attacker_mask2)
				return tempStructure;

			if(tempStructure->getItemID() == Structure_Wall) {
					structureDistance += 20000000.0f; //so that walls are targeted very last
            }

            if(structureDistance < closestDistance)	{
                closestDistance = structureDistance;
                closestStructure = tempStructure;
            }
        }
	}

	return closestStructure;
}

const UnitBase* ObjectBase::findClosestTargetUnit() const {
	UnitBase	*closestUnit = NULL;
	float		closestDistance = INFINITY;

    RobustList<UnitBase*>::const_iterator iter;
    for(iter = unitList.begin(); iter != unitList.end(); ++iter) {
        UnitBase* tempUnit = *iter;

		if(canAttack(tempUnit)) {
			Coord closestPoint = tempUnit->getClosestPoint(getLocation());
			float unitDistance = blockDistance(getLocation(), closestPoint);
			Uint32 titemID = tempUnit->getItemID();
			bool priority_mask = (itemID == Unit_Ornithopter && tempUnit->isActive()) && (titemID == Unit_MCV || titemID == Unit_Ornithopter || titemID == Unit_Harvester || titemID == Unit_Launcher || titemID == Unit_Troopers) ;
			bool attacker_mask = tempUnit->isActive() && !tempUnit->isDestoyed() && tempUnit->hasATarget() && tempUnit->getTarget() != NULL &&
					tempUnit->getTarget() ==  this && tempUnit->canAttack(this) && tempUnit->targetInWeaponRange() ;
			bool attacker_mask2 = tempUnit->isActive() && !tempUnit->isDestoyed() && tempUnit->hasAOldTarget() && tempUnit->getOldTarget() != NULL &&
								tempUnit->getOldTarget() ==  this && tempUnit->canAttack(this) && tempUnit->oldtargetInWeaponRange() ;

			if (attacker_mask || attacker_mask2)
				return tempUnit;
														// FIXME : carryall should be regular target hard to shoot but regular,
														//instead unit should abandon this target if in danger
			if(tempUnit->getItemID() == Unit_Sandworm || tempUnit->getItemID() == Unit_Carryall) {
				unitDistance += 4000.0f; //so that worms are targeted last
			}

			if(unitDistance < closestDistance) {
				if (priority_mask) {
					unitDistance -= 400.0f;
					if (unitDistance == 0) unitDistance = 0.1;
				}
                closestDistance = unitDistance;
				closestUnit = tempUnit;

            }
        }
	}

	return closestUnit;
}






const ObjectBase* ObjectBase::findClosestTarget() const {

	ObjectBase	*closestObject = NULL;
	float			closestDistance = INFINITY;

    RobustList<StructureBase*>::const_iterator iter1;
    for(iter1 = structureList.begin(); iter1 != structureList.end(); ++iter1) {
        StructureBase* tempStructure = *iter1;

        if(canAttack(tempStructure)) {
			Coord closestPoint = tempStructure->getClosestPoint(getLocation());
			float structureDistance = blockDistance(getLocation(), closestPoint);
			bool attacker_mask = tempStructure->isActive() && tempStructure->hasATarget() && tempStructure->getTarget() != NULL &&
					tempStructure->getTarget() ==  this && tempStructure->canAttack(this) && tempStructure->targetInWeaponRange() ;
			bool attacker_mask2 = tempStructure->isActive() && tempStructure->hasAOldTarget() && tempStructure->getOldTarget() != NULL
								&& tempStructure->getOldTarget() ==  this && tempStructure->canAttack(this) && tempStructure->oldtargetInWeaponRange() ;

			if (attacker_mask || attacker_mask2 )
				return tempStructure;

			if(tempStructure->getItemID() == Structure_Wall) {
					structureDistance += 20000000.0f; //so that walls are targeted very last
            }

            if(structureDistance < closestDistance)	{
                closestDistance = structureDistance;
                closestObject = tempStructure;
            }
        }
	}

    RobustList<UnitBase*>::const_iterator iter2;
    for(iter2 = unitList.begin(); iter2 != unitList.end(); ++iter2) {
        UnitBase* tempUnit = *iter2;

		if(canAttack(tempUnit)) {
			Coord closestPoint = tempUnit->getClosestPoint(getLocation());
			float unitDistance = blockDistance(getLocation(), closestPoint);
			bool attacker_mask = tempUnit->isActive() && !tempUnit->isDestoyed() && tempUnit->hasATarget() && tempUnit->getTarget() != NULL &&
					tempUnit->getTarget() ==  this && tempUnit->canAttack(this) && tempUnit->targetInWeaponRange() ;
			bool attacker_mask2 = tempUnit->isActive() && tempUnit->hasAOldTarget() && tempUnit->getOldTarget() != NULL &&
											tempUnit->getOldTarget() ==  this && tempUnit->canAttack(this) && tempUnit->oldtargetInWeaponRange() ;

			if (attacker_mask || attacker_mask2 )
				return tempUnit;

			if(unitDistance < closestDistance) {
                closestDistance = unitDistance;
				closestObject = tempUnit;
            }
        }
	}

	return closestObject;
}


const ObjectBase* ObjectBase::findTarget() const {
 	ObjectBase	*tempTarget, *closestTarget2, *closestTarget;

 	closestTarget = closestTarget2 = NULL;

	int	checkRange = 0;
	int	xPos = location.x;
	int	yPos = location.y;
	int tTID;


	float closestDistance = INFINITY;
	float avg_Distance = 0.0f;
	double agg_Distance = 0;
	int count = 0;

//searches for a target in an area like as shown below
//                     *****
//                   *********
//                  *****T*****
//                   *********
//                     *****

    switch(attackMode) {
        case GUARD: {
            checkRange = getWeaponRange();
        } break;

        case AREAGUARD: {
            checkRange = getAreaGuardRange();
        } break;

        case AMBUSH: {
            checkRange = getViewRange();
        } break;

        case HUNT: {
            // check whole map
        	// TODO Optimize this : keep a cache of relative distance from caller object
           /* const ObjectBase* closestUnit = findClosestTargetUnit();
            if (closestUnit == NULL)
            	 return findClosestTarget();
            else return closestUnit;*/
        	checkRange =  currentGameMap->getSizeX();
        } break;

        case STOP:
        default: {
            return NULL;
        } break;
    }

	int xCheck = xPos - checkRange;

	if(xCheck < 0) {
		xCheck = 0;
	}

	while((xCheck < currentGameMap->getSizeX()) && ((xCheck - xPos) <= checkRange)) {
		int yCheck = (yPos - lookDist[abs(xCheck - xPos)]);

		if(yCheck < 0) {
			yCheck = 0;
		}

		while((yCheck < currentGameMap->getSizeY()) && ((yCheck - yPos) <=  lookDist[abs(xCheck - xPos)])) {


			if(currentGameMap->getTile(xCheck,yCheck)->hasAnObject()) {
				tempTarget = currentGameMap->getTile(xCheck,yCheck)->getObject();
				  if (canAttack(tempTarget))
				  {
					float targetDistance = blockDistance(location, tempTarget->getLocation());
					tTID = tempTarget->getItemID();

				//	prevent_wall = ( (tTID != Structure_Wall) );
				//	no_caryall = ( (tTID != Unit_Carryall) );


					if (tTID == Structure_Wall || tTID == Unit_Carryall )
						targetDistance *= 10;

					bool fre_inf = (tempTarget->isInfantry() && tempTarget->getOriginalHouseID() == HOUSE_FREMEN);

					if( isAUnit() ) {
						if  ( itemID != Unit_Sandworm || (itemID == Unit_Sandworm && ((UnitBase*)tempTarget)->isMoving() && !fre_inf)) {
							agg_Distance += targetDistance ;
							avg_Distance = (float)agg_Distance / ++count;

							// Retaliate on personnel attacker first
							if ( (tempTarget->getTarget() == this && targetDistance <= avg_Distance)) {
									closestTarget = tempTarget;
									closestDistance = targetDistance;
									// This vengence is in plain sight, target is found, period
									if (targetDistance <= getWeaponRange())
										return closestTarget;
							}

							if (targetDistance < closestDistance) {
								closestTarget = tempTarget;
								closestDistance = targetDistance;
							}
						}
					} else {

						float targetDistance = blockDistance(location, tempTarget->getLocation());
						if (targetDistance < closestDistance) {
							closestTarget = tempTarget;
							closestDistance = targetDistance;
						}
					}

				} /* canAttack(tempTarget) */

			}
			yCheck++;
		}
		xCheck++;
	}


	if (closestTarget == this) {
//		if (this->isSelected()) err_print("ObjectBase::findTarget bogus is self ! %d\n",closestTarget->getObjectID());
	}

	if (closestTarget !=NULL) {
//		if (this->isSelected()) err_print("ObjectBase::findTarget finally is %d\n",closestTarget->getObjectID());
	} else {
//		if (this->isSelected()) err_print("ObjectBase::findTarget finally NOONE\n");
	}

	return closestTarget;
}

int ObjectBase::getViewRange() const {
    return currentGame->objectData.data[itemID][originalHouseID].viewrange;
}

int ObjectBase::getAreaGuardRange() const {
    return 2*getWeaponRange();
}

int ObjectBase::getWeaponRange() const {
    return currentGame->objectData.data[itemID][originalHouseID].weaponrange;
}

int ObjectBase::getWeaponReloadTime() const {
    return currentGame->objectData.data[itemID][originalHouseID].weaponreloadtime;
}

int ObjectBase::getInfSpawnProp() const {
    return currentGame->objectData.data[itemID][originalHouseID].infspawnprop;
}



ObjectBase* ObjectBase::createObject(int itemID, House* Owner, Uint32 objectID) {

	ObjectBase* newObject = NULL;
	switch(itemID) {
		case Structure_Barracks:			newObject = new Barracks(Owner); break;
		case Structure_ConstructionYard:	newObject = new ConstructionYard(Owner); break;
		case Structure_GunTurret:			newObject = new GunTurret(Owner); break;
		case Structure_HeavyFactory:		newObject = new HeavyFactory(Owner); break;
		case Structure_HighTechFactory:		newObject = new HighTechFactory(Owner); break;
		case Structure_IX:					newObject = new IX(Owner); break;
		case Structure_LightFactory:		newObject = new LightFactory(Owner); break;
		case Structure_Palace:				newObject = new Palace(Owner); break;
		case Structure_Radar:				newObject = new Radar(Owner); break;
		case Structure_Refinery:			newObject = new Refinery(Owner); break;
		case Structure_RepairYard:			newObject = new RepairYard(Owner); break;
		case Structure_RocketTurret:		newObject = new RocketTurret(Owner); break;
		case Structure_Silo:				newObject = new Silo(Owner); break;
		case Structure_StarPort:			newObject = new StarPort(Owner); break;
		case Structure_Wall:				newObject = new Wall(Owner); break;
		case Structure_WindTrap:			newObject = new WindTrap(Owner); break;
		case Structure_WOR:					newObject = new WOR(Owner); break;

		case Unit_Carryall:					newObject = new Carryall(Owner); break;
		case Unit_Devastator:				newObject = new Devastator(Owner); break;
		case Unit_Deviator:					newObject = new Deviator(Owner); break;
		case Unit_Frigate:					newObject = new Frigate(Owner); break;
		case Unit_Harvester:				newObject = new Harvester(Owner); break;
		case Unit_Soldier:					newObject = new Soldier(Owner); break;
		case Unit_Launcher:					newObject = new Launcher(Owner); break;
		case Unit_MCV:						newObject = new MCV(Owner); break;
		case Unit_Ornithopter:				newObject = new Ornithopter(Owner); break;
		case Unit_Quad:						newObject = new Quad(Owner); break;
		case Unit_Saboteur:					newObject = new Saboteur(Owner); break;
		case Unit_Sandworm:					newObject = new Sandworm(Owner); break;
		case Unit_SiegeTank:				newObject = new SiegeTank(Owner); break;
		case Unit_SonicTank:				newObject = new SonicTank(Owner); break;
		case Unit_Tank:						newObject = new Tank(Owner); break;
		case Unit_Trike:					newObject = new Trike(Owner); break;
		case Unit_RaiderTrike:				newObject = new RaiderTrike(Owner); break;
		case Unit_Trooper:					newObject = new Trooper(Owner); break;
		case Unit_Special: {
            switch(Owner->getHouseID()) {
                case HOUSE_HARKONNEN:       newObject = new Devastator(Owner); break;
                case HOUSE_ATREIDES:        newObject = new SonicTank(Owner); break;
                case HOUSE_ORDOS:           newObject = new Deviator(Owner); break;
                case HOUSE_FREMEN:
                case HOUSE_SARDAUKAR:
                case HOUSE_MERCENARY: {
                    if(currentGame->randomGen.randBool()) {
                         newObject = new SonicTank(Owner);
                    } else {
                        newObject = new Devastator(Owner);
                    }
                } break;
                default:    /* should never be reached */  break;
            }
		} break;

		default:							newObject = NULL;
											fprintf(stderr,"ObjectBase::createObject(): %d is no valid ItemID!\n",itemID);
											break;
	}

	if(newObject == NULL) {
		return NULL;
	}

	if(objectID == NONE) {
		objectID = currentGame->getObjectManager().addObject(newObject);
		newObject->setObjectID(objectID);
	} else {
		newObject->setObjectID(objectID);
	}

	return newObject;
}

ObjectBase* ObjectBase::loadObject(InputStream& stream, int itemID, Uint32 objectID) {
	ObjectBase* newObject = NULL;
	switch(itemID) {
		case Structure_Barracks:			newObject = new Barracks(stream); break;
		case Structure_ConstructionYard:	newObject = new ConstructionYard(stream); break;
		case Structure_GunTurret:			newObject = new GunTurret(stream); break;
		case Structure_HeavyFactory:		newObject = new HeavyFactory(stream); break;
		case Structure_HighTechFactory:		newObject = new HighTechFactory(stream); break;
		case Structure_IX:					newObject = new IX(stream); break;
		case Structure_LightFactory:		newObject = new LightFactory(stream); break;
		case Structure_Palace:				newObject = new Palace(stream); break;
		case Structure_Radar:				newObject = new Radar(stream); break;
		case Structure_Refinery:			newObject = new Refinery(stream); break;
		case Structure_RepairYard:			newObject = new RepairYard(stream); break;
		case Structure_RocketTurret:		newObject = new RocketTurret(stream); break;
		case Structure_Silo:				newObject = new Silo(stream); break;
		case Structure_StarPort:			newObject = new StarPort(stream); break;
		case Structure_Wall:				newObject = new Wall(stream); break;
		case Structure_WindTrap:			newObject = new WindTrap(stream); break;
		case Structure_WOR:					newObject = new WOR(stream); break;

		case Unit_Carryall:					newObject = new Carryall(stream); break;
		case Unit_Devastator:				newObject = new Devastator(stream); break;
		case Unit_Deviator:					newObject = new Deviator(stream); break;
		case Unit_Frigate:					newObject = new Frigate(stream); break;
		case Unit_Harvester:				newObject = new Harvester(stream); break;
		case Unit_Soldier:					newObject = new Soldier(stream); break;
		case Unit_Launcher:					newObject = new Launcher(stream); break;
		case Unit_MCV:						newObject = new MCV(stream); break;
		case Unit_Ornithopter:				newObject = new Ornithopter(stream); break;
		case Unit_Quad:						newObject = new Quad(stream); break;
		case Unit_Saboteur:					newObject = new Saboteur(stream); break;
		case Unit_Sandworm:					newObject = new Sandworm(stream); break;
		case Unit_SiegeTank:				newObject = new SiegeTank(stream); break;
		case Unit_SonicTank:				newObject = new SonicTank(stream); break;
		case Unit_Tank:						newObject = new Tank(stream); break;
		case Unit_Trike:					newObject = new Trike(stream); break;
		case Unit_RaiderTrike:				newObject = new RaiderTrike(stream); break;
		case Unit_Trooper:					newObject = new Trooper(stream); break;

		default:							newObject = NULL;
											fprintf(stderr,"ObjectBase::loadObject(): %d is no valid ItemID!\n",itemID);
											break;
	}

	if(newObject == NULL) {
		return NULL;
	}

	newObject->setObjectID(objectID);

	return newObject;
}

bool ObjectBase::targetInWeaponRange() const {
	if (!target || target.getObjPointer() == NULL)	return false;
    Coord coord = (target.getObjPointer())->getClosestPoint(location);
    float dist = blockDistance(location,coord);
    return ( dist <= getWeaponRange());
}

bool ObjectBase::oldtargetInWeaponRange() const {
	if (!oldtarget || oldtarget.getObjPointer() == NULL)	return false;
    Coord coord = (oldtarget.getObjPointer())->getClosestPoint(location);
    float dist = blockDistance(location,coord);
    return ( dist <= getWeaponRange());
}

bool ObjectBase::targetInAreaGuardRange() const {
	if (!target || target.getObjPointer() == NULL)	return false;
    Coord coord = (target.getObjPointer())->getClosestPoint(location);
    float dist = blockDistance(location,coord);
    return ( dist <= getAreaGuardRange());
}

bool ObjectBase::oldtargetInAreaGuardRange() const {
	if (!oldtarget || oldtarget.getObjPointer() == NULL)	return false;
    Coord coord = (oldtarget.getObjPointer())->getClosestPoint(location);
    float dist = blockDistance(location,coord);
    return ( dist <= getAreaGuardRange());
}



bool ObjectBase::trueFunction() const {
	return true;
}

const ObjectBase* ObjectBase::getNearerTarget(bool inWeaponRange, bool inAreaGuardRange) const {
	const ObjectBase * tmp = NULL , *tmp_old = NULL;
	Coord tmp_Target_Location, tmp_OldTarget_Location;
	tmp_Target_Location = tmp_OldTarget_Location = Coord().Invalid();
	float closeTargetDistance, closeOldTargetDistance  ;



	std::function<bool(void)> f_TargetInWeaponRange = inWeaponRange ? std::bind(&ObjectBase::targetInWeaponRange, this) : std::bind(&ObjectBase::trueFunction,this);
	std::function<bool(void)> f_OldTargetInWeaponRange = inWeaponRange ? std::bind(&ObjectBase::oldtargetInWeaponRange, this) : std::bind(&ObjectBase::trueFunction,this);
	std::function<bool(void)> f_TargetinAreaGuardRange = inAreaGuardRange ? std::bind(&ObjectBase::targetInAreaGuardRange, this) : std::bind(&ObjectBase::trueFunction,this);
	std::function<bool(void)> f_OldTargetinAreaGuardRange = inAreaGuardRange ? std::bind(&ObjectBase::oldtargetInAreaGuardRange, this) : std::bind(&ObjectBase::trueFunction,this);

	if (target && getTarget() != NULL && getTarget()->isActive() &&
			(!getTarget()->isAUnit()  || (getTarget()->isAUnit() && !((UnitBase*)getTarget())->isDestoyed()) ) &&
			f_TargetinAreaGuardRange && f_TargetInWeaponRange()  )
	{
		tmp = getTarget();
		tmp_Target_Location =  getTarget()->getClosestPoint(location);
	} else if (oldtarget && getOldTarget() != NULL && getOldTarget()->isActive() &&
			(!getOldTarget()->isAUnit()  || (getOldTarget()->isAUnit() && !((UnitBase*)getOldTarget())->isDestoyed()) ) &&
			f_OldTargetinAreaGuardRange && f_OldTargetInWeaponRange )
	{
		tmp_old = getOldTarget();
		tmp_OldTarget_Location =  getOldTarget()->getClosestPoint(location);
	}

	tmp_Target_Location != INVALID_POS ?  		closeTargetDistance = blockDistance(location, tmp_Target_Location) :
												closeTargetDistance = std::numeric_limits<float>::infinity();
	tmp_OldTarget_Location != INVALID_POS ?  	closeOldTargetDistance = blockDistance(location, tmp_OldTarget_Location) :
												closeOldTargetDistance = std::numeric_limits<float>::infinity();

	return closeTargetDistance <= closeOldTargetDistance ? tmp : tmp_old;
}


