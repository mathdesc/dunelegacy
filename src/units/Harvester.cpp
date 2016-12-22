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

#include <units/Harvester.h>
#include <units/Carryall.h>
#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <House.h>
#include <Game.h>
#include <Map.h>
#include <Explosion.h>
#include <SoundPlayer.h>
#include <ScreenBorder.h>

#include <players/HumanPlayer.h>

#include <structures/Refinery.h>

#include <misc/draw_util.h>

#include <algorithm>

/* how often is the same sandframe redrawn */
#define HARVESTERDELAY 30

/* how often to change harvester position while harvesting */
#define RANDOMHARVESTMOVE 500

/* how much is the harvester movement slowed down when full  */
#define MAXIMUMHARVESTERSLOWDOWN 0.4f

Harvester::Harvester(House* newOwner) : TrackedUnit(newOwner)
{
    Harvester::init();

    setHealth(getMaxHealth());

    spice = 0.0f;
    harvestingMode = false;
	returningToRefinery = false;
	spiceCheckCounter = 100;
	currentMaxSpeed = 2.6f;
    attackMode = GUARD;

}

Harvester::Harvester(InputStream& stream) : TrackedUnit(stream)
{
    Harvester::init();

	harvestingMode = stream.readBool();
	returningToRefinery = stream.readBool();
    spice = stream.readFloat();
    spiceCheckCounter = stream.readUint32();
    currentMaxSpeed = stream.readFloat();
    prospectionSamples = stream.readUint32CoordPairVector();
    // TODO : save/load preference and Add a button to set preference on UnitInterface

}

void Harvester::init()
{
    itemID = Unit_Harvester;
    owner->incrementUnits(itemID);

	canAttackStuff = false;

	graphicID = ObjPic_Harvester;
	graphic = pGFXManager->getObjPic(graphicID,getOwner()->getHouseID());

	numImagesX = NUM_ANGLES;
	numImagesY = 1;
	preference = DENSITY;
	prospectionSamples.reserve(10);
	famine = 0;

}

Harvester::~Harvester()
{
	;
}

void Harvester::save(OutputStream& stream) const
{
	TrackedUnit::save(stream);
	stream.writeBool(harvestingMode);
	stream.writeBool(returningToRefinery);
    stream.writeFloat(spice);
    stream.writeUint32(spiceCheckCounter);
    stream.writeFloat(currentMaxSpeed);
    stream.writeUint32CoordPairVector(prospectionSamples);
}

void Harvester::blitToScreen()
{
    SDL_Surface* pUnitGraphic = graphic[currentZoomlevel];
    int imageW = pUnitGraphic->w/numImagesX;
    int imageH = pUnitGraphic->h/numImagesY;
    int x = screenborder->world2screenX(realX);
    int y = screenborder->world2screenY(realY);

    SDL_Rect source = { drawnAngle * imageW, drawnFrame * imageH, imageW, imageH };
    SDL_Rect dest = { x - imageW/2, y - imageH/2, imageW, imageH };

    SDL_BlitSurface(pUnitGraphic, &source, screen, &dest);

    if(isHarvesting() == true) {

        const Coord harvesterSandOffset[] = {   Coord(-56, 4),
                                                Coord(-28, 20),
                                                Coord(0, 24),
                                                Coord(28, 20),
                                                Coord(56, 4),
                                                Coord(40, -24),
                                                Coord(0, -36),
                                                Coord(-36, -24)
                                            };


        SDL_Surface** sand = pGFXManager->getObjPic(ObjPic_Harvester_Sand,getOwner()->getHouseID());

        SDL_Surface* pSandGraphic = sand[currentZoomlevel];
        int sandImageW = pSandGraphic->w/8;
        int sandImageH = pSandGraphic->h/3;
        int sandX = screenborder->world2screenX(realX + harvesterSandOffset[drawnAngle].x);
        int sandY = screenborder->world2screenY(realY + harvesterSandOffset[drawnAngle].y);

        int frame = ((currentGame->getGameCycleCount() + (getObjectID() * 10)) / HARVESTERDELAY) % (2*LASTSANDFRAME);
        if(frame > LASTSANDFRAME) {
            frame -= LASTSANDFRAME;
        }

        SDL_Rect sandSource = { drawnAngle* sandImageW, frame * sandImageH, sandImageW, sandImageH };
        SDL_Rect sandDest = { sandX - sandImageW/2, sandY - sandImageH/2, sandImageW, sandImageH };

        SDL_BlitSurface(pSandGraphic, &sandSource, screen, &sandDest);
    }

    if(isBadlyDamaged()) {
        drawSmoke(x, y);
    }
}


