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

#include <Tile.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>

#include <sand.h>
#include <Game.h>
#include <Map.h>
#include <House.h>
#include <SoundPlayer.h>
#include <ScreenBorder.h>
#include <ConcatIterator.h>
#include <Explosion.h>

#include <structures/StructureBase.h>
#include <units/InfantryBase.h>
#include <units/AirUnit.h>

#include <misc/draw_util.h>

Tile::Tile() {
	type = Terrain_Sand;

	for(int i = 0; i < NUM_HOUSES; i++) {
		explored[i] = currentGame->getGameInitSettings().getGameOptions().startWithExploredMap;
		lastAccess[i] = 0;
	}

	fogColor = COLOR_BLACK;

	owner = INVALID;
	sandRegion = NONE;

	spice = 0.0f;

	sprite = pGFXManager->getObjPic(ObjPic_Terrain);

	for(int i=0; i < NUM_ANGLES; i++) {
        tracksCounter[i] = 0;
	}

	location.x = 0;
	location.y = 0;

	destroyedStructureTile = DestroyedStructure_None;
}


Tile::~Tile() {
}

void Tile::load(InputStream& stream) {
	type = stream.readUint32();

    stream.readBools(&explored[0], &explored[1], &explored[2], &explored[3], &explored[4], &explored[5]);

    bool bLastAccess[NUM_HOUSES];
    stream.readBools(&bLastAccess[0], &bLastAccess[1], &bLastAccess[2], &bLastAccess[3], &bLastAccess[4], &bLastAccess[5]);

    for(int i=0;i<NUM_HOUSES;i++) {
        if(bLastAccess[i] == true) {
            lastAccess[i] = stream.readUint32();
        }
	}

	fogColor = stream.readUint32();

	owner = stream.readSint32();
	sandRegion = stream.readUint32();

	spice = stream.readFloat();

	bool bHasDamage, bHasDeadUnits, bHasAirUnits, bHasInfantry, bHasUndergroundUnits, bHasNonInfantryGroundObjects;
	stream.readBools(&bHasDamage, &bHasDeadUnits, &bHasAirUnits, &bHasInfantry, &bHasUndergroundUnits, &bHasNonInfantryGroundObjects);

    if(bHasDamage) {
        Uint32 numDamage = stream.readUint32();
        for(Uint32 i=0; i<numDamage; i++) {
            DAMAGETYPE newDamage;
            newDamage.damageType = stream.readUint32();
            newDamage.tile = stream.readSint32();
            newDamage.realPos.x = stream.readSint32();
            newDamage.realPos.y = stream.readSint32();

            damage.push_back(newDamage);
        }
    }

    if(bHasDeadUnits) {
        Uint32 numDeadUnits = stream.readUint32();
        for(Uint32 i=0; i<numDeadUnits; i++) {
            DEADUNITTYPE newDeadUnit;
            newDeadUnit.type = stream.readUint8();
            newDeadUnit.house = stream.readUint8();
            newDeadUnit.onSand = stream.readBool();
            newDeadUnit.realPos.x = stream.readSint32();
            newDeadUnit.realPos.y = stream.readSint32();
            newDeadUnit.timer = stream.readSint16();

            deadUnits.push_back(newDeadUnit);
        }
    }

	destroyedStructureTile = stream.readSint32();

    bool bTrackCounter[NUM_ANGLES];
    stream.readBools(&bTrackCounter[0], &bTrackCounter[1], &bTrackCounter[2], &bTrackCounter[3], &bTrackCounter[4], &bTrackCounter[5], &bTrackCounter[6], &bTrackCounter[7]);

    for(int i=0; i < NUM_ANGLES; i++) {
        if(bTrackCounter[i] == true) {
            tracksCounter[i] = stream.readSint16();
        }
    }

    if(bHasAirUnits) {
        assignedAirUnitList = stream.readUint32List();
    }

    if(bHasInfantry) {
        assignedInfantryList = stream.readUint32List();
    }

	if(bHasUndergroundUnits) {
	    assignedUndergroundUnitList = stream.readUint32List();
	}

	if(bHasNonInfantryGroundObjects) {
	    assignedNonInfantryGroundObjectList = stream.readUint32List();
	}
}

void Tile::save(OutputStream& stream) const {
	stream.writeUint32(type);

	stream.writeBools(explored[0], explored[1], explored[2], explored[3], explored[4], explored[5]);

    stream.writeBools((lastAccess[0] != 0), (lastAccess[1] != 0), (lastAccess[2] != 0), (lastAccess[3] != 0), (lastAccess[4] != 0), (lastAccess[5] != 0));
    for(int i=0;i<NUM_HOUSES;i++) {
        if(lastAccess[i] != 0) {
            stream.writeUint32(lastAccess[i]);
        }
	}

	stream.writeUint32(fogColor);

	stream.writeUint32(owner);
	stream.writeUint32(sandRegion);

	stream.writeFloat(spice);

	stream.writeBools(  !damage.empty(), !deadUnits.empty(), !assignedAirUnitList.empty(),
                        !assignedInfantryList.empty(), !assignedUndergroundUnitList.empty(), !assignedNonInfantryGroundObjectList.empty());

    if(!damage.empty()) {
        stream.writeUint32(damage.size());
        for(std::vector<DAMAGETYPE>::const_iterator iter = damage.begin(); iter != damage.end(); ++iter) {
            stream.writeUint32(iter->damageType);
            stream.writeSint32(iter->tile);
            stream.writeSint32(iter->realPos.x);
            stream.writeSint32(iter->realPos.y);
        }
    }

    if(!deadUnits.empty()) {
        stream.writeUint32(deadUnits.size());
        for(std::vector<DEADUNITTYPE>::const_iterator iter = deadUnits.begin(); iter != deadUnits.end(); ++iter) {
            stream.writeUint8(iter->type);
            stream.writeUint8(iter->house);
            stream.writeBool(iter->onSand);
            stream.writeSint32(iter->realPos.x);
            stream.writeSint32(iter->realPos.y);
            stream.writeSint16(iter->timer);
        }
    }

    stream.writeSint32(destroyedStructureTile);


    stream.writeBools(  (tracksCounter[0] != 0), (tracksCounter[1] != 0), (tracksCounter[2] != 0), (tracksCounter[3] != 0),
                        (tracksCounter[4] != 0), (tracksCounter[5] != 0), (tracksCounter[6] != 0), (tracksCounter[7] != 0));
    for(int i=0; i < NUM_ANGLES; i++) {
        if(tracksCounter[i] != 0) {
            stream.writeSint16(tracksCounter[i]);
        }
    }

    if(!assignedAirUnitList.empty()) {
        stream.writeUint32List(assignedAirUnitList);
    }

	if(!assignedInfantryList.empty()) {
        stream.writeUint32List(assignedInfantryList);
	}

	if(!assignedUndergroundUnitList.empty()) {
	    stream.writeUint32List(assignedUndergroundUnitList);
	}

	if(!assignedNonInfantryGroundObjectList.empty()) {
	    stream.writeUint32List(assignedNonInfantryGroundObjectList);
	}
}

void Tile::assignAirUnit(Uint32 newObjectID) {
	assignedAirUnitList.push_back(newObjectID);
}

void Tile::assignNonInfantryGroundObject(Uint32 newObjectID) {
	assignedNonInfantryGroundObjectList.push_back(newObjectID);
}

int Tile::assignInfantry(Uint32 newObjectID, Sint8 currentPosition) {
	Sint8 i = currentPosition;

	if(currentPosition == -1) {
		bool used[NUM_INFANTRY_PER_TILE];
		int pos;
		for (i = 0; i < NUM_INFANTRY_PER_TILE; i++)
			used[i] = false;


		std::list<Uint32>::const_iterator iter;
		for(iter = assignedInfantryList.begin(); iter != assignedInfantryList.end() ;++iter) {
			InfantryBase* infant = (InfantryBase*) currentGame->getObjectManager().getObject(*iter);
			if(infant == NULL) {
				continue;
			}

			pos = infant->getTilePosition();
			if ((pos >= 0) && (pos < NUM_INFANTRY_PER_TILE))
				used[pos] = true;
		}

		for (i = 0; i < NUM_INFANTRY_PER_TILE; i++) {
			if (used[i] == false) {
				break;
			}
		}

		if ((i < 0) || (i >= NUM_INFANTRY_PER_TILE))
			i = 0;
	}

	assignedInfantryList.push_back(newObjectID);
	return i;
}


