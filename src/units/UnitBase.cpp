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

#include <units/UnitBase.h>
#include <units/Carryall.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>

#include <SoundPlayer.h>
#include <Map.h>
#include <Bullet.h>
#include <ScreenBorder.h>
#include <House.h>

#include <players/HumanPlayer.h>

#include <misc/draw_util.h>

#include <AStarSearch.h>

#include <GUI/ObjectInterfaces/UnitInterface.h>

#include <structures/Refinery.h>
#include <structures/RepairYard.h>
#include <units/Harvester.h>

#include <misc/strictmath.h>

#include <units/TankBase.h>

#define SMOKEDELAY 30
#define UNITIDLETIMER (GAMESPEED_DEFAULT *  315)  // about every 5s

UnitBase::UnitBase(House* newOwner) : ObjectBase(newOwner) {

    UnitBase::init();

    drawnAngle = currentGame->randomGen.rand(0, 7);
	angle = (float) drawnAngle;

    goingToRepairYard = false;
    pickedUp = false;
    bFollow = false;
    guardPoint = Coord::Invalid();
    attackPos = Coord::Invalid();

    isGroupLeader = false;
    moving = false;
    turning = false;
    justStoppedMoving = false;
	salving = false;
	regulatedSpeed = 0.0f;
    xSpeed = 0.0f;
    ySpeed = 0.0f;
    bumpyOffsetX = 0.0f;
    bumpyOffsetY = 0.0f;

    targetDistance = 0.0f;
	targetAngle = INVALID;

	noCloserPointCount = 0;
    nextSpotFound = false;
	nextSpotAngle = drawnAngle;
    recalculatePathTimer = 0;
	nextSpot = Coord::Invalid();

	findTargetTimer = 0;
    primaryWeaponTimer = 0;
    salveWeapon = 0;
	secondaryWeaponTimer = INVALID;
	for (int i=0; i < salveWeapon  && salveWeapon < MAX_SALVE; i++) {
		salveWeaponTimer[i] =  (salveWeaponDelay*i)+1;
	}
	salveWeaponDelaybase = BASE_SALVO_TIMER;
	salveWeaponDelay = salveWeaponDelaybase;
	deviationTimer = INVALID;
}

UnitBase::UnitBase(InputStream& stream) : ObjectBase(stream) {

    UnitBase::init();

    stream.readBools(&goingToRepairYard, &pickedUp, &bFollow);
    guardPoint.x = stream.readSint32();
	guardPoint.y = stream.readSint32();
	attackPos.x = stream.readSint32();
	attackPos.y = stream.readSint32();


	stream.readBools(&isGroupLeader, &moving, &turning, &justStoppedMoving, &salving);
	regulatedSpeed = stream.readFloat();
	xSpeed = stream.readFloat();
	ySpeed = stream.readFloat();
	bumpyOffsetX = stream.readFloat();
	bumpyOffsetY = stream.readFloat();

	targetDistance = stream.readFloat();
	targetAngle = stream.readSint8();

	noCloserPointCount = stream.readUint8();
	nextSpotFound = stream.readBool();
	nextSpotAngle = stream.readSint8();
	recalculatePathTimer = stream.readSint32();
	nextSpot.x = stream.readSint32();
	nextSpot.y = stream.readSint32();
	int numPathNodes = stream.readUint32();
	for(int i=0;i<numPathNodes; i++) {
	    Sint32 x = stream.readSint32();
	    Sint32 y = stream.readSint32();
        pathList.push_back(Coord(x,y));
	}

	findTargetTimer = stream.readSint32();
	primaryWeaponTimer = stream.readSint32();
	secondaryWeaponTimer = stream.readSint32();
	for (int i=0; i  < MAX_SALVE; i++) {
		salveWeaponTimer[i] = stream.readSint32();
	}
	salveWeaponDelaybase = stream.readSint32();
	salveWeaponDelay = stream.readSint32();
	deviationTimer = stream.readSint32();
}

void UnitBase::init() {
    aUnit = true;
    canAttackStuff = true;
	canCaptureStuff = false;
	tracked = false;
	turreted = false;
	numWeapons = 0;

	drawnFrame = 0;

	unitList.push_back(this);
}

UnitBase::~UnitBase() {
	pathList.clear();
	removeFromSelectionLists();

	currentGame->getObjectManager().removeObject(objectID);
}


void UnitBase::save(OutputStream& stream) const {

	ObjectBase::save(stream);

	stream.writeBools(goingToRepairYard, pickedUp, bFollow);
	stream.writeSint32(guardPoint.x);
	stream.writeSint32(guardPoint.y);
	stream.writeSint32(attackPos.x);
	stream.writeSint32(attackPos.y);

	stream.writeBools(isGroupLeader,moving, turning, justStoppedMoving,salving);
	stream.writeFloat(regulatedSpeed);
	stream.writeFloat(xSpeed);
	stream.writeFloat(ySpeed);
	stream.writeFloat(bumpyOffsetX);
	stream.writeFloat(bumpyOffsetY);

	stream.writeFloat(targetDistance);
	stream.writeSint8(targetAngle);

	stream.writeUint8(noCloserPointCount);
	stream.writeBool(nextSpotFound);
	stream.writeSint8(nextSpotAngle);
	stream.writeSint32(recalculatePathTimer);
	stream.writeSint32(nextSpot.x);
	stream.writeSint32(nextSpot.y);
    stream.writeUint32(pathList.size());
    for(std::list<Coord>::const_iterator iter = pathList.begin(); iter != pathList.end(); ++iter) {
        stream.writeSint32(iter->x);
        stream.writeSint32(iter->y);
    }

	stream.writeSint32(findTargetTimer);
	stream.writeSint32(primaryWeaponTimer);
	stream.writeSint32(secondaryWeaponTimer);
	for (int i=0; i <  MAX_SALVE; i++) {
		stream.writeSint32(salveWeaponTimer[i]);
	}
	stream.writeSint32(salveWeaponDelaybase);
	stream.writeSint32(salveWeaponDelay);
	stream.writeSint32(deviationTimer);
}

void UnitBase::attack() {

	if(numWeapons) {
		Coord targetCenterPoint;
		Coord centerPoint = getCenterPoint();
		bool bAirBullet;

		if(target.getObjPointer() != NULL) {
			targetCenterPoint = target.getObjPointer()->getClosestCenterPoint(location);
			bAirBullet = target.getObjPointer()->isAFlyingUnit();
		} else {
			targetCenterPoint = currentGameMap->getTile(attackPos)->getCenterPoint();
			bAirBullet = false;
		}

		int currentBulletType = bulletType;
		Sint32 currentWeaponDamage = currentGame->objectData.data[itemID][originalHouseID].weapondamage;
		if(bAirBullet && getItemID() == Unit_Launcher) {
			currentBulletType = Bullet_SmallRocket;
			currentWeaponDamage *= .75;
		}

		if(getItemID() == Unit_Trooper) {
		    // Troopers change weapon type depending on distance

            float distance = distanceFrom(centerPoint, targetCenterPoint);
            if(distance > 2*TILESIZE) {
                currentBulletType = Bullet_SmallRocket;
            }
		}

		bool orni_primary =  (!( getItemID() == Unit_Ornithopter && target && target.getObjPointer() != NULL && target.getObjPointer()->isInfantry()));
		bool primary_weapon_enable = orni_primary;
		if(primaryWeaponTimer == 0 && primary_weapon_enable) {
			bulletList.push_back( new Bullet( objectID, &centerPoint, &targetCenterPoint, currentBulletType, currentWeaponDamage, bAirBullet) );
			playAttackSound();
			primaryWeaponTimer = getWeaponReloadTime();

			secondaryWeaponTimer = 15;

			if(attackPos && getItemID() != Unit_SonicTank && currentGameMap->getTile(attackPos)->isSpiceBloom()) {
				setDestination(location);
				forced = false;
				attackPos.invalidate();
			}

			// shorten deviation time
			if(deviationTimer > 0) {
				deviationTimer = std::max(0,deviationTimer - MILLI2CYCLES(10*1000));
			}
		} else if (!primary_weapon_enable && secondaryWeaponTimer <= -1 ) {
			// Reload secondary if primary not potent and second has fired
			secondaryWeaponTimer = 15;
		}

		if((numWeapons == 2) && (secondaryWeaponTimer == 0) && (isBadlyDamaged() == false)) {
			if(getItemID() == Unit_Ornithopter) {
				// Orni have a second weapon for infantry
				if (target && target.getObjPointer() != NULL && target.getObjPointer()->isInfantry() ) {
					currentBulletType = Bullet_ShellSmall;
					currentWeaponDamage *= .75;
				} else {
					return;
				}
			}

			bulletList.push_back( new Bullet( objectID, &centerPoint, &targetCenterPoint, currentBulletType, currentWeaponDamage, bAirBullet) );
			playAttackSound();
			secondaryWeaponTimer = -1;

			if(attackPos && getItemID() != Unit_SonicTank && currentGameMap->getTile(attackPos)->isSpiceBloom()) {
				setDestination(location);
				forced = false;
				attackPos.invalidate();
			}

			// shorten deviation time
			if(deviationTimer > 0) {
				deviationTimer = std::max(0,deviationTimer - MILLI2CYCLES(10*1000));
			}
		}



		if (salveWeapon && salving) {
			salveAttack(attackPos,targetCenterPoint);
		}


	}

}

