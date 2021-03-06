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

#include <misc/draw_util.h>

#define SALVO_TIMER_LAUNCHER 85

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
	salveWeaponDelay = SALVO_TIMER_LAUNCHER;
	bulletType = Bullet_Rocket;
	for (int i=0; i < salveWeapon  && salveWeapon < MAX_SALVE; i++) {
		salveWeaponTimer[i] =  (salveWeaponDelay*i)+salveWeaponDelay;
	}
	salving = false;
}

Launcher::~Launcher() {
}

void Launcher::drawSelectionBox()
{
    SDL_Surface* selectionBox = NULL;

    switch(currentZoomlevel) {
        case 0:     selectionBox = pGFXManager->getUIGraphic(UI_SelectionBox_Zoomlevel0);   break;
        case 1:     selectionBox = pGFXManager->getUIGraphic(UI_SelectionBox_Zoomlevel1);   break;
        case 2:
        default:    selectionBox = pGFXManager->getUIGraphic(UI_SelectionBox_Zoomlevel2);   break;
    }
	int x = screenborder->world2screenX(realX) - selectionBox->w/2;
	int y = screenborder->world2screenY(realY) - selectionBox->h/2;

	if (bFollow)
	    	selectionBox = mapSurfaceColorRange(selectionBox, COLOR_WHITE, COLOR_WINDTRAP_COLORCYCLE);

    SDL_Rect dest = {   x,
                        y,
                        selectionBox->w,
                        selectionBox->h };

	SDL_BlitSurface(selectionBox, NULL, screen, &dest);

	for(int i=1;i<=currentZoomlevel+1;i++) {
        drawRect(screen, dest.x+1, dest.y-1, dest.x+1 + ((int)((getHealth()/(float)getMaxHealth())*(selectionBox->w-3))),dest.y-2,  bFollow ? COLOR_WINDTRAP_COLORCYCLE : getHealthColor());

	}

	int salvotimer=0,max=0;
	for (int i=0; i < salveWeapon  && salveWeapon < MAX_SALVE; i++) {
		salvotimer +=  salveWeaponTimer[i] ;
		max +=   (SALVO_TIMER_LAUNCHER*i)+SALVO_TIMER_LAUNCHER;
	}

	if((getOwner() == pLocalHouse || debug)  && (salvotimer > 0)   ) {
        for(int i=1;i<=currentZoomlevel+1;i++) {
            drawRect(screen, dest.x+1, dest.y-3, dest.x+1 +  ((int)((((float)salvotimer)/max)*(selectionBox->w-3))), dest.y-4, bFollow ? COLOR_WINDTRAP_COLORCYCLE : COLOR_YELLOW);
        }
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
	dbg_relax_print("Launcher::salveAttack salveWeaponTimer=%i %i %i %i %i %i %i %i %i %i\n",salveWeaponTimer[0],salveWeaponTimer[1],salveWeaponTimer[2],salveWeaponTimer[3],salveWeaponTimer[4],salveWeaponTimer[5],salveWeaponTimer[6],salveWeaponTimer[7]);

	if (salveWeapon < 1 || !salving) {
		dbg_relax_print("Launcher::salveAttack no more salving\n");
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
			int precision = 0;

			if (target && target.getObjPointer() != NULL) {
				targetCenterPoint = target.getObjPointer()->getClosestCenterPoint(location);
				targetDistance = blockDistance(location, targetCenterPoint);
				dbg_relax_print("Launcher::salveAttack targetdistance %lf weaponreach %d targetvalid? %s\n",targetDistance,getWeaponRange(),Target.isValid() ? "true" : "false" );
			}

			if (target.getObjPointer() != NULL && Target.isValid()) {
				Tile *pTile = currentGameMap->getTile(target.getObjPointer()->getLocation());
				isFogged 	= pTile->isFogged(owner->getHouseID());
				isExplored 	= pTile->isExplored(owner->getHouseID());
				/// TODO : should be made a tech-option : precision firing in FOW
				if (true && (isFogged || !isExplored )) {
					targetCenterPoint = pTile->getUnpreciseCenterPoint();
					precision = ceil(distanceFrom(targetCenterPoint,target.getObjPointer()->getClosestCenterPoint(location)));
				}
				else {
					targetCenterPoint = target.getObjPointer()->getClosestCenterPoint(location);
				}
				bAirBullet = target.getObjPointer()->isAFlyingUnit();
				/* Target is "locked" */

			} else if(currentGameMap->tileExists(Target)) {
				Tile *pTile = currentGameMap->getTile(Target);
				isFogged 	= pTile->isFogged(owner->getHouseID());
				isExplored 	= pTile->isExplored(owner->getHouseID());
				/// TODO : should be made a tech-option : precision firing in FOW
				if (true && (isFogged || !isExplored )) {
					targetCenterPoint = pTile->getUnpreciseCenterPoint();
					precision = ceil(distanceFrom(targetCenterPoint, currentGameMap->getTile(Target)->getCenterPoint()));
				}
				else {
					targetCenterPoint = currentGameMap->getTile(Target)->getCenterPoint();
				}
				bAirBullet = false;
			} else 	if (currentGameMap->tileExists(Pos)){
				Tile *pTile = currentGameMap->getTile(Pos);
				isFogged 	= pTile->isFogged(owner->getHouseID());
				isExplored 	= pTile->isExplored(owner->getHouseID());
				/// TODO : should be made a tech-option : precision firing in FOW
				if (true && (isFogged || !isExplored )) {
					targetCenterPoint = pTile->getUnpreciseCenterPoint();
					precision = ceil(distanceFrom(targetCenterPoint, currentGameMap->getTile(Pos)->getCenterPoint()));
				}
				else {
					targetCenterPoint = currentGameMap->getTile(Pos)->getCenterPoint();
				}
				bAirBullet = false;

			} else {
				dbg_relax_print("Launcher::salveAttack cannot be done !\n");
				// Give a chance to navigate to the target if we are able to move
		        if(checkSalveRealoaded(salving)) {
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
				 *	Build take few damage from salvo, salvo consist of a specific ammunition to barrage attack
				 */
				if (!(target.getObjPointer() != NULL && Target.isValid())) currentWeaponDamage *= 0.5; else currentWeaponDamage *= 0.75;
				if ( currentGameMap->getTile(location)->isRock() || currentGameMap->getTile(location)->isDunes() ) currentWeaponDamage *= 1.15 ;
				if (target.getObjPointer() != NULL  && (target.getObjPointer())->isAStructure()) currentWeaponDamage *= .35;
				dbg_relax_print("Launcher::salveAttack dmg=%i/%i : air? %s struct?%s locked? %s advg? %s fog? %s expl? %s\n", currentWeaponDamage, baseweapondmg,
															bAirBullet ? "y" : "n",
															target.getObjPointer() != NULL  && (target.getObjPointer())->isAStructure() ? "y" : "n",
															(target.getObjPointer() != NULL && Target.isValid()) ? "y" : "n" ,
															( currentGameMap->getTile(location)->isRock() || currentGameMap->getTile(location)->isDunes() ) ? "y" : "n",
															isFogged ? "y" : "n",
															isExplored ? "y" : "n"
				);

				salveWeaponDelay = SALVO_TIMER_LAUNCHER;
				primaryWeaponTimer = getWeaponReloadTime();
				bulletList.push_back( new Bullet( objectID, &centerPoint, &targetCenterPoint, currentBulletType, currentWeaponDamage, bAirBullet, precision) );
				playAttackSound();

		}
	}
}


bool Launcher::canAttack(const ObjectBase* object) const {
	return ((object != NULL)
			&& ((object->getOwner()->getTeam() != owner->getTeam()) || (object->getItemID() == Unit_Sandworm && getOwner()->getHouseID()!= HOUSE_FREMEN))
			&& object->isVisible(getOwner()->getTeam()));
}

void Launcher::playAttackSound() {
	soundPlayer->playSoundAt(Sound_Rocket,location);
}