void Tile::assignUndergroundUnit(Uint32 newObjectID) {
	assignedUndergroundUnitList.push_back(newObjectID);
}

void Tile::blitGround(int xPos, int yPos) {
	SDL_Rect	source = { getTerrainTile()*world2zoomedWorld(TILESIZE), 0, world2zoomedWorld(TILESIZE), world2zoomedWorld(TILESIZE) };
	SDL_Rect    drawLocation = { xPos, yPos, world2zoomedWorld(TILESIZE), world2zoomedWorld(TILESIZE) };

	if((hasANonInfantryGroundObject() == false) || (getNonInfantryGroundObject()->isAStructure() == false)) {

		//draw terrain
		if(destroyedStructureTile == DestroyedStructure_None || destroyedStructureTile == DestroyedStructure_Wall) {
            SDL_BlitSurface(sprite[currentZoomlevel], &source, screen, &drawLocation);
		}

		if(destroyedStructureTile != DestroyedStructure_None) {
		    SDL_Surface** pDestroyedStructureSurface = pGFXManager->getObjPic(ObjPic_DestroyedStructure);
		    SDL_Rect source2 = { destroyedStructureTile*world2zoomedWorld(TILESIZE), 0, world2zoomedWorld(TILESIZE), world2zoomedWorld(TILESIZE) };
            SDL_BlitSurface(pDestroyedStructureSurface[currentZoomlevel], &source2, screen, &drawLocation);
		}

		if(!isFogged(pLocalHouse->getHouseID())) {
		    // tracks
		    for(int i=0;i<NUM_ANGLES;i++) {
                if(tracksCounter[i] > 0) {
                    source.x = ((10-i)%8)*world2zoomedWorld(TILESIZE);
                    SDL_BlitSurface(pGFXManager->getObjPic(ObjPic_Terrain_Tracks)[currentZoomlevel], &source, screen, &drawLocation);
                }
		    }

            // damage
		    for(std::vector<DAMAGETYPE>::const_iterator iter = damage.begin(); iter != damage.end(); ++iter) {
                source.x = iter->tile*world2zoomedWorld(TILESIZE);
                SDL_Rect dest = {   screenborder->world2screenX(iter->realPos.x) - world2zoomedWorld(TILESIZE)/2,
                                    screenborder->world2screenY(iter->realPos.y) - world2zoomedWorld(TILESIZE)/2,
                                    world2zoomedWorld(TILESIZE),
                                    world2zoomedWorld(TILESIZE) };

                if(iter->damageType == Terrain_RockDamage) {
                    SDL_BlitSurface(pGFXManager->getObjPic(ObjPic_RockDamage)[currentZoomlevel], &source, screen, &dest);
                } else {
                    SDL_BlitSurface(pGFXManager->getObjPic(ObjPic_SandDamage)[currentZoomlevel], &source, screen, &drawLocation);
                }
		    }
		}
	}

}

void Tile::blitStructures(int xPos, int yPos) {
	if (hasANonInfantryGroundObject() && getNonInfantryGroundObject()->isAStructure()) {
		//if got a structure, draw the structure, and dont draw any terrain because wont be seen
		bool	done = false;	//only draw it once
		StructureBase* structure = (StructureBase*) getNonInfantryGroundObject();

		for(int i = structure->getX(); (i < structure->getX() + structure->getStructureSizeX()) && !done;  i++) {
            for(int j = structure->getY(); (j < structure->getY() + structure->getStructureSizeY()) && !done;  j++) {
                if(screenborder->isTileInsideScreen(Coord(i,j))
                    && currentGameMap->tileExists(i, j) && (currentGameMap->getTile(i, j)->isExplored(pLocalHouse->getHouseID()) || debug))
                {

                    structure->setFogged(isFogged(pLocalHouse->getHouseID()));

                    if ((i == location.x) && (j == location.y)) {
                        //only this tile will draw it, so will be drawn only once
                        structure->blitToScreen();
                    }

                    done = true;
                }
            }
        }
	}
}

void Tile::blitUndergroundUnits(int xPos, int yPos) {
	if(hasAnUndergroundUnit() && !isFogged(pLocalHouse->getHouseID())) {
	    UnitBase* current = getUndergroundUnit();

	    if(current->isVisible(pLocalHouse->getTeam())) {
		    if(location == current->getLocation()) {
                current->blitToScreen();
		    }
		}
	}
}

void Tile::blitDeadUnits(int xPos, int yPos) {
	if(!isFogged(pLocalHouse->getHouseID())) {
	    for(std::vector<DEADUNITTYPE>::const_iterator iter = deadUnits.begin(); iter != deadUnits.end(); ++iter) {
	        SDL_Rect source = { 0, 0, world2zoomedWorld(TILESIZE), world2zoomedWorld(TILESIZE)};
	        SDL_Surface** pSurface = NULL;
	        switch(iter->type) {
                case DeadUnit_Infantry: {
                    pSurface = pGFXManager->getObjPic(ObjPic_DeadInfantry, iter->house);
                    source.x = (iter->timer < 1000 && iter->onSand) ? world2zoomedWorld(TILESIZE) : 0;
                } break;

                case DeadUnit_Infantry_Squashed1: {
                    pSurface = pGFXManager->getObjPic(ObjPic_DeadInfantry, iter->house);
                    source.x = 4 * world2zoomedWorld(TILESIZE);
                } break;

                case DeadUnit_Infantry_Squashed2: {
                    pSurface = pGFXManager->getObjPic(ObjPic_DeadInfantry, iter->house);
                    source.x = 5 * world2zoomedWorld(TILESIZE);
                } break;

                case DeadUnit_Carrall: {
                    pSurface = pGFXManager->getObjPic(ObjPic_DeadAirUnit, iter->house);
                    if(iter->onSand) {
                        source.x = (iter->timer < 1000) ? 5*world2zoomedWorld(TILESIZE) : 4*world2zoomedWorld(TILESIZE);
                    } else {
                        source.x = 3*world2zoomedWorld(TILESIZE);
                    }
                } break;

                case DeadUnit_Ornithopter: {
                    pSurface = pGFXManager->getObjPic(ObjPic_DeadAirUnit, iter->house);
                    if(iter->onSand) {
                        source.x = (iter->timer < 1000) ? 2*world2zoomedWorld(TILESIZE) : world2zoomedWorld(TILESIZE);
                    } else {
                        source.x = 0;
                    }
                } break;

                default: {
                    pSurface = NULL;
                } break;
	        }

	        if(pSurface != NULL) {
                SDL_Rect dest = {   screenborder->world2screenX(iter->realPos.x) - world2zoomedWorld(TILESIZE)/2,
                                    screenborder->world2screenY(iter->realPos.y) - world2zoomedWorld(TILESIZE)/2,
                                    pSurface[currentZoomlevel]->w,
                                    pSurface[currentZoomlevel]->h };
                SDL_BlitSurface(pSurface[currentZoomlevel], &source, screen, &dest);
	        }
	    }
	}
}

void Tile::blitInfantry(int xPos, int yPos) {
	if(hasInfantry() && !isFogged(pLocalHouse->getHouseID())) {
		std::list<Uint32>::const_iterator iter;
		for(iter = assignedInfantryList.begin(); iter != assignedInfantryList.end() ;++iter) {
			InfantryBase* current = (InfantryBase*) currentGame->getObjectManager().getObject(*iter);

			if(current == NULL) {
				continue;
			}

			if(current->isVisible(pLocalHouse->getTeam())) {
			    if(location == current->getLocation()) {
                    current->blitToScreen();
			    }
			}
		}
	}
}

void Tile::blitNonInfantryGroundUnits(int xPos, int yPos) {
	if(hasANonInfantryGroundObject() && !isFogged(pLocalHouse->getHouseID())) {
        std::list<Uint32>::const_iterator iter;
		for(iter = assignedNonInfantryGroundObjectList.begin(); iter != assignedNonInfantryGroundObjectList.end() ;++iter) {
			ObjectBase* current =  currentGame->getObjectManager().getObject(*iter);

            if(current->isAUnit() && current->isVisible(pLocalHouse->getTeam())) {
                if(location == current->getLocation()) {
                    current->blitToScreen();
                }
            }
		}
	}
}