void UnitBase::salveAttack(Coord Pos, Coord Target) {

	if (!salveWeapon || !salving) {
		dbg_relax_print("UnitBase::salveAttack no more salving\n");
		return;
	}


	for (int i=0; i < salveWeapon  && salveWeapon < MAX_SALVE; i++) {
		if((salveWeaponTimer[i] <= 0) && (isBadlyDamaged() == false) && salveWeaponDelay <= 0) {
			Coord centerPoint = getCenterPoint();
			Coord targetCenterPoint;
			bool bAirBullet;
			int currentBulletType = bulletType;
			Sint32 currentWeaponDamage = currentGame->objectData.data[itemID][originalHouseID].weapondamage / salveWeapon;

			if (target && target.getObjPointer() != NULL) {
				targetCenterPoint = target.getObjPointer()->getClosestCenterPoint(location);
				targetDistance = blockDistance(location, targetCenterPoint);
				dbg_relax_print("UnitBase::salveAttack targetdistance %lf weaponreach %d\n",targetDistance,getWeaponRange() );
			}


			if (currentGameMap->tileExists(Pos)){
				targetCenterPoint = currentGameMap->getTile(Pos)->getCenterPoint();
				bAirBullet = false;
			} else if(currentGameMap->tileExists(Target)) {
				targetCenterPoint = currentGameMap->getTile(Target)->getCenterPoint();
				bAirBullet = false;
			} else if (target && target.getObjPointer() != NULL) {
				targetCenterPoint = target.getObjPointer()->getClosestCenterPoint(location);
				bAirBullet = target.getObjPointer()->isAFlyingUnit();
			} else {
				dbg_print("UnitBase::salveAttack cannot be done !\n");
				return;
			}



				bulletList.push_back( new Bullet( objectID, &centerPoint, &targetCenterPoint, currentBulletType, currentWeaponDamage, bAirBullet) );
				playAttackSound();
				salveWeaponDelay = BASE_SALVO_TIMER;
				salveWeaponTimer[i] = getWeaponReloadTime()+ salveWeaponDelay*i+salveWeaponDelay;
				primaryWeaponTimer = getWeaponReloadTime();

		}
	}
}

void UnitBase::blitToScreen() {

    SDL_Surface* pUnitGraphic = graphic[currentZoomlevel];
    int imageW = pUnitGraphic->w/numImagesX;
    int imageH = pUnitGraphic->h/numImagesY;
    int x = screenborder->world2screenX(realX);
    int y = screenborder->world2screenY(realY);

    SDL_Rect source = { drawnAngle * imageW, drawnFrame * imageH, imageW, imageH };
    SDL_Rect dest = { x - imageW/2, y - imageH/2, imageW, imageH };

    SDL_BlitSurface(pUnitGraphic, &source, screen, &dest);

    if(isBadlyDamaged()) {
        drawSmoke(x, y);
    }
}

ObjectInterface* UnitBase::getInterfaceContainer() {
	if((pLocalHouse == owner && isRespondable()) || (debug == true)) {
		return UnitInterface::create(objectID);
	} else {
		return DefaultObjectInterface::create(objectID);
	}
}

int UnitBase::getCurrentAttackAngle() const {
	return drawnAngle;
}

void UnitBase::deploy(const Coord& newLocation, bool sound) {

	if(currentGameMap->tileExists(newLocation)) {
		setLocation(newLocation);

		if (guardPoint.isInvalid())
			guardPoint = location;
		setDestination(guardPoint);
		pickedUp = false;
		setRespondable(true);
		setActive(true);
		setVisible(VIS_ALL, true);
		setFellow(NULL);

		if(sound) {
			Voice_enum voice = NUM_VOICE;

			if (isAFlyingUnit()) {
				voice = UnitLaunched;
			} else if (isInfantry()) {
				voice = UnitDeployed;
			} else if (isAUnit() && itemID != Unit_Harvester) {
				voice = VehiculeDeployed;
			} else if (isAUnit() && itemID == Unit_Harvester) {
				voice = HarvesterDeployed;
			}


			if(getOwner() == pLocalHouse && voice < NUM_VOICE) {
				soundPlayer->playVoiceAt(voice,getOwner()->getHouseID(),location);
			}
		}
	}

}


void UnitBase::destroy() {

	setFellow(NULL);
	setTarget(NULL);
	currentGameMap->removeObjectFromMap(getObjectID());	//no map point will reference now
	currentGame->getObjectManager().removeObject(objectID);

	currentGame->getHouse(originalHouseID)->decrementUnits(itemID);

	unitList.remove(this);

    if(isVisible()) {
        if(currentGame->randomGen.rand(1,100) <= getInfSpawnProp()) {
            UnitBase* pNewUnit = currentGame->getHouse(originalHouseID)->createUnit(Unit_Soldier);
            pNewUnit->setHealth(pNewUnit->getMaxHealth()/2);
            pNewUnit->deploy(location, false);

            if(owner->getHouseID() != originalHouseID) {
                // deviation is inherited
                pNewUnit->owner = owner;
                pNewUnit->graphic = pGFXManager->getObjPic(pNewUnit->graphicID,owner->getHouseID());
                pNewUnit->deviationTimer = deviationTimer;
            }
        }
    }

	delete this;
}

void UnitBase::deviate(House* newOwner) {

    if(wasDeviated() && newOwner->getHouseID() == originalHouseID) {
        quitDeviation();
        return;
    }
    if (owner != newOwner) {
        removeFromSelectionLists();
        setTarget(NULL);
        setFellow(NULL);
        setGuardPoint(location);
        setDestination(location);
        clearPath();
        doSetAttackMode(GUARD);
        owner = newOwner;
        graphic = pGFXManager->getObjPic(graphicID,getOwner()->getHouseID());
        deviationTimer = DEVIATIONTIME;
    } else if (wasDeviated()) {
    	deviationTimer = DEVIATIONTIME;
    }
}

void UnitBase::drawSelectionBox() {

    SDL_Surface* selectionBox = NULL;
    SDL_Surface* icon = NULL;

    switch(currentZoomlevel) {
        case 0:     selectionBox = pGFXManager->getUIGraphic(UI_SelectionBox_Zoomlevel0);   break;
        case 1:     selectionBox = pGFXManager->getUIGraphic(UI_SelectionBox_Zoomlevel1);   break;
        case 2:
        default:    selectionBox = pGFXManager->getUIGraphic(UI_SelectionBox_Zoomlevel2);   break;
    }


    if (bFollow)
    	selectionBox = mapSurfaceColorRange(selectionBox, COLOR_WHITE, COLOR_WINDTRAP_COLORCYCLE);

    SDL_Rect dest = {   screenborder->world2screenX(realX) - selectionBox->w/2,
                        screenborder->world2screenY(realY) - selectionBox->h/2,
                        selectionBox->w,
                        selectionBox->h };
	SDL_BlitSurface(selectionBox, NULL, screen, &dest);

	int x = screenborder->world2screenX(realX) - selectionBox->w/2;
	int y = screenborder->world2screenY(realY) - selectionBox->h/2;
	for(int i=1;i<=currentZoomlevel+1;i++) {
        drawRect(screen, x+1, y-1, x+1 + ((int)((getHealth()/(float)getMaxHealth())*(selectionBox->w-3))), y-2, bFollow ? COLOR_WINDTRAP_COLORCYCLE : getHealthColor());
	}

	if (isLeader()) {

		SDL_Surface** star = pGFXManager->getObjPic(ObjPic_Star,pLocalHouse->getHouseID());

		int imageW = star[currentZoomlevel]->w/3;

	    SDL_Rect dest = {   x - imageW/2,
	    					y - 2*star[currentZoomlevel]->h,
	                        imageW,
	                        star[currentZoomlevel]->h };

		SDL_BlitSurface(star[currentZoomlevel],NULL, screen, &dest);

	}

}

void UnitBase::drawOtherPlayerSelectionBox() {
    SDL_Surface* selectionBox = NULL;

    switch(currentZoomlevel) {
        case 0:     selectionBox = pGFXManager->getUIGraphic(UI_OtherPlayerSelectionBox_Zoomlevel0);   break;
        case 1:     selectionBox = pGFXManager->getUIGraphic(UI_OtherPlayerSelectionBox_Zoomlevel1);   break;
        case 2:
        default:    selectionBox = pGFXManager->getUIGraphic(UI_OtherPlayerSelectionBox_Zoomlevel2);   break;
    }

    SDL_Rect dest = {   screenborder->world2screenX(realX) - selectionBox->w/2,
                        screenborder->world2screenY(realY) - selectionBox->h/2,
                        selectionBox->w,
                        selectionBox->h };
	SDL_BlitSurface(selectionBox, NULL, screen, &dest);
}