Refinery* Harvester::findRefinery() {

	Refinery	*bestRefinery = NULL;

	if (!structureList.empty()) {

		int	leastNumBookings = 1000000; //huge amount so refinery couldn't possibly compete with any refinery num bookings
		float	closestLeastBookedRefineryDistance = std::numeric_limits<float>::infinity();


        RobustList<StructureBase*>::const_iterator iter;
        for(iter = structureList.begin(); iter != structureList.end(); ++iter) {
			StructureBase* tempStructure = *iter;

			if((tempStructure->getItemID() == Structure_Refinery) && (tempStructure->getOwner() == owner)) {
				Refinery* tempRefinery = static_cast<Refinery*>(tempStructure);
				Coord closestPoint = tempRefinery->getClosestPoint(location);
				float tempDistance = distanceFrom(location, closestPoint);
				int tempNumBookings = tempRefinery->getNumBookings();

				if (tempNumBookings < leastNumBookings)	{
					leastNumBookings = tempNumBookings;
					closestLeastBookedRefineryDistance = tempDistance;
					bestRefinery = tempRefinery;
				} else if (tempNumBookings == leastNumBookings) {
					if (tempDistance < closestLeastBookedRefineryDistance) {
						closestLeastBookedRefineryDistance = tempDistance;
						bestRefinery = tempRefinery;
					}
				}
			}
		}
	}

	return bestRefinery;
}

Coord Harvester::startProspection(const Coord & zone) {
	Tile * pTile = currentGameMap->tileExists(zone) ? currentGameMap->getTile(zone) : NULL;

	if (pTile == NULL) return Coord::Invalid();

	Coord spicelocation= zone;
	spiceCheckCounter = 100;

	return currentGameMap->findSpice(spicelocation, zone, this) ? spicelocation : Coord::Invalid();
}


bool Harvester::findSpice(const Coord & zone, Coord &destination, bool noPropection) {

	Coord olddest = destination;
	destination = noPropection ? Coord::Invalid(): startProspection(zone);
	if (destination.isInvalid() || destination == zone) {
		// Prospection is a failure, look for ViewRange spice
		for (int i = zone.x - this->getViewRange(); i <= zone.x + this->getViewRange(); i++)
			for (int j = zone.y - this->getViewRange(); j <= zone.y + this->getViewRange(); j++)
				if (	currentGameMap->tileExists(i, j) &&
						currentGameMap->getTile(i,j)->hasSpice() &&
						(!currentGameMap->getTile(i,j)->hasAGroundObject() || currentGameMap->getTile(i,j)->getGroundObject()->getDestination() != Coord(i,j) )
					)
					{
						destination = Coord(i,j);
						addProspectionSample(destination,currentGameMap->getTile(destination)->getExtractionSpeed());
					}
	}

	if (destination.isValid() )
		return true;
	else {
		destination = olddest;
		return false;
	}

}

void Harvester::checkPos()
{
	TrackedUnit::checkPos();

	if(attackMode == STOP) {
        harvestingMode = false;
	}

	if(active)	{
		if (isSelected()) {
			/*  err_relax_print("Harvester::checkPos Harvester %d #samples:%d spiceunder:%f(spice?%s)\n",getObjectID(),getProspectionSamples().size(),
					  currentGameMap->getTile(location)->getSpiceRemaining(),
					  currentGameMap->getTile(location)->hasSpice() ? "yes" : "no");*/
		}
		if (returningToRefinery) {
			if (fellow && (fellow.getObjPointer() != NULL) && (fellow.getObjPointer()->getItemID() == Structure_Refinery)) {
				//find a refinery to return to
				Coord closestPoint = fellow.getObjPointer()->getClosestPoint(location);

				if(!moving && !justStoppedMoving && blockDistance(location, closestPoint) <= 1.5f)	{
					awaitingPickup = false;
					if (((Refinery*)fellow.getObjPointer())->isFree())
						setReturned();
				} else if(!awaitingPickup && blockDistance(location, closestPoint) >= 5.0f) {
					requestCarryall();
				}
			} else if (!structureList.empty() && attackMode != STOP) {
					Refinery* bestRefinery= findRefinery();
					if (bestRefinery != NULL) {
						bestRefinery->startAnimate();
						doMove2Object(findRefinery(), getTarget());
					}
			}
		} else if (harvestingMode && !hasBookedCarrier() && (blockDistance(location, destination) > 10.0f)) {
			requestCarryall();
        } else if(respondable && !harvestingMode && attackMode != STOP) {
            if(spiceCheckCounter <= 0) {
            	if (destination == location) {
					if(findSpice(guardPoint, destination)) {
						harvestingMode = true;
						setGuardPoint(destination);
					} else if (getProspectionSamplesIsEmpty()) {
						if (currentGameMap->getTile(location)->hasSpice()) {
							// harvest the last patch around
							harvestingMode = true;
						}
						else {
							// last patch harvested , nothing all around
							doReturn();
							spiceCheckCounter = 0;
						}
					}
            	}
                spiceCheckCounter = 100;
            } else {
            	// XXX : should be in a prospection attackmode displayed on the interface like a filled rectangle that depletes to "stop" attackmode
                spiceCheckCounter--;
            }
		}
	}
}



