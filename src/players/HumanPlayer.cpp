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

#include <players/HumanPlayer.h>

#include <Game.h>

#include <globals.h>

#include <Network/NetworkManager.h>

#include <Game.h>
#include <GameInitSettings.h>
#include <Map.h>
#include <sand.h>
#include <House.h>

#include <structures/StructureBase.h>
#include <structures/BuilderBase.h>
#include <structures/StarPort.h>
#include <structures/ConstructionYard.h>
#include <units/UnitBase.h>
#include <units/MCV.h>
#include <units/Harvester.h>

#include <algorithm>


#define AIUPDATEINTERVAL 100

HumanPlayer::HumanPlayer(House* associatedHouse, std::string playername) : Player(associatedHouse, playername) {
    HumanPlayer::init();
}

HumanPlayer::HumanPlayer(InputStream& stream, House* associatedHouse) : Player(stream, associatedHouse) {
    HumanPlayer::init();

    for(int i=0;i < NUMSELECTEDLISTS; i++) {
        selectedLists[i] = stream.readUint32List();
    }
}

void HumanPlayer::init() {
    nextExpectedCommandsCycle = 0;
}

HumanPlayer::~HumanPlayer() {
}

void HumanPlayer::save(OutputStream& stream) const {
    Player::save(stream);

    // write out selection groups (Key 1 to 9)
    for(int i=0; i < NUMSELECTEDLISTS; i++) {
        stream.writeUint32List(selectedLists[i]);
    }
}

void HumanPlayer::update() {
	 if( (getGameCylceCount() + getHouse()->getHouseID()) % AIUPDATEINTERVAL != 0) {
	        // we are not updating this player helper AI this cycle
	        return;
	    }
	checkAllUnits();
	build();

}

void onDamage(const ObjectBase* pObject, int damage, Uint32 damagerID) {

}


