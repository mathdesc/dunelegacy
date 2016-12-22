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

#include <structures/RepairYard.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <House.h>
#include <Map.h>
#include <SoundPlayer.h>

#include <units/Carryall.h>
#include <units/GroundUnit.h>

#include <GUI/ObjectInterfaces/RepairYardInterface.h>

RepairYard::RepairYard(House* newOwner) : StructureBase(newOwner) {
    RepairYard::init();

    setHealth(getMaxHealth());
    bookings = 0;
	repairing = false;
}

RepairYard::RepairYard(InputStream& stream) : StructureBase(stream) {
    RepairYard::init();

	repairing = stream.readBool();
	repairUnit.load(stream);
	bookings = stream.readUint32();
}

void RepairYard::init() {
    itemID = Structure_RepairYard;
	owner->incrementStructures(itemID);

	structureSize.x = 3;
	structureSize.y = 2;

	graphicID = ObjPic_RepairYard;
	graphic = pGFXManager->getObjPic(graphicID,getOwner()->getHouseID());
	numImagesX = 10;
	numImagesY = 1;
	firstAnimFrame = 2;
	lastAnimFrame = 5;
}

RepairYard::~RepairYard() {
    if(repairing) {
		unBook();
		repairUnit.getUnitPointer()->destroy();
	}
}

void RepairYard::save(OutputStream& stream) const {
	StructureBase::save(stream);

	stream.writeBool(repairing);
	repairUnit.save(stream);
	stream.writeUint32(bookings);
}


ObjectInterface* RepairYard::getInterfaceContainer() {
	if((pLocalHouse == owner) || (debug == true)) {
		return RepairYardInterface::create(objectID);
	} else {
		return DefaultObjectInterface::create(objectID);
	}
}

bool RepairYard::deployRepairUnit(Carryall* pCarryall) {


    UnitBase* pRepairUnit = repairUnit.getUnitPointer();
    dbg_print("RepairYard::deployRepairUnit (carry:%s)  \n", (pCarryall != NULL) ? "yes" : "no");
    bool deployed = false;

    if (repairUnit && (repairUnit.getObjPointer() != NULL)) {
    	unBook();
    	soundPlayer->playSoundAt(Sound_Steam,this->location);
    	repairing = false;
    	firstAnimFrame = 2;
    	lastAnimFrame = 5;


		if(pCarryall != NULL) {
			pCarryall->giveCargo(pRepairUnit);
			pCarryall->setFellow(NULL);
			pCarryall->giveDeliveryOrders(pRepairUnit, pRepairUnit->getGuardPoint(), pRepairUnit->getGuardPoint(),
											currentGameMap->findDeploySpot(pRepairUnit, location, pRepairUnit->getGuardPoint(), structureSize));
			  dbg_print("RepairYard::deployRepairUnit dest:%d,%d \n", pCarryall->getDestination().x,pCarryall->getDestination().y);
		} else {
			Coord deployPos = currentGameMap->findDeploySpot(pRepairUnit, location, pRepairUnit->getGuardPoint(), structureSize);
			pRepairUnit->deploy(deployPos, false);
			pRepairUnit->setFellow(NULL);
		//    pRepairUnit->setDestination(pRepairUnit->getLocation());
			pRepairUnit->setDestination(pRepairUnit->getGuardPoint());
		}

		repairUnit.pointTo(NONE);
		deployed = true;

		if(getOwner() == pLocalHouse ) {
			if ( pRepairUnit->getHealth() == pRepairUnit->getMaxHealth()) {
				soundPlayer->playVoice(VehicleRepaired,getOwner()->getHouseID());
			} else {
				soundPlayer->playVoice(VehicleDeployed,getOwner()->getHouseID());
			}
		}
    }

    return deployed;
}

void RepairYard::updateStructureSpecificStuff() {
	if(repairing) {
		if(curAnimFrame < 6) {
			firstAnimFrame = 6;
			lastAnimFrame = 9;
			curAnimFrame = 6;
		}
	} else {
		if(curAnimFrame > 5) {
			firstAnimFrame = 2;
			lastAnimFrame = 5;
			curAnimFrame = 2;
		}
	}

	if(repairing == true) {
	    UnitBase* pRepairUnit = repairUnit.getUnitPointer();

		if (pRepairUnit->getHealth()*100/pRepairUnit->getMaxHealth() < 100) {
			if (owner->takeCredits(UNIT_REPAIRCOST) > 0) {
				pRepairUnit->addHealth();
				if (!owner->isRepairOnDuty()) {
					if (owner->getHouseID() == pLocalHouse->getHouseID())
						soundPlayer->playVoice(RepairActivated, owner->getHouseID());
					owner->informRepairBegins();
				}
			} else {
				if (owner->isRepairOnDuty()) {
					if (owner->getHouseID() == pLocalHouse->getHouseID())
						soundPlayer->playVoice(RepairDeactivated, owner->getHouseID());
					owner->informRepairStops();
				}
			}
		} else if(((GroundUnit*)pRepairUnit)->isAwaitingPickup() == false) {
		    // find nearest carryall TODO groundunit->requestCarryall instead
		    Carryall* pCarryall = NULL;
		    float distance = std::numeric_limits<float>::infinity();
            if((pRepairUnit->getGuardPoint().isValid()) && getOwner()->hasCarryalls())	{
                RobustList<UnitBase*>::const_iterator iter;
                for(iter = unitList.begin(); iter != unitList.end(); ++iter) {
                    UnitBase* unit = *iter;
                    if ((unit->getOwner() == owner) && (unit->getItemID() == Unit_Carryall)) {

					if ( !((Carryall*)unit)->isBooked() && ((Carryall*)unit)->getAttackMode() != STOP) {
                        	if (distance >  std::min(distance,blockDistance(this->location, unit->getLocation())) ) {
                        		distance = std::min(distance,blockDistance(this->location, unit->getLocation()));
                        		pCarryall = (Carryall*)unit;
                        	}
                        }
                    }
                }
            }


            if(pCarryall != NULL ) {
                pCarryall->setFellow(this);
                pCarryall->clearPath();
                ((GroundUnit*)pRepairUnit)->bookCarrier(pCarryall);
                pRepairUnit->setFellow(NULL);
                pRepairUnit->setDestination(pRepairUnit->getGuardPoint());
                soundPlayer->playSoundAt(Sound_Steam,this->location);
            } else {
                deployRepairUnit();
            }
		} else if(((GroundUnit*)pRepairUnit)->hasBookedCarrier() == false) {
           deployRepairUnit();
		}
	}
}