void UnitBase::releaseTarget() {
	/*  Mathdesc : IMO whether forced or not
	 *  it should be allowed to return to guardpoint (as a retreat point)
	 *  otherwise forced units scatters in the wilderness
    if(forced == true) {
        guardPoint = location;
    }
    */
	if (!bFollow)
		setDestination(guardPoint);

    findTargetTimer = 0;
    setForced(false);
    setTarget(NULL);
    if (salving) salving=false;
}


bool UnitBase::checkSalveRealoaded(bool stillSalvingWhenReloaded) {
	bool cant_while_salve_reload = false;

	if (salveWeapon > 0) {
		for (int i=0; i < salveWeapon  && salveWeapon < MAX_SALVE; i++) {
			if (salveWeaponTimer[i] > salveWeaponDelay*i+salveWeaponDelay)  {
				cant_while_salve_reload = true;
				break;
			}
		}
		if (!cant_while_salve_reload) salving = stillSalvingWhenReloaded;
	}
	return !cant_while_salve_reload;
}


void UnitBase::engageFollow(ObjectBase* pTarget) {
	ObjectBase* old = (getFellow());
	Coord oldLoc = old != NULL ?  old->getLocation() : Coord::Invalid();
	Coord targetLocation = pTarget != NULL ? pTarget->getClosestPoint(location) : Coord::Invalid();
	targetDistance = blockDistance(location, targetLocation);

	if(old != NULL) {
		if (old->isAUnit() && ((UnitBase*)old)->isSalving()) {
					salving=true;
		} else {
					salving=false;
		}
		if (destination != oldLoc) {
			// the location of the friend unit has moved
			// => recalculate path
			clearPath();
		}
		// The friend followed unit changed its target : follow firing orders
		// but only if our current target is out of range and followed unit is already far ahead
		// or even rather if the followed unit is itself forced.
		if (old->getTarget() != NULL && old->getTarget() != pTarget &&
				(targetDistance > getWeaponRange() && ( !isInFollowingRange() || (old->isAUnit() && ((UnitBase*)old)->wasForced()) ) )
			)   {


			setTarget(old->getTarget());
		}

	}
	//if (salving) salving=false;
	dbg_relax_print("UnitBase::follow  %d target x=%d y=%d oldtarget  x=%d y=%d \n", objectID, targetLocation.x,targetLocation.y,oldLoc.x,oldLoc.y);
	//setDestination(oldLoc);


}


void UnitBase::engageTarget() {

    if(target && (target.getObjPointer() == NULL)) {
        // the target does not exist anymore
    	// release if not salving or following while not forced to
    	if (!salving || (bFollow && !forced)) {
    		releaseTarget();
    		attackPos.invalidate();
    	} else {
    	     setDestination(location);
    		 findTargetTimer = 0;
    		 setForced(false);
    		 setTarget(NULL);
    	}

        return;
    }

    if(target && (target.getObjPointer()->isActive() == false)) {
        // the target changed its state to inactive
        releaseTarget();
        attackPos.invalidate();
        return;
    }

    if(target && !targetFriendly && !canAttack(target.getObjPointer())) {
        // the (non-friendly) target cannot be attacked anymore
        releaseTarget();
        return;
    }

    if(target && !targetFriendly && (!forced && !bFollow) && !isInAttackRange(target.getObjPointer())) {
        // the (non-friendly) target left the attack mode range (and we were not forced to attack it nor we are following)
        releaseTarget();
        return;
    }




	ObjectBase* old = (getFellow());
	Coord oldLoc = old != NULL ?  old->getLocation() : Coord::Invalid();


    if(target) {
        // we have a target unit or structure

    	if (target.getObjPointer()->getItemID() == Unit_Carryall &&  ((Carryall*)target.getObjPointer())->isIdle() && !((Carryall*)target.getObjPointer())->hasCargo()  ) {
    		// we don't want to pursuit idle flyaround carryall
    		 releaseTarget();
    		 return;
    	}


        Coord targetLocation = target.getObjPointer()->getClosestPoint(location);
        targetDistance = blockDistance(location, targetLocation);

        if(bFollow) {
            // we are following someone : we need to choose between following target or the friend unit
        	// if the friend unit is attacking a target & is in danger = move to his destination , attack his target
        	engageFollow(target.getObjPointer());

        	if (target) {
        		targetLocation = target.getObjPointer()->getClosestPoint(location);
        		targetDistance = blockDistance(location, targetLocation);
        	}

        	if (!target || targetDistance > getWeaponRange() || target.getObjPointer()->getObjectID() == objectID) {
        		// friend unit has just disengage, do same (also disengage if he engage this)
        		// if no current target or got out of range
        		findTargetTimer = 0;
        		if (!target ||  target.getObjPointer()->getObjectID() == objectID)
        			return;
        		//err_print("UnitBase::engageTarget just disengage %d\n ",target && target.getObjPointer() ? target.getObjPointer()->getObjectID() : -1);

        	}
        } else if(destination != targetLocation) {
			// the location of the target has moved
			// => recalculate path
			clearPath();
		}

        Sint8 newTargetAngle = lround(8.0f/256.0f*destinationAngle(location, targetLocation));
        if(newTargetAngle == 8) {
            newTargetAngle = 0;
        }

        if(targetDistance > getWeaponRange()) {
            // we are not in attack range
            // => follow the target (or indirectly with the leader unit)
        	if (!salving) {
        		if (!isFollowing()) {
        			setDestination(targetLocation);
        		} else  {
        			if (!isInAttackRange(target.getObjPointer()) && isInFollowingRange()) {
        				setDestination(targetLocation);
        			}
        			else
        				setDestination(oldLoc);
					dbg_relax_print("UnitBase::engageTarget %d notinrange target x=%d y=%d oldtarget  x=%d y=%d\n ", objectID, targetLocation.x,targetLocation.y,oldLoc.x,oldLoc.y);
        		}
        	}
        	else {
        		dbg_relax_print("UnitBase::engageTarget setting salveattack unreached x=%d y=%d\n ",attackPos.x,attackPos.y);
                salveAttack(attackPos,targetLocation.Invalid());
        	}
            return;
        }

        // we are in attack range

        if (isFollowing() && !isInFollowingRange() && target && target.getObjPointer()->isActive()) {
        	// Follow the leader in any case when out of the following range
        	releaseTarget();
			return;
        }

        if(targetFriendly && (!forced && !bFollow)) {
            // the target is friendly and we only attack these if were forced to do so or not in following mode (firing orders)
            return;
        }

        if(goingToRepairYard) {
            // we are going to the repair yard
            // => we do not need to change the destination
            targetAngle = INVALID;
        } else if(attackMode == CAPTURE) {
            // we want to capture the target building
        	if (salving) salving=false;
            setDestination(targetLocation);
            targetAngle = INVALID;
        } else if((isTracked() && target.getObjPointer()->isInfantry() && currentGameMap->tileExists(targetLocation) && !currentGameMap->getTile(targetLocation)->isMountain()) &&
					(forced ||
							(target.getUnitPointer() != NULL && target.getUnitPointer()->getTarget() == this &&
							(targetDistance <= target.getUnitPointer()->getWeaponRange() ))
					) && getItemID() != Unit_SonicTank
				  ) {
            // we squash the infantry unit because we are forced to or we can (anytime for harvester or) because it is in weaponrange
        	if (salving) salving=false;
        	err_print("UnitBase::engageTarget will squash 'EM %d !\n",target.getUnitPointer()->getObjectID());
            setDestination(targetLocation);
            //targetAngle = INVALID;
        } else if (target.getObjPointer()->getItemID() == Unit_Carryall &&
        			(	( ((Carryall*)target.getObjPointer())->hasCargo()  &&
        						( ((Carryall*)target.getObjPointer())->hasAFellow() && (!(((Carryall*)target.getObjPointer())->getFellow())->isAStructure()) )
        				) ||
						( (!((Carryall*)target.getObjPointer())->hasCargo()) &&
								(  ((Carryall*)target.getObjPointer())->hasAFellow() &&  (((Carryall*)target.getObjPointer())->getFellow())->isAUnit() )
						)
					)
				  ) {
        	// we don't want to try to attack a fast unit like carryall except  :
        	//	- when having a cargo that is not intended by a structure (ie. Repairyard,Refinery)
        	//  - when not having a cargo but intent to pickup one (a unit)
        	return;
        } else {
            // we decide to fire on the target thus we can stop moving
            setDestination(location);
            if (checkSalveRealoaded(salving)) {
            	targetAngle = newTargetAngle;
            	//dbg_relax_print("UnitBase::engageTarget get new angle (salving %s)\n",salving ? "yes" :"no");
            }
        }
        attackPos = targetLocation; // Saving attackPos when target become unreachable

        if (salving && (getCurrentAttackAngle() == newTargetAngle)) {
        		dbg_print("UnitBase::engageTarget setting salveattack on TARGET x=%d y=%d\n ",attackPos.x,attackPos.y);
               	salveAttack(attackPos,targetLocation);
               	return;
        }
        if (salving && getCurrentAttackAngle() != newTargetAngle) {
        		// can't attack
               	return;
        }
        if(!salving && getCurrentAttackAngle() == newTargetAngle) {
        	//dbg_relax_print("UnitBase::engageTarget nosalving attack x=%d y=%d!\n",targetLocation.x,targetLocation.y);
            attack();
        }

    } /* if target */
    else if(attackPos.isValid()) {
        // we attack a position


    	        targetDistance = blockDistance(location, attackPos);

    	        Sint8 newTargetAngle = lround(8.0f/256.0f*destinationAngle(location, attackPos));
    	        if(newTargetAngle == 8) {
    	            newTargetAngle = 0;
    	        }

    	        if(targetDistance <= getWeaponRange()) {
    	            // we are in weapon range thus we can stop moving
    	            setDestination(location);
    	            targetAngle = newTargetAngle;

    	            if(getCurrentAttackAngle() == newTargetAngle ) {
    	            	dbg_relax_print("attackPos (x=%d,y=%d) attack \n ",attackPos.x,attackPos.y);
    	                attack();
    	            }
    	        } else {

    	            targetAngle = INVALID;
    	        }


    }
}