void Tile::blitAirUnits(int xPos, int yPos) {
	if(hasAnAirUnit()) {
		std::list<Uint32>::const_iterator iter;
		for(iter = assignedAirUnitList.begin(); iter != assignedAirUnitList.end() ;++iter) {
			AirUnit* airUnit = (AirUnit*) currentGame->getObjectManager().getObject(*iter);

			if(airUnit == NULL) {
				continue;
			}

			if(!isFogged(pLocalHouse->getHouseID()) || airUnit->getOwner() == pLocalHouse) {
                if(airUnit->isVisible(pLocalHouse->getTeam())) {
                    if(location == airUnit->getLocation()) {
                        airUnit->blitToScreen();
                    }
                }
			}
		}
	}
}

void Tile::drawLight(Uint32 color) {
	int x,y;

		x  = screenborder->world2screenX((this->getLocation().x*TILESIZE));
		y  = screenborder->world2screenY((this->getLocation().y*TILESIZE));


		int zoomedTileSize = world2zoomedWorld(TILESIZE);
		//int fogTile = pTile->getFogTile(pLocalHouse->getHouseID());
		//int fogTile = Terrain_HiddenFull;
		int fogTile = Terrain_HiddenFull;


		 SDL_Rect source = { fogTile*zoomedTileSize, 0,
							zoomedTileSize, zoomedTileSize };
		SDL_Rect drawLocation = {   x, y,
									zoomedTileSize, zoomedTileSize };

		SDL_Rect mini = {0, 0, 1, 1};

		SDL_Rect drawLoc = {drawLocation.x, drawLocation.y, 0, 0};
		SDL_Surface** hiddenSurf = pGFXManager->getObjPic(ObjPic_Terrain_Hidden);

		SDL_Surface* fogSurf = pGFXManager->getTransparent150Surface();

		if(!SDL_MUSTLOCK(screen) || (SDL_LockSurface(screen) == 0)) {
			SDL_LockSurface(hiddenSurf[currentZoomlevel]);

		for(int i=0;i<zoomedTileSize; i++) {
			for(int j=0;j<zoomedTileSize; j++) {
				if(getPixel(hiddenSurf[currentZoomlevel],source.x+i,source.y+j) == 12 ) {
					drawLoc.x = drawLocation.x + i;
					drawLoc.y = drawLocation.y + j;
					//fogSurf = mapSurfaceColorRange(fogSurf, 12, color);
					SDL_BlitSurface(fogSurf,&mini,screen,&drawLoc);
				}
			}
		}

				//SDL_BlitSurface(fogSurf,&source2,screen,&drawLoc);



			SDL_UnlockSurface(hiddenSurf[currentZoomlevel]);
		}


		if(SDL_MUSTLOCK(screen)) {
			SDL_UnlockSurface(screen);
		}



}

void Tile::drawOverlay(ObjectBase* obj, Uint32 color, Tile* tile) {

	// XXX




	int x,y;

	if (obj != NULL) {
		x  = screenborder->world2screenX((obj->getLocation().x*TILESIZE));
		y  = screenborder->world2screenY((obj->getLocation().y*TILESIZE));
	} else if (tile != NULL){
		x  = screenborder->world2screenX(((tile->getLocation()).x*TILESIZE));
		y  = screenborder->world2screenY(((tile->getLocation()).y*TILESIZE));
	} else {
		fprintf(stderr,"Tile::drawOverlay : no object or tile given !\n");
		return;
	}

	int zoomedTileSize = world2zoomedWorld(TILESIZE);
	//int fogTile = pTile->getFogTile(pLocalHouse->getHouseID());
	//int fogTile = Terrain_HiddenFull;
	int fogTile = Terrain_HiddenIsland;


	 SDL_Rect source = { fogTile*zoomedTileSize, 0,
						zoomedTileSize, zoomedTileSize };
	SDL_Rect drawLocation = {   x, y,
								zoomedTileSize, zoomedTileSize };

	SDL_Rect mini = {0, 0, 1, 1};


	/*
	SDL_Rect source2 ;

	if (obj->isAUnit())
		source2 = { obj->getDrawnAngle() * obj->getGraphic()[currentZoomlevel]->w/obj->getNumImagesX(), 0, world2zoomedWorld(TILESIZE), world2zoomedWorld(TILESIZE)};
	else
		source2 = { 0, 0, world2zoomedWorld(TILESIZE), world2zoomedWorld(TILESIZE)};
	*/

	SDL_Rect drawLoc = {drawLocation.x, drawLocation.y, 0, 0};





	int objcolor;

	if (obj != NULL) {
		switch ((obj->getOwner())->getHouseID())
		{
			case HOUSE_HARKONNEN:
				objcolor = COLOR_HARKONNEN;
				break;
			case HOUSE_ATREIDES:
				objcolor = COLOR_ATREIDES;
				break;
			case HOUSE_ORDOS:
				objcolor = COLOR_ORDOS;
				break;
			case HOUSE_FREMEN:
				objcolor = COLOR_FREMEN;
				break;
			case HOUSE_SARDAUKAR:
				objcolor = COLOR_SARDAUKAR;
				break;
			case HOUSE_MERCENARY:
				objcolor = COLOR_MERCENARY;
				break;
			default:
				objcolor = 12;
				break;
		}
	}

	color = COLOR_LIGHTRED;
	SDL_Surface** hiddenSurf = pGFXManager->getObjPic(ObjPic_Terrain_Hidden);

	SDL_Surface* fogSurf = // obj->getGraphic()[currentZoomlevel] ;
			pGFXManager->getTransparent150Surface();

				//mapSurfaceColorRange(obj->getGraphic()[currentZoomlevel],objcolor, color);


	//mapSurfaceColorRange(resolveItemPicture(obj->getItemID(),(HOUSETYPE)(obj->getOwner())->getHouseID()),objcolor , color);



   // fogSurf = mapSurfaceColorRange(fogSurf, objcolor, color);
  //  replaceColor(*hiddenSurf, objcolor, objcolor << 8);


	if(!SDL_MUSTLOCK(screen) || (SDL_LockSurface(screen) == 0)) {
		SDL_LockSurface(hiddenSurf[currentZoomlevel]);

	for(int i=0;i<zoomedTileSize; i++) {
		for(int j=0;j<zoomedTileSize; j++) {
			if(getPixel(hiddenSurf[currentZoomlevel],source.x+i,source.y+j) == 12 ) {
				drawLoc.x = drawLocation.x + i;
				drawLoc.y = drawLocation.y + j;
				//fogSurf = mapSurfaceColorRange(fogSurf, 12, color);
				SDL_BlitSurface(fogSurf,&mini,screen,&drawLoc);
			}
		}
	}

			//SDL_BlitSurface(fogSurf,&source2,screen,&drawLoc);



		SDL_UnlockSurface(hiddenSurf[currentZoomlevel]);
	}


	if(SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}


}

