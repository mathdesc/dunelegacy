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

#include <units/Launcher.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <House.h>
#include <Game.h>
#include <Map.h>
#include <Explosion.h>
#include <ScreenBorder.h>
#include <SoundPlayer.h>
#include <Bullet.h>


Launcher::Launcher(House* newOwner) : TrackedUnit(newOwner) {
    Launcher::init();

    setHealth(getMaxHealth());
}

Launcher::Launcher(InputStream& stream) : TrackedUnit(stream) {
    Launcher::init();
}
void Launcher::init() {
    itemID = Unit_Launcher;
    owner->incrementUnits(itemID);

	graphicID = ObjPic_Tank_Base;
	gunGraphicID = ObjPic_Launcher_Gun;
	graphic = pGFXManager->getObjPic(graphicID,getOwner()->getHouseID());
	turretGraphic = pGFXManager->getObjPic(gunGraphicID,getOwner()->getHouseID());

	numImagesX = NUM_ANGLES;
	numImagesY = 1;

	numWeapons  = 2;
	salveWeapon = 8;
	canSalveAttackStuff = true;
	salveWeaponDelay = 55;
	bulletType = Bullet_Rocket;
	for (int i=0; i < salveWeapon  && salveWeapon < MAX_SALVE; i++) {
		salveWeaponTimer[i] =  (salveWeaponDelay*i)+salveWeaponDelay;
	}
	salving = false;
}

Launcher::~Launcher() {
}

void Launcher::blitToScreen() {
    SDL_Surface* pUnitGraphic = graphic[currentZoomlevel];
    int imageW1 = pUnitGraphic->w/numImagesX;
    int x1 = screenborder->world2screenX(realX);
    int y1 = screenborder->world2screenY(realY);

    SDL_Rect source1 = { drawnAngle * imageW1, 0, imageW1, pUnitGraphic->h };
    SDL_Rect dest1 = { x1 - imageW1/2, y1 - pUnitGraphic->h/2, imageW1, pUnitGraphic->h };

    SDL_BlitSurface(pUnitGraphic, &source1, screen, &dest1);

    const Coord launcherTurretOffset[] =    {   Coord(0, -12),
                                                Coord(0, -8),
                                                Coord(0, -8),
                                                Coord(0, -8),
                                                Coord(0, -12),
                                                Coord(0, -8),
                                                Coord(0, -8),
                                                Coord(0, -8)
                                            };

    SDL_Surface* pTurretGraphic = turretGraphic[currentZoomlevel];
    int imageW2 = pTurretGraphic->w/numImagesX;
    int x2 = screenborder->world2screenX(realX + launcherTurretOffset[drawnAngle].x);
    int y2 = screenborder->world2screenY(realY + launcherTurretOffset[drawnAngle].y);

    SDL_Rect source2 = { drawnAngle * imageW2, 0, imageW2, pTurretGraphic->h };
    SDL_Rect dest2 = { x2 - imageW2/2, y2 - pTurretGraphic->h/2, imageW2, pTurretGraphic->h };

    SDL_BlitSurface(pTurretGraphic, &source2, screen, &dest2);

    if(isBadlyDamaged()) {
        drawSmoke(x1, y1);
    }
}

void Launcher::destroy() {
    if(currentGameMap->tileExists(location) && isVisible()) {
        Coord realPos(lround(realX), lround(realY));
        Uint32 explosionID = currentGame->randomGen.getRandOf(3,Explosion_Medium1, Explosion_Medium2,Explosion_Flames);
        currentGame->getExplosionList().push_back(new Explosion(explosionID, realPos, owner->getHouseID()));

        if(isVisible(getOwner()->getTeam()))
            soundPlayer->playSoundAt(Sound_ExplosionMedium,location);
    }

    TrackedUnit::destroy();
}