void UnitBase::move() {

	if(!moving && !justStoppedMoving && itemID != Unit_Sandworm) {
		currentGameMap->viewMap(owner->getTeam(), location, getViewRange() );
	}

	if (salveWeapon >= 1 && !checkSalveRealoaded(salving)) {
		return;
	}


	if(moving && !justStoppedMoving) {
		float maxxspeed, maxyspeed;

		maxxspeed = getxSpeed();
		maxyspeed = getySpeed();

		if((isBadlyDamaged() == true) && !isAFlyingUnit()) {
			maxxspeed /= 2;
			maxyspeed /= 2;
		}


		realX += maxxspeed;
		realY += maxyspeed;


		// check if vehicle is on the first half of the way
		float fromDistanceX;
		float fromDistanceY;
		float toDistanceX;
		float toDistanceY;
		if(location != nextSpot) {
		    // check if vehicle is half way out of old tile

		    fromDistanceX = strictmath::abs(location.x*TILESIZE - (realX-bumpyOffsetX) + TILESIZE/2);
		    fromDistanceY = strictmath::abs(location.y*TILESIZE - (realY-bumpyOffsetY) + TILESIZE/2);
		    toDistanceX = strictmath::abs(nextSpot.x*TILESIZE - (realX-bumpyOffsetX) + TILESIZE/2);
		    toDistanceY = strictmath::abs(nextSpot.y*TILESIZE - (realY-bumpyOffsetY) + TILESIZE/2);

            if((fromDistanceX >= TILESIZE/2) || (fromDistanceY >= TILESIZE/2)) {
                // let something else go in
                unassignFromMap(location);
                oldLocation = location;
                location = nextSpot;

                if(isAFlyingUnit() == false && itemID != Unit_Sandworm) {
                    currentGameMap->viewMap(owner->getTeam(), location, getViewRange());
                }
		    }

		} else {
			// if vehicle is out of old tile

			fromDistanceX = strictmath::abs(oldLocation.x*TILESIZE - (realX-bumpyOffsetX) + TILESIZE/2);
		    fromDistanceY = strictmath::abs(oldLocation.y*TILESIZE - (realY-bumpyOffsetY) + TILESIZE/2);
		    toDistanceX = strictmath::abs(location.x*TILESIZE - (realX-bumpyOffsetX) + TILESIZE/2);
		    toDistanceY = strictmath::abs(location.y*TILESIZE - (realY-bumpyOffsetY) + TILESIZE/2);

			if ((fromDistanceX >= TILESIZE) || (fromDistanceY >= TILESIZE)) {

				//FIXME : follow mode
				if(forced && (location == destination) && !target  /*&& !bFollow*/) {
					setForced(false);
				}

				moving = false;
				justStoppedMoving = true;
				realX = location.x * TILESIZE + TILESIZE/2;
                realY = location.y * TILESIZE + TILESIZE/2;
                bumpyOffsetX = 0.0f;
                bumpyOffsetY = 0.0f;

                oldLocation.invalidate();
			}

		}

		bumpyMovementOnRock(fromDistanceX, fromDistanceY, toDistanceX, toDistanceY);

	} else {
		justStoppedMoving = false;
	}

	checkPos();
}

void UnitBase::bumpyMovementOnRock(float fromDistanceX, float fromDistanceY, float toDistanceX, float toDistanceY) {

    if(hasBumpyMovementOnRock() && ((currentGameMap->getTile(location)->getType() == Terrain_Rock)
                                    || (currentGameMap->getTile(location)->getType() == Terrain_Mountain)
                                    || (currentGameMap->getTile(location)->getType() == Terrain_ThickSpice)
									|| (currentGameMap->getTile(location)->getType() == Terrain_Dunes) )) {
        // bumping effect

        const float epsilon = 0.005f;
        const float bumpyOffset = 2.5f;
        const float absXSpeed = strictmath::abs(xSpeed);
        const float absYSpeed = strictmath::abs(ySpeed);


        if((strictmath::abs(xSpeed) >= epsilon) && (strictmath::abs(fromDistanceX - absXSpeed) < absXSpeed/2)) { realY -= bumpyOffset; bumpyOffsetY -= bumpyOffset; }
        if((strictmath::abs(ySpeed) >= epsilon) && (strictmath::abs(fromDistanceY - absYSpeed) < absYSpeed/2)) { realX += bumpyOffset; bumpyOffsetX += bumpyOffset; }

        if((strictmath::abs(xSpeed) >= epsilon) && (strictmath::abs(fromDistanceX - 4*absXSpeed) < absXSpeed/2)) { realY += bumpyOffset; bumpyOffsetY += bumpyOffset; }
        if((strictmath::abs(ySpeed) >= epsilon) && (strictmath::abs(fromDistanceY - 4*absYSpeed) < absYSpeed/2)) { realX -= bumpyOffset; bumpyOffsetX -= bumpyOffset; }


        if((strictmath::abs(xSpeed) >= epsilon) && (strictmath::abs(fromDistanceX - 10*absXSpeed) < absXSpeed/2)) { realY -= bumpyOffset; bumpyOffsetY -= bumpyOffset; }
        if((strictmath::abs(ySpeed) >= epsilon) && (strictmath::abs(fromDistanceY - 20*absYSpeed) < absYSpeed/2)) { realX += bumpyOffset; bumpyOffsetX += bumpyOffset; }

        if((strictmath::abs(xSpeed) >= epsilon) && (strictmath::abs(fromDistanceX - 14*absXSpeed) < absXSpeed/2)) { realY += bumpyOffset; bumpyOffsetY += bumpyOffset; }
        if((strictmath::abs(ySpeed) >= epsilon) && (strictmath::abs(fromDistanceY - 14*absYSpeed) < absYSpeed/2)) { realX -= bumpyOffset; bumpyOffsetX -= bumpyOffset; }


        if((strictmath::abs(xSpeed) >= epsilon) && (strictmath::abs(toDistanceX - absXSpeed) < absXSpeed/2)) { realY -= bumpyOffset; bumpyOffsetY -= bumpyOffset; }
        if((strictmath::abs(ySpeed) >= epsilon) && (strictmath::abs(toDistanceY - absYSpeed) < absYSpeed/2)) { realX += bumpyOffset; bumpyOffsetX += bumpyOffset; }

        if((strictmath::abs(xSpeed) >= epsilon) && (strictmath::abs(toDistanceX - 4*absXSpeed) < absXSpeed/2)) { realY += bumpyOffset; bumpyOffsetY += bumpyOffset; }
        if((strictmath::abs(ySpeed) >= epsilon) && (strictmath::abs(toDistanceY - 4*absYSpeed) < absYSpeed/2)) { realX -= bumpyOffset; bumpyOffsetX -= bumpyOffset; }

        if((strictmath::abs(xSpeed) >= epsilon) && (strictmath::abs(toDistanceX - 10*absXSpeed) < absXSpeed/2)) { realY -= bumpyOffset; bumpyOffsetY -= bumpyOffset; }
        if((strictmath::abs(ySpeed) >= epsilon) && (strictmath::abs(toDistanceY - 10*absYSpeed) < absYSpeed/2)) { realX += bumpyOffset; bumpyOffsetX += bumpyOffset; }

        if((strictmath::abs(xSpeed) >= epsilon) && (strictmath::abs(toDistanceX - 14*absXSpeed) < absXSpeed/2)) { realY += bumpyOffset; bumpyOffsetY += bumpyOffset; }
        if((strictmath::abs(ySpeed) >= epsilon) && (strictmath::abs(toDistanceY - 14*absYSpeed) < absYSpeed/2)) { realX -= bumpyOffset; bumpyOffsetX -= bumpyOffset; }

    }
}