void Harvester::destroy()
{
    if(currentGameMap->tileExists(location) && isVisible()) {
        int xpos = location.x;
        int ypos = location.y;

        if(currentGameMap->tileExists(xpos,ypos)) {
            float spiceSpreaded = 0.75f * spice;
            int availableSandPos = 0;

            int circleRadius = lroundf(spice / 210);

            /* how many regions have sand */
            for(int i = -circleRadius; i <= circleRadius; i++) {
                for(int j = -circleRadius; j <= circleRadius; j++) {
                    if(currentGameMap->tileExists(xpos + i, ypos + j)
                        && (distanceFrom(xpos, ypos, xpos + i, ypos + j) + 0.0005 <= circleRadius))
                    {
                        Tile *pTile = currentGameMap->getTile(xpos + i, ypos + j);
                        if((pTile != NULL) & ((pTile->isSand()) || (pTile->isSpice()) )) {
                            availableSandPos++;
                        }
                    }
                }
            }

            /* now we can spread spice */
            for(int i = -circleRadius; i <= circleRadius; i++) {
                for(int j = -circleRadius; j <= circleRadius; j++) {
                    if(currentGameMap->tileExists(xpos + i, ypos + j)
                        && (distanceFrom(xpos, ypos, xpos + i, ypos + j) + 0.0005  <= circleRadius))
                    {
                        Tile *pTile = currentGameMap->getTile(xpos + i, ypos + j);
                        if((pTile != NULL) & ((pTile->isSand()) || (pTile->isSpice()) )) {
                            pTile->setSpice(pTile->getSpice() + spiceSpreaded / availableSandPos);
                        }
                    }
                }
            }
        }

        setTarget(NULL);
        setFellow(NULL);

        Coord realPos(lround(realX), lround(realY));
        Uint32 explosionID = currentGame->randomGen.getRandOf(2,Explosion_Medium1, Explosion_Medium2);
        currentGame->getExplosionList().push_back(new Explosion(explosionID, realPos, owner->getHouseID()));

        if(isVisible(getOwner()->getTeam())) {
            screenborder->shakeScreen(18);
            soundPlayer->playSoundAt(Sound_ExplosionLarge,location);
        }
    }

	TrackedUnit::destroy();
}

void Harvester::drawSelectionBox()
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

    SDL_Rect dest = {   x,
                        y,
                        selectionBox->w,
                        selectionBox->h };

	SDL_BlitSurface(selectionBox, NULL, screen, &dest);

	for(int i=1;i<=currentZoomlevel+1;i++) {
        drawRect(screen, dest.x+1, dest.y-1, dest.x+1 + ((int)((getHealth()/(float)getMaxHealth())*(selectionBox->w-3))),dest.y-2, bFollow ? COLOR_WINDTRAP_COLORCYCLE : getHealthColor());
	}

	if((getOwner() == pLocalHouse || debug) && (spice > 0.0f)) {
        for(int i=1;i<=currentZoomlevel+1;i++) {
            drawRect(screen, dest.x+1, dest.y-3, dest.x+1 + ((int)((((float)spice)/HARVESTERMAXSPICE)*(selectionBox->w-3))),dest.y-4, COLOR_ORANGE);
        }
	}

	if (isLeader()) {
		SDL_Surface** star = pGFXManager->getObjPic(ObjPic_Star,pLocalHouse->getHouseID());

		int imageW = star[currentZoomlevel]->w/3;

	    SDL_Rect dest = {   x - imageW/2,
	                        y - 2*star[currentZoomlevel]->h ,
	                        imageW,
	                        star[currentZoomlevel]->h };

		SDL_BlitSurface(star[currentZoomlevel],NULL, screen, &dest);

	}


}