void HumanPlayer::build() {
	bool bConstructionYardChecked = false;

    RobustList<const StructureBase*>::const_iterator iter;

    for(iter = getStructureList().begin(); iter != getStructureList().end(); ++iter) {
        const StructureBase* pStructure = *iter;

        ///TODO make this an option
        if(pStructure->getOwner() == getHouse()) {
            if((pStructure->isRepairing() == false) && (pStructure->getHealth() < pStructure->getMaxHealth()) && (getHouse()->getCredits() > 500)) {
                doRepair(pStructure);
                currentGame->addToNewsTicker("Auto-repairing structure");
            }
        }

        Uint32 itemID = NONE;
        const BuilderBase* pBuilder = dynamic_cast<const BuilderBase*>(pStructure);
		if(pBuilder != NULL && pBuilder->getOwner() == getHouse()) {
			 if((pBuilder->getProductionQueueSize() < 1) && (pBuilder->getBuildListSize() > 0)) {
				 switch (pStructure->getItemID()) {
					 case Structure_ConstructionYard: {
						 if(bConstructionYardChecked == false && !pBuilder->isUpgrading()) {
							 if(getHouse()->getNumStructures() > 3 && ((getHouse()->getProducedPower() - getHouse()->getPowerRequirement())*100) / (getHouse()->getPowerRequirement() +1) < 10 && pBuilder->isAvailableToBuild(Structure_WindTrap) ) {
							      itemID = Structure_WindTrap;
							      err_print("build windtrap\n");
							 }
							 else if(getHouse()->getCredits() > 2000 && ((getHouse()->getStoredCredits()+100 > getHouse()->getCapacity()) ) && pBuilder->isAvailableToBuild(Structure_Silo)) {
								  itemID = Structure_Silo;
								  err_print("build silo\n");
							 }
						 }

					 } break;
				 }
			 }
			 if(pBuilder->isWaitingToPlaceAI()) {
			 							//find total region of possible placement and place in random ok position
			 							int itemID = pBuilder->getCurrentProducedItem();
			 							Coord itemsize = getStructureSize(itemID);

			 							//see if there is already a spot to put it stored
			 							if(!placeLocations.empty()) {
			 								Coord location = placeLocations.front();
			 								const ConstructionYard* pConstYard = dynamic_cast<const ConstructionYard*>(pBuilder);
			 								if(getMap().okayToPlaceStructure(location.x, location.y, itemsize.x, itemsize.y, false, pConstYard->getOwner())) {
			 									err_print("build - place structure\n");
			 									doPlaceStructure(pConstYard, location.x, location.y);
			 									placeLocations.pop_front();
			 								} else if(itemID == Structure_Slab1) {
			 									//forget about concrete
			 									doCancelItem(pConstYard, Structure_Slab1);
			 									placeLocations.pop_front();
			 								} else if(itemID == Structure_Slab4) {
			 									//forget about concrete
			 									doCancelItem(pConstYard, Structure_Slab4);
			 									placeLocations.pop_front();
			 								} else {
			 									//cancel item
			 									doCancelItem(pConstYard, itemID);
			 									placeLocations.pop_front();
			 								}
			 							} else err_print("no place locations ! \n");
			 						}
		}
	   if(itemID != NONE) {
		err_print("build - place location\n");
		Coord location = findPlaceLocation(itemID);

		if(location.isValid()) {
			Coord placeLocation = location;
			if(getGameInitSettings().getGameOptions().concreteRequired) {
				int incI;
				int incJ;
				int startI;
				int startJ;

				if(getMap().isWithinBuildRange(location.x, location.y, getHouse())) {
					startI = location.x, startJ = location.y, incI = 1, incJ = 1;
				} else if(getMap().isWithinBuildRange(location.x + getStructureSize(itemID).x - 1, location.y, getHouse())) {
					startI = location.x + getStructureSize(itemID).x - 1, startJ = location.y, incI = -1, incJ = 1;
				} else if(getMap().isWithinBuildRange(location.x, location.y + getStructureSize(itemID).y - 1, getHouse())) {
					startI = location.x, startJ = location.y + getStructureSize(itemID).y - 1, incI = 1, incJ = -1;
				} else {
					startI = location.x + getStructureSize(itemID).x - 1, startJ = location.y + getStructureSize(itemID).y - 1, incI = -1, incJ = -1;
				}

				for(int i = startI; abs(i - startI) < getStructureSize(itemID).x; i += incI) {
					for(int j = startJ; abs(j - startJ) < getStructureSize(itemID).y; j += incJ) {
						const Tile *pTile = getMap().getTile(i, j);

						if((getStructureSize(itemID).x > 1) && (getStructureSize(itemID).y > 1)
							&& pBuilder->isAvailableToBuild(Structure_Slab4)
							&& (abs(i - location.x) < 2) && (abs(j - location.y) < 2)) {
							if( (i == location.x) && (j == location.y) && pTile->getType() != Terrain_Slab) {
								placeLocations.push_back(Coord(i,j));
								err_print("build - produce slab1 \n");
								doProduceItemAI(pBuilder, Structure_Slab4);
							}
						} else if(pTile->getType() != Terrain_Slab) {
							placeLocations.push_back(Coord(i,j));
							err_print("build - produce slab1 \n");
							doProduceItemAI(pBuilder, Structure_Slab1);
						}
					}
				}
			}
			err_print("build - produce silo \n");
			placeLocations.push_back(placeLocation);
			doProduceItemAI(pBuilder, itemID);
		} else {
			err_print("build - can't find location  \n");
			// we havn't found a placing location => build some random slabs
			location = findPlaceLocation(Structure_Slab1);
			if(location.isValid() && getMap().isWithinBuildRange(location.x, location.y, getHouse())) {
				placeLocations.push_back(location);
				doProduceItemAI(pBuilder, Structure_Slab1);
			}
		}
	   }




    }
}

