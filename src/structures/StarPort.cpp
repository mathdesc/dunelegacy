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

#include <structures/StarPort.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/TextManager.h>
#include <House.h>
#include <Game.h>
#include <Choam.h>
#include <Map.h>
#include <SoundPlayer.h>

#include <players/HumanPlayer.h>

#include <units/Frigate.h>
#include <units/GroundUnit.h>
#include <units/Carryall.h>


// Starport is counting in 30s from 10 to 0
#define STARPORT_ARRIVETIME			(MILLI2CYCLES(30*1000))

#define STARPORT_NO_ARRIVAL_AWAITED -1



StarPort::StarPort(House* newOwner) : BuilderBase(newOwner) {
    StarPort::init();

    setHealth(getMaxHealth());

    arrivalTimer = STARPORT_NO_ARRIVAL_AWAITED;
    deploying = false;
}

StarPort::StarPort(InputStream& stream) : BuilderBase(stream) {
    StarPort::init();

    arrivalTimer = stream.readSint32();
    if(stream.readBool() == true) {
        startDeploying();
    } else {
        deploying = false;
    }

}
void StarPort::init() {
	itemID = Structure_StarPort;
	owner->incrementStructures(itemID);

	structureSize.x = 3;
	structureSize.y = 3;

	graphicID = ObjPic_Starport;
	graphic = pGFXManager->getObjPic(graphicID,getOwner()->getHouseID());
	numImagesX = 10;
	numImagesY = 1;
	firstAnimFrame = 2;
	lastAnimFrame = 3;
	unitproducer = true;
}

StarPort::~StarPort() {
	/*if (deploying)
		arrivedUnit.getUnitPointer()->destroy();*/
}

void StarPort::save(OutputStream& stream) const {
	BuilderBase::save(stream);
	stream.writeSint32(arrivalTimer);
	stream.writeBool(deploying);

}

void StarPort::doBuildRandom() {
	int Item2Produce = ItemID_Invalid;

	do {
		int randNum = currentGame->randomGen.rand(0, getBuildListSize()-1);
		int i = 0;
		std::list<BuildItem>::iterator iter;
		for(iter = buildList.begin(); iter != buildList.end(); ++iter,i++) {
			if(i == randNum) {
				Item2Produce = iter->itemID;
				break;
			}
		}
	} while((Item2Produce == Unit_Harvester) || (Item2Produce == Unit_MCV) || (Item2Produce == Unit_Carryall));

	doProduceItem(Item2Produce);
}

void StarPort::handleProduceItemClick(Uint32 itemID, bool multipleMode) {
    Choam& choam = owner->getChoam();
    int numAvailable = choam.getNumAvailable(itemID);

    if(numAvailable <= 0) {
        soundPlayer->playSound(InvalidAction);
        currentGame->addToNewsTicker(_("This unit is sold out"));
        return;
    }

    std::list<BuildItem>::iterator iter;
	for(iter = buildList.begin(); iter != buildList.end(); ++iter) {
		if(iter->itemID == itemID) {
		    if((owner->getCredits() < (int) iter->price)) {
		        soundPlayer->playSound(InvalidAction);
                currentGame->addToNewsTicker(_("Not enough money"));
                return;
		    }
		}
	}

	BuilderBase::handleProduceItemClick(itemID, multipleMode);
}

void StarPort::handlePlaceOrderClick() {
	currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_STARPORT_PLACEORDER, objectID));
}

void StarPort::handleCancelOrderClick() {
	currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_STARPORT_CANCELORDER, objectID));
}

void StarPort::doProduceItem(Uint32 itemID, bool multipleMode) {
    Choam& choam = owner->getChoam();

	std::list<BuildItem>::iterator iter;
	for(iter = buildList.begin(); iter != buildList.end(); ++iter) {
		if(iter->itemID == itemID) {
			for(int i = 0; i < (multipleMode ? 5 : 1); i++) {
			    int numAvailable = choam.getNumAvailable(itemID);

			    if(numAvailable <= 0) {
                    break;
			    }

				if((owner->getCredits() >= (int) iter->price)) {
                    iter->num++;
					currentProductionQueue.push_back( ProductionQueueItem(itemID,iter->price) );
					owner->takeCredits(iter->price);

				    if(choam.setNumAvailable(itemID, numAvailable - 1) == false) {
                        // sold out
                        break;
					}
				}
			}
			break;
		}
	}
}