void Harvester::handleDamage(int damage, Uint32 damagerID, House* damagerOwner)
{
    TrackedUnit::handleDamage(damage, damagerID, damagerOwner);

    ObjectBase* damager = currentGame->getObjectManager().getObject(damagerID);
    if (!damager) return;


    bool mask_retaliate =  target && target.getObjPointer() && target.getObjPointer()->isAUnit() && target.getObjPointer()->isActive() &&
    						damager->getObjectID() != target.getObjPointer()->getObjectID() &&
							blockDistance((target.getObjPointer())->getClosestPoint(location),damager->getClosestPoint(location)) >= 3.0f;

    bool damager_nearer = isNearer(damager) ;
    bool mask_retaliate3 = !forced && canAttack(damager) && (attackMode != STOP);
    err_print("Harvester::handleDamage Harvester %d \n",getObjectID());

    if((!target && mask_retaliate3) || (mask_retaliate && damager_nearer && mask_retaliate3 )) {
        setTarget(damager);
        err_print("Harvester::handleDamage2 Harvester %d set a new target %d \n",getObjectID(),damager->getObjectID());
    }
}

void Harvester::handleReturnClick() {
	currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_HARVESTER_RETURN,objectID));
}

void Harvester::doReturn()
{
	returningToRefinery = true;
	harvestingMode = false;
	oldspice = 0.0f;

	if(getAttackMode() == STOP) {
        setGuardPoint(Coord::Invalid());
	}
}

void Harvester::deploy(const Coord& newLocation)
{
	if(currentGameMap->tileExists(newLocation)) {
		UnitBase::deploy(newLocation);

		if (this->getFellow() !=NULL) {
			this->swapOldNewFellow();
			if (this->getOldFellow() != NULL &&
					(this->getOldFellow()->getItemID() == Structure_Refinery || this->getOldFellow()->getItemID() == Unit_Carryall)
				) {
				this->setOldFellow(NULL);
			}

		}
		err_print("Harvester::deploy Harvester %d f:%d of:%d \n",this->getObjectID(),
							(this->getFellow() !=NULL ? this->getFellow()->getObjectID() :  -1),
							(this->getOldFellow() != NULL ? this->getOldFellow()->getObjectID() : -1));
		// xxx manual exit
		//if(spice == 0.0f) {
			if((attackMode != STOP) && !getProspectionSamplesIsEmpty()) {
				harvestingMode = true;
				destination  = getPreference() == DISTANCE ? this->getProspectionSampleBestDistance().first : this->getProspectionSampleBestDensity().first;
				setGuardPoint(destination);
				spiceCheckCounter = 0;
			} else if ((attackMode != STOP) && findSpice(guardPoint, destination)) {
				harvestingMode = true;
				setGuardPoint(destination);
				spiceCheckCounter = 100;
			} else {
				harvestingMode = false;
				if (attackMode != STOP) {
					famine++;
					if (spiceCheckCounter == 0) {
						if (findSpice(guardPoint, destination)) {
							famine = 0;
							harvestingMode = true;
							setGuardPoint(destination);
							spiceCheckCounter = 100;
						}
					}
					if (famine > 3) {
						doSetAttackMode(STOP);
						famine=0;
					}
				}
			}
		//}
	}
}

/*void Harvester::doDeploy(Coord location)
{
	if(currentGameMap->tileExists(location)) {
		UnitBase::deploy(location);
		//if (getFellow()->getItemID() == Structure_Refinery)
		//	setFellow(NULL);
			// adding attmode !=STOP to mirror above
			if((attackMode != STOP)  && currentGameMap->findSpice(destination, guardPoint, this)) {
				harvestingMode = true;
				guardPoint = destination;
				attackMode = GUARD;
			} else {
				harvestingMode = false;
			}

	}

}*/


void Harvester::setAmountOfSpice(float newSpice)
{
	if((newSpice >= 0.0f) && (newSpice <= HARVESTERMAXSPICE)) {
		spice = newSpice;
	}
}

