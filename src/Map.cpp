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

#include <Map.h>

#include <globals.h>

#include <Game.h>
#include <House.h>
#include <ScreenBorder.h>
#include <ConcatIterator.h>
#include <sand.h>

#include <units/UnitBase.h>
#include <units/InfantryBase.h>
#include <units/AirUnit.h>
#include <structures/StructureBase.h>

#include <limits.h>
#include <stack>
#include <set>

#include <AStarSearch.h>
#include <misc/strictmath.h>

Map::Map(int xSize, int ySize)
 : sizeX(xSize), sizeY(ySize), tiles(NULL), lastSinglySelectedObject(NULL) {

	tiles = new Tile[sizeX*sizeY];

	for(int i=0; i<sizeX; i++) {
		for(int j=0; j<sizeY; j++) {
			tiles[i+j*sizeX].location.x = i;
			tiles[i+j*sizeX].location.y = j;
		}
	}
}


Map::~Map() {
	delete[] tiles;
}

void Map::load(InputStream& stream) {
	sizeX = stream.readSint32();
	sizeY = stream.readSint32();

	for (int i = 0; i < sizeX; i++) {
		for (int j = 0; j < sizeY; j++) {
			getTile(i,j)->load(stream);
			getTile(i,j)->location.x = i;
			getTile(i,j)->location.y = j;
		}
	}
}

void Map::save(OutputStream& stream) const {
	stream.writeSint32(sizeX);
	stream.writeSint32(sizeY);

	for (int i = 0; i < sizeX; i++) {
		for (int j = 0; j < sizeY; j++) {
			getTile(i,j)->save(stream);
		}
	}
}

void Map::createSandRegions() {
	std::stack<Tile*> tileQueue;
	std::vector<bool> visited(sizeX * sizeY);

	for(int i = 0; i < sizeX; i++) {
		for(int j = 0; j < sizeY; j++)	{
			getTile(i,j)->setSandRegion(NONE);
		}
	}

    int	region = 0;
	for(int i = 0; i < sizeX; i++) {
		for(int j = 0; j < sizeY; j++) {
			if(!getTile(i,j)->isRock() && !visited[j*sizeX+i]) {
				tileQueue.push(getTile(i,j));

				while(!tileQueue.empty()) {
					Tile* pTile = tileQueue.top();
					tileQueue.pop();

					pTile->setSandRegion(region);
					for(int angle = 0; angle < NUM_ANGLES; angle++) {
						Coord pos = getMapPos(angle, pTile->location);
						if(tileExists(pos) && !getTile(pos)->isRock() && !visited[pos.y*sizeX + pos.x]) {
							tileQueue.push(getTile(pos));
							visited[pos.y*sizeX + pos.x] = true;
						}
					}
				}
				region++;
			}
		}
	}
}