void UnitBase::party() {

	if(isAFlyingUnit() || (((currentGame->getGameCycleCount() + getObjectID()*1337) % 5) == 0)) {
		if(!moving && !justStoppedMoving) {
			if(nextSpotFound == false)	{
				 if(pathList.empty() && (recalculatePathTimer == 0)) {
					 recalculatePathTimer = 100;


					 if(!SearchPathWithAStar() && (++noCloserPointCount >= 3) && (location != oldLocation)) {
							// Destination here is the friend unit position
							if (pathList.empty() && !target) {
								 if(((currentGame->getGameCycleCount() + getObjectID()*1337) % MILLI2CYCLES(UNITIDLETIMER/2)) == 0) {
									 idleAction();
								 }
							}
							forced = false;

					 }

				 }
				 if(!pathList.empty()) {
					 nextSpot = pathList.front();
					 pathList.pop_front();
					 nextSpotFound = true;
					 recalculatePathTimer = 0;
					 noCloserPointCount = 0;
				 }
			} else {
				int tempAngle = currentGameMap->getPosAngle(location, nextSpot);
				if(tempAngle != INVALID) {
					nextSpotAngle = tempAngle;
				}

				if(!canPass(nextSpot.x, nextSpot.y)) {
					clearPath();
				} else {
					if (drawnAngle == nextSpotAngle)	{
						moving = true;
						nextSpotFound = false;

						assignToMap(nextSpot);
						angle = drawnAngle;
						setSpeeds();
					}
				}
			}
		}
	}

}

void UnitBase::navigate() {

	if (bFollow) {
		if (oldTarget  && getFellow() != NULL &&  getFellow()->isActive()) {
			if (getFellow()->isAUnit() && ((UnitBase*)getFellow())->wasDeviated() && (getItemID()!= Unit_Carryall && getFellow()->getItemID() != Unit_Harvester)) {
				// Do not party with deviated unit except to steal credits from a deviate harvester
				return;
			}
			party();
			return;
		} else {
			// Follow friend unit has been destroyed or gone inactive
			if (!oldTarget || getFellow() == NULL) {
				setFellow(NULL);
				setDestination(guardPoint);
			}
		}
	}
    if(isAFlyingUnit() || (((currentGame->getGameCycleCount() + getObjectID()*1337) % 5) == 0)) {
        // navigation is only performed every 5th frame

        if(!moving && !justStoppedMoving) {
            if(location != destination) {
                if(nextSpotFound == false)	{

                    if(pathList.empty() && (recalculatePathTimer == 0)) {
                        recalculatePathTimer = 100;

                        if(!SearchPathWithAStar() && (++noCloserPointCount >= 3)
                            && (location != oldLocation))
                        {	//try searching for a path a number of times then give up
                        	// after requesting for air lift if possible
                        	ObjectBase* blockingObj = currentGameMap->getTile(destination.x,destination.y)->getGroundObject();
                        	if (isAGroundUnit() && !((GroundUnit*)this)->isAwaitingPickup() && (blockingObj == NULL || getTarget() == blockingObj) ) {
                        		if (!((GroundUnit*)this)->requestCarryall()) {
                        			setDestination(location);	//can't get any closer, give up
									forced = false;
                        		} else {
                        			setFellow(((GroundUnit*)this)->getCarrier());
                        			((Carryall*)((GroundUnit*)this)->getCarrier())->setDeployPos(getClosestCenterPoint(destination));
                        		}
                        	} else if (!isAGroundUnit() || (blockingObj != NULL  && blockingObj != this && getTarget() != blockingObj)  ) {
                            	setDestination(location);	//can't get any closer, give up
                            	forced = false;
                        	}

                        }
                    }



                    if(!pathList.empty()) {
                        nextSpot = pathList.front();
                        pathList.pop_front();
                        nextSpotFound = true;
                        recalculatePathTimer = 0;
                        noCloserPointCount = 0;
                    }
                } else {
                    int tempAngle = currentGameMap->getPosAngle(location, nextSpot);
                    if(tempAngle != INVALID) {
                        nextSpotAngle = tempAngle;
                    }

                    if(!canPass(nextSpot.x, nextSpot.y)) {
                        clearPath();
                    } else {
                        if (drawnAngle == nextSpotAngle)	{
                            moving = true;
                            nextSpotFound = false;

                            assignToMap(nextSpot);
                            angle = drawnAngle;
                            setSpeeds();
                        }
                    }
                }
            } else if(!target) {
                if(((currentGame->getGameCycleCount() + getObjectID()*1337) % MILLI2CYCLES(UNITIDLETIMER)) == 0) {
                    idleAction();
                }
            }
        }
    }
}


void UnitBase::idleAction() {

	if ((!target || target.getObjPointer() == NULL) && attackPos.isInvalid()) {
		//not moving and not wanting to go anywhere, do some random turning
		if(isAGroundUnit() && (getItemID() != Unit_Harvester) && (getAttackMode() == GUARD || getAttackMode() == AREAGUARD )) {
			// we might turn this cycle with 20% chance
			if(currentGame->randomGen.rand(0, 4) == 0) {
				// choose a random one of the eight possible angles
				nextSpotAngle = currentGame->randomGen.rand(0, 7);
			}
		}
	}
}

void UnitBase::handleFormationActionClick(int xPos, int yPos) {
	if(respondable) {
		if(currentGameMap->tileExists(xPos, yPos)) {
			if(currentGameMap->getTile(xPos,yPos)->hasAnObject()) {
 				// attack unit/structure or move to structure
				ObjectBase* tempTarget 		 = currentGameMap->getTile(xPos,yPos)->getObject();
				ObjectBase* tempTargetTarget = currentGameMap->getTile(xPos,yPos)->getObject()->getTarget();
				Uint32 tTTID = 0;
				if (tempTargetTarget != NULL) {
					tTTID = tempTargetTarget->getObjectID();
				}

				if(tempTarget->getOwner()->getTeam() != getOwner()->getTeam()) {
					// attack
					currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_UNIT_ATTACKOBJECT,objectID,tempTarget->getObjectID()));
				} else {
					// move this unit
					currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_UNIT_MOVE2POS,objectID,(Uint32) xPos, (Uint32) yPos, (Uint32) true));
				}
			} else {
				// move this unit
				currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_UNIT_MOVE2POS,objectID,(Uint32) xPos, (Uint32) yPos, (Uint32) true));
			}
		}
	}
}

void UnitBase::handleActionClick(int xPos, int yPos) {
	if(respondable) {
		if(currentGameMap->tileExists(xPos, yPos)) {
			if(currentGameMap->getTile(xPos,yPos)->hasAnObject()) {
 				// attack unit/structure or move to structure
				ObjectBase* tempTarget 		 = currentGameMap->getTile(xPos,yPos)->getObject();
				ObjectBase* tempTargetTarget = currentGameMap->getTile(xPos,yPos)->getObject()->getTarget();
				Uint32 tTTID = 0;
				if (tempTargetTarget != NULL) {
					tTTID = tempTargetTarget->getObjectID();
				}

				if(tempTarget->getOwner()->getTeam() != getOwner()->getTeam()) {
					// attack
					currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_UNIT_ATTACKOBJECT,objectID,tempTarget->getObjectID()));
				} else {
					// move to object/structure)
					currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_UNIT_MOVE2OBJECT,objectID,tempTarget->getObjectID(),tTTID));
				}
			} else {
				// move this unit
				currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_UNIT_MOVE2POS,objectID,(Uint32) xPos, (Uint32) yPos, (Uint32) true));
			}
		}
	}
}

void UnitBase::handleSalveAttackClick(int xPos, int yPos) {
	if(respondable) {
		if(currentGameMap->tileExists(xPos, yPos)) {
			if(currentGameMap->getTile(xPos,yPos)->hasAnObject()) {
				// attack unit/structure or move to structure
				ObjectBase* tempTarget = currentGameMap->getTile(xPos,yPos)->getObject();

				currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_UNIT_SALVEATTACKOBJECT,objectID,tempTarget->getObjectID()));
			} else {
				// attack pos
				currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_UNIT_SALVEATTACKPOS,objectID,(Uint32) xPos, (Uint32) yPos, (Uint32) true));
			}
		}
	}

}

void UnitBase::handleAttackClick(int xPos, int yPos) {
	if(respondable) {
		if(currentGameMap->tileExists(xPos, yPos)) {
			if(currentGameMap->getTile(xPos,yPos)->hasAnObject()) {
				// attack unit/structure or move to structure
				ObjectBase* tempTarget = currentGameMap->getTile(xPos,yPos)->getObject();
				currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_UNIT_ATTACKOBJECT,objectID,tempTarget->getObjectID()));
			} else {
				// attack pos
				currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_UNIT_ATTACKPOS,objectID,(Uint32) xPos, (Uint32) yPos, (Uint32) true));
			}
		}
	}

}

void UnitBase::handleMoveClick(int xPos, int yPos) {
	if(respondable) {
		if(currentGameMap->tileExists(xPos, yPos)) {
			// move to pos
            currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_UNIT_MOVE2POS,objectID,(Uint32) xPos, (Uint32) yPos, (Uint32) true));
		}
	}
}

void UnitBase::handleSetAttackModeClick(ATTACKMODE newAttackMode) {
	currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_UNIT_SETMODE,objectID,(Uint32) newAttackMode));
}

void UnitBase::handleCancel() {
	currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_UNIT_CANCEL,objectID));
}