void Harvester::setDestination(int newX, int newY)
{
	TrackedUnit::setDestination(newX, newY);

	harvestingMode =  (attackMode != STOP) && (currentGameMap->tileExists(newX, newY) && (currentGameMap->getTile(newX,newY)->hasSpice() || hasProspectionSamples(destination)));
}

void Harvester::setGuardPoint(int newX, int newY) {

	TrackedUnit::setGuardPoint(newX, newY);

	if (guardPoint.isValid()) {
        if(currentGameMap->getTile(newX, newY)->hasSpice()) {
            if(attackMode == STOP) {
                attackMode = GUARD;
            }
        } else {
        	if (attackMode != STOP) {
        		// We should set stop harvesting operation if :
        		// - were forced except onto prospection site (manual removal ie safe rock ground on worm signs)
        		// - were are idling without any prospected site from where to harvest
        		if ((wasForced() && !getProspectionSamplesIsEmpty()  && !this->hasProspectionSamples(guardPoint)) ||
        				(getProspectionSamplesIsEmpty() && location==destination && isIdle())) {
        			// XXX : hacky workaround piling harvester : they come on the same spot and tends to enter stop mode
        			if (!((currentGameMap->getTile(guardPoint)->hasAGroundObject() &&
        					currentGameMap->getTile(guardPoint)->getGroundObject()->getDestination() == guardPoint ))
        				)
        				attackMode = STOP;
        		}
            }
        }
	}
}

void Harvester::setFellow(const ObjectBase* newFellow) {

	if(returningToRefinery && fellow && (fellow.getObjPointer()!= NULL)
		&& (fellow.getObjPointer()->getItemID() == Structure_Refinery))
	{
		((Refinery*)fellow.getObjPointer())->unBook();
		returningToRefinery = false;
	}

	TrackedUnit::setFellow(newFellow);

	if(fellow && (fellow.getObjPointer() != NULL)
		&& (fellow.getObjPointer()->getOwner() == getOwner())
		&& (fellow.getObjPointer()->getItemID() == Structure_Refinery))
	{
		((Refinery*)fellow.getObjPointer())->book();
		returningToRefinery = true;
	}
}


void Harvester::setTarget(const ObjectBase* newTarget)
{

	if (canAttack(newTarget))
		TrackedUnit::setTarget(newTarget);

}

void Harvester::setReturned()
{

	if (fellow && getFellow()->getItemID() == Structure_Refinery) {
		if(selected) {
			removeFromSelectionLists();
		}

		currentGameMap->removeObjectFromMap(getObjectID());


		((Refinery*)getFellow())->assignHarvester(this);

		/*if ((!fellow || getFellow() != this) && getOldFellow() == NULL) {
				setOldFellow(getFellow());
		}*/
		soundPlayer->playSoundAt(Sound_Steam, location);
		returningToRefinery = false;
		moving = false;
		respondable = false;
		setActive(false);
		oldspice = 0.0f;

		setLocation(INVALID_POS, INVALID_POS);
		setVisible(VIS_ALL, false);
	}

}