void Map::damage(Uint32 damagerID, House* damagerOwner, const Coord& realPos, Uint32 bulletID, float damage, int damageRadius, bool air) {
	Coord location = Coord(realPos.x/TILESIZE, realPos.y/TILESIZE);

    std::set<Uint32>	affectedAirUnits;
    std::set<Uint32>    affectedGroundAndUndergroundUnits;

	for(int i = location.x-2; i <= location.x+2; i++) {
		for(int j = location.y-2; j <= location.y+2; j++) {
			if(tileExists(i, j)) {
			    Tile* pTile = getTile(i,j);

                affectedAirUnits.insert(pTile->getAirUnitList().begin(), pTile->getAirUnitList().end());
                affectedGroundAndUndergroundUnits.insert(pTile->getInfantryList().begin(), pTile->getInfantryList().end());
                affectedGroundAndUndergroundUnits.insert(pTile->getUndergroundUnitList().begin(), pTile->getUndergroundUnitList().end());
                affectedGroundAndUndergroundUnits.insert(pTile->getNonInfantryGroundObjectList().begin(), pTile->getNonInfantryGroundObjectList().end());
			}
		}
	}

    if(bulletID == Bullet_Sandworm) {
        std::set<Uint32>::const_iterator iter;
        for(iter = affectedGroundAndUndergroundUnits.begin(); iter != affectedGroundAndUndergroundUnits.end() ;++iter) {
            ObjectBase* pObject = currentGame->getObjectManager().getObject(*iter);
            if((pObject->getItemID() != Unit_Sandworm) && (pObject->isAGroundUnit() || pObject->isInfantry()) && (pObject->getLocation() == location)) {
                pObject->setVisible(VIS_ALL, false);
                pObject->handleDamage( lroundf(damage), damagerID, damagerOwner);
            }
        }
    } else {

        if(air == true) {
            // air damage
            if((bulletID == Bullet_DRocket) || bulletID == Bullet_GasCloud || (bulletID == Bullet_Rocket) || (bulletID == Bullet_TurretRocket)|| (bulletID == Bullet_SmallRocket) || (bulletID == Bullet_LargeRocket)) {
                std::set<Uint32>::const_iterator iter;
                for(iter = affectedAirUnits.begin(); iter != affectedAirUnits.end() ;++iter) {
                    AirUnit* pAirUnit = dynamic_cast<AirUnit*>(currentGame->getObjectManager().getObject(*iter));

                    if(pAirUnit == NULL)
                        continue;


                    Coord centerPoint = pAirUnit->getCenterPoint();
                    int distance = lroundf(distanceFrom(centerPoint, realPos));

                    if(distance <= damageRadius) {
                        if(bulletID == Bullet_DRocket || bulletID == Bullet_GasCloud) {
                            if((pAirUnit->getItemID() != Unit_Carryall) && (pAirUnit->getItemID() != Unit_Sandworm) && (pAirUnit->getItemID() != Unit_Frigate)) {
                                // try to deviate
                                if((currentGame->randomGen.randFloat() < getDeviateWeakness((HOUSETYPE) pAirUnit->getOriginalHouseID()))) {
                                    pAirUnit->deviate(damagerOwner);
                                }
                            }
                        } else {
                            int scaledDamage = lroundf(damage) / (distance/4 + 1);
                            pAirUnit->handleDamage(scaledDamage, damagerID, damagerOwner);
                        }
                    }

                }
            }
        } else {
            // non air damage
            std::set<Uint32>::const_iterator iter;
            for(iter = affectedGroundAndUndergroundUnits.begin(); iter != affectedGroundAndUndergroundUnits.end() ;++iter) {
                ObjectBase* pObject = currentGame->getObjectManager().getObject(*iter);

                if(pObject->isAStructure()) {
                    StructureBase* pStructure = dynamic_cast<StructureBase*>(pObject);

                    Coord topLeftCorner = pStructure->getLocation()*TILESIZE;
                    Coord bottomRightCorner = topLeftCorner + pStructure->getStructureSize()*TILESIZE;

                    if(realPos.x >= topLeftCorner.x && realPos.y >= topLeftCorner.y && realPos.x < bottomRightCorner.x && realPos.y < bottomRightCorner.y) {
                        pStructure->handleDamage(damage, damagerID, damagerOwner);

                        if( (bulletID == Bullet_LargeRocket || bulletID == Bullet_Rocket || bulletID == Bullet_TurretRocket || bulletID == Bullet_SmallRocket)
                            && (pStructure->getHealth() < pStructure->getMaxHealth()/2)) {
                            if(pStructure->getNumSmoke() < 5) {
                                pStructure->addSmoke(realPos, currentGame->getGameCycleCount());
                            }
                        }
                    }


                } else if(pObject->isAUnit()) {
                    UnitBase* pUnit = dynamic_cast<UnitBase*>(pObject);

                    Coord centerPoint = pUnit->getCenterPoint();
                    int distance = lroundf(distanceFrom(centerPoint, realPos));

                    if(distance <= damageRadius) {
                        if(bulletID == Bullet_DRocket || bulletID == Bullet_GasCloud) {
                            if((pUnit->getItemID() != Unit_Carryall) && (pUnit->getItemID() != Unit_Sandworm) && (pUnit->getItemID() != Unit_Frigate)) {
                            	// This Bullet hit and is destroyed
                            	if (bulletID == Bullet_DRocket) {
                            		// try to deviate
                            		if((currentGame->randomGen.randFloat() < getDeviateWeakness((HOUSETYPE) pUnit->getOriginalHouseID()) )) {
                            			pUnit->deviate(damagerOwner);
                            		}
                            	}
                            	// This Bullet is a cloud it deals damage for a while
                            	if (bulletID == Bullet_GasCloud) {
									// try to deviate
									if((currentGame->randomGen.randFloat() < getDeviateWeakness((HOUSETYPE) pUnit->getOriginalHouseID()) )) {
										pUnit->deviate(damagerOwner);
									}
                            	}
                            }

                        } else if(bulletID == Bullet_Sonic) {
                        	if ((pUnit->getItemID() != Unit_SonicTank  || pUnit->getOwner()->getTeam() != pLocalHouse->getTeam()))
                        		pUnit->handleDamage(lroundf(damage), damagerID, damagerOwner);
                        } else {
                            int scaledDamage = lroundf(damage) / (distance/16 + 1);
                            pUnit->handleDamage(scaledDamage, damagerID, damagerOwner);
                        }
                    }
                }
            }


            if(currentGameMap->tileExists(location)) {

                Tile* pTile = currentGameMap->getTile(location);

                if(((bulletID == Bullet_Rocket) || (bulletID == Bullet_TurretRocket) || (bulletID == Bullet_SmallRocket) || (bulletID == Bullet_LargeRocket))
                    && (!pTile->hasAGroundObject() || !pTile->getGroundObject()->isAStructure()) )
                {

                    if(((pTile->getType() == Terrain_Rock) && (pTile->getTerrainTile() == Tile::TerrainTile_RockFull)) || (pTile->getType() == Terrain_Slab)) {
                        if(pTile->getType() == Terrain_Slab) {
                            pTile->setType(Terrain_Rock);
                            pTile->setDestroyedStructureTile(Destroyed1x1Structure);
                            pTile->setOwner(NONE);
                        }

                        pTile->addDamage(Tile::Terrain_RockDamage, (bulletID==Bullet_SmallRocket) ? Tile::RockDamage1 : Tile::RockDamage2, realPos);

                    } else if((pTile->getType() == Terrain_Sand) || (pTile->getType() == Terrain_Spice) || (pTile->getType() == Terrain_ThickSpice) ) {
                        if(bulletID==Bullet_SmallRocket) {
                            pTile->addDamage(Tile::Terrain_SandDamage, currentGame->randomGen.rand(Tile::SandDamage1, Tile::SandDamage2), realPos);
                        } else {
                            pTile->addDamage(Tile::Terrain_SandDamage, currentGame->randomGen.rand(Tile::SandDamage3, Tile::SandDamage4), realPos);
                        }
                    }
                }
                bool terrain_mask = (pTile->getType() == Terrain_Spice || pTile->getType() == Terrain_ThickSpice ) ;
                bool bullet_mask = (  (bulletID == Bullet_DRocket) || (bulletID == Bullet_GasCloud)  || (bulletID == Bullet_LargeRocket) ||
                		(bulletID == Bullet_Rocket) || (bulletID == Bullet_TurretRocket) || (bulletID == Bullet_ShellSmall) ||
						(bulletID == Bullet_ShellMedium) || (bulletID == Bullet_ShellLarge) || (bulletID == Bullet_SmallRocket) );
                if (terrain_mask && bullet_mask &&  (!pTile->hasAGroundObject() || !pTile->getGroundObject()->isAStructure()) )   {
                	float damage;
                	switch (bulletID) {
            			case Bullet_SmallRocket:
            				damage = 20.0;
            				break;
                		case Bullet_LargeRocket:
                			damage = 200.0;
                			break;
                		case Bullet_TurretRocket:
						case Bullet_Rocket:
							damage = 25.0;
							break;
                		case Bullet_ShellLarge:
                			damage = 15.0;
                			break;
                		case Bullet_ShellMedium:
                			damage = 10.0;
                			break;
                		case Bullet_GasCloud:
                		case Bullet_DRocket:
                		case Bullet_ShellSmall:
                		default:
                			damage = 0.0f;
                	}
                	pTile->setSpice(pTile->getSpiceRemaining() - damage);
                }


            }

        } // end of non air damage
    }

	if((bulletID != Bullet_Sonic) && (bulletID != Bullet_Sandworm) && tileExists(location) && getTile(location)->isSpiceBloom()) {
        getTile(location)->triggerSpiceBloom(damagerOwner);
	}
}