void UnitBase::doMove2Pos(int xPos, int yPos, bool bForced) {
    if(attackMode == CAPTURE || attackMode == HUNT) {
        doSetAttackMode(GUARD);
	}

	if((xPos != destination.x) || (yPos != destination.y)) {
		clearPath();
		findTargetTimer = 0;
	}

	setTarget(NULL);
	setDestination(xPos,yPos);
	setForced(bForced);
	setGuardPoint(xPos,yPos);
}

void UnitBase::doMove2Pos(const Coord& coord, bool bForced) {
	doMove2Pos(coord.x, coord.y, bForced);
}

void UnitBase::doMove2Object(const ObjectBase* pFellowObject, const ObjectBase* pTargetObject) {
	if(pFellowObject != NULL && pFellowObject->getObjectID() == getObjectID()) {
		return;
	}

	if (pFellowObject != NULL  && pFellowObject->isAUnit() && ((UnitBase*)pFellowObject)->wasDeviated()
			&& pFellowObject->getOwner() == pLocalHouse && (!(pFellowObject->getItemID() == Unit_Harvester && getItemID() == Unit_Carryall))) {
		// do not follow deviated unit, except for deviated harvester that can be pickup for credit theft
		return;
	}

    if(attackMode == CAPTURE || attackMode == HUNT) {
        doSetAttackMode(GUARD);
	}

	setDestination(pFellowObject->getLocation());
	err_print("ID:%d FellowID:%d TargetID:%d\n", objectID, pFellowObject !=NULL ? pFellowObject->getObjectID() : 0,pTargetObject != NULL ? pTargetObject->getObjectID() : 0);
	setFellow(pFellowObject);
	if (pFellowObject->getAttackMode() == CAPTURE) {
		doSetAttackMode(CAPTURE);
	}

	setTarget(pTargetObject);
	setForced(true);


	clearPath();
	findTargetTimer = 0;
}

void UnitBase::doMove2Object(Uint32 followObjectID, Uint32 targetObjectID, bool follow) {
	ObjectBase* pObjectfellow = NULL;
	ObjectBase* pObjecttarget = NULL;
	if (targetObjectID != 0) {
		pObjecttarget = currentGame->getObjectManager().getObject(targetObjectID);
	}

	if (follow) {
		pObjectfellow = currentGame->getObjectManager().getObject(followObjectID);
		if (pObjectfellow == NULL)
			return;
		if (!pObjectfellow->isAUnit()) {
			//bFollow = false;
			// XXX Make this an option ? 'Enable structure guard'
			//doMove2Pos(pObjectfellow->getLocation(),false);
			//return;

		} else {
			if (pObjectfellow->getOwner() == pLocalHouse && !((UnitBase*)pObjectfellow)->wasDeviated()  && isLeader()) {
				setLeader(false);
				currentGame->setGroupLeader(NULL);
			}
		}
	} else {
		pObjectfellow = NULL;
	}

	doMove2Object(pObjectfellow,pObjecttarget);
}

void UnitBase::doSalveAttackPos(int xPos, int yPos, bool bForced) {
    if(attackMode == CAPTURE) {
        doSetAttackMode(GUARD);
	}

	setDestination(xPos,yPos);
	setTarget(NULL);
	setForced(bForced);
	attackPos.x = xPos;
	attackPos.y = yPos;
	err_print("UnitBase::doSalveAttackPos (x=%d,y=%d)  \n ",xPos,yPos);
	salving = true;
	clearPath();
	findTargetTimer = 0;
}

void UnitBase::doAttackPos(int xPos, int yPos, bool bForced) {
    if(attackMode == CAPTURE) {
        doSetAttackMode(GUARD);
	}

	setDestination(xPos,yPos);
	setTarget(NULL);
	setForced(bForced);
	attackPos.x = xPos;
	attackPos.y = yPos;
	err_print("UnitBase::doAttackPos (x=%d,y=%d)  \n ",xPos,yPos);
	salving = false;
	clearPath();
	findTargetTimer = 0;
}

void UnitBase::doAttackObject(const ObjectBase* pTargetObject, bool bForced) {
	if(pTargetObject == NULL || pTargetObject->getObjectID() == getObjectID() || (!canAttack() && (getItemID() != Unit_Harvester ))) {
		return;
	}

    if(attackMode == CAPTURE) {
        doSetAttackMode(GUARD);
	}

	setDestination(INVALID_POS,INVALID_POS);

	setTarget(pTargetObject);

	setForced(bForced);
	clearPath();
	findTargetTimer = 0;
}

void UnitBase::doSalveAttackObject(Uint32 TargetObjectID, bool bForced) {
	ObjectBase* pObject = currentGame->getObjectManager().getObject(TargetObjectID);

	if(pObject == NULL) {
        return;
	}
	if (salveWeapon>0) {
		salving = true;
		doAttackObject(pObject, bForced);
	}
	else {
		salving = false;
		doAttackObject(pObject, bForced);
	}
	err_print("UnitBase::doSalveAttackObject (id=%d,salving? %s)  \n ",TargetObjectID, salving ? "true" : "false");
}

void UnitBase::doAttackObject(Uint32 TargetObjectID, bool bForced) {
	ObjectBase* pObject = currentGame->getObjectManager().getObject(TargetObjectID);

	if(pObject == NULL) {
        return;
	}
	err_print("UnitBase::doAttackObject (id=%d,salving? %s)  \n ",TargetObjectID, salving ? "true" : "false");
    doAttackObject(pObject, bForced);
}

void UnitBase::doSetAttackMode(ATTACKMODE newAttackMode) {
	if((newAttackMode >= 0) && (newAttackMode < ATTACKMODE_MAX)) {
        attackMode = newAttackMode;
	}

	if(attackMode == GUARD || attackMode == STOP) {
	    if(moving && !justStoppedMoving) {
            doMove2Pos(nextSpot, false);
	    } else {
	        doMove2Pos(location, false);
	    }
	    if (attackMode == STOP) {
	    	setFellow(NULL);
	    	salving = false;
	    }
	}
}

void UnitBase::doCancel() {

	if (isFollowing()) {
		doMove2Pos(guardPoint, false);
		setFellow(NULL);
	} else {
	    if(moving && !justStoppedMoving) {
            doMove2Pos(nextSpot, false);
	    } else {
	        doMove2Pos(location, false);
	    }
	}
	salving = false;
}

void UnitBase::handleDamage(int damage, Uint32 damagerID, House* damagerOwner) {
    // shorten deviation time
    if(deviationTimer > 0) {
        deviationTimer = std::max(0,deviationTimer - MILLI2CYCLES(damage*10*1000));
    }

    ObjectBase::handleDamage(damage, damagerID, damagerOwner);

    if ((attackMode == HUNT && !forced) || isFollowing() ) {
        ObjectBase* pDamager = currentGame->getObjectManager().getObject(damagerID);
        if(pDamager != NULL && canAttack(pDamager)) {
            if(!target || target.getObjPointer() == NULL || !isInWeaponRange(target.getObjPointer())) {
                // no target or target not on weapon range => switch target
                doAttackObject(pDamager, false);
            }
        }
    }


    //TODO : Followers might be of some help ?
}

bool UnitBase::isInGuardRange(const ObjectBase* pObject) const	{
	int checkRange;
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
            return true;
        } break;

        case STOP:
        default: {
            return false;
        } break;
    }

    if(getItemID() == Unit_Sandworm) {
        checkRange = getViewRange();
    }

    return (blockDistance(guardPoint*TILESIZE + Coord(TILESIZE/2, TILESIZE/2), pObject->getCenterPoint()) <= checkRange*TILESIZE);
}

bool UnitBase::isInAttackRange(const ObjectBase* pObject) const {
	int checkRange;
    if(pObject == NULL) {
        return false;
    }

    // Object is unarmed but it can squash infantry badly
    if (isTracked() && getItemID() == Unit_Harvester && pObject->isInfantry())
    	return true;

    switch(attackMode) {
        case GUARD: {
            checkRange = getWeaponRange();
        } break;

        case AREAGUARD: {
            checkRange = getAreaGuardRange() + getWeaponRange() + 1;
        } break;

        case AMBUSH: {
            checkRange = getViewRange() + 1;
        } break;

        case HUNT: {
            return true;
        } break;

        case STOP:
        default: {
            return false;
        } break;
    }

    if(getItemID() == Unit_Sandworm) {
        checkRange = getViewRange() + 10;
    }

    return (blockDistance(location*TILESIZE + Coord(TILESIZE/2, TILESIZE/2), pObject->getCenterPoint()) <= checkRange*TILESIZE);
}



bool UnitBase::isInFollowingRange() const {
    if(!oldTarget) {
        return true;
    }

    Coord LeaderLocation = oldTarget.getObjPointer() != NULL ? oldTarget.getObjPointer()->getCenterPoint() : location;
	float range = 2*getViewRange();

		switch(attackMode) {
			case GUARD: {
				range += getWeaponRange();
			} break;

			case AREAGUARD: {
				range += getAreaGuardRange();
			} break;

			case AMBUSH: {
				range += getViewRange();
			} break;

			case HUNT: {
				range += getAreaGuardRange()*2;
			} break;

			default : {
				range += getWeaponRange();
			}
		}

	return (blockDistance(location*TILESIZE + Coord(TILESIZE/2, TILESIZE/2), LeaderLocation) <= range*TILESIZE);
}