void Tile::drawRallyPoint(ObjectBase* obj, Uint32 color, Coord size) {

 if (obj->getDestination().isValid() ) {
	SDL_Surface* rally;

	   switch(currentZoomlevel) {
			case 0:     rally = pGFXManager->getUIGraphic(UI_CursorMove_Zoomlevel0); break;
			case 1:     rally = pGFXManager->getUIGraphic(UI_CursorMove_Zoomlevel1); break;
			case 2:
			default:    rally = pGFXManager->getUIGraphic(UI_CursorMove_Zoomlevel2); break;
		}

	int x,y, origx,origy;

	if (obj->getDestination().isValid()) {
		x  = screenborder->world2screenX((obj->getDestination().x*TILESIZE));
		y  = screenborder->world2screenY((obj->getDestination().y*TILESIZE));
	} else {
		x  = screenborder->world2screenX((obj->getX()*TILESIZE));
		y  = screenborder->world2screenY((obj->getY()*TILESIZE));
	}

	origx = screenborder->world2screenX( (obj->getX()*TILESIZE) + (size.x*TILESIZE/2) );
	origy = screenborder->world2screenY( (obj->getY()*TILESIZE) + (size.y*TILESIZE/2) );

	SDL_Rect dest;
	dest.x = x;
	dest.y = y;
	dest.w = rally->w;
	dest.h = rally->h;

	//dbg_relax_print("Dest  %d,%d,%d,%d %d,%d\n", dest.x, dest.y, dest.w, dest.h, origx, origy);

	//now draw the selection box thing, with parts at all corners of structure
	if(!SDL_MUSTLOCK(screen) || (SDL_LockSurface(screen) == 0)) {

		   // top left bit
		   for(int i=0;i<=currentZoomlevel;i++) {
			   drawHLineNoLock(screen,dest.x+i, dest.y+i, dest.x+(currentZoomlevel+1)*2, color);
			   drawVLineNoLock(screen,dest.x+i, dest.y+i, dest.y+(currentZoomlevel+1)*2, color);
		   }

		   // top right bit
		   for(int i=0;i<=currentZoomlevel;i++) {
			   drawHLineNoLock(screen,dest.x + dest.w-1 - i, dest.y+i, dest.x + dest.w-1 - (currentZoomlevel+1)*2, color);
			   drawVLineNoLock(screen,dest.x + dest.w-1 - i, dest.y+i, dest.y+(currentZoomlevel+1)*2, color);
		   }

		   // bottom left bit
		   for(int i=0;i<=currentZoomlevel;i++) {
			   drawHLineNoLock(screen,dest.x+i, dest.y + dest.h-1 - i, dest.x+(currentZoomlevel+1)*2, color);
			   drawVLineNoLock(screen,dest.x+i, dest.y + dest.h-1 - i, dest.y + dest.h-1 - (currentZoomlevel+1)*2, color);
		   }

		   // bottom right bit
		   for(int i=0;i<=currentZoomlevel;i++) {
			   drawHLineNoLock(screen,dest.x + dest.w-1 - i, dest.y + dest.h-1 - i, dest.x + dest.w-1 - (currentZoomlevel+1)*2, color);
			   drawVLineNoLock(screen,dest.x + dest.w-1 - i, dest.y + dest.h-1 - i, dest.y + dest.h-1 - (currentZoomlevel+1)*2, color);
		   }
	}



   for (int i=-2; i<3; i++) {

		drawLineNoLock(screen, origx+i, origy, dest.x + (rally->w / 2), dest.y + (rally->h / 2), color);

   }
   SDL_BlitSurface(rally,NULL,screen,&dest);


	if(SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}
 }

}


void Tile::blitSelectionRects(int xPos, int yPos) {
    // draw underground selection rectangles
    if(hasAnUndergroundUnit() && !isFogged(pLocalHouse->getHouseID())) {
	    UnitBase* current = getUndergroundUnit();

        if(current != NULL) {
            if(current->isVisible(pLocalHouse->getTeam()) && (location == current->getLocation())) {
                if(current->isSelected()) {
                    current->drawSelectionBox();
                }

                if(current->isSelectedByOtherPlayer()) {
                    current->drawOtherPlayerSelectionBox();
                }
            }
        }
	}


    // draw infantry selection rectangles
    if(hasInfantry()) {
		std::list<Uint32>::const_iterator iter;
		for(iter = assignedInfantryList.begin(); iter != assignedInfantryList.end() ;++iter) {
			InfantryBase* current = dynamic_cast<InfantryBase*>(currentGame->getObjectManager().getObject(*iter));

			if(current == NULL) {
				continue;
			}

            if(current->isVisible(pLocalHouse->getTeam()) && (location == current->getLocation())
            		&& ( !isFogged(pLocalHouse->getHouseID()) || current->getOwner() == pLocalHouse ) ) {
                if(current->isSelected()) {
                    current->drawSelectionBox();
                }

                if(current->isSelectedByOtherPlayer()) {
                    current->drawOtherPlayerSelectionBox();
                }
            }
		}
	}

    // draw non infantry ground object selection rectangles
	if(hasANonInfantryGroundObject()) {
	    std::list<Uint32>::const_iterator iter;
		for(iter = assignedNonInfantryGroundObjectList.begin(); iter != assignedNonInfantryGroundObjectList.end() ;++iter) {
            ObjectBase* current = currentGame->getObjectManager().getObject(*iter);

            if(current == NULL) {
				continue;
			}

            if(current->isVisible(pLocalHouse->getTeam()) && (location == current->getLocation())
            		&& ( !isFogged(pLocalHouse->getHouseID()) || current->getOwner() == pLocalHouse ) ) {
                if(current->isSelected()) {
                    current->drawSelectionBox();
                }

                if(current->isSelectedByOtherPlayer()) {
                    current->drawOtherPlayerSelectionBox();
                }


                // Draw rally point for structures
			   if (current != NULL &&  current->isSelected() && current->getOwner() == pLocalHouse && current->isAStructure()) {
				   StructureBase* pStruct = static_cast<StructureBase*>(current);
				   if (pStruct->isUnitProducer())
					   drawRallyPoint(current,houseColor[pLocalHouse->getHouseID()],pStruct->getStructureSize());
			   }


            }
		}
	}

    // draw air unit selection rectangles
	if(hasAnAirUnit()) {
		std::list<Uint32>::const_iterator iter;
		for(iter = assignedAirUnitList.begin(); iter != assignedAirUnitList.end() ;++iter) {
			AirUnit* airUnit = dynamic_cast<AirUnit*>(currentGame->getObjectManager().getObject(*iter));

			if(airUnit == NULL) {
				continue;
			}

            if(airUnit->isVisible(pLocalHouse->getTeam()) && (location == airUnit->getLocation())
            		&& ( !isFogged(pLocalHouse->getHouseID()) || airUnit->getOwner() == pLocalHouse )) {
                if(airUnit->isSelected()) {
                    airUnit->drawSelectionBox();
                }

                if(airUnit->isSelectedByOtherPlayer()) {
                    airUnit->drawOtherPlayerSelectionBox();
                }
            }
		}
	}
}


void Tile::clearTerrain() {
    damage.clear();
    deadUnits.clear();
}


void Tile::selectAllPlayersUnits(int houseID, ObjectBase** lastCheckedObject, ObjectBase** lastSelectedObject, ObjectBase* groupLeader) {
	ConcatIterator<Uint32> iterator;

	std::list<Uint32>::const_iterator iter;
	iterator.addList(assignedInfantryList);
	iterator.addList(assignedNonInfantryGroundObjectList);
	iterator.addList(assignedUndergroundUnitList);
	iterator.addList(assignedAirUnitList);
	UnitBase* unit;

	while(!iterator.isIterationFinished()) {
		*lastCheckedObject = currentGame->getObjectManager().getObject(*iterator);
		unit = dynamic_cast<UnitBase*>(currentGame->getObjectManager().getObject(*iterator));

		if (((*lastCheckedObject)->getOwner()->getHouseID() == houseID)
			&& !(*lastCheckedObject)->isSelected()
			&& (*lastCheckedObject)->isAUnit()
			&& ((*lastCheckedObject)->isRespondable()))	{


			if (groupLeader == NULL && !unit->isFollowing()) {
				currentGame->setGroupLeader (*lastCheckedObject);
				groupLeader = *lastCheckedObject;
				dynamic_cast<UnitBase*>(*lastCheckedObject)->setLeader(true);
				err_print("Tile::selectAllPlayersUnits obj:%d Become group leader (f:%s %d)\n",
						(*lastCheckedObject)->getObjectID(), unit->isFollowing() ? "yes" : "no",
								unit->isFollowing() ? unit->getFellow()->getItemID() : 0);
			} else if (groupLeader == (unit)  ) {
				if (!unit->isFollowing()) {
					currentGame->setGroupLeader (*lastCheckedObject);
					dynamic_cast<UnitBase*>(*lastCheckedObject)->setLeader(true);
					groupLeader = *lastCheckedObject;
					err_print("Tile::selectAllPlayersUnits obj:%d is group leader\n",(*lastCheckedObject)->getObjectID());
				}
				else {
					err_print("Tile::selectAllPlayersUnits obj:%d a group leader CAN'T BE FOLLOWING\n",(*lastCheckedObject)->getObjectID());
					dynamic_cast<UnitBase*>(*lastCheckedObject)->setLeader(false);
					currentGame->setGroupLeader(NULL);
				}
			}
			else {
				dynamic_cast<UnitBase*>(*lastCheckedObject)->setLeader(false);
			}


			(*lastCheckedObject)->setSelected(true);
			currentGame->getSelectedList().push_back((*lastCheckedObject)->getObjectID());

			if (groupLeader != NULL) {
				currentGame->getSelectedListCoord().push_back(std::make_pair<Uint32,Coord> ( (*lastCheckedObject)->getObjectID(), (*lastCheckedObject)->getLocation() - (groupLeader)->getLocation() ) );
				err_print("Tile::selectAllPlayersUnits (o:%d) Obj:%d %d,%d \n",(groupLeader)->getObjectID(), (*lastCheckedObject)->getObjectID(),((*lastCheckedObject)->getLocation() - (groupLeader)->getLocation()).x , ((*lastCheckedObject)->getLocation() - (groupLeader)->getLocation()).y);
			}

			currentGame->selectionChanged();
			*lastSelectedObject = *lastCheckedObject;
		}

		++iterator;

	}
}