bool Map::okayToPlaceStructure(int x, int y, int buildingSizeX, int buildingSizeY, bool tilesRequired, const House* pHouse, bool bIgnoreUnits) const {
	bool withinBuildRange = false;

	for(int i = x; i < x + buildingSizeX; i++) {
		for(int j = y; j < y + buildingSizeY; j++) {
            if(!currentGameMap->tileExists(i,j)) {
                return false;
            }

            const Tile* pTile = getTile(i,j);

            if(!pTile->isRock() || (tilesRequired && !pTile->isConcrete()) || (!bIgnoreUnits && pTile->isBlocked())) {
				return false;
			}

			if((pHouse == NULL) || isWithinBuildRange(i, j, pHouse)) {
				withinBuildRange = true;
			}
		}
	}
	return withinBuildRange;
}


bool Map::isWithinBuildRange(int x, int y, const House* pHouse) const {
	bool withinBuildRange = false;

	for (int i = x - BUILDRANGE; i <= x + BUILDRANGE; i++)
		for (int j = y - BUILDRANGE; j <= y + BUILDRANGE; j++)
			if (tileExists(i, j) && (getTile(i,j)->getOwner() == pHouse->getHouseID()))
				withinBuildRange = true;

	return withinBuildRange;
}

/**
    This method figures out the direction of tile pos relative to tile source.
    \param  source  the starting point
    \param  pos     the destination
    \return one of RIGHT, RIGHTUP, UP, LEFTUP, LEFT, LEFTDOWN, DOWN, RIGHTDOWN or INVALID
*/
int Map::getPosAngle(const Coord& source, const Coord& pos) const {
	if(pos.x > source.x) {
		if(pos.y > source.y) {
			return RIGHTDOWN;
        } else if(pos.y < source.y) {
			return RIGHTUP;
		} else {
			return RIGHT;
		}
	} else if(pos.x < source.x) {
		if(pos.y > source.y) {
			return LEFTDOWN;
		} else if(pos.y < source.y) {
			return LEFTUP;
		} else {
			return LEFT;
		}
	} else {
		if(pos.y > source.y) {
			return DOWN;
		} else if(pos.y < source.y) {
			return UP;
		} else {
			return INVALID;
		}
	}
}

/**
    This method calculates the coordinate of one of the neighbor tiles of source
    \param  angle   one of RIGHT, RIGHTUP, UP, LEFTUP, LEFT, LEFTDOWN, DOWN, RIGHTDOWN
    \param  source  the tile to calculate neighbor tiles from
*/
Coord Map::getMapPos(int angle, const Coord& source) const {
	switch (angle)
	{
		case (RIGHT):       return Coord(source.x + 1 , source.y);       break;
		case (RIGHTUP):     return Coord(source.x + 1 , source.y - 1);   break;
		case (UP):          return Coord(source.x     , source.y - 1);   break;
		case (LEFTUP):      return Coord(source.x - 1 , source.y - 1);   break;
		case (LEFT):        return Coord(source.x - 1 , source.y);       break;
		case (LEFTDOWN):    return Coord(source.x - 1 , source.y + 1);   break;
		case (DOWN):        return Coord(source.x     , source.y + 1);   break;
		case (RIGHTDOWN):   return Coord(source.x + 1 , source.y + 1);   break;
		default:            return Coord(source.x     , source.y);       break;
	}
	return source;
}

//building size is num squares
Coord Map::findDeploySpot(UnitBase* pUnit, const Coord origin, const Coord gatherPoint, const Coord buildingSize) const {
	float		closestDistance = INFINITY;
	Coord       closestPoint;
	Coord		size;

	bool	found = false;
    bool    foundClosest = false;

	int	counter = 0;
	int	depth = 0;
	int edge;

    if(pUnit->isAFlyingUnit()) {
        return origin;
    }

	int ranX = origin.x;
	int ranY = origin.y;
	std::list<Coord> pathList;

	do {
		edge = currentGame->randomGen.rand(0, 3);
		switch(edge) {
            case 0:	//right edge
                ranX = origin.x + buildingSize.x + depth;
                ranY = currentGame->randomGen.rand(origin.y - depth, origin.y + buildingSize.y + depth);
                break;
            case 1:	//top edge
                ranX = currentGame->randomGen.rand(origin.x - depth, origin.x + buildingSize.x + depth);
                ranY = origin.y - depth - ((buildingSize.y == 0) ? 0 : 1);
                break;
            case 2:	//left edge
                ranX = origin.x - depth - ((buildingSize.x == 0) ? 0 : 1);
                ranY = currentGame->randomGen.rand(origin.y - depth, origin.y + buildingSize.y + depth);
                break;
            case 3: //bottom edge
                ranX = currentGame->randomGen.rand(origin.x - depth, origin.x + buildingSize.x + depth);
                ranY = origin.y + buildingSize.y + depth;
                break;
            default:
                break;
		}
		Coord temp = Coord(ranX, ranY);

		bool bOK2Deploy = pUnit->canPass(ranX, ranY);

		if(pUnit->isTracked() && tileExists(ranX, ranY) && getTile(ranX, ranY)->hasInfantry()) {
		    // we do not deploy on enemy infantry
		    bOK2Deploy = false;
		}


		if (gatherPoint.isInvalid()  && bOK2Deploy) {
					closestPoint.x = ranX;
					closestPoint.y = ranY;
					found = true;
		} else if (bOK2Deploy) {
			// A gather point is defined, let's find a Reacheable path to it
			AStarSearch pathfinder(currentGameMap, pUnit, temp, gatherPoint);
			pathList = pathfinder.getFoundReacheablePath();


			if(!pathList.empty()) {
				closestDistance = blockDistance(temp, gatherPoint);
				closestPoint.x = ranX;
				closestPoint.y = ranY;
				foundClosest = true;
			}
		}



		if(counter++ >= 40) {
		    //if hasn't found a spot on tempObject layer in 100 tries, goto next

			counter = 0;
			if(++depth > (std::max(currentGameMap->getSizeX(), currentGameMap->getSizeY()))) {
				closestPoint.invalidate();
				found = true;
				fprintf(stderr, "Cannot find deploy position because the map is full pathlist.size=%d!\n",pathList.size()); fflush(stderr);
			}
		}
	} while (!found && (!foundClosest || (counter > 0)));

	//if (counter > 10) dbg_print("Map::findDeploySpot tries %d for %d(%d)!\n",counter,pUnit->getItemID(),pUnit->getObjectID());
	//else dbg_print("Map::findDeploySpot (in %d tries) assigned %d(%d) to (%d,%d)!\n",counter,pUnit->getItemID(),pUnit->getObjectID(),closestPoint.x,closestPoint.y);

	return closestPoint;
}