void HumanPlayer::checkAllUnits() {
    RobustList<const UnitBase*>::const_iterator iter;
    for(iter = getUnitList().begin(); iter != getUnitList().end(); ++iter) {
        const UnitBase* pUnit = *iter;

		/* Return harvester and detach a small force to protect */
        if(pUnit->getItemID() == Unit_Sandworm && pUnit->isActive()) {
                RobustList<const UnitBase*>::const_iterator iter2;
                for(iter2 = getUnitList().begin(); iter2 != getUnitList().end(); ++iter2) {
                    const UnitBase* pUnit2 = *iter2;

                    if(pUnit2->getOwner() == getHouse() && pUnit2->getItemID() == Unit_Harvester) {
                        const Harvester* pHarvester = dynamic_cast<const Harvester*>(pUnit2);
                        if( pHarvester != NULL
                            && getMap().tileExists(pHarvester->getLocation())
                            && !getMap().getTile(pHarvester->getLocation())->isRock()
                            && blockDistance(pUnit->getLocation(), pHarvester->getLocation()) <= 10) {
                            doReturn(pHarvester);
                           // scrambleUnitsAndDefend(pUnit,5);
                        }
                    }
                }
            continue;
        }
    }
}

int HumanPlayer::getNumAdjacentStructureTiles(Coord pos, int structureSizeX, int structureSizeY) {

    int numAdjacentStructureTiles = 0;

    for(int y = pos.y; y < pos.y + structureSizeY; y++) {
        if(getMap().tileExists(pos.x-1, y) && getMap().getTile(pos.x-1, y)->hasAStructure()) {
            numAdjacentStructureTiles++;
        }
        if(getMap().tileExists(pos.x+structureSizeX, y) && getMap().getTile(pos.x+structureSizeX, y)->hasAStructure()) {
            numAdjacentStructureTiles++;
        }
    }

    for(int x = pos.x; x < pos.x + structureSizeX; x++) {
        if(getMap().tileExists(x, pos.y-1) && getMap().getTile(x, pos.y-1)->hasAStructure()) {
            numAdjacentStructureTiles++;
        }
        if(getMap().tileExists(x, pos.y+structureSizeY) && getMap().getTile(x, pos.y+structureSizeY)->hasAStructure()) {
            numAdjacentStructureTiles++;
        }
    }

    return numAdjacentStructureTiles;
}