bool UnitBase::isInWeaponRange(const ObjectBase* object) const {
    if(object == NULL) {
        return false;
    }

    Coord targetLocation = target.getObjPointer()->getClosestPoint(location);

    return (blockDistance(location, targetLocation) <= getWeaponRange());
}

bool UnitBase::isNearer(const ObjectBase* object) const {
    if(object == NULL) {
        return false;
    }

    if (!target) {
    	return true;
    }

    Coord targetLocation = target.getObjPointer()->getClosestPoint(location);
    Coord objectLocation = object->getClosestPoint(location);

    return blockDistance(location, targetLocation) > blockDistance(location, objectLocation);
}



void UnitBase::setAngle(int newAngle) {
	if(!moving && !justStoppedMoving && (newAngle >= 0) && (newAngle < NUM_ANGLES)) {
		angle = drawnAngle = newAngle;
		clearPath();
	}
}

void UnitBase::setGettingRepaired() {
	if(oldTarget  && getFellow() != NULL && (getFellow()->getItemID() == Structure_RepairYard)) {
		if(selected) {
			removeFromSelectionLists();
        }
		soundPlayer->playSoundAt(Sound_Steam, location);
		currentGameMap->removeObjectFromMap(getObjectID());

		((RepairYard*)getFellow())->assignUnit(this);

		respondable = false;
		setActive(false);
		setVisible(VIS_ALL, false);
		goingToRepairYard = false;
		badlyDamaged = false;


		setDestination(location);
		nextSpotAngle = DOWN;
	}
}

void UnitBase::setGuardPoint(int newX, int newY) {
	if(currentGameMap->tileExists(newX, newY) || ((newX == INVALID_POS) && (newY == INVALID_POS))) {
		guardPoint.x = newX;
		guardPoint.y = newY;

		if((getItemID() == Unit_Harvester) && guardPoint.isValid()) {
            if(currentGameMap->getTile(newX, newY)->hasSpice()) {
                if(attackMode == STOP) {
                    attackMode = GUARD;
                }
            } else {
                if(attackMode != STOP) {
                    attackMode = STOP;
                }
            }
		}
	}
}

void UnitBase::setLocation(int xPos, int yPos) {

	if((xPos == INVALID_POS) && (yPos == INVALID_POS)) {
		ObjectBase::setLocation(xPos, yPos);
	} else if (currentGameMap->tileExists(xPos, yPos)) {
		ObjectBase::setLocation(xPos, yPos);
		realX += TILESIZE/2;
		realY += TILESIZE/2;
		bumpyOffsetX = 0.0f;
		bumpyOffsetY = 0.0f;
	}

	moving = false;
	pickedUp = false;
	setTarget(NULL);

	clearPath();
}

void UnitBase::setPickedUp(UnitBase* newCarrier) {

	if(selected) {
		removeFromSelectionLists();
    }

	currentGameMap->removeObjectFromMap(getObjectID());

	if(goingToRepairYard && getFellow()!=NULL) {
		((RepairYard*)getFellow())->unBook();
	}

	if(getItemID() == Unit_Harvester) {
		Harvester* harvester = (Harvester*) this;
		if(harvester->isReturning() && oldTarget && (getFellow()!= NULL) && (getFellow()->getItemID() == Structure_Refinery)) {
			((Refinery*)getFellow())->unBook();
		}
	}

	oldTarget.pointTo(newCarrier);
	goingToRepairYard = false;
	forced = false;
	moving = false;
	pickedUp = true;
	respondable = false;
	setActive(false);
	setVisible(VIS_ALL, false);

	clearPath();
}


float UnitBase::getMaxSpeed() const {

	float maxrelativespeed = currentGame->objectData.data[itemID][originalHouseID].maxspeed;
	bool inList = false;

	if (currentGame->getSelectedList().size() > 1 ) {
		std::list<Uint32> *list = &currentGame->getSelectedList();
		std::list<Uint32>::iterator test;
		for(test = list->begin() ; test != list->end(); ++test) {
			ObjectBase *obj = currentGame->getObjectManager().getObject(*test);
			UnitBase *unit = dynamic_cast<UnitBase*>(obj);
			if(obj->isAUnit() && unit->getRegulatedSpeed() != 0) {
				maxrelativespeed = std::min(maxrelativespeed,currentGame->objectData.data[obj->getItemID()][obj->getOriginalHouseID()].maxspeed);
				if (this  == unit) {
					unit->setRegulatedSpeed(maxrelativespeed);
					inList = true;
				}
			}
		}
	}

	if (inList) {
		//if (isSelected()) dbg_print("UnitBase::getMaxSpeed(L) %d speed:%.1f(%.1f) \n", objectID, maxrelativespeed,regulatedSpeed);
		return maxrelativespeed;
	}
	else if (regulatedSpeed != 0 && currentGame->getSelectedList().size() > 1 ) {
		//if (isSelected()) dbg_print("UnitBase::getMaxSpeed(R) %d speed:%.1f(%.1f) \n", objectID, maxrelativespeed,regulatedSpeed);
		return regulatedSpeed;
	}
	else {
		//if (isSelected()) dbg_print("UnitBase::getMaxSpeed(N) %d speed:%.1f(%.1f) \n", objectID, maxrelativespeed,regulatedSpeed);
		return currentGame->objectData.data[itemID][originalHouseID].maxspeed;
	}
}