/**
    This method finds the tile which is at a map border and is at minimal distance to the structure
    specified by origin and buildingsSize. This method is especcially useful for Carryalls and Frigates
    that have to enter the map to deliver units.
    \param origin           the position of the structure in map coordinates
    \param buildingSize    the number of tiles occupied by the building (e.g. 3x2 for refinery)
*/
Coord Map::findClosestEdgePoint(const Coord& origin, const Coord& buildingSize) const {
	int closestDistance = INT_MAX;
	Coord closestPoint;

	if(origin.x < (sizeX - (origin.x + buildingSize.x))) {
		closestPoint.x = 0;
		closestDistance = origin.x;
	} else {
		closestPoint.x = sizeX - 1;
		closestDistance = sizeX - (origin.x + buildingSize.x);
	}
	closestPoint.y = origin.y;

	if(origin.y < closestDistance) {
		closestPoint.x = origin.x;
		closestPoint.y = 0;
		closestDistance = origin.y;
	}

	if((sizeY - (origin.y + buildingSize.y)) < closestDistance) {
		closestPoint.x = origin.x;
		closestPoint.y = sizeY - 1;
		// closestDistance = origin.y;  //< Not needed anymore
	}

	return closestPoint;
}


void Map::removeObjectFromMap(Uint32 objectID) {
	for(int y = 0; y < sizeY ; y++) {
		for(int x = 0 ; x < sizeX ; x++) {
			getTile(x,y)->unassignObject(objectID);
		}
	}
}


void Map::recalutateCoordinates(const ObjectBase* objLeader, bool forcedRecal = false) {


	/*{
		std::list<std::pair<Uint32,Coord>>::iterator test;
		for(test = currentGame->getSelectedListCoord().begin() ; test != currentGame->getSelectedListCoord().end(); ++test) {
			ObjectBase *obj3 = currentGame->getObjectManager().getObject(test->first);
			Coord coord = Coord((test->second).x,(test->second).y);
			if(obj3->isAUnit()) {
				fprintf(stdout,"%d(%d,%d) ",obj3->getObjectID(),coord.x,coord.y);

			}
		}
		fprintf(stdout,"\n");
	}*/


	std::list<Uint32>::iterator itlist;
	std::list<std::pair<Uint32,Coord>>::iterator itcoord,recalc;
	std::list<Uint32> *list = &currentGame->getSelectedList();
	std::list<std::pair<Uint32,Coord>> *listc = &currentGame->getSelectedListCoord();


	for(itlist = list->begin() , itcoord = listc->begin() ; itlist != list->end(), itcoord != listc->end() ; ++itlist, ++itcoord) {
		ObjectBase *obj2 = currentGame->getObjectManager().getObject(*itlist);

		if (obj2 == NULL) {
			err_print("Map::recalutateCoordinates Out of bound Selectionlist (most probably getSelectedListCoord().size() != getSelectedList().size()) !\n");
			break;
		}

		if(obj2->isAUnit()) {

			if (obj2 != objLeader) {
				// Reset all the other to non-leader
				((UnitBase*)(obj2))->setLeader(false);
			}
			else {
				// Set new leader, put it in front of list and listCoord
				((UnitBase*)(obj2))->setLeader(true);
				// if leading unit is not in front of list, put it
				if (itlist != list->begin())
					list->splice(list->begin(),*list,itlist,list->end());

				// if leading unit is not in front of coordlist, put it
				if (itcoord != listc->begin())		{
					listc->splice(listc->begin(),*listc,itcoord,listc->end());
					forcedRecal=true;
				}

				if (forcedRecal) {
					// Recalculate relative formation coordinates based on new group leader
					for(recalc = currentGame->getSelectedListCoord().begin() ; recalc != currentGame->getSelectedListCoord().end(); ++recalc) {
						ObjectBase *obj3 = currentGame->getObjectManager().getObject(recalc->first);
						Coord *coord3 = &recalc->second;
						if(obj3->isAUnit()) {
							if (obj3 != obj2) {
								// Recalculate based on obj2(leader)
								*coord3 = obj3->getLocation() - obj2->getLocation() ;
								err_print("Map::recalutateCoordinates calculating %d,%d (%d) \n",coord3->x,coord3->y,obj3->getObjectID());
							} else {
								// Recalculate on self (0,0)
								*coord3 = Coord (0,0);
								err_print("Map::recalutateCoordinates resetting 0,0 (%d) \n",obj2->getObjectID());
							}
						}
					}
				}


			}
		}

	}

	/*{
		std::list<std::pair<Uint32,Coord>>::iterator test;
		for(test = currentGame->getSelectedListCoord().begin() ; test != currentGame->getSelectedListCoord().end(); ++test) {
			ObjectBase *obj3 = currentGame->getObjectManager().getObject(test->first);
			Coord coord = Coord((test->second).x,(test->second).y);
			if(obj3->isAUnit()) {
				fprintf(stderr,"%d(%d,%d) ",obj3->getObjectID(),coord.x,coord.y);

			}
		}
		fprintf(stderr,"\n");
	}*/

}

