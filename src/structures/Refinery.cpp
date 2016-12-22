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

#include <structures/Refinery.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <House.h>
#include <SoundPlayer.h>
#include <Map.h>

#include <units/UnitBase.h>
#include <units/Harvester.h>
#include <units/Carryall.h>

#include <GUI/ObjectInterfaces/RefineryAndSiloInterface.h>

/* how fast is spice extracted */
#define MAXIMUMHARVESTEREXTRACTSPEED 0.625f

Refinery::Refinery(House* newOwner) : StructureBase(newOwner) {
    Refinery::init();

    setHealth(getMaxHealth());

	extractingSpice = false;
    bookings = 0;

	firstRun = true;

    firstAnimFrame = 2;
	lastAnimFrame = 3;
}

Refinery::Refinery(InputStream& stream) : StructureBase(stream) {
    Refinery::init();

	extractingSpice = stream.readBool();
	harvester.load(stream);
	bookings = stream.readSint32();

	if(extractingSpice) {
		firstAnimFrame = 8;
		lastAnimFrame = 9;
		curAnimFrame = 8;
	} else if(bookings == 0) {
		stopAnimate();
	} else {
		startAnimate();
	}

	firstRun = false;
}

void Refinery::init() {
    itemID = Structure_Refinery;
	owner->incrementStructures(itemID);

	structureSize.x = 3;
	structureSize.y = 2;

	graphicID = ObjPic_Refinery;
	graphic = pGFXManager->getObjPic(graphicID,getOwner()->getHouseID());
	numImagesX = 10;
	numImagesY = 1;
}

Refinery::~Refinery() {
    if(extractingSpice && harvester) {
		if(harvester.getUnitPointer() != NULL)
			harvester.getUnitPointer()->destroy();
		harvester.pointTo(NONE);
	}
}

void Refinery::save(OutputStream& stream) const {
	StructureBase::save(stream);

    stream.writeBool(extractingSpice);
	harvester.save(stream);
	stream.writeSint32(bookings);
}

ObjectInterface* Refinery::getInterfaceContainer() {
	if((pLocalHouse == owner) || (debug == true)) {
		return RefineryAndSiloInterface::create(objectID);
	} else {
		return DefaultObjectInterface::create(objectID);
	}
}

void Refinery::assignHarvester(Harvester* newHarvester) {
	extractingSpice = true;
	harvester.pointTo(newHarvester);
	drawnAngle = 1;
	firstAnimFrame = 8;
	lastAnimFrame = 9;
	curAnimFrame = 8;
}

bool Refinery::deployHarvester(Carryall* pCarryall) {

	bool deployed = false;
	Harvester* pHarvester = (Harvester*) harvester.getObjPointer();

	if (harvester && (harvester.getObjPointer() != NULL)) {
		if(firstRun) {
			if(getOwner() == pLocalHouse) {
				soundPlayer->playVoice(HarvesterDeployed,getOwner()->getHouseID());
			}
			firstRun = false;
			// Start prospection from refinery zone
			if (pHarvester->startProspection(location) == Coord::Invalid()) {
				if(pHarvester->getOwner()->hasRadarOn()) {
					// TODO : lead harvester to a known spice area
					// that is from the pool of harvesters to get
					// it from (.ie by the closure of a graph of all collected samples)
					// only if radar is on
				}
			}
		}


		 {
			if (owner->getStoredCredits() >= owner->getCapacity()) {
				Coord deployPos = currentGameMap->findDeploySpot(pHarvester, location, location, structureSize);
				pHarvester->deploy(deployPos);
				if (pHarvester->getAmountOfSpice() >= HARVESTERMAXSPICE -1)
					pHarvester->doSetAttackMode(STOP);
				deployed = true;
			} else if(pCarryall != NULL) {
				pCarryall->giveCargo(pHarvester);
				pCarryall->setFellow(NULL);
				pCarryall->giveDeliveryOrders(pHarvester, pHarvester->getGuardPoint(), pHarvester->getGuardPoint(),
														currentGameMap->findDeploySpot(pHarvester, location, pHarvester->getGuardPoint(), structureSize));
				deployed = true;
			} else {
				Coord deployPos = currentGameMap->findDeploySpot(pHarvester, location, pHarvester->getGuardPoint(), structureSize);
				pHarvester->deploy(deployPos);
				deployed = true;
			}
		}

		if (deployed) {


			/// XXX
			/*
			if (pHarvester->getFellow() !=NULL) {
				pHarvester->swapOldNewFellow();
				if (pHarvester->getOldFellow() != NULL &&
						(pHarvester->getOldFellow()->getItemID() == Structure_Refinery || pHarvester->getOldFellow()->getItemID() == Unit_Carryall
								|| !pHarvester->getOldFellow()->isActive())
					) {
					pHarvester->setOldFellow(NULL);
				}

			}
			if (pHarvester->getFellow() !=NULL) {
				if (pHarvester->getFellow() != NULL &&
										(pHarvester->getFellow()->getItemID() == Structure_Refinery || pHarvester->getFellow()->getItemID() == Unit_Carryall
												|| !pHarvester->getFellow()->isActive())
					) {
					pHarvester->setFellow(NULL);
				}
			}
			 	*/


			err_print("Refinery::deployHarvester Harvester %d f:%d of:%d \n",pHarvester->getObjectID(),
								(pHarvester->getFellow() !=NULL && pHarvester->getFellow()->getObjectID()) || -1,
								(pHarvester->getOldFellow() != NULL && pHarvester->getOldFellow()->getObjectID()) || -1);
			harvester.pointTo(NONE);
			unBook();
			drawnAngle = 0;
			extractingSpice = false;
		}

		if(bookings == 0) {
			stopAnimate();
		} else {
			startAnimate();
		}

	}
	return deployed;
}