Coord HumanPlayer::findPlaceLocation(Uint32 itemID) {
    int structureSizeX = getStructureSize(itemID).x;
    int structureSizeY = getStructureSize(itemID).y;

	int minX = getMap().getSizeX();
	int maxX = -1;
    int minY = getMap().getSizeY();
    int maxY = -1;

    if(itemID == Structure_ConstructionYard || itemID == Structure_Slab1) {
        // construction yard can only be build with mcv and thus be build anywhere
        minX = 0;
        minY = 0;
        maxX = getMap().getSizeX() - 1;
        maxY = getMap().getSizeY() - 1;
    } else {
        RobustList<const StructureBase*>::const_iterator iter;
        for(iter = getStructureList().begin(); iter != getStructureList().end(); ++iter) {
            const StructureBase* structure = *iter;
            if (structure->getOwner() == getHouse()) {
                if (structure->getX() < minX)
                    minX = structure->getX();
                if (structure->getX() > maxX)
                    maxX = structure->getX();
                if (structure->getY() < minY)
                    minY = structure->getY();
                if (structure->getY() > maxY)
                    maxY = structure->getY();
            }
        }
	}

    // make search rect a bit bigger to make it possible to build on places far off the main base and only connected through slab
	minX -= structureSizeX + 5;
	maxX += 5;
	minY -= structureSizeY + 5;
	maxY += 5;
	if (minX < 0) minX = 0;
	if (maxX >= getMap().getSizeX()) maxX = getMap().getSizeX() - structureSizeX;
	if (minY < 0) minY = 0;
	if (maxY >= getMap().getSizeY()) maxY = getMap().getSizeY() - structureSizeY;

    float bestrating = 0.0f;
	Coord bestLocation = Coord::Invalid();
	int count = 0;
	do {
	    int x = getRandomGen().rand(minX, maxX);
        int y = getRandomGen().rand(minY, maxY);

	    Coord pos = Coord(x, y);

		count++;

		if(getMap().okayToPlaceStructure(pos.x, pos.y, structureSizeX, structureSizeY, false, (itemID == Structure_ConstructionYard) ? NULL : getHouse())) {
            float rating;

		    switch(itemID) {
                case Structure_Slab1: {
                    rating = 10000000.0f;
                } break;

                case Structure_Refinery: {
                    // place near spice
                    Coord spicePos;
                    if(getMap().findSpice(spicePos, pos)) {
                        rating = 10000000.0f - blockDistance(pos, spicePos);
                    } else {
                        rating = 10000000.0f;
                    }
                } break;


                case Structure_ConstructionYard: {
                    float nearestUnit = 10000000.0f;

                    RobustList<const UnitBase*>::const_iterator iter;
                    for(iter = getUnitList().begin(); iter != getUnitList().end(); ++iter) {
                        const UnitBase* pUnit = *iter;
                        if(pUnit->getOwner() == getHouse()) {
                            float tmp = blockDistance(pos, pUnit->getLocation());
                            if(tmp < nearestUnit) {
                                nearestUnit = tmp;
                            }
                        }
                    }

                    rating = 10000000.0f - nearestUnit;
                } break;

                case Structure_Barracks:
                case Structure_HeavyFactory:
                case Structure_LightFactory:
                case Structure_RepairYard:
                case Structure_StarPort:
                case Structure_WOR: {
                    // place near sand

                    float nearestSand = 10000000.0f;

                    for(int y = 0 ; y < currentGameMap->getSizeY(); y++) {
                        for(int x = 0; x < currentGameMap->getSizeX(); x++) {
                            int type = currentGameMap->getTile(x,y)->getType();

                            if(type != Terrain_Rock || type != Terrain_Slab || type != Terrain_Mountain) {
                                float tmp = blockDistance(pos, Coord(x,y));
                                if(tmp < nearestSand) {
                                    nearestSand = tmp;
                                }
                            }
                        }
                    }

                    rating = 10000000.0f - nearestSand;
                    rating *= (1+getNumAdjacentStructureTiles(pos, structureSizeX, structureSizeY));
                } break;

                case Structure_Wall:
                case Structure_GunTurret:
                case Structure_RocketTurret: {
                    // place towards enemy
                    float nearestEnemy = 10000000.0f;

                    RobustList<const StructureBase*>::const_iterator iter2;
                    for(iter2 = getStructureList().begin(); iter2 != getStructureList().end(); ++iter2) {
                        const StructureBase* pStructure = *iter2;
                        if(pStructure->getOwner()->getTeam() != getHouse()->getTeam()) {

                            float tmp = blockDistance(pos, pStructure->getLocation());
                            if(tmp < nearestEnemy) {
                                nearestEnemy = tmp;
                            }
                        }
                    }

                    rating = 10000000.0f - nearestEnemy;
                } break;

                case Structure_HighTechFactory:
                case Structure_IX:
                case Structure_Palace:
                case Structure_Radar:
                case Structure_Silo:
                case Structure_WindTrap:
                default: {
                    // place at a save place
                    float nearestEnemy = 10000000.0f;

                    RobustList<const StructureBase*>::const_iterator iter2;
                    for(iter2 = getStructureList().begin(); iter2 != getStructureList().end(); ++iter2) {
                        const StructureBase* pStructure = *iter2;
                        if(pStructure->getOwner()->getTeam() != getHouse()->getTeam()) {

                            float tmp = blockDistance(pos, pStructure->getLocation());
                            if(tmp < nearestEnemy) {
                                nearestEnemy = tmp;
                            }
                        }
                    }

                    rating = nearestEnemy;
                    rating *= (1+getNumAdjacentStructureTiles(pos, structureSizeX, structureSizeY));
                } break;
		    }

		    if(rating > bestrating) {
                bestLocation = pos;
                bestrating = rating;
		    }
		}

	} while(count <= ((itemID == Structure_ConstructionYard) ? 10000 : 100));

	return bestLocation;
}


void HumanPlayer::setGroupList(int groupListIndex, const std::list<Uint32>& newGroupList) {
    selectedLists[groupListIndex].clear();

    std::list<Uint32>::const_iterator iter;
    for(iter = newGroupList.begin(); iter != newGroupList.end(); ++iter) {
        if(currentGame->getObjectManager().getObject(*iter) != NULL) {
            selectedLists[groupListIndex].push_back(*iter);
        }
    }

    if((pNetworkManager != NULL) && (pLocalPlayer == this)) {
        // the local player has changed his group assignment
        pNetworkManager->sendSelectedList(selectedLists[groupListIndex], groupListIndex);
    }
}