void Launcher::salveAttack(Coord Pos, Coord Target) {
	//fprintf(stderr,"Launcher::salveAttack salveWeaponTimer=%i %i %i %i %i %i %i %i %i %i\n",salveWeaponTimer[0],salveWeaponTimer[1],salveWeaponTimer[2],salveWeaponTimer[3],salveWeaponTimer[4],salveWeaponTimer[5],salveWeaponTimer[6],salveWeaponTimer[7]);

	if (salveWeapon < 1 || !salving) {
		fprintf(stderr,"Launcher::salveAttack no more salving\n");
		return;
	}

	for (int i=0; i < salveWeapon  && salveWeapon < MAX_SALVE; i++) {
		if((salveWeaponTimer[i] <= 0 ) && (isBadlyDamaged() == false)  && salveWeaponDelay <= 0 ) {
			Coord targetCenterPoint;
			Coord centerPoint = getCenterPoint();
			bool bAirBullet;
			bool isFogged, isExplored;
			int currentBulletType;
			Sint32 currentWeaponDamage;

			if (target && target.getObjPointer() != NULL) {
				targetCenterPoint = target.getObjPointer()->getClosestCenterPoint(location);
				targetDistance = blockDistance(location, targetCenterPoint);
			//	fprintf(stderr,"Launcher::salveAttack targetdistance %lf weaponreach %d targetvalid? %s\n",targetDistance,getWeaponRange(),Target.isValid() ? "true" : "false" );
			}

			if (target.getObjPointer() != NULL && Target.isValid()) {
				targetCenterPoint = target.getObjPointer()->getClosestCenterPoint(location);
				bAirBullet = target.getObjPointer()->isAFlyingUnit();
				/* Target is "locked" */
				isFogged = false;
				isExplored = true;
			} else if(currentGameMap->tileExists(Target)) {
				targetCenterPoint = currentGameMap->getTile(Target)->getCenterPoint();
				bAirBullet = false;
				isFogged = currentGameMap->getTile(Target)->isFogged(owner->getHouseID());
				isExplored = currentGameMap->getTile(Target)->isExplored(owner->getHouseID());
			} else 	if (currentGameMap->tileExists(Pos)){
				targetCenterPoint = currentGameMap->getTile(Pos)->getCenterPoint();
				bAirBullet = false;
				isFogged = currentGameMap->getTile(Pos)->isFogged(owner->getHouseID());
				isExplored = currentGameMap->getTile(Pos)->isExplored(owner->getHouseID());
			} else {
				//fprintf(stderr,"Launcher::salveAttack cannot be done !\n");
				// Give a chance to navigate to the target if we are able to move
		        if(checkSalveRealoaded(salving) && currentGame->randomGen.rand(0, 50) == 0) {
		            navigate();

		        }
				return;
			}


				bAirBullet ? currentBulletType = Bullet_SmallRocket : currentBulletType = bulletType;
				bAirBullet ? currentWeaponDamage = currentGame->objectData.data[itemID][originalHouseID].weapondamage / (salveWeapon / 2) :
							 currentWeaponDamage = currentGame->objectData.data[itemID][originalHouseID].weapondamage / (salveWeapon / 3)  ;
				int baseweapondmg = currentWeaponDamage;
				bAirBullet ? salveWeaponTimer[i] = getWeaponReloadTime()+ salveWeaponDelay*.75*i+salveWeaponDelay : salveWeaponTimer[i] = getWeaponReloadTime()+ salveWeaponDelay*i+salveWeaponDelay;
				/* Damage modifier :
				 * 	Target is not locked ? (ie. position attack or barrage fire)
				 * 	Have we gain a stable ground or high point advantage ?
				 *	Except when target locked decrease damage (to simulate lack of precision) when target is fogged or in unexplored terrain
				 */
				if (!(target.getObjPointer() != NULL && Target.isValid())) currentWeaponDamage *= 0.5; else currentWeaponDamage *= 0.75;
				if ( currentGameMap->getTile(location)->isRock() || currentGameMap->getTile(location)->isDunes() ) currentWeaponDamage *= 1.15 ;
				if (isFogged) currentWeaponDamage *= .80;
				if (!isExplored) currentWeaponDamage *= 0.25;
				/*fprintf(stderr,"Launcher::salveAttack dmg=%i/%i : air? %s locked? %s advg? %s fog? %s expl? %s\n", currentWeaponDamage, baseweapondmg,
															bAirBullet ? "y" : "n",
															(target.getObjPointer() != NULL && Target.isValid()) ? "y" : "n" ,
															( currentGameMap->getTile(location)->isRock() || currentGameMap->getTile(location)->isDunes() ) ? "y" : "n",
															isFogged ? "y" : "n",
															isExplored ? "y" : "n"
				);*/

				salveWeaponDelay = 55;
				primaryWeaponTimer = getWeaponReloadTime();
				bulletList.push_back( new Bullet( objectID, &centerPoint, &targetCenterPoint, currentBulletType, currentWeaponDamage, bAirBullet) );
				playAttackSound();

		}
	}
}

/**
    Is this object in a range we can attack.
    \param  object  the object to check
*/
bool Launcher::isInWeaponRange(const ObjectBase* object) const {
	   if(object == NULL) {
	        return false;
	    }

	    Coord targetLocation = target.getObjPointer()->getClosestPoint(location);

	    return (blockDistance(location, targetLocation) <= getWeaponRange());
}

bool Launcher::canAttack(const ObjectBase* object) const {
	return ((object != NULL)
			&& ((object->getOwner()->getTeam() != owner->getTeam()) || object->getItemID() == Unit_Sandworm)
			&& object->isVisible(getOwner()->getTeam()));
}

void Launcher::playAttackSound() {
	soundPlayer->playSoundAt(Sound_Rocket,location);
}