void Tile::selectAllPlayersUnitsOfType(int houseID, ObjectBase* lastSinglySelectedObject, ObjectBase** lastCheckedObject, ObjectBase** lastSelectedObject, ObjectBase* groupLeader) {
	ConcatIterator<Uint32> iterator;
	iterator.addList(assignedInfantryList);
	iterator.addList(assignedNonInfantryGroundObjectList);
	iterator.addList(assignedUndergroundUnitList);
	iterator.addList(assignedAirUnitList);
	UnitBase* unit, *sunit;
    int itemid = lastSinglySelectedObject->getItemID();

	while(!iterator.isIterationFinished()) {
		*lastCheckedObject = currentGame->getObjectManager().getObject(*iterator);
		unit = dynamic_cast<UnitBase*>(currentGame->getObjectManager().getObject(*iterator));
		sunit = dynamic_cast<UnitBase*>(lastSinglySelectedObject);
		if (((*lastCheckedObject)->getOwner()->getHouseID() == houseID)
			&& !(*lastCheckedObject)->isSelected()
			&& ((*lastCheckedObject)->getItemID() == itemid)
			&& ((*lastCheckedObject)->isRespondable())
			&& ( unit->isSalving() == sunit->isSalving() ) ) {

			if (groupLeader == NULL && !unit->isFollowing()) {
				currentGame->setGroupLeader (*lastCheckedObject);
				dynamic_cast<UnitBase*>(*lastCheckedObject)->setLeader(true);
				groupLeader = *lastCheckedObject;
				err_print("Tile::selectAllPlayersUnitsOfType obj:%d promoted to group leader\n",(*lastCheckedObject)->getObjectID());
			} else if (groupLeader == (unit)  ) {
					if (!unit->isFollowing()) {
						currentGame->setGroupLeader (*lastCheckedObject);
						dynamic_cast<UnitBase*>(*lastCheckedObject)->setLeader(true);
						groupLeader = *lastCheckedObject;
						err_print("Tile::selectAllPlayersUnitsOfType obj:%d is group leader\n",(*lastCheckedObject)->getObjectID());
					}
					else {
						err_print("Tile::selectAllPlayersUnitsOfType obj:%d a group leader CAN'T BE FOLLOWING\n",(*lastCheckedObject)->getObjectID());
						dynamic_cast<UnitBase*>(*lastCheckedObject)->setLeader(false);
						currentGame->setGroupLeader(NULL);
						// FIXME : can be finishing iteration and not having a leader !
					}
			}
			else {
				dynamic_cast<UnitBase*>(*lastCheckedObject)->setLeader(false);
			}


			(*lastCheckedObject)->setSelected(true);
			currentGame->getSelectedList().push_back((*lastCheckedObject)->getObjectID());
			currentGame->getSelectedListCoord().push_back(std::make_pair<Uint32,Coord> ((*lastCheckedObject)->getObjectID(), ((*lastCheckedObject)->getLocation() - lastSinglySelectedObject->getLocation())) );
			err_print("Tile::selectAllPlayersUnitsOfType obj:%d %d,%d \n", (*lastCheckedObject)->getObjectID(),((*lastCheckedObject)->getLocation() - lastSinglySelectedObject->getLocation()).x , ((*lastCheckedObject)->getLocation() - lastSinglySelectedObject->getLocation()).y);
			currentGame->selectionChanged();
			*lastSelectedObject = *lastCheckedObject;
		}
		++iterator;
	}
	//dbg_print("Tile::selectAllPlayersUnitsOfType size <%d,%d>\n",currentGame->getSelectedList().size(),currentGame->getSelectedListCoord().size());
}



void Tile::unassignAirUnit(Uint32 objectID) {
	assignedAirUnitList.remove(objectID);
}


void Tile::unassignNonInfantryGroundObject(Uint32 objectID) {
	assignedNonInfantryGroundObjectList.remove(objectID);
}

void Tile::unassignUndergroundUnit(Uint32 objectID) {
	assignedUndergroundUnitList.remove(objectID);
}

void Tile::unassignInfantry(Uint32 objectID, int currentPosition) {
	assignedInfantryList.remove(objectID);
}

void Tile::unassignObject(Uint32 objectID) {
	unassignInfantry(objectID,-1);
	unassignUndergroundUnit(objectID);
	unassignNonInfantryGroundObject(objectID);
	unassignAirUnit(objectID);
}


void Tile::setType(int newType, bool resetSpice) {
	type = newType;
	destroyedStructureTile = DestroyedStructure_None;

	if (type == Terrain_Spice) {
		if (!resetSpice) {
			float oldspice=spice;
			spice += currentGame->randomGen.rand(RANDOMSPICEMIN, RANDOMSPICEMAX);
			dbg_print("Tile::setType spice %0.1f %0.1f _\n",oldspice,spice);
			if (spice > RANDOMSPICEMAX) {
				type = Terrain_ThickSpice;
			}
		} else {
			spice = currentGame->randomGen.rand(RANDOMSPICEMIN, RANDOMSPICEMAX);
		}
	} else if (type == Terrain_ThickSpice) {
		if (!resetSpice) {
			spice += currentGame->randomGen.rand(RANDOMTHICKSPICEMIN, RANDOMTHICKSPICEMAX);
		} else {
			spice = currentGame->randomGen.rand(RANDOMTHICKSPICEMIN, RANDOMTHICKSPICEMAX);
		}
	} else if (type == Terrain_Dunes) {
	} else {
		spice = 0;
		if (isRock()) {
			sandRegion = NONE;
			if (hasAnUndergroundUnit())	{
				ObjectBase* current;
				std::list<Uint32>::const_iterator iter;
				iter = assignedUndergroundUnitList.begin();

				do {
					current = currentGame->getObjectManager().getObject(*iter);
					++iter;

					if(current == NULL)
						continue;

					unassignUndergroundUnit(current->getObjectID());
					current->destroy();
				} while(iter != assignedUndergroundUnitList.end());
			}

			if(type == Terrain_Mountain) {
				if(hasANonInfantryGroundObject()) {
					ObjectBase* current;
					std::list<Uint32>::const_iterator iter;
					iter = assignedNonInfantryGroundObjectList.begin();

					do {
						current = currentGame->getObjectManager().getObject(*iter);
						++iter;

						if(current == NULL)
							continue;

						unassignNonInfantryGroundObject(current->getObjectID());
						current->destroy();
					} while(iter != assignedNonInfantryGroundObjectList.end());
				}
			}
		}
	}

	for (int i=location.x; i <= location.x+3; i++) {
		for (int j=location.y; j <= location.y+3; j++) {
			if (currentGameMap->tileExists(i, j)) {
				currentGameMap->getTile(i, j)->clearTerrain();
			}
		}
	}
}


void Tile::squash() {
	if(hasInfantry()) {
		InfantryBase* current;
		std::list<Uint32>::const_iterator iter;
		iter = assignedInfantryList.begin();

		do {
			current = (InfantryBase*) currentGame->getObjectManager().getObject(*iter);
			++iter;

			if(current == NULL)
				continue;

			current->squash();
		} while(iter != assignedInfantryList.end());
	}
}


int Tile::getInfantryTeam() {
	int team = NONE;
	if (hasInfantry())
		team = getInfantry()->getOwner()->getTeam();
	return team;
}

int Tile::getExtractionSpeed () {

	float speed 	= hasSpice() ? getSpiceExtractionSpeed(false,true): 0;
	float ref_speed = hasSpice() ? getSpiceExtractionSpeed(true,true): 1;
	return (int)((speed/ref_speed)*100);
}

float Tile::getSpiceExtractionSpeed(bool nobonus, bool deepharvest) {
	float extractionspeed = deepharvest ? HARVESTSPEED : HARVESTALONGSPEED ;
	// XXX : Should be made a tech-option
	if (true && !nobonus) {
		if (spice >= RANDOMTHICKSPICEMIN && spice <= RANDOMTHICKSPICEMAX) {
			// thick spice [148-296] : high density
			extractionspeed *= 0.500f + (spice/((RANDOMTHICKSPICEMAX)/2));
		} else if (spice >= RANDOMSPICEMIN  && spice < RANDOMTHICKSPICEMIN) {
			// regular spice [74-148] : normal density
			extractionspeed *= 0.250f + (spice/((RANDOMTHICKSPICEMAX)/2));
		} else if (spice > 0.0f  && spice < RANDOMSPICEMIN) {
			// lower density [ 0.1 - 74] : lower density
			extractionspeed *= 0.125f + (spice/(RANDOMTHICKSPICEMAX/2));
		}
	}

	return (extractionspeed < 0.01f ? 0.01f : extractionspeed);
}