void Harvester::move()
{
	UnitBase::move();

		///XXX : Harvest on the go should be made a tech option (instead of plain true)
		// It can harvest on the go with a lower extraction sleep or deepharvest on destination
		if (attackMode != STOP &&((true && active && harvestingMode) || (active && !moving && !justStoppedMoving))) {

			if(spice >= HARVESTERMAXSPICE) doReturn();
			else {
				// Harvest Spice along
				Tile* tile = currentGameMap->getTile(location);
				int beforeTileType = tile->getType();
				oldspice  = tile->getSpiceRemaining();
				spice += tile->harvestSpice(tile->getSpiceExtractionSpeed(false,location == destination ? true : false));
				int afterTileType = tile->getType();

				// we are just start idling, no spice is there (no harvesting anymore), but have samples of extraction sites
				if (isIdle() && !getProspectionSamplesIsEmpty() && !currentGameMap->getTile(location)->hasSpice()) {
					// if a PS was located there, now there is no more spice, so remove it anyway
					if (hasProspectionSamples(location))
						this->removeProspectionSample(location);
					harvestingMode = false;
					if (!getProspectionSamplesIsEmpty()) {
						// yet we still have work to do
						harvestingMode = true;
						setGuardPoint(destination);
						if (!wasForced())
							doMove2Pos(getPreference() == DISTANCE ? 	this->getProspectionSampleBestDistance().first :
																		this->getProspectionSampleBestDensity().first,false);
					}
					return;
				}

				// We were forced to move there so add a prospection sample if spice is found
				if (wasForced() && location == destination && spice > 0 && oldspice > spice ) {
					tile = currentGameMap->getTile(location);
					addProspectionSample(location,tile != NULL ? tile->getExtractionSpeed() : 0);
					spiceCheckCounter = 50;
				}

				if(beforeTileType != afterTileType) {
					currentGameMap->spiceRemoved(location);
					// We are at destination point
					if (location == destination) {
						if(!findSpice(guardPoint, destination)) {
							spiceCheckCounter = 100;
							if (getProspectionSamplesIsEmpty() && tile->getSpiceRemaining() == 0)
								doReturn();
							if (!getProspectionSamplesIsEmpty())
								doMove2Pos(getPreference() == DISTANCE ? 	this->getProspectionSampleBestDistance().first :
																			this->getProspectionSampleBestDensity().first,false);
						} else {
							if (!getProspectionSamplesIsEmpty()) {
								/*std::pair<Coord,int> shortest_site = getProspectionSampleBestDistance();
								std::pair<Coord,int> densest_site = getProspectionSampleBestDensity();
								Coord spiceCoord = getPreference() == DISTANCE ? shortest_site.first : densest_site.first;
								Tile * pTile = currentGameMap->tileExists(spiceCoord) ? currentGameMap->getTile(spiceCoord) : NULL;
								dbg_print("Harvester::move NbSamples(%d) %s(%d,%d) distance:%f spice:%d(@ %d)\n",getProspectionSamplesNumber(),
										getPreference() == DISTANCE ? "shorter":"denser",
										spiceCoord.x,spiceCoord.y,
										distanceFrom(spiceCoord,location),
										pTile !=NULL ? pTile->getSpiceRemaining(): -1,
										getPreference() == DISTANCE ? shortest_site.second : densest_site.second);
								doMove2Pos(destination,false);*/
								doMove2Pos(getPreference() == DISTANCE ? 	this->getProspectionSampleBestDistance().first :
																			this->getProspectionSampleBestDensity().first,false);
							}
						}
					}
				}

			}
		} else {
			oldspice = 0.0f;
		}

}

bool Harvester::isIdle() const {
	return !isHarvesting() && !returningToRefinery && !moving && respondable && !target && attackMode !=STOP;
}


bool Harvester::isHarvesting() const {
    return harvestingMode && /*(blockDistance(location, destination) <= DIAGONALCOST) &&*/ oldspice > 0.0f && currentGameMap->tileExists(location) && currentGameMap->getTile(location)->hasSpice();
}

bool Harvester::canAttack(const ObjectBase* object) const
{
	return((object != NULL)
			&& object->isInfantry()
			&& (object->getOwner()->getTeam() != owner->getTeam())
			&& object->isVisible(getOwner()->getTeam()));
}

float Harvester::extractSpice(float extractionSpeed)
{
	float oldSpice = spice;

	if((spice - extractionSpeed) >= 0.0f) {
		spice -= extractionSpeed;
	} else {
		spice = 0.0f;
	}

	return (oldSpice - spice);
}

float Harvester::getMaxSpeed()  {

    float dist = distanceFrom(location.x*TILESIZE + TILESIZE/2, location.y*TILESIZE + TILESIZE/2,
                                destination.x*TILESIZE + TILESIZE/2, destination.y*TILESIZE + TILESIZE/2);

    if((target || fellow) && dist < 256.0f) {
		currentMaxSpeed = (((2.0f - currentGame->objectData.data[itemID][originalHouseID].maxspeed)/256.0f) * (256.0f - dist)) + currentGame->objectData.data[itemID][originalHouseID].maxspeed;

    } else {
        currentMaxSpeed = currentGame->objectData.data[itemID][originalHouseID].maxspeed;

    }

	if(isBadlyDamaged()) {
		currentMaxSpeed *= HEAVILYDAMAGEDSPEEDMULTIPLIER;
	}

    float percentFull = spice/HARVESTERMAXSPICE;
    currentMaxSpeed = currentMaxSpeed * (1 - MAXIMUMHARVESTERSLOWDOWN*percentFull);


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


void Harvester::setSpeeds()
{
	float speed = getMaxSpeed();


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
