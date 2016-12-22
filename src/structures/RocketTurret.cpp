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

#include <structures/RocketTurret.h>

#include <globals.h>

#include <Bullet.h>
#include <SoundPlayer.h>
#include <sand.h>
#include <FileClasses/GFXManager.h>
#include <FileClasses/SFXManager.h>
#include <House.h>
#include <Game.h>
#include <Map.h>

RocketTurret::RocketTurret(House* newOwner) : TurretBase(newOwner) {
    RocketTurret::init();

    setHealth(getMaxHealth());
}

RocketTurret::RocketTurret(InputStream& stream) : TurretBase(stream) {
    RocketTurret::init();
}

void RocketTurret::init() {
	itemID = Structure_RocketTurret;
	owner->incrementStructures(itemID);

	attackSound = Sound_Rocket;
	bulletType = Bullet_TurretRocket;

	graphicID = ObjPic_RocketTurret;
	graphic = pGFXManager->getObjPic(graphicID,getOwner()->getHouseID());
	numImagesX = 10;
	numImagesY = 1;
	curAnimFrame = firstAnimFrame = lastAnimFrame = ((10-drawnAngle) % 8) + 2;
}

RocketTurret::~RocketTurret() {
}

void RocketTurret::updateStructureSpecificStuff() {
	if( ( !currentGame->getGameInitSettings().getGameOptions().rocketTurretsNeedPower || getOwner()->hasPower() )
      /*  || ( ((currentGame->gameType == GAMETYPE_CAMPAIGN) || (currentGame->gameType == GAMETYPE_SKIRMISH)) && getOwner()->isAI())*/ ) {
		TurretBase::updateStructureSpecificStuff();
	}
}

bool RocketTurret::canAttack(const ObjectBase* object) const {
	if((object != NULL)
		&& ((object->getOwner()->getTeam() != owner->getTeam()) || (object->getItemID() == Unit_Sandworm && getOwner()->getHouseID()!= HOUSE_FREMEN))
		&& object->isVisible(getOwner()->getTeam())) {
		return true;
	} else {
		return false;
	}
}

void RocketTurret::attack() {

	// XXX swap so that in case two targets are set, the flying unit always is on the oldtarget slot
	if (target.getObjPointer() != NULL && getTarget()->isAFlyingUnit() &&
		oldtarget.getObjPointer() != NULL && !getOldTarget()->isAFlyingUnit()) {
		ObjectBase * tmp = getOldTarget();
		setOldTarget(getTarget());
		setTarget(tmp);
	}


	if((weaponTimer == 0) && (target.getObjPointer() != NULL)) {
		Coord centerPoint = getCenterPoint();
		Coord targetCenterPoint;
		int precision = 0;
		/// TODO : should be made a tech-option : precision firing in FOW
		Tile *pTile 	=currentGameMap->getTile(target.getObjPointer()->getLocation()) ;
		bool isFogged 	= pTile->isFogged(owner->getHouseID());
		bool isExplored = pTile->isExplored(owner->getHouseID());
		if (true && (isFogged || !isExplored ) ) {
			targetCenterPoint = pTile->getUnpreciseCenterPoint();
			precision = ceil(distanceFrom(targetCenterPoint,target.getObjPointer()->getClosestCenterPoint(location)));
		}
		else {
			targetCenterPoint = target.getObjPointer()->getClosestCenterPoint(location);
		}

		if (this->isSelected()) err_relax_print("TurretBase::updateStructureSpecificStuff target:%s(%d) oldtarget:%s(%d) \n",
											getItemNameByID(target.getObjPointer() != NULL ? target.getObjPointer()->getItemID() : ItemID_Invalid).c_str(),
											target.getObjPointer() != NULL ? target.getObjPointer()->getObjectID() : -1,
											getItemNameByID(oldtarget.getObjPointer() != NULL ? oldtarget.getObjPointer()->getItemID() : ItemID_Invalid).c_str(),
											oldtarget.getObjPointer() != NULL ? oldtarget.getObjPointer()->getObjectID() : -1);

		if(distanceFrom(centerPoint, targetCenterPoint) < 3 * TILESIZE &&  !target.getObjPointer()->isAFlyingUnit() ) {
            // we are just shooting a bullet as a gun turret would do
            bulletList.push_back( new Bullet( objectID, &centerPoint, &targetCenterPoint, Bullet_ShellMedium,
                                                   currentGame->objectData.data[Structure_GunTurret][originalHouseID].weapondamage,
                                                   target.getObjPointer()->isAFlyingUnit(), precision ) );

            soundPlayer->playSoundAt(Sound_MountedCannon, location);
            weaponTimer = currentGame->objectData.data[Structure_GunTurret][originalHouseID].weaponreloadtime;
		} else  {
            // we are in normal shooting mode
            bulletList.push_back( new Bullet( objectID, &centerPoint, &targetCenterPoint, bulletType,
                                                   currentGame->objectData.data[itemID][originalHouseID].weapondamage,
                                                   target.getObjPointer()->isAFlyingUnit(), precision ) );

            soundPlayer->playSoundAt(attackSound, location);

            // XXX : should be made a tech-option : rturret able to take an opportunistic second shoot from the same angle at a flying oldtarget
            if (true && oldtarget.getObjPointer() != NULL && oldtarget.getObjPointer()->isActive() &&  oldtarget.getObjPointer()->isAFlyingUnit()) {
				pTile 		= currentGameMap->getTile(oldtarget.getObjPointer()->getLocation()) ;
				isFogged  	= pTile->isFogged(owner->getHouseID());
				isExplored 	= pTile->isExplored(owner->getHouseID());
				Coord oldtargetCenterPoint;
				if (true && (isFogged || !isExplored ) ) {
					oldtargetCenterPoint = pTile->getUnpreciseCenterPoint();
					precision = ceil(distanceFrom(oldtargetCenterPoint,oldtarget.getObjPointer()->getClosestCenterPoint(location)));
				}
				else {
					oldtargetCenterPoint = oldtarget.getObjPointer()->getClosestCenterPoint(location);
				}
				Coord closestPoint = oldtarget.getObjPointer()->getClosestPoint(location);
				float destAngle = destinationAngle(location, closestPoint);
				int wantedAngle = lround(8.0f/256.0f*destAngle);

				if(wantedAngle == 8) {
					wantedAngle = 0;
				}

				if (drawnAngle == wantedAngle) {
					 // we are in normal shooting mode
					bulletList.push_back( new Bullet( objectID, &centerPoint, &oldtargetCenterPoint, bulletType,
														   currentGame->objectData.data[itemID][originalHouseID].weapondamage,
														   oldtarget.getObjPointer()->isAFlyingUnit(), precision ) );
				}
            }
            weaponTimer = getWeaponReloadTime();


		}

	}
}