float Tile::harvestSpice(float extractionspeed) {
	float oldSpice = spice;

	if((spice - extractionspeed) >= 0.0f) {
		spice -= extractionspeed;
	} else {
		spice = 0.0f;
	}

    if(oldSpice >= RANDOMTHICKSPICEMIN && spice < RANDOMTHICKSPICEMIN) {
        setType(Terrain_Spice);
    }

    if(oldSpice > 0.0f && spice == 0.0f) {
        setType(Terrain_Sand);
    }

	return (oldSpice - spice);
}


void Tile::setSpice(float newSpice) {
	if(newSpice <= 0.0f) {
		type = Terrain_Sand;
	} else if(newSpice >= RANDOMTHICKSPICEMIN) {
		type = Terrain_ThickSpice;
	} else {
		type = Terrain_Spice;
	}
	spice = newSpice;
}


AirUnit* Tile::getAirUnit() {
	return dynamic_cast<AirUnit*>(currentGame->getObjectManager().getObject(assignedAirUnitList.front()));
}

ObjectBase* Tile::getGroundObject() {
	if (hasANonInfantryGroundObject())
		return getNonInfantryGroundObject();
	else if (hasInfantry())
		return getInfantry();
	else
		return NULL;
}

bool Tile::isInfantryPacked() {
	if (hasInfantry()) {
		return assignedInfantryList.size() >= 5 ? true : false;
	}
	else return false;
}

InfantryBase* Tile::getInfantry() {
	return dynamic_cast<InfantryBase*>(currentGame->getObjectManager().getObject(assignedInfantryList.front()));
}

ObjectBase* Tile::getNonInfantryGroundObject() {
	return currentGame->getObjectManager().getObject(assignedNonInfantryGroundObjectList.front());
}

UnitBase* Tile::getUndergroundUnit() {
	return dynamic_cast<UnitBase*>(currentGame->getObjectManager().getObject(assignedUndergroundUnitList.front()));
}


ObjectBase* Tile::getObject() {
	ObjectBase* temp = NULL;
	if (hasAnAirUnit())
		temp = getAirUnit();
	else if (hasANonInfantryGroundObject())
		temp = getNonInfantryGroundObject();
	else if (hasInfantry())
		temp = getInfantry();
	else if (hasAnUndergroundUnit())
		temp = getUndergroundUnit();
	return temp;
}


ObjectBase* Tile::getObjectAt(int x, int y) {
	ObjectBase* temp = NULL;
	if (hasAnAirUnit())
		temp = getAirUnit();
	else if (hasANonInfantryGroundObject())
		temp = getNonInfantryGroundObject();
	else if (hasInfantry())	{
		float closestDistance = INFINITY;
		Coord atPos, centerPoint;
		InfantryBase* infantry;
		atPos.x = x;
		atPos.y = y;

		std::list<Uint32>::const_iterator iter;
		for(iter = assignedInfantryList.begin(); iter != assignedInfantryList.end() ;++iter) {
			infantry = (InfantryBase*) currentGame->getObjectManager().getObject(*iter);
			if(infantry == NULL)
				continue;

			centerPoint = infantry->getCenterPoint();
			if(distanceFrom(atPos, centerPoint) < closestDistance) {
				closestDistance = distanceFrom(atPos, centerPoint);
				temp = infantry;
			}
		}
	}
	else if (hasAnUndergroundUnit())
		temp = getUndergroundUnit();

	return temp;
}


ObjectBase* Tile::getObjectWithID(Uint32 objectID) {
	ConcatIterator<Uint32> iterator;
	iterator.addList(assignedInfantryList);
	iterator.addList(assignedNonInfantryGroundObjectList);
	iterator.addList(assignedUndergroundUnitList);
	iterator.addList(assignedAirUnitList);

	while(!iterator.isIterationFinished()) {
		if(*iterator == objectID) {
			return currentGame->getObjectManager().getObject(*iterator);
		}
		++iterator;
	}

	return NULL;
}

void Tile::triggerSpiceBloom(House* pTrigger) {
    if (isSpiceBloom()) {
        //a spice bloom
        soundPlayer->playSoundAt(Sound_Bloom, getLocation());
        screenborder->shakeScreen(18);
        if(pTrigger == pLocalHouse) {
            soundPlayer->playVoice(BloomLocated, pLocalHouse->getHouseID());
        }

        setType(Terrain_Spice); // Set this tile to spice first
        currentGameMap->createSpiceField(location, 5,false,true,true);

        Coord realLocation = location*TILESIZE + Coord(TILESIZE/2, TILESIZE/2);

        if(damage.size() < DAMAGE_PER_TILE) {
            DAMAGETYPE newDamage;
            newDamage.tile = SandDamage1;
            newDamage.damageType = Terrain_SandDamage;
            newDamage.realPos = realLocation;

            damage.push_back(newDamage);
        }

        currentGame->getExplosionList().push_back(new Explosion(Explosion_SpiceBloom,realLocation,pTrigger->getHouseID()));
    }
}

void Tile::triggerSpecialBloom(House* pTrigger) {
    if(isSpecialBloom()) {
        setType(Terrain_Sand);

        switch(currentGame->randomGen.rand(0,3)) {
            case 0: {
                // the player gets an randomly choosen amount of credits between 150 and 400
                pTrigger->addCredits(currentGame->randomGen.rand(150, 400),false);
            } break;

            case 1: {
                // The house gets a Trike for free. It spawns beside the special bloom.
                UnitBase* pNewUnit = pTrigger->createUnit(Unit_Trike);
                if(pNewUnit != NULL) {
                    Coord spot = currentGameMap->findDeploySpot(pNewUnit, location);
                    pNewUnit->deploy(spot,false);
                }
            } break;

            case 2: {
                // One of the AI players on the map (one that has at least one unit) gets a Trike for free. It spawns beside the special bloom.
                int numCandidates = 0;
                for(int i=0;i<NUM_HOUSES;i++) {
                    House* pHouse = currentGame->getHouse(i);
                    if(pHouse != NULL && pHouse->getTeam() != pTrigger->getTeam() && pHouse->getNumUnits() > 0) {
                        numCandidates++;
                    }
                }

                if(numCandidates == 0) {
                    break;
                }

                House* pEnemyHouse = NULL;
                for(int i=0;i<NUM_HOUSES;i++) {
                    House* pHouse = currentGame->getHouse(i);
                    if(pHouse != NULL && pHouse->getTeam() != pTrigger->getTeam() && pHouse->getNumUnits() > 0) {
                        numCandidates--;
                        if(numCandidates == 0) {
                            pEnemyHouse = pHouse;
                            break;
                        }
                    }
                }

                UnitBase* pNewUnit = pEnemyHouse->createUnit(Unit_Trike);
                if(pNewUnit != NULL) {
                    Coord spot = currentGameMap->findDeploySpot(pNewUnit, location);
                    pNewUnit->deploy(spot,false);
                }

            } break;

            case 3:
            default: {
                // One of the AI players on the map (one that has at least one unit) gets an Infantry unit (3 Soldiers) for free. The spawn beside the special bloom.
                int numCandidates = 0;
                for(int i=0;i<NUM_HOUSES;i++) {
                    House* pHouse = currentGame->getHouse(i);
                    if(pHouse != NULL && pHouse->getTeam() != pTrigger->getTeam() && pHouse->getNumUnits() > 0) {
                        numCandidates++;
                    }
                }

                if(numCandidates == 0) {
                    break;
                }

                House* pEnemyHouse = NULL;
                for(int i=0;i<NUM_HOUSES;i++) {
                    House* pHouse = currentGame->getHouse(i);
                    if(pHouse != NULL && pHouse->getTeam() != pTrigger->getTeam() && pHouse->getNumUnits() > 0) {
                        numCandidates--;
                        if(numCandidates == 0) {
                            pEnemyHouse = pHouse;
                            break;
                        }
                    }
                }

                for(int i=0;i<3;i++) {
                    UnitBase* pNewUnit = pEnemyHouse->createUnit(Unit_Soldier);
                    if(pNewUnit != NULL) {
                        Coord spot = currentGameMap->findDeploySpot(pNewUnit, location);
                        pNewUnit->deploy(spot,false);
                    }
                }
            } break;
        }
    }
}