void Refinery::startAnimate() {
	if(extractingSpice == false) {
		firstAnimFrame = 2;
		lastAnimFrame = 7;
		curAnimFrame = 2;
        justPlacedTimer = 0;
        animationCounter = 0;
	}
}

void Refinery::stopAnimate() {
    firstAnimFrame = 2;
	lastAnimFrame = 3;
	curAnimFrame = 2;
}

void Refinery::updateStructureSpecificStuff() {
	if(extractingSpice) {
	    Harvester* pHarvester = (Harvester*) harvester.getObjPointer();

		if(pHarvester->getAmountOfSpice() > 0.0f) {
		    float extractionSpeed = MAXIMUMHARVESTEREXTRACTSPEED;

		    int scale = (int) (5*getHealth()/getMaxHealth());
		    if(scale == 0) {
                scale = 1;
		    }

		    extractionSpeed = (extractionSpeed * scale) / 5;

		    if (owner->getStoredCredits() < owner->getCapacity()) {
		    	owner->addCredits(pHarvester->extractSpice(extractionSpeed), true);
		    } else {
		    	// refuse cargo, we are full !
		    	deployHarvester();
		    }
		} else if(pHarvester->isAwaitingPickup() == false) {
		    // find carryall TODO groundunit->requestCarryall instead
		    Carryall* pCarryall = NULL;
		    float distance = std::numeric_limits<float>::infinity();
            if((pHarvester->getGuardPoint().isValid()) && getOwner()->hasCarryalls())	{
                RobustList<UnitBase*>::const_iterator iter;
                for(iter = unitList.begin(); iter != unitList.end(); ++iter) {
                    UnitBase* unit = *iter;
                    if ((unit->getOwner() == owner) && (unit->getItemID() == Unit_Carryall) && !((Carryall*)unit)->isBooked() && ((Carryall*)unit)->getAttackMode() != STOP ) {

                    	if (distance >  std::min(distance,blockDistance(this->location, unit->getLocation())) ) {
                    		distance = std::min(distance,blockDistance(this->location, unit->getLocation()));
                    		pCarryall = (Carryall*)unit;
                    	}
                    }
                }
            }

            if(pCarryall != NULL) {
                pCarryall->setFellow(this);
                pCarryall->clearPath();
                pHarvester->bookCarrier(pCarryall);
                pHarvester->setFellow(NULL);
                pHarvester->setDestination(pHarvester->getGuardPoint());
                soundPlayer->playSoundAt(Sound_Steam,this->location);
            } else {
            	soundPlayer->playSoundAt(Sound_Steam,this->location);
                deployHarvester();
            }
		} else if(!pHarvester->hasBookedCarrier()) {
			soundPlayer->playSoundAt(Sound_Steam,this->location);
		    deployHarvester();
		}
	}
}