void Map::selectObjects(int houseID, int x1, int y1, int x2, int y2, int realX, int realY, const Uint32 objectARGMode) {

	ObjectBase	*lastCheckedObject = NULL;
	ObjectBase *lastSelectedObject = NULL;

	err_print("Map::selectObjects : [%d,%d]-[%d,%d] %d,%d \n",x1,y1,x2,y2,realX,realY);
	//if selection rectangle is checking only one tile and has shift selected we want to add/ remove that unit from the selected group of units
	if((!objectARGMode) & KMOD_SHIFT) {
		currentGame->unselectAll(currentGame->getSelectedList());
		currentGame->getSelectedList().clear();
		currentGame->getSelectedListCoord().clear();
		currentGame->selectionChanged();
		currentGame->setGroupLeader(NULL);
	}

	/* selectAllPlayersUnitsOfType */
	if((x1 == x2) && (y1 == y2) && tileExists(x1, y1)) {

        if(getTile(x1,y1)->isExplored(houseID) || debug) {
            lastCheckedObject = getTile(x1,y1)->getObjectAt(realX, realY);
            if (lastCheckedObject != NULL)
          	 err_print("Map::selectObjects : selectAllPlayersUnitsOfType %d AreaGuardRange:%d\n",lastCheckedObject->getObjectID(), lastCheckedObject->getAreaGuardRange());
        } else {
		    lastCheckedObject = NULL;
		}

		if((lastCheckedObject != NULL) && (lastCheckedObject->getOwner()->getHouseID() == houseID)) {
			if((lastCheckedObject == lastSinglySelectedObject) && ( !lastCheckedObject->isAStructure())) {
					currentGame->setGroupLeader(lastSinglySelectedObject);

                for(int i = screenborder->getTopLeftTile().x; i <= screenborder->getBottomRightTile().x; i++) {
                    for(int j = screenborder->getTopLeftTile().y; j <= screenborder->getBottomRightTile().y; j++) {
                        if(tileExists(i,j) && getTile(i,j)->hasAnObject()) {
                            getTile(i,j)->selectAllPlayersUnitsOfType(houseID, lastSinglySelectedObject, &lastCheckedObject, &lastSelectedObject, currentGame->getGroupLeader());
                        }
                    }
				}
				lastSinglySelectedObject = NULL;

				if (currentGame->getGroupLeader() != NULL)
					currentGameMap->recalutateCoordinates(currentGame->getGroupLeader(),true);

			} else if(!lastCheckedObject->isSelected())	{
				lastCheckedObject->setSelected(true);
				currentGame->getSelectedList().emplace_back(lastCheckedObject->getObjectID());
				if (currentGame->getSelectedList().size() == 2) {
					currentGame->setGroupLeader( currentGame->getObjectManager().getObject((currentGame->getSelectedList()).front()) );
				}

				if (currentGame->getGroupLeader() == NULL)
					currentGame->getSelectedListCoord().push_back(std::make_pair<Uint32,Coord> ( (lastCheckedObject)->getObjectID(), ((lastCheckedObject)->getLocation() - (lastCheckedObject)->getLocation()) ) );
				else
					currentGame->getSelectedListCoord().push_back(std::make_pair<Uint32,Coord> ((lastCheckedObject)->getObjectID(), ((lastCheckedObject)->getLocation() - (currentGame->getGroupLeader())->getLocation())) );
				currentGame->selectionChanged();
				lastSelectedObject = lastCheckedObject;
				lastSinglySelectedObject = lastSelectedObject;
				 if (currentGame->getGroupLeader() != NULL)
					 currentGameMap->recalutateCoordinates(currentGame->getGroupLeader(),true);

			} else if(objectARGMode & KMOD_SHIFT) {
			    //holding down shift, unselect this unit
				lastCheckedObject->setSelected(false);
				currentGame->getSelectedList().remove(lastCheckedObject->getObjectID());

				std::list<std::pair<Uint32,Coord>>::iterator iter2 = std::remove_if(currentGame->getSelectedListCoord().begin(),
						currentGame->getSelectedListCoord().end(), [=](std::pair<Uint32,Coord> & item) { return item.first == (lastCheckedObject)->getObjectID() ;} );
				currentGame->getSelectedListCoord().erase(iter2,currentGame->getSelectedListCoord().end());
				//currentGame->getSelectedListCoord().pop_back();

				currentGame->selectionChanged();
				// If the removed object was the leader, reset a new leader
				if (lastCheckedObject->isAUnit() && ((UnitBase*)lastCheckedObject)->isLeader()) {
					((UnitBase*)lastCheckedObject)->setLeader(false);
					if (currentGame->getSelectedList().size() > 1) {
						std::list<Uint32> *list = &currentGame->getSelectedList();
						std::list<Uint32>::iterator iter = list->begin();
						while (iter != list->end()) {
							ObjectBase *obj = currentGame->getObjectManager().getObject(*iter);
							if (obj != NULL && obj->isAUnit()) {
								((UnitBase*)obj)->setLeader(true);
								currentGame->setGroupLeader(obj);
								break;
							}
							else iter++;
						}

						 if (currentGame->getGroupLeader() != NULL)
							 currentGameMap->recalutateCoordinates(currentGame->getGroupLeader(),true);

					} else {
						 ;
					}
				}
			}

		} else {
			lastSinglySelectedObject = NULL;
		}

	}
	/* selectAllPlayersUnits */
	else {

		err_print("Map::selectObjects : selectAllPlayersUnits \n");
		bool alsoAir = false;

		if (objectARGMode & KMOD_CTRL) {
			//holding down ctrl to select also airborn unit
			alsoAir = true;
			err_print("Map::selectObjects : Also AIR \n");
		}

		for(int i = std::min(x1, x2); i <= std::max(x1, x2); i++) {
            for(int j = std::min(y1, y2); j <= std::max(y1, y2); j++) {
                if(tileExists(i,j) && getTile(i,j)->hasAnObject() && getTile(i,j)->isExplored(houseID) /*&& !getTile(i,j)->isFogged(houseID)*/) {
                    getTile(i,j)->selectAllPlayersUnits(houseID, &lastCheckedObject, &lastSelectedObject, currentGame->getGroupLeader());
                    lastSinglySelectedObject = lastCheckedObject;
                }
            }
        }


		std::list<Uint32> *list = &currentGame->getSelectedList();
		std::list<Uint32>::iterator iter = list->begin();
		// Do we only have air units in the selection ?
		bool onlyAir = true;
		while (iter != list->end()) {
			ObjectBase *obj = currentGame->getObjectManager().getObject(*iter);
			UnitBase *unit = dynamic_cast<UnitBase*>(obj);
			if(obj->isAUnit() && !unit->isAFlyingUnit()) {
				onlyAir = false;
				break;
			}
			iter++;
		}

		// We are told not to select Air units and selection does not only have air born unit : only keep ground unit
		if (!alsoAir && !onlyAir) {
			err_print("Tile::selectAllPlayersUnits alsoAir:%s onlyAir:%s grouplead:%d \n", alsoAir ? "y" :"n", onlyAir ? "y" : "n",  currentGame->getGroupLeader() == NULL ? 0 : currentGame->getGroupLeader()->getObjectID() );
			if (currentGame->getSelectedList().size() > 1 ) {
				iter = list->begin();

				while (iter != list->end()) {
					ObjectBase *obj = currentGame->getObjectManager().getObject(*iter);
					UnitBase *unit = dynamic_cast<UnitBase*>(obj);
					if(obj->isAUnit() && unit->isAFlyingUnit()) {
						unit->setSelected(false);
						unit->setLeader(false);

						iter = currentGame->getSelectedList().erase(iter);
						std::list<std::pair<Uint32,Coord>>::iterator iter2 = std::remove_if(currentGame->getSelectedListCoord().begin(),
								currentGame->getSelectedListCoord().end(), [=](std::pair<Uint32,Coord> & item) { return item.first == (unit)->getObjectID() ;} );
						currentGame->getSelectedListCoord().erase(iter2,currentGame->getSelectedListCoord().end());
						//currentGame->getSelectedListCoord().pop_back();
						//err_print("Tile::selectAllPlayersUnits removing %d -- Coord (%d,%d) \n", unit->getObjectID(),  ((unit)->getLocation() - ((currentGame->getGroupLeader()))->getLocation()).x, ((unit)->getLocation() - ((currentGame->getGroupLeader()))->getLocation()).y);

						if  (currentGame->getSelectedList().size() > 1 ) {
							std::list<Uint32>::iterator iter2 = list->begin();
							// current obj is a fly follower & can't be leading
							// try to assign a non follower ground leader
							if ( obj->isAUnit() &&    ( unit->isAFlyingUnit() || (obj->isAGroundUnit() && ((UnitBase*)(obj))->isFollowing()) ) ) {

								while (iter2 != list->end() ) {
									obj = currentGame->getObjectManager().getObject(*iter2);
									if (obj->isAUnit() &&    ! unit->isAFlyingUnit() && (obj->isAGroundUnit() && !((UnitBase*)(obj))->isFollowing()) )
										break;
									iter2++;
								}
							}

							if (obj->isAGroundUnit() && !((UnitBase*)(obj))->isFollowing()) {
								((UnitBase*)(obj))->setLeader(true);
							 	 currentGame->setGroupLeader(obj);
							 	 err_print("Tile::selectAllPlayersUnits promoted group leader %d \n", (currentGame->getGroupLeader())->getObjectID());
							} /*else {
								// All are follower or not independent ground unit
								currentGame->setGroupLeader(NULL);
							}*/

						} else {
							 currentGame->setGroupLeader(NULL);
						}
						currentGame->selectionChanged();

					} else  {
						((UnitBase*)(obj))->setLeader(false);
						iter++;
						if  ( currentGame->getGroupLeader() == NULL && currentGame->getSelectedList().size() > 1 ) {
							std::list<Uint32>::iterator iter2 = list->begin();
							// current obj is a follower & can't be leading
							// try to assign a non follower ground leader
							if ( obj->isAUnit() &&   (obj->isAGroundUnit() && ((UnitBase*)(obj))->isFollowing()) ) {

								while (iter2 != list->end() ) {
									obj = currentGame->getObjectManager().getObject(*iter2);
									if (obj->isAUnit() &&  obj->isAGroundUnit() && ! ((UnitBase*)(obj))->isFollowing() )
										break;
									iter2++;
								}
							}

							if (obj->isAGroundUnit() && !((UnitBase*)(obj))->isFollowing()) {
								((UnitBase*)(obj))->setLeader(true);
								 currentGame->setGroupLeader(obj);
								 err_print("Tile::selectAllPlayersUnits XXX promoted group leader %d \n", (currentGame->getGroupLeader())->getObjectID());
							} else {
								// All are follower ground unit
								//currentGame->setGroupLeader(NULL);
							}
						} else if (currentGame->getSelectedList().size() == 1) {
							currentGame->setGroupLeader(NULL);
						}

					}


				} /* iter != list->end() */

				 if ( currentGame->getGroupLeader() != NULL)
					 currentGameMap->recalutateCoordinates(currentGame->getGroupLeader(),true);
			}

		}


		if(objectARGMode & KMOD_SHIFT) {
			// Multiple object were added : reset a new leader
			bool resetLead = false;
			if (currentGame->getSelectedList().size() > 1) {
				dbg_print("Tile::selectAllPlayersUnits resetlead on added OBJ \n");

				std::list<Uint32> *list = &currentGame->getSelectedList();
				std::list<Uint32>::iterator iter = list->begin();
				while (iter != list->end()) {
					ObjectBase *obj = currentGame->getObjectManager().getObject(*iter);
					if (obj != NULL && obj->isAUnit() && !resetLead) {
						((UnitBase*)obj)->setLeader(true);
						currentGame->setGroupLeader(obj);
						resetLead = true;
						iter++;
					} else if (obj != NULL && obj->isAUnit() && resetLead && ((UnitBase*)obj)->isLeader()) {
						((UnitBase*)obj)->setLeader(false);
						iter++;
					} else {
						iter++;
					}

				}
				 if (currentGame->getGroupLeader() != NULL)
					 currentGameMap->recalutateCoordinates(currentGame->getGroupLeader(),true);
			}
		}

		lastSinglySelectedObject = NULL;
	}

	//select an enemy unit if none of your units found
	if(currentGame->getSelectedList().empty() && (lastCheckedObject != NULL) && !lastCheckedObject->isSelected()) {
		lastCheckedObject->setSelected(true);
		lastSelectedObject = lastCheckedObject;
		currentGame->getSelectedList().push_back(lastCheckedObject->getObjectID());
		currentGame->selectionChanged();
	} else if (lastSelectedObject != NULL) {
		lastSelectedObject->playSelectSound();	//we only want one unit responding
	}

}