bool Tile::hasAStructure() const {
    if(!hasANonInfantryGroundObject()) {
        return false;
    }

    ObjectBase* pObject = currentGame->getObjectManager().getObject(assignedNonInfantryGroundObjectList.front());
    return ( (pObject != NULL) && pObject->isAStructure() );
}

bool Tile::isFogged(int houseID) {
	if(debug)
		return false;

	if(currentGame->getGameInitSettings().getGameOptions().fogOfWar == false) {
		return false;
	} else if((currentGame->getGameCycleCount() - lastAccess[houseID]) >= MILLI2CYCLES(10*1000)) {
		// TODO : shroud regain should be made an option
		return true;
	} else {
		return false;
	}
}

Uint32 Tile::getRadarColor(House* pHouse, bool radar) {
	if(isExplored(pHouse->getHouseID()) || debug) {
		if(isFogged(pHouse->getHouseID()) && radar) {
			return fogColor;
		} else {
			ObjectBase* pObject = getObject();
			if(pObject != NULL) {
			    int color;

			    if(pObject->getItemID() == Unit_Sandworm) {
					color = COLOR_WHITE;
				} else {
                    switch(pObject->getOwner()->getHouseID()) {
                        case HOUSE_HARKONNEN:   color = COLOR_HARKONNEN;  break;
                        case HOUSE_ATREIDES:    color = COLOR_ATREIDES;   break;
                        case HOUSE_ORDOS:       color = COLOR_ORDOS;      break;
                        case HOUSE_FREMEN:      color = COLOR_FREMEN;     break;
                        case HOUSE_SARDAUKAR:   color = COLOR_SARDAUKAR;  break;
                        case HOUSE_MERCENARY:   color = COLOR_MERCENARY;  break;
                        default:                color = COLOR_BLACK;      break;
                    }
				}

				if(pObject->isAUnit()) {
                    fogColor = getColorByTerrainType(getType());
				} else {
                    fogColor = color;
				}

				// units and structures of the enemy are not visible if no radar
				if(!radar && !debug && (pObject->getOwner()->getTeam() != pHouse->getTeam())) {
					return COLOR_BLACK;
				} else {
                    return color;
				}
			} else {
			    fogColor = getColorByTerrainType(getType());

				if(!radar && !debug) {
					return COLOR_BLACK;
				} else {
                    return fogColor;
				}
			}
		}
	} else {
		return COLOR_BLACK;
	}
}

int Tile::getTerrainTile() const {
    switch(type) {
        case Terrain_Slab: {
            return TerrainTile_Slab;
        } break;

        case Terrain_Sand: {
            return TerrainTile_Sand;
        } break;

        case Terrain_Rock: {
            //determine which surounding tiles are rock
            bool up = (currentGameMap->tileExists(location.x,location.y-1) == false) || (currentGameMap->getTile(location.x, location.y-1)->isRock() == true);
            bool right = (currentGameMap->tileExists(location.x+1,location.y) == false) || (currentGameMap->getTile(location.x+1, location.y)->isRock() == true);
            bool down = (currentGameMap->tileExists(location.x,location.y+1) == false) || (currentGameMap->getTile(location.x, location.y+1)->isRock() == true);
            bool left = (currentGameMap->tileExists(location.x-1,location.y) == false) || (currentGameMap->getTile(location.x-1, location.y)->isRock() == true);

            return TerrainTile_Rock + (up | (right << 1) | (down << 2) | (left << 3));
        } break;

        case Terrain_Dunes: {
            //determine which surounding tiles are dunes
            bool up = (currentGameMap->tileExists(location.x,location.y-1) == false) || (currentGameMap->getTile(location.x, location.y-1)->getType() == Terrain_Dunes);
            bool right = (currentGameMap->tileExists(location.x+1,location.y) == false) || (currentGameMap->getTile(location.x+1, location.y)->getType() == Terrain_Dunes);
            bool down = (currentGameMap->tileExists(location.x,location.y+1) == false) || (currentGameMap->getTile(location.x, location.y+1)->getType() == Terrain_Dunes);
            bool left = (currentGameMap->tileExists(location.x-1,location.y) == false) || (currentGameMap->getTile(location.x-1, location.y)->getType() == Terrain_Dunes);

            return TerrainTile_Dunes + (up | (right << 1) | (down << 2) | (left << 3));
        } break;

        case Terrain_Mountain: {
            //determine which surounding tiles are mountains
            bool up = (currentGameMap->tileExists(location.x,location.y-1) == false) || (currentGameMap->getTile(location.x, location.y-1)->isMountain() == true);
            bool right = (currentGameMap->tileExists(location.x+1,location.y) == false) || (currentGameMap->getTile(location.x+1, location.y)->isMountain() == true);
            bool down = (currentGameMap->tileExists(location.x,location.y+1) == false) || (currentGameMap->getTile(location.x, location.y+1)->isMountain() == true);
            bool left = (currentGameMap->tileExists(location.x-1,location.y) == false) || (currentGameMap->getTile(location.x-1, location.y)->isMountain() == true);

            return TerrainTile_Mountain + (up | (right << 1) | (down << 2) | (left << 3));
        } break;

        case Terrain_Spice: {
            //determine which surounding tiles are spice
            bool up = (currentGameMap->tileExists(location.x,location.y-1) == false) || (currentGameMap->getTile(location.x, location.y-1)->isSpice() == true);
            bool right = (currentGameMap->tileExists(location.x+1,location.y) == false) || (currentGameMap->getTile(location.x+1, location.y)->isSpice() == true);
            bool down = (currentGameMap->tileExists(location.x,location.y+1) == false) || (currentGameMap->getTile(location.x, location.y+1)->isSpice() == true);
            bool left = (currentGameMap->tileExists(location.x-1,location.y) == false) || (currentGameMap->getTile(location.x-1, location.y)->isSpice() == true);

            return TerrainTile_Spice + (up | (right << 1) | (down << 2) | (left << 3));
        } break;

        case Terrain_ThickSpice: {
            //determine which surounding tiles are thick spice
            bool up = (currentGameMap->tileExists(location.x,location.y-1) == false) || (currentGameMap->getTile(location.x, location.y-1)->getType() == Terrain_ThickSpice);
            bool right = (currentGameMap->tileExists(location.x+1,location.y) == false) || (currentGameMap->getTile(location.x+1, location.y)->getType() == Terrain_ThickSpice);
            bool down = (currentGameMap->tileExists(location.x,location.y+1) == false) || (currentGameMap->getTile(location.x, location.y+1)->getType() == Terrain_ThickSpice);
            bool left = (currentGameMap->tileExists(location.x-1,location.y) == false) || (currentGameMap->getTile(location.x-1, location.y)->getType() == Terrain_ThickSpice);

            return TerrainTile_ThickSpice + (up | (right << 1) | (down << 2) | (left << 3));
        } break;

        case Terrain_SpiceBloom: {
            return TerrainTile_SpiceBloom;
        } break;

        case Terrain_SpecialBloom: {
            return TerrainTile_SpecialBloom;
        } break;

        default: {
            throw std::runtime_error("Tile::getTerrainTile(): Invalid terrain type");
        } break;
    }
}

int Tile::getHideTile(int houseID) const {

    // are all surounding tiles explored?
    if( ((currentGameMap->tileExists(location.x,location.y-1) == false) || (currentGameMap->getTile(location.x, location.y-1)->isExplored(houseID) == true))
        && ((currentGameMap->tileExists(location.x+1,location.y) == false) || (currentGameMap->getTile(location.x+1, location.y)->isExplored(houseID) == true))
        && ((currentGameMap->tileExists(location.x,location.y+1) == false) || (currentGameMap->getTile(location.x, location.y+1)->isExplored(houseID) == true))
        && ((currentGameMap->tileExists(location.x-1,location.y) == false) || (currentGameMap->getTile(location.x-1, location.y)->isExplored(houseID) == true))) {
        return 0;
    }

    //determine what tiles are unexplored
    bool up = (currentGameMap->tileExists(location.x,location.y-1) == false) || (currentGameMap->getTile(location.x, location.y-1)->isExplored(houseID) == false);
    bool right = (currentGameMap->tileExists(location.x+1,location.y) == false) || (currentGameMap->getTile(location.x+1, location.y)->isExplored(houseID) == false);
    bool down = (currentGameMap->tileExists(location.x,location.y+1) == false) || (currentGameMap->getTile(location.x, location.y+1)->isExplored(houseID) == false);
    bool left = (currentGameMap->tileExists(location.x-1,location.y) == false) || (currentGameMap->getTile(location.x-1, location.y)->isExplored(houseID) == false);

    return (up | (right << 1) | (down << 2) | (left << 3));
}