void StarPort::doCancelItem(Uint32 itemID, bool multipleMode) {
    Choam& choam = owner->getChoam();

	std::list<BuildItem>::iterator iter;
	for(iter = buildList.begin(); iter != buildList.end(); ++iter) {
		if(iter->itemID == itemID) {
			for(int i = 0; i < (multipleMode ? 5 : 1); i++) {
				if(iter->num > 0) {
					iter->num--;
					choam.setNumAvailable(iter->itemID, choam.getNumAvailable(iter->itemID) + 1);

                    // find the most expensive item to cancel
                    std::list<ProductionQueueItem>::iterator iterMostExpensiveItem = currentProductionQueue.end();
					std::list<ProductionQueueItem>::iterator iter2;
					for(iter2 = currentProductionQueue.begin(); iter2 != currentProductionQueue.end(); ++iter2) {
						if(iter2->itemID == itemID) {

						    // have we found a better item to cancel?
						    if(iterMostExpensiveItem == currentProductionQueue.end() || iter2->price > iterMostExpensiveItem->price) {
                                iterMostExpensiveItem = iter2;
						    }
						}
					}

					// Cancel the best found item if any was found
					if(iterMostExpensiveItem != currentProductionQueue.end()) {
                        owner->returnCredits(iterMostExpensiveItem->price);
                        currentProductionQueue.erase(iterMostExpensiveItem);
					}
				}
			}
			break;
		}
	}
}

void StarPort::doPlaceOrder() {

	if (currentProductionQueue.size() > 0) {

        if(currentGame->getGameInitSettings().getGameOptions().instantBuild == true) {
            arrivalTimer = 1;
        } else {
            arrivalTimer = STARPORT_ARRIVETIME;
        }

		firstAnimFrame = 2;
		lastAnimFrame = 7;
	}
}

void StarPort::doCancelOrder() {
    if (arrivalTimer == STARPORT_NO_ARRIVAL_AWAITED) {
        while(currentProductionQueue.empty() == false) {
            doCancelItem(currentProductionQueue.back().itemID, false);
        }

		currentProducedItem = ItemID_Invalid;
	}
}


void StarPort::updateBuildList() {
	std::list<BuildItem>::iterator iter = buildList.begin();

    Choam& choam = owner->getChoam();

    for(int i = 0; itemOrder[i] != ItemID_Invalid; ++i) {
        if(choam.getNumAvailable(itemOrder[i]) != INVALID) {
            insertItem(buildList, iter, itemOrder[i], choam.getPrice(itemOrder[i]));
        } else {
            removeItem(buildList, iter, itemOrder[i]);
        }
    }
}

bool StarPort::deployOrderedUnit(Carryall* pCarryall) {

    UnitBase* pUnit = arrivedUnit.getUnitPointer();
	bool unitDeployed = false;

    if (!arrivedUnit && (arrivedUnit.getObjPointer() == NULL)) {
    	pCarryall->setTarget(NULL);
    	//pCarryall->setDestination(location);
    	return unitDeployed ;
    }
    Voice_enum voice = NUM_VOICE;

	if (pUnit->isAFlyingUnit()) {
		voice = UnitLaunched;
	} else if (pUnit->isInfantry()) {
		voice = UnitDeployed;
	} else if (pUnit->isAUnit() && pUnit->getItemID() != Unit_Harvester) {
		voice = VehiculeDeployed;
	} else if (pUnit->isAUnit() && pUnit->getItemID() == Unit_Harvester) {
		voice = HarvesterDeployed;
	}

    dbg_print(" StarPort::deployOrderedUnit(carry:%s) \n", (pCarryall != NULL) ? "yes" : "no");
	if (pCarryall != NULL) {
			// Check if the that carryall was attended by the arrived unit
			if (pUnit != NULL && ((GroundUnit*)(pUnit))->getCarrier()->getObjectID() == pCarryall->getObjectID()) {
				pCarryall->giveCargo(pUnit);
				pCarryall->setTarget(NULL);
				pUnit->setTarget(NULL);
				if (destination.isValid()) {
						pCarryall->setDestination(pUnit->getDestination());
						pCarryall->setDeployPos(pUnit->getDestination());
						pCarryall->setFallbackPos(currentGameMap->findDeploySpot(pUnit, location, pUnit->getDestination(), structureSize));
						dbg_print(" StarPort::deployOrderedUnit carryall destination(%d,%d) \n", pCarryall->getDestination().x,pCarryall->getDestination().y);

				}
				else {
					Coord dp = currentGameMap->findDeploySpot(pUnit, location, location, structureSize);
					pCarryall->setDestination(dp);
					pCarryall->setDeployPos(dp);
					pCarryall->setFallbackPos(dp+structureSize);
					dbg_print(" StarPort::deployOrderedUnit@Home carryall destination(%d,%d) \n", pCarryall->getDestination().x,pCarryall->getDestination().y);
				}


				arrivedUnit.pointTo(NONE);
				unitDeployed = true;

				if(pUnit->getOwner() == pLocalHouse && voice < NUM_VOICE) {
						soundPlayer->playVoiceAt(voice,pUnit->getOwner()->getHouseID(),pCarryall->getDestination());
				}
			}

	} else {
		if (pUnit != NULL) {
			Coord spot = pUnit->isAFlyingUnit() ? location + Coord(1,1) : currentGameMap->findDeploySpot(pUnit, location, destination, structureSize);
			pUnit->deploy(spot,true);
			pUnit->setTarget(NULL);
			arrivedUnit.pointTo(NONE);
			unitDeployed = true;
		}
	}

    if (unitDeployed)
    	updateStructureProductionQueue();

    return unitDeployed;

}