bool Map::findSpice(Coord& destination, const Coord& origin, Harvester* harvester, int radius) const {
	// TODO : get this algo smarter (not going where its crowed with enemy)
	// XXX make used of unit param : it should check first in viewrange, second the all map (should be made a tech-option)
	float shorter = std::numeric_limits<float>::infinity(), distance, _distance;
	int denser = 0, ext_speed;
	Coord shortest = Coord(0,0);
	Coord densest = Coord(0,0);
	Coord t;
	Coord spiceCoord = Coord::Invalid();
	int max_x=0 , max_y = 0;
	int sample = 0;
	int shortersample = 0;
	int densersample = 0;

	int max_radius = radius;

	for (int a=9; a <= max_radius; a++) {
     for(int i=8;i<a;i++) {
		int r = currentGame->randomGen.rand( ((i-5) > i/2 ? i/2 : (i-5)) , (i/2 > (i-5) ? i/2 : (i-5)) );
		float angle = 2.0f * strictmath::pi * currentGame->randomGen.randFloat();
		t =  Coord( (int) (r*strictmath::sin(angle)), (int) (-r*strictmath::cos(angle)));
		spiceCoord = Coord(origin + t);
		if (abs(spiceCoord.x-origin.x) > max_x) max_x = abs(spiceCoord.x-origin.x);
		if (abs(spiceCoord.y-origin.y) > max_y) max_y = abs(spiceCoord.y-origin.y);
		distance = distanceFrom(spiceCoord,origin);
		//dbg_print("Map::findSpice lookup r%d destination(%d,%d) distance:%f\n",r,spiceCoord.x,spiceCoord.y,distance);
		UnitBase * blocker = NULL ;	// XXX maybe move the blocker after all
		// XXX : should not be able to find spice in not yet explored area !
		Tile * pTile;
		if (harvester != NULL) {
			pTile = currentGameMap->tileExists(spiceCoord) && currentGameMap->getTile(spiceCoord)->isExplored(harvester->getOwner()->getHouseID()) ? currentGameMap->getTile(spiceCoord) : NULL;
		} else {
			pTile = currentGameMap->tileExists(spiceCoord) ? currentGameMap->getTile(spiceCoord) : NULL;
		}

		if (pTile != NULL && pTile->hasAGroundObject()  && pTile->getGroundObject()->isAUnit()) {
			blocker = (UnitBase*)(pTile->getGroundObject());
		}

		if(pTile != NULL && pTile->hasSpice() && (!pTile->hasAGroundObject() || (blocker != NULL && blocker->isMoving())) ) {
			sample++;
			ext_speed = pTile != NULL ? pTile->getExtractionSpeed() : 0;
			int spice = pTile->hasSpice() ? pTile->getSpiceRemaining(): 0;
			if (distance < shorter) {
				shorter = distance;
				shortest = t; shortersample++;
				dbg_print("Map::findSpice NbSamples(%d) lookup r%d shorter-destination(%d,%d) distance:%f spice:%d(@ %d)\n",harvester != NULL ? harvester->getProspectionSamplesNumber() : -1,r, Coord(origin+shortest).x,Coord(origin+shortest).y,distance,spice,ext_speed);
				if (harvester != NULL) {
					//harvester->addProspectionSample((origin+t),ext_speed);
					harvester->addProspectionSample(spiceCoord,ext_speed);
				}
			}
			if (ext_speed > denser) {
				denser = ext_speed;
				_distance = distance;
				densest = t; densersample++;
				dbg_print("Map::findSpice NbSamples(%d) lookup r%d denser-destination(%d,%d) distance:%f spice:%d(@ %d)\n",harvester != NULL ? harvester->getProspectionSamplesNumber() : -1,r, Coord(origin+densest).x,Coord(origin+densest).y,distance,spice,ext_speed);
				if (harvester != NULL) {
					//harvester->addProspectionSample((origin+t), ext_speed);
					harvester->addProspectionSample(spiceCoord,ext_speed);
				}
			}
		}
	 }
	}

     Coord _shortest = origin+shortest;
     Tile * pTile_shortest = currentGameMap->getTile(_shortest);
     float speed_shortest 	= pTile_shortest->hasSpice() ? pTile_shortest->getSpiceExtractionSpeed(): 0;
     float ref_speed_shortest = pTile_shortest->hasSpice() ? pTile_shortest->getSpiceExtractionSpeed(true): 1;
     int ext_speed_shortest = (int)((speed_shortest/ref_speed_shortest)*100);

     Coord _densest =  origin+densest;
     Tile * pTile_densest = currentGameMap->getTile(_densest);
     float speed_densest 	= pTile_densest->hasSpice() ? pTile_densest->getSpiceExtractionSpeed(): 0;
     float ref_speed_densest = pTile_densest->hasSpice() ? pTile_densest->getSpiceExtractionSpeed(true): 1;
     int ext_speed_densest = (int)((speed_densest/ref_speed_densest)*100);

     if (harvester != NULL) {
    	 if (harvester->getPreference() == DISTANCE) {
    		 if (shortest != Coord(0,0)) {
    			 destination = origin+shortest;
    		 }
    	 }
    	 else if (harvester->getPreference() == DENSITY) {
    		 if (densest != Coord(0,0)) {
    			 destination = origin+densest;
    		 } else destination = origin+shortest;
    	 }

     } else destination = origin+shortest;

   /*  dbg_print("Map::findSpice origin(%d,%d) shortest-destination(%d,%d) distance:%f spiceleft:%f(@%d) [max_x:%d, max_y:%d shortersample/Total:%d/%d]\n",
    		 origin.x,origin.y, _shortest.x,_shortest.y,shorter, pTile_shortest->getSpiceRemaining(),ext_speed_shortest ,max_x,max_y,shortersample,sample);
     dbg_print("Map::findSpice origin(%d,%d) densest-destination(%d,%d) distance:%f spiceleft:%f(@%d) [max_x:%d, max_y:%d densersample/Total:%d/%d]\n",
    		 origin.x,origin.y, _densest.x,_densest.y,_distance, pTile_densest->getSpiceRemaining(),ext_speed_densest ,max_x,max_y,densersample,sample);*/
     return destination != origin ? true : false;



}

