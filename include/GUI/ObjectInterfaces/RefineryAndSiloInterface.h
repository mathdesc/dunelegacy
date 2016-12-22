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

#ifndef REFINERYANDSILOINTERFACE_H
#define REFINERYANDSILOINTERFACE_H

#include "DefaultStructureInterface.h"

#include <FileClasses/FontManager.h>
#include <FileClasses/TextManager.h>

#include <House.h>

#include <GUI/ProgressBar.h>
#include <GUI/Label.h>
#include <GUI/VBox.h>

#include <misc/string_util.h>

#include <units/UnitBase.h>
#include <units/Harvester.h>
#include <structures/Refinery.h>

class RefineryAndSiloInterface : public DefaultStructureInterface {
public:
	static RefineryAndSiloInterface* create(int objectID) {
		RefineryAndSiloInterface* tmp = new RefineryAndSiloInterface(objectID);
		tmp->pAllocated = true;
		return tmp;
	}

protected:
	RefineryAndSiloInterface(int objectID) : DefaultStructureInterface(objectID) {
	    int color = houseColor[pLocalHouse->getHouseID()];
	    mainHBox.addWidget(HSpacer::create(15));
		mainHBox.addWidget(&harvesterUnitProgressBar);



        capacityLabel.setTextFont(FONT_STD10);
        capacityLabel.setTextColor(color+3);
		textVBox.addWidget(&capacityLabel, 0.005);
		storedCreditsLabel.setTextFont(FONT_STD10);
        storedCreditsLabel.setTextColor(color+3);
        textVBox.addWidget(&storedCreditsLabel, 0.005);
    	bookingLabel.setTextFont(FONT_STD10);
        bookingLabel.setTextColor(color+3);
        textVBox.addWidget(&bookingLabel, 0.005);
		textVBox.addWidget(Spacer::create(), 0.99);
		mainHBox.addWidget(Spacer::create());
	    mainHBox.addWidget(&textVBox);

	    harvesterUnitProgressBar.setTooltipText(_("Emergency Exit (Right-Click)"));
		harvesterUnitProgressBar.setOnClick(std::bind(&RefineryAndSiloInterface::exitUnit, this  ,std::placeholders::_1, std::placeholders::_2));
	}

	/**
		This method updates the object interface.
		If the object doesn't exists anymore then update returns false.
		\return true = everything ok, false = the object container should be removed
	*/
	virtual bool update() {
		ObjectBase* pObject = currentGame->getObjectManager().getObject(objectID);
		if(pObject == NULL) {
			return false;
		}

		House* pOwner = pObject->getOwner();

        capacityLabel.setText(" " + _("Capacity") + ": " + stringify(pOwner->getCapacity()));
        storedCreditsLabel.setText(" " + _("Stored") + ": " + stringify(lround(pOwner->getStoredCredits())));

       if (pObject->getItemID() == Structure_Refinery) {
			Refinery* pRefinery = dynamic_cast<Refinery*>(pObject);

			if(pRefinery != NULL) {
				Harvester* pHarvester = pRefinery->getHarvester();

				if(pHarvester != NULL) {
					harvesterUnitProgressBar.setVisible(true);
					harvesterUnitProgressBar.setSurface(resolveItemPicture(pHarvester->getItemID()),false);
					harvesterUnitProgressBar.setProgress((pHarvester->getAmountOfSpice()*100)/HARVESTERMAXSPICE);
				} else {
					harvesterUnitProgressBar.setVisible(false);
				}

				bookingLabel.setText(" " + _("Bookings") + ": " + stringify(pRefinery->getNumBookings()));
			}
        }


		return DefaultStructureInterface::update();
	}

	inline void exitUnit(int x, int y) {
		ObjectBase* pObject = currentGame->getObjectManager().getObject(objectID);
		if(pObject == NULL) {
			return ;
		}
		if (pObject->getItemID() == Structure_Refinery) {
			Refinery* pRefinery = dynamic_cast<Refinery*>(pObject);
			if(pRefinery != NULL) {
				pRefinery->deployHarvester();
			}
		}
	}

private:
    VBox    textVBox;

	Label   capacityLabel;
	Label   storedCreditsLabel;
	Label   bookingLabel;

	PictureProgressBar	harvesterUnitProgressBar;
};

#endif // REFINERYANDSILOINTERFACE_H
