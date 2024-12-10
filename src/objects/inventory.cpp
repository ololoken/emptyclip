/******************************************************************************
* Empty Clip
* Copyright (C) 2024 Alan Witkowski
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include <objects/inventory.h>
#include <ae/buffer.h>
#include <objects/item.h>
#include <constants.h>
#include <stats.h>

// Number of slots for each bag
const size_t BagSizes[(size_t)BagType::COUNT] = {
	GearType::COUNT,
	INVENTORY_BAGSIZE,
};

// Constructor
_Inventory::_Inventory() {
	Containers.resize((size_t)BagType::COUNT);
	Containers[(size_t)BagType::OUTFIT].reserve(INVENTORY_MAX_OUTFITS);
	Containers[(size_t)BagType::BACKPACK].reserve(INVENTORY_MAX_BACKPACKS);
	Containers[(size_t)BagType::OUTFIT].resize(2);
	Containers[(size_t)BagType::BACKPACK].resize(1);
	Containers[(size_t)BagType::OUTFIT].front().Gear = true;

	for(size_t i = 0; i < Containers.size(); i++) {
		for(auto &Bag : Containers[i])
			Bag.Slots.resize(BagSizes[i], nullptr);
	}
}

// Destructor
_Inventory::~_Inventory() {
	for(const auto &Container : Containers) {
		for(const auto &Bag : Container) {
			for(const auto &Item : Bag.Slots) {
				delete Item;
			}
		}
	}
}

// Serialize to a buffer
void _Inventory::Serialize(ae::_Buffer &Buffer) {

	// Write container count
	Buffer.Write<int32_t>((int32_t)Containers.size());

	// Write containers
	for(const auto &Container : Containers)
		SerializeContainer(Buffer, Container);
}

// Unserialize buffer
void _Inventory::Unserialize(ae::_Buffer &Buffer) {

	// Read container count
	int ContainerCount = Buffer.Read<int32_t>();
	Containers.clear();

	// Read containers
	for(int i = 0; i < ContainerCount; i++) {
		_Container Container;
		UnserializeContainer(Buffer, Container, BagSizes[i], i == (int)BagType::OUTFIT);
		Containers.push_back(Container);
	}

	// Default to two outfits
	if(Containers[(size_t)BagType::OUTFIT].size() == 1)
		AddBag(BagType::OUTFIT);
}

// Add a bag
void _Inventory::AddBag(BagType Type) {
	_Bag Bag;
	Bag.Slots.resize(BagSizes[(size_t)Type]);
	Containers[(size_t)Type].push_back(Bag);
}

// Get count of items in a bag
int _Inventory::GetItemCount(BagType Type, size_t BagIndex) {
	_Container &Container = Containers[(size_t)Type];
	if(BagIndex >= Container.size())
		return 0;

	return Container[BagIndex].GetItemCount();
}

// Update the type counts for each bag
void _Inventory::UpdateTypeCount() {
	_Container &Container = Containers[(size_t)BagType::BACKPACK];
	for(auto &Bag : Container) {

		// Reset bag count
		std::fill(Bag.TypeCount.begin(), Bag.TypeCount.end(), 0);

		// Keep track of highest count
		Bag.HighestType = BAGICON_DEFAULT;
		Bag.Full = true;
		int HighestCount = INVENTORY_ICONTYPE_THRESHOLD - 1;
		for(const auto &Item : Bag.Slots) {
			if(!Item) {
				Bag.Full = false;
				continue;
			}

			size_t IconType = Item->GetIconType();
			Bag.TypeCount[IconType]++;
			if(Bag.TypeCount[IconType] > HighestCount) {
				HighestCount = Bag.TypeCount[IconType];
				Bag.HighestType = IconType;
			}
		}
	}
}

// Determine the best icon to represent a backpack bag
const char *_Inventory::GetBackpackIcon(size_t BagIndex) {

	// Only change icon when more than one bag is present
	_Container &Container = Containers[(size_t)BagType::BACKPACK];
	if(Container.size() <= 1)
		return "textures/hud/backpack.png";

	// Return icon
	switch(Container[BagIndex].HighestType) {
		case BAGICON_DEFAULT:
			return "textures/hud/backpack.png";
		break;
		case BAGICON_GEAR_WEAPON:
			return "textures/hud/tab_gear_weapon.png";
		break;
		case BAGICON_GEAR_ARMOR:
			return "textures/hud/tab_gear_armor.png";
		break;
		case BAGICON_MOD_WEAPON:
			return "textures/hud/tab_mod_weapon.png";
		break;
		case BAGICON_MOD_ARMOR:
			return "textures/hud/tab_mod_armor.png";
		break;
		case BAGICON_MOD_SPECIAL:
			return "textures/hud/tab_mod_special.png";
		break;
		case BAGICON_MOD_CHANGE:
			return "textures/hud/tab_mod_change.png";
		break;
		case BAGICON_USABLE:
			return "textures/hud/tab_usable_whetstone.png";
		break;
	}

	return "textures/hud/backpack.png";
}

// Find suitable bag for item based on type
size_t _Inventory::FindSuitableBackpackBag(const _Item *Item, size_t StartIndex, bool SkipStartIndex, bool PreferStartIndex) {
	if(!Item)
		return StartIndex;

	_Container &Container = Containers[(size_t)BagType::BACKPACK];
	if(PreferStartIndex && !Container[StartIndex].Full)
		return StartIndex;

	size_t BagIndex = StartIndex;
	size_t BestIndex = (size_t)-1;
	size_t DefaultIndex = (size_t)-1;
	size_t FirstEmptyIndex = (size_t)-1;

	for(size_t i = 0; i < Container.size(); i++) {
		_Bag &Bag = Container[BagIndex];

		// Skip full bags or starting index if necessary
		if(!Bag.Full && (!SkipStartIndex || (SkipStartIndex && BagIndex != StartIndex))) {

			// Keep track of first empty bag
			if(FirstEmptyIndex == (size_t)-1)
				FirstEmptyIndex = BagIndex;

			// Keep track of matching bag type
			if(BestIndex == (size_t)-1 && Bag.HighestType == Item->GetIconType())
				BestIndex = BagIndex;
			// Keep track of first default bag
			else if(DefaultIndex == (size_t)-1 && Bag.HighestType == BAGICON_DEFAULT)
				DefaultIndex = BagIndex;
		}

		BagIndex++;
		if(BagIndex >= Container.size())
			BagIndex = 0;
	}

	// Didn't find a better bag, use empty bag if available
	if(BestIndex == (size_t)-1) {
		if(DefaultIndex == (size_t)-1) {
			if(FirstEmptyIndex == (size_t)-1)
				return StartIndex;
			else
				return FirstEmptyIndex;
		}
		else
			return DefaultIndex;
	}

	return BestIndex;
}

// Search bags for similar gear item
void _Inventory::FindSimilarGearItem(const _Item *GearItem, bool SkipBackpack, _Slot &Slot) {
	for(size_t ContainerIndex = 0; ContainerIndex < Containers.size(); ContainerIndex++) {
		_Container &Container = Containers[ContainerIndex];
		for(size_t BagIndex = 0; BagIndex < Container.size(); BagIndex++) {
			_Bag &Bag = Container[BagIndex];

			// Skip backpack if item is inside backpack
			if(!Bag.Gear && SkipBackpack)
				continue;

			// Check bag
			Slot.Index = Bag.FindSimilarGearItem(GearItem);
			if(Slot.Index != (size_t)-1) {
				Slot.Bag = &Bag;
				return;
			}
		}
	}
}

// Serialize a bag
void _Inventory::SerializeContainer(ae::_Buffer &Buffer, const _Container &Container) const {

	// Write bag count
	Buffer.Write<int32_t>((int32_t)Container.size());

	// Write each bag
	for(const auto &Bag : Container) {

		// Write item count
		int ItemCount = Bag.GetItemCount();
		Buffer.Write<int32_t>(ItemCount);

		// No items to write
		if(!ItemCount)
			continue;

		// Write slots
		for(size_t i = 0; i < Bag.Slots.size(); i++) {
			_Item *Item = Bag.Slots[i];
			if(!Item)
				continue;

			// Write slot and count
			Buffer.Write<int32_t>((int32_t)i);
			Buffer.Write<int32_t>((int32_t)Item->Count);

			// Write item
			Item->Serialize(Buffer);
		}
	}
}

// Unserialize bag from buffer
void _Inventory::UnserializeContainer(ae::_Buffer &Buffer, _Container &Container, size_t BagSize, bool Gear) {

	// Get bag count
	int BagCount = Buffer.Read<int32_t>();
	Container.clear();

	// Read each bag
	for(int i = 0; i < BagCount; i++) {
		_Bag Bag;
		Bag.Gear = Gear;
		Bag.Slots.resize(BagSize, nullptr);

		// Read item count
		int ItemCount = Buffer.Read<int32_t>();

		// Read slots
		for(int i = 0; i < ItemCount; i++) {

			// Read slot and count
			size_t Slot = (size_t)Buffer.Read<int32_t>();
			int Count = Buffer.Read<int32_t>();

			// Read item
			_Item *Item = UnserializeItem(Buffer);
			Item->Count = Count;

			// Read mods
			if(Item->Type == _Object::WEAPON || Item->Type == _Object::ARMOR) {
				float ExtraMods = Buffer.Read<float>();
				Item->ExtraMods = ExtraMods;
				Item->SetMaxMods();

				// Load mods
				int ModCount = Buffer.Read<int>();
				for(int i = 0; i < ModCount; i++) {
					_Item *Mod = UnserializeItem(Buffer);
					if(!Item->AddMod(Mod, false))
						delete Mod;
				}

				Item->RecalculateStats();
			}

			// Read ammo
			if(Item->Type == _Object::WEAPON)
				Item->SetAmmo(Buffer.Read<int>());

			// Set slot
			if(Slot < Bag.Slots.size())
				Bag.Slots[Slot] = Item;
		}

		Container.push_back(Bag);
	}
}

// Create item from buffer
_Item *_Inventory::UnserializeItem(ae::_Buffer &Buffer) {

	// Read item
	std::string ID = Buffer.ReadString();
	int Level = Buffer.Read<int32_t>();
	int Quality = Buffer.Read<int32_t>();

	// Create item
	_Item *Item = Stats.CreateItem(ID, glm::vec2(0), Level, Quality, false, 0, 0);

	return Item;
}

// Bag constructor
_Bag::_Bag() : TypeCount(BAGICON_COUNT) {
}

// Get count of items in bag
int _Bag::GetItemCount() const {
	int Count = 0;
	for(const auto &Item : Slots) {
		if(Item)
			Count++;
	}

	return Count;
}

// Find similar gear item and return the slot index
size_t _Bag::FindSimilarGearItem(const _Item *GearItem) {

	// Search slots
	for(size_t i = 0; i < Slots.size(); i++) {
		const _Item *Item = Slots[i];
		if(!Item || !Item->CanEquip())
			continue;

		if(Item->Template.ID == GearItem->Template.ID)
			return i;
	}

	return (size_t)-1;
}

// Find an empty slot in the bag
size_t _Bag::FindEmptySlot() {
	for(size_t i = 0; i < Slots.size(); i++) {
		if(!Slots[i])
			return i;
	}

	return (size_t)-1;
}

// Delete slot item
void _Slot::DeleteItem() const {
	delete Bag->Slots[Index];
	Bag->Slots[Index] = nullptr;
}