/**
    This method fixes surounding thick spice tiles after spice gone to make things look smooth.
    \param coord    the coordinate where spice was removed from
*/
void Map::spiceRemoved(const Coord& coord) {

	if(tileExists(coord)) {	//this is the center tile
		if(getTile(coord)->getType() == Terrain_Sand) {
			//thickspice tiles can't handle non-(thick)spice tiles next to them, if this happens after changes, make it non thick
			for(int i = coord.x-1; i <= coord.x+1; i++) {
                for(int j = coord.y-1; j <= coord.y+1; j++) {
                    if (tileExists(i, j) && (((i==coord.x) && (j!=coord.y)) || ((i!=coord.x) && (j==coord.y))) && getTile(i,j)->isThickSpice()) {
                        //only check tile right, up, left and down of this one
                        getTile(i,j)->setType(Terrain_Spice);
                    }
                }
			}
		}
	}
}

void Map::viewMap(int playerTeam, const Coord& location, int maxViewRange) {

//makes map viewable in an area like as shown below

//				       *****
//                   *********
//                  *****T*****
//                   *********
//                     *****

    Coord   check;
	check.x = location.x - maxViewRange;
	if(check.x < 0) {
		check.x = 0;
	}

	while((check.x < sizeX) && ((check.x - location.x) <=  maxViewRange)) {
		check.y = (location.y - lookDist[abs(check.x - location.x)]);
		if (check.y < 0) check.y = 0;

		while((check.y < sizeY) && ((check.y - location.y) <= lookDist[abs(check.x - location.x)])) {
			if(distanceFrom(location, check) <= maxViewRange) {
                for(int i = 0; i < NUM_HOUSES; i++) {
                    House* pHouse = currentGame->getHouse(i);
                    if((pHouse != NULL) && (pHouse->getTeam() == playerTeam)) {
                        getTile(check)->setExplored(i,currentGame->getGameCycleCount());
                    }
                }
			}

			check.y++;
		}

		check.x++;
		check.y = location.y;
	}
}