void StarPort::updateStructureProductionQueue() {

    std::list<BuildItem>::iterator iter2;
    for(iter2 = buildList.begin(); iter2 != buildList.end(); ++iter2) {
        if(iter2->itemID == currentProductionQueue.front().itemID) {
            iter2->num--;
            break;
        }
    }

    currentProductionQueue.pop_front();

    if(currentProductionQueue.empty() == true) {
        arrivalTimer = STARPORT_NO_ARRIVAL_AWAITED;
        deploying = false;
        // Remove box from starport
        firstAnimFrame = 2;
        lastAnimFrame = 3;
    } else {
        deployTimer = MILLI2CYCLES(2000);
    }
}


void StarPort::updateStructureSpecificStuff() {
    updateBuildList();

	if (arrivalTimer > 0) {
		if (--arrivalTimer == 0) {

			Frigate*		frigate;
			Coord		pos;
			countdown = 10;

			//make a frigate with all the cargo
			frigate = (Frigate*)owner->createUnit(Unit_Frigate);
			pos = currentGameMap->findClosestEdgePoint(getLocation() + Coord(1,1), Coord(1,1));
			frigate->deploy(pos);
			frigate->setTarget(this);
			Coord closestPoint = getClosestPoint(frigate->getLocation());
			frigate->setDestination(closestPoint);

			if (pos.x == 0)
				frigate->setAngle(RIGHT);
			else if (pos.x == currentGameMap->getSizeX()-1)
				frigate->setAngle(LEFT);
			else if (pos.y == 0)
				frigate->setAngle(DOWN);
			else if (pos.y == currentGameMap->getSizeY()-1)
				frigate->setAngle(UP);

            deployTimer = MILLI2CYCLES(2000);

			currentProducedItem = ItemID_Invalid;

			if(getOwner() == pLocalHouse) {
				soundPlayer->playVoice(FrigateHasArrived,getOwner()->getHouseID());
				currentGame->addToNewsTicker(_("@DUNE.ENG|80#Frigate has arrived"));
			}

		} else {
            if(getOwner() == pLocalHouse) {
				if ( ((arrivalTimer*10)/(MILLI2CYCLES(30*1000))) + 1 != countdown) {
					countdown = ((arrivalTimer*10)/(MILLI2CYCLES(30*1000))) + 1;
					if (countdown == 5 ) {
						soundPlayer->playVoiceAt(Five,getOwner()->getHouseID(),location);
					}
					if (countdown == 4 ) {
						soundPlayer->playVoiceAt(Four,getOwner()->getHouseID(),location);
					}
					if (countdown == 3 ) {
						soundPlayer->playVoiceAt(Three,getOwner()->getHouseID(),location);
					}
					if (countdown == 2 ) {
						soundPlayer->playVoiceAt(Two,getOwner()->getHouseID(),location);
					}
					if (countdown == 1 ) {
						soundPlayer->playVoiceAt(One,getOwner()->getHouseID(),location);
					}
				}
            }
		}
	} else if(deploying == true) {
		bool unitDeployed = false;

        deployTimer--;
        if(deployTimer == 0) {


            if(currentProductionQueue.empty() == false) {
                Uint32 newUnitItemID = currentProductionQueue.front().itemID;

                int num2Place = 1;

                if(newUnitItemID == Unit_Infantry) {
                    // make three
                    newUnitItemID = Unit_Soldier;
                    num2Place = 3;
                } else if(newUnitItemID == Unit_Troopers) {
                    // make three
                    newUnitItemID = Unit_Trooper;
                    num2Place = 3;
                }

                for(int i = 0; i < num2Place; i++) {

                	if (arrivedUnit.getObjPointer()  == NULL) {
						UnitBase* newUnit = getOwner()->createUnit(newUnitItemID);


						if (newUnit != NULL) {
							arrivedUnit = newUnit;

							deploySpot = newUnit->isAFlyingUnit() ? location + Coord(1,1) : currentGameMap->findDeploySpot(newUnit, location, destination.isValid() ? destination : location , structureSize);

							// Deploy itself
							if (!getOwner()->hasCarryalls() || newUnit->isAFlyingUnit() || newUnit->getItemID() == Unit_MCV ) {
								newUnit->deploy(deploySpot,true);
								arrivedUnit.pointTo(NONE);
								unitDeployed = true;
							}
							// Assisted deployment
							else if ( newUnit->isAGroundUnit() && getOwner()->hasCarryalls()) {


								GroundUnit* gUnit = static_cast<GroundUnit*>(newUnit);

								// It's not going to be pickup
								if (!gUnit->isAwaitingPickup()) {
								// find carryall
								Carryall* pCarryall = NULL;
								float distance = std::numeric_limits<float>::infinity();
								   RobustList<UnitBase*>::const_iterator iter;
								   for(iter = unitList.begin(); iter != unitList.end(); ++iter) {
									   UnitBase* unit = *iter;
									   if ((unit->getOwner() == owner) && (unit->getItemID() == Unit_Carryall) && !((Carryall*)unit)->isBooked()) {

					                    	if (distance >  std::min(distance,blockDistance(this->location, unit->getLocation())) ) {
					                    		distance = std::min(distance,blockDistance(this->location, unit->getLocation()));
					                    		pCarryall = (Carryall*)unit;
					                    	}
									   }
								   }


								   // tell carryall to come here, tell unit to book that carryall
								   if(pCarryall != NULL) {
									   pCarryall->setTarget(this);
									  // pCarryall->setDestination(location+Coord(-1,1));
									   pCarryall->clearPath();
									   gUnit->bookCarrier(pCarryall);
									   gUnit->setTarget(NULL);
									   // Rally point is set
									   if (destination.isValid()) {
										   gUnit->setGuardPoint(destination);
										   gUnit->setDestination(destination);
										   dbg_print(" StarPort::updateStructureSpecificStuff UnitAwaitingDeploy destination(%d,%d) rallypoint(%d,%d)\n", gUnit->getDestination().x,gUnit->getDestination().y,destination.x,destination.y);
										   gUnit->setAngle(lround(8.0f/256.0f*destinationAngle(gUnit->getLocation(), gUnit->getDestination())));
									   } else {
										   // Rally point is not set deploy locally
										   gUnit->setGuardPoint(deploySpot);
										   gUnit->setDestination(deploySpot);
										   gUnit->setAngle(lround(8.0f/256.0f*destinationAngle(gUnit->getLocation(), gUnit->getDestination())));
									   }

								   } else {
									   // No Carrier available, Deploy itself
									   gUnit->deploy(deploySpot,true);
									   arrivedUnit.pointTo(NONE);
									   unitDeployed = true;
								   }
								}/* else if(gUnit->hasBookedCarrier()) {
									// Waiting for pickup and a carrier has been Booked
									// deploy in the surroundings, to speed up deployment
									gUnit->deploy(location);
									//(gUnit->getCarrier())->setDestination(location);
									arrivedUnit.pointTo(NONE);
									if(getOwner() == pLocalHouse && voice != NULL) {
											soundPlayer->playVoiceAt(voice,getOwner()->getHouseID(),location);
									}
									unitDeployed = false;
								}*/
							} else {
								// Unit is not able to be carried
								// Or we simply don't have any carryall , self deploy
								newUnit->deploy(deploySpot,true);
								arrivedUnit.pointTo(NONE);
								unitDeployed = true;
							}

							if (getOwner()->isAI()
								&& (newUnit->getItemID() != Unit_Carryall)
								&& (newUnit->getItemID() != Unit_Harvester)
								&& (newUnit->getItemID() != Unit_MCV)) {
								newUnit->doSetAttackMode(AREAGUARD);
							}

							if (unitDeployed) {


									newUnit->setGuardPoint(deploySpot);
									newUnit->setDestination(destination.isValid() ? destination : deploySpot);

								dbg_print(" StarPort::updateStructureSpecificStuff Unit destination(%d,%d) \n", newUnit->getDestination().x,newUnit->getDestination().y);
								newUnit->setAngle(lround(8.0f/256.0f*destinationAngle(newUnit->getLocation(), newUnit->getDestination())));
							}

							// inform owner of its new unit
							newUnit->getOwner()->informWasBuilt(newUnitItemID);

						}

                	}
                }

                if (unitDeployed)
                	updateStructureProductionQueue();

            }
        }
	}



}

void StarPort::informFrigateDestroyed() {
    currentProductionQueue.clear();
    arrivalTimer = STARPORT_NO_ARRIVAL_AWAITED;
    deployTimer = 0;
    deploying = false;
    // stop blinking
    firstAnimFrame = 2;
    lastAnimFrame = 3;
}