int Tile::getFogTile(int houseID) const {

    // are all surounding tiles fogged?
    if( ((currentGameMap->tileExists(location.x,location.y-1) == false) || (currentGameMap->getTile(location.x, location.y-1)->isFogged(houseID) == false))
        && ((currentGameMap->tileExists(location.x+1,location.y) == false) || (currentGameMap->getTile(location.x+1, location.y)->isFogged(houseID) == false))
        && ((currentGameMap->tileExists(location.x,location.y+1) == false) || (currentGameMap->getTile(location.x, location.y+1)->isFogged(houseID) == false))
        && ((currentGameMap->tileExists(location.x-1,location.y) == false) || (currentGameMap->getTile(location.x-1, location.y)->isFogged(houseID) == false))) {
        return 0;
    }

    //determine what tiles are fogged
    bool up = (currentGameMap->tileExists(location.x,location.y-1) == false) || (currentGameMap->getTile(location.x, location.y-1)->isFogged(houseID) == true);
    bool right = (currentGameMap->tileExists(location.x+1,location.y) == false) || (currentGameMap->getTile(location.x+1, location.y)->isFogged(houseID) == true);
    bool down = (currentGameMap->tileExists(location.x,location.y+1) == false) || (currentGameMap->getTile(location.x, location.y+1)->isFogged(houseID) == true);
    bool left = (currentGameMap->tileExists(location.x-1,location.y) == false) || (currentGameMap->getTile(location.x-1, location.y)->isFogged(houseID) == true);

    return (up | (right << 1) | (down << 2) | (left << 3));
}


Coord Tile::getUnpreciseCenterPoint() const {

	int sign = rand()%3;
	int precision = 8; // [ 1 - TILESIZE ]
	int offset = TILESIZE/precision; // the greater the less accurate
	int r_mx = (currentGame->randomGen.rand()%offset)+1;
	int r_my = (currentGame->randomGen.rand()%offset)+1;
	int x,y,_x,_y;

	_x  =  (currentGame->randomGen.rand()%(TILESIZE+1));
	_y  =  (currentGame->randomGen.rand()%(TILESIZE+1));
	r_mx > offset/2 ? _x /= ((offset+1)-r_mx) : _x *= (r_mx) ;
	r_my > offset/2 ? _y /= ((offset+1)-r_my) : _y *= (r_my) ;

	if (sign == 0) {
		x = location.x*TILESIZE + _x;
		y = location.y*TILESIZE + _y;
	} else if (sign == 1) {
		x = location.x*TILESIZE - _x;
		y = location.y*TILESIZE + _y;
	} else if (sign == 2) {
		x = location.x*TILESIZE + _x;
		y = location.y*TILESIZE - _y;
	} else if (sign == 3) {
		x = location.x*TILESIZE - _x;
		y = location.y*TILESIZE - _y;
	}
	return Coord(x,y);
}

std::pair<std::vector<Coord>,int> Tile::getFreeTile() {
    //determine which surrounding tiles are rock
    bool up = (currentGameMap->tileExists(location.x,location.y-1) == true) || (currentGameMap->getTile(location.x, location.y-1)->hasAGroundObject() == false);
    bool right = (currentGameMap->tileExists(location.x+1,location.y) == true) || (currentGameMap->getTile(location.x+1, location.y)->hasAGroundObject() == false);
    bool down = (currentGameMap->tileExists(location.x,location.y+1) == true) || (currentGameMap->getTile(location.x, location.y+1)->hasAGroundObject() == false);
    bool left = (currentGameMap->tileExists(location.x-1,location.y) == true) || (currentGameMap->getTile(location.x-1, location.y)->hasAGroundObject() == false);

    bool cupleft = (currentGameMap->tileExists(location.x-1,location.y-1) == true) || (currentGameMap->getTile(location.x-1, location.y-1)->hasAGroundObject() == false);
    bool cupright = (currentGameMap->tileExists(location.x+1,location.y-1) == true) || (currentGameMap->getTile(location.x+1, location.y-1)->hasAGroundObject() == false);
    bool cdoleft = (currentGameMap->tileExists(location.x-1,location.y+1) == true) || (currentGameMap->getTile(location.x-1, location.y+1)->hasAGroundObject() == false);
    bool cdoright = (currentGameMap->tileExists(location.x+1,location.y+1) == true) || (currentGameMap->getTile(location.x+1, location.y+1)->hasAGroundObject() == false);

    int free = (up | (right << 1) | (down << 2) | (left << 3));



    std::vector<Coord> v;

    if (currentGameMap->getTile(location.x, location.y)->hasAGroundObject() == false) {
   		  // this very tile itself is free
		v.push_back(Coord(0,0));
   	}
	if((left == true) && (right == true) && (up == true) && (down == true)) {
	  // free surroundings
								   v.push_back(Coord(-1,0)) ;
		v.push_back(Coord(-1,0)) ; 						    ; v.push_back(Coord(+1,0));
								   v.push_back(Coord(-1,1)) ;
	} else if((left == false) && (right == true) && (up == true) && (down == true)) {
								 ; v.push_back(Coord(-1,0)) ;
		 	 	 	 	 	 	 ; 						  	; v.push_back(Coord(+1,0));
								 ; v.push_back(Coord(-1,1)) ;
	} else if((left == true) && (right == false)&& (up == true) && (down == true)) {
									v.push_back(Coord(-1,0)) ;
		v.push_back(Coord(-1,0)) ;
									v.push_back(Coord(-1,1)) ;
	} else if((left == true) && (right == true) && (up == false) && (down == true)) {

		v.push_back(Coord(-1,0)) ; 						 	; v.push_back(Coord(+1,0));
		 	 	 	 	 	 	 	 v.push_back(Coord(-1,1)) ;
	} else if((left == true) && (right == true) && (up == true) && (down == false)) {
									v.push_back(Coord(-1,0)) ;
		v.push_back(Coord(-1,0)) ; 						  	; v.push_back(Coord(+1,0));

	} else if((left == false) && (right == true) && (up == false) && (down == true)) {

															; v.push_back(Coord(+1,0));
								; v.push_back(Coord(-1,1)) ;
	} else if((left == true) && (right == false) && (up == true) && (down == false)) {
		 	 	 	 	 	 	   v.push_back(Coord(-1,0)) ;
		v.push_back(Coord(-1,0)) ;
	} else if((left == true) && (right == false) && (up == false) && (down == true)) {

		v.push_back(Coord(-1,0)) ;
								; v.push_back(Coord(-1,1)) ;
	} else if((left == false) && (right == true) && (up == true) && (down == false)) {
									; v.push_back(Coord(-1,0))
															; v.push_back(Coord(+1,0));

	} else if((left == true) && (right == false) && (up == false) && (down == false)) {

		v.push_back(Coord(-1,0)) ;

	} else if((left == false) && (right == true) && (up == false) && (down == false)) {

															; v.push_back(Coord(+1,0));

	} else if((left == false) && (right == false) && (up == true) && (down == false)) {
		 	 	 	 	 	 	 v.push_back(Coord(-1,0)) ;


	} else if((left == false) && (right == false) && (up == false) && (down == true)) {


								v.push_back(Coord(-1,1));
	} else if((left == true) && (right == true) && (up == false) && (down == false)) {

		v.push_back(Coord(-1,0)) ; 						    ; v.push_back(Coord(+1,0));

	} else if((left == false) && (right == false) && (up == true) && (down == true)) {
									v.push_back(Coord(-1,0)) ;

									v.push_back(Coord(-1,1)) ;
	} else if((left == false) && (right == false) && (up == false) && (down == false)) {

	}


	if (cupleft) v.push_back(Coord(-1,-1));
	if (cupright) v.push_back(Coord(+1,-1));
	if (cdoleft) v.push_back(Coord(-1,+1));
	if (cdoright) v.push_back(Coord(+1,+1));


	return std::make_pair(v, free);




}