/**
    Creates a spice field of the given radius at the given location.
    \param  location            the location in tile coordinates
    \param  radius              the radius in tiles (0 = only one tile is filled)
    \param  centerIsThickSpice  if set the center is filled with thick spice
    \param  nobloom				if set inssure no Spicebloom could be created
    \return totalSpiceAllocated the number of spice allocated
*/
float Map::createSpiceField(Coord location, int radius, bool centerIsThickSpice, bool noBloom, bool noSpiceTrim) {
    Coord offset;
    float totalSpiceAllocated = 0;
    for(offset.x = -radius; offset.x <= radius; offset.x++) {
        for(offset.y = -radius; offset.y <= radius; offset.y++) {
            if(currentGameMap->tileExists(location + offset)) {
                Tile* pTile = currentGameMap->getTile(location + offset);

                if((pTile->isSand() || pTile->isSpice() || pTile->isThickSpice() ) && (distanceFrom(location, location + offset) <= radius)) {
                    if(centerIsThickSpice && (distanceFrom(location, location + offset) <= radius/5)) {
                    	 if (currentGame->randomGen.rand(1,10) <= 8) {
                    		 pTile->setType(Terrain_ThickSpice,false);
                    		 currentGameMap->spiceRemoved(pTile->location+offset);
                    		 if (!noBloom && currentGame->randomGen.rand(1,30) <= 1)
                    			 pTile->setType(Terrain_SpiceBloom);
                    	 }
                    	 else  pTile->setType(Terrain_Spice,false);
                    } else {
                    	 // distribute spice a bit naturally
                    	 if ((distanceFrom(location, location + offset) > radius-2) ) {
                    		 // trim a bit spice on radius asif already harvested
                    		 if ( (currentGameMap->tileExists(location - offset) && currentGameMap->getTile(location - offset)->getType() == Terrain_Spice) ||
								  (currentGameMap->tileExists(location + offset) && currentGameMap->getTile(location + offset)->getType() == Terrain_Spice) )
							 {
                    			 if (!noSpiceTrim && currentGame->randomGen.rand(1,20) <= 5) {
                    				 pTile->setType(Terrain_Sand);
                    			 }
                    		 } else if (currentGame->randomGen.rand(1,20) <= 18) {
                    			 pTile->setType(Terrain_Spice,false);
                    		 } else {
                        		 if (!noBloom && currentGame->randomGen.rand(1,20) <= 1)
                        			 pTile->setType(Terrain_SpiceBloom);
                    		 }
                    	 } else {
                    		 pTile->setType(Terrain_Spice,false);
                    	 }
                    }
                }
                totalSpiceAllocated += pTile->getSpiceRemaining();

            }
        }
    }
    return totalSpiceAllocated;
}