void UnitBase::setSpeeds() {


	float speed = getMaxSpeed();

	if(!isAFlyingUnit()) {
		speed += speed*(1.0f - getTerrainDifficulty((TERRAINTYPE) currentGameMap->getTile(location)->getType()));
	}

	if(isBadlyDamaged()) {
        speed *= HEAVILYDAMAGEDSPEEDMULTIPLIER;
	}

	if(itemID == Unit_Carryall && isSelected()) {
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

void UnitBase::setTarget(const ObjectBase* newTarget) {
	attackPos.invalidate();
	//FIXME perf : set target is running permanently, why ?
	targetAngle = INVALID;


	//err_print("UnitBase::setTarget 1 %d FellowID:%d TargetID:%d\n", objectID, getFellow() != NULL ? getFellow()->getObjectID() : 0, this->getTarget() != NULL ? this->getTarget()->getObjectID() : 0 );
	ObjectBase::setTarget(newTarget);
	//err_print("UnitBase::setTarget 2 %d FellowID:%d TargetID:%d\n", objectID,getFellow() != NULL ? getFellow()->getObjectID() : 0, this->getTarget() != NULL ? this->getTarget()->getObjectID() : 0 );

}


inline ObjectBase* UnitBase::getFellow() {

	return oldTarget.getObjPointer() ;
}


void UnitBase::setFellow(const ObjectBase* newFellow) {

	if (newFellow != NULL ) {
		bFollow = true;
	}
	else
		bFollow = false;

	if(goingToRepairYard && oldTarget && (oldTarget.getObjPointer() !=NULL && oldTarget.getObjPointer()->getItemID() == Structure_RepairYard)) {
		((RepairYard*)oldTarget.getObjPointer())->unBook();
		goingToRepairYard = false;
	}

	ObjectBase::setFellow(newFellow);

	if(oldTarget.getObjPointer()  != NULL
			&& (oldTarget.getObjPointer()->getOwner() == getOwner())
			&& (oldTarget.getObjPointer() ->getItemID() == Structure_RepairYard)
			&& (itemID != Unit_Carryall) && (itemID != Unit_Frigate)
			&& (itemID != Unit_Ornithopter)) {
			((RepairYard*)oldTarget.getObjPointer())->book();
			goingToRepairYard = true;
		}
}


void UnitBase::targeting() {


	if (this->isSelected()) err_relax_print("UnitBase::targeting ID:%d Destination:[%d,%d] AreaGuardRange:%d salv:%s nofollow:%s(%d) notarget:%s(%d) noattackpos:%s notmoving:%s notjuststopped:%s notforced:%s guardpoint:[%d,%d] isidle:%s attackmode=%s timer=%d\n",
			objectID, destination.x, destination.y, getAreaGuardRange(), salving ? "yes" : "no", isFollowing() ? "y" : "n", oldTarget.getObjectID(),!target ? "y" : "n", target.getObjectID(),
			!attackPos ? "y" : "n", !moving ? "y" : "n", !justStoppedMoving ? "y" : "n", !forced ? "y" : "n", guardPoint.x ,guardPoint.y, ((Harvester*)this)->isIdle() ? "y" : "n", getAttackModeNameByMode(attackMode).c_str(), findTargetTimer);


    if(findTargetTimer == 0) {


        if(attackMode != STOP) {
            if(!target && ( (!attackPos || attackPos.isInvalid() || !isSalving()) ) && ((!moving && !justStoppedMoving && !forced) || bFollow)) {
                // we have no target, we have stopped moving and we weren't forced to do anything else or following
            	attackPos.invalidate();

                const ObjectBase* pNewTarget = NULL ;

                if (isFollowing() && getFellow() !=NULL ) {
                	pNewTarget =  getFellow()->getTarget();
                	setTarget(pNewTarget);
                	if (this->isSelected()) err_relax_print("UnitBase::targeting-- ID:%d follower is given the target %d\n", objectID, pNewTarget != NULL ? pNewTarget->getObjectID() : 0);
                } else {
                	pNewTarget = findTarget();
                	if (this->isSelected()) err_relax_print("UnitBase::targeting-- ID:%d choose NEW target %d\n", objectID,pNewTarget != NULL ? pNewTarget->getObjectID() : 0);

                }

                if (getItemID() == Unit_Sandworm) {

                	if(pNewTarget != NULL && isInAttackRange(pNewTarget)) {
                		doAttackObject(pNewTarget, false);
                		if(attackMode == AMBUSH) {
                		   doSetAttackMode(HUNT);
                		}
					}
                } else {

					if(pNewTarget != NULL && isInGuardRange(pNewTarget)) {
						// we have found a new target => attack it
						if(attackMode == AMBUSH) {
							doSetAttackMode(HUNT);
						}
						doAttackObject(pNewTarget, false);

					} else if(attackMode == HUNT) {
						setGuardPoint(location);
						doSetAttackMode(GUARD);
					}
                }

                // reset target timer
                findTargetTimer = 100;
            }
        }

    }

	engageTarget();
}

void UnitBase::turn() {
	if(!moving && !justStoppedMoving) {
		int wantedAngle = INVALID;

		// Can not turn while reloading 'n salving
		if (salveWeapon >= 1 && !checkSalveRealoaded(salving)) return;

        // if we have to decide between moving and shooting we opt for moving
		// except opposite if we have a close target and are a turreted unit
		if(nextSpotAngle != INVALID && !(isTracked() && isTurreted() && ( ((TankBase*)this)->getCloseTarget() || isInAttackRange(target.getUnitPointer()) ))) {
            wantedAngle = nextSpotAngle;
        } else if(targetAngle != INVALID) {
            wantedAngle = targetAngle;
        }

		if(wantedAngle != INVALID) {
            float	angleLeft = 0.0f;
            float   angleRight = 0.0f;

            if(angle > wantedAngle) {
                angleRight = angle - wantedAngle;
                angleLeft = strictmath::abs(8.0f-angle)+wantedAngle;
            } else if (angle < wantedAngle) {
                angleRight = strictmath::abs(8.0f-wantedAngle) + angle;
                angleLeft = wantedAngle - angle;
            }

            if(angleLeft <= angleRight) {
                turnLeft();
            } else {
                turnRight();
            }
		}
	}
}





void UnitBase::turnLeft() {
	angle += currentGame->objectData.data[itemID][originalHouseID].turnspeed;
	if(angle >= 7.5f) {
	    drawnAngle = lround(angle) - 8;
        angle -= 8.0f;
	} else {
        drawnAngle = lround(angle);
	}
}

void UnitBase::turnRight() {
	angle -= currentGame->objectData.data[itemID][originalHouseID].turnspeed;
	if(angle <= -0.5f) {
	    drawnAngle = lround(angle) + 8;
		angle += 8.0f;
	} else {
	    drawnAngle = lround(angle);
	}
}

void UnitBase::quitDeviation() {
    if(wasDeviated()) {
        // revert back to real owner
        removeFromSelectionLists();
        setTarget(NULL);
        setGuardPoint(location);
        setDestination(location);
        owner = currentGame->getHouse(originalHouseID);
        graphic = pGFXManager->getObjPic(graphicID,getOwner()->getHouseID());
        deviationTimer = INVALID;
    }
}

bool UnitBase::update() {
    if(active) {
        targeting();
        if(active && (isTracked() && isTurreted())) {
          	((TankBase*)this)->turnTurret();
      	}

        navigate();
        move();
        if(active) {
            turn();
        }
    }

    if(isBadlyDamaged()) {
        salving=false;
    }


    if(getHealth() <= 0.0f) {
        destroy();
        return false;
    }

    if(active && (getHealthColor() != COLOR_LIGHTGREEN) && !goingToRepairYard && owner->hasRepairYard() && (owner->isAI() || owner->hasCarryalls())
            && !isInfantry() && !isAFlyingUnit() && !forced && getFellow() == NULL && !wasDeviated()) {
        doRepair();
    }


    if(recalculatePathTimer > 0) recalculatePathTimer--;
    if(findTargetTimer > 0) findTargetTimer--;
    if(primaryWeaponTimer > 0) primaryWeaponTimer--;
    if(secondaryWeaponTimer > 0) secondaryWeaponTimer--;


	for (int i=0; i < salveWeapon  && salveWeapon < MAX_SALVE; i++) {
		if (salveWeaponTimer[i] > 0) salveWeaponTimer[i]--;
	}
	if (salveWeaponDelay > 0 ) salveWeaponDelay--;

    if(deviationTimer != INVALID) {
        if(--deviationTimer <= 0) {
            quitDeviation();
        }
    }

	return true;
}

bool UnitBase::canPass(int xPos, int yPos) const {
	return (currentGameMap->tileExists(xPos, yPos) && !currentGameMap->getTile(xPos, yPos)->hasAGroundObject() && !currentGameMap->getTile(xPos, yPos)->isMountain());
}

bool UnitBase::SearchPathWithAStar() {

	Coord destinationCoord = destination;

	if ( isFollowing() ) {
		if (getFellow() !=NULL && getFellow()->isAStructure()) {
			if(itemID == Unit_Carryall && getFellow()->getItemID() == Structure_Refinery) {
				destinationCoord = getFellow()->getLocation() + Coord(2,0);
			} else if(itemID == Unit_Frigate && getFellow()->getItemID() == Structure_StarPort) {
				destinationCoord = getFellow()->getLocation() + Coord(1,1);
			}
			else {
				destinationCoord = getFellow()->getLocation();
			}
		} else if (getFellow() !=NULL && getFellow()->isAUnit()) {
			if (forced) {
				// We're forced to move to a destination in the first place
				// at arrival, we'll status and move on further oldTarget location
				destinationCoord = destination;
			} else if (/*!((UnitBase*)getFellow())->pickedUp &&*/ getFellow()->isActive()) {
				if (isSelected()) {
					std::list<Uint32>::iterator iter;
					std::list<std::pair<Uint32,Coord>>::iterator iter2;

					iter = std::find(currentGame->getSelectedList().begin(), currentGame->getSelectedList().end(), getObjectID());
					iter2 = std::find_if(currentGame->getSelectedListCoord().begin(), currentGame->getSelectedListCoord().end(), [=](std::pair<Uint32,Coord> & item) { return item.first == getObjectID() ;} );

					if (iter != std::end(currentGame->getSelectedList())) {
						/*err_print("UnitBase::SearchPathWithAStar found fellow unit %d [%d(%d),%d(%d)] \n",currentGame->getObjectManager().getObject(*iter)->getObjectID(),
									iter2 != std::end(currentGame->getSelectedListCoord()) ? getFellow()->getLocation()+iter2->second.x : INVALID_POS,
									iter2 != std::end(currentGame->getSelectedListCoord()) ?	iter2->second.x : INVALID_POS,
									iter2 != std::end(currentGame->getSelectedListCoord()) ? getFellow()->getLocation()+iter2->second.y : INVALID_POS,
									iter2 != std::end(currentGame->getSelectedListCoord()) ?	iter2->second.y : INVALID_POS
								);*/

						if (iter2 != std::end(currentGame->getSelectedListCoord())) {
							destinationCoord = getFellow()->getLocation()+iter2->second;
						} else
							destinationCoord = getFellow()->getLocation();

					} else {
						//err_print("UnitBase::SearchPathWithAStar cannot find fellow unit %d \n",getObjectID());
						destinationCoord = getFellow()->getLocation();
					}
				} else	destinationCoord = getFellow()->getLocation();
	    	} else	destinationCoord = getFellow()->getLocation();
	    }
	}
	if((!isFollowing() || forced) && target && target.getObjPointer() != NULL) {
            destinationCoord = target.getObjPointer()->getClosestPoint(location);
	}

	AStarSearch pathfinder(currentGameMap, this, location, destinationCoord);
	pathList = pathfinder.getFoundPath();

	if(pathList.empty() == true) {
        nextSpotFound = false;
        return false;
	} else {
	    return true;
	}
}

void UnitBase::drawSmoke(int x, int y) {
	int frame = ((currentGame->getGameCycleCount() + (getObjectID() * 10)) / SMOKEDELAY) % (2*2);
	if(frame == 3) {
        frame = 1;
	}

	SDL_Surface** smoke = pGFXManager->getObjPic(ObjPic_Smoke,getOwner()->getHouseID());

	int imageW = smoke[currentZoomlevel]->w/3;

    SDL_Rect dest = {   x - imageW/2,
                        y - smoke[currentZoomlevel]->h,
                        imageW,
                        smoke[currentZoomlevel]->h };

    SDL_Rect source = { imageW * frame, 0,
                        imageW, smoke[currentZoomlevel]->h };

	SDL_BlitSurface(smoke[currentZoomlevel], &source, screen, &dest);
}

void UnitBase::playAttackSound() {
}
