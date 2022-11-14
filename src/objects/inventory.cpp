/******************************************************************************
* Empty Clip
* Copyright (C) 2022  Alan Witkowski
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
#include <objects/item.h>
#include <ae/buffer.h>
#include <stats.h>
#include <constants.h>

// Number of slots for each bag
const size_t BagSizes[(size_t)BagType::COUNT] = {
	EquipmentType::COUNT,
	INVENTORY_BAGSIZE,
};

// Constructor
_Inventory::_Inventory() {
	Containers.resize((size_t)BagType::COUNT);
	Containers[(size_t)BagType::OUTFIT].reserve(INVENTORY_MAX_OUTFITS);
	Containers[(size_t)BagType::BACKPACK].reserve(INVENTORY_MAX_BACKPACKS);
	Containers[(size_t)BagType::OUTFIT].resize(2);
	Containers[(size_t)BagType::BACKPACK].resize(1);
	Containers[(size_t)BagType::OUTFIT].front().Equipment = true;

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
void _Inventory::UnserializeContainer(ae::_Buffer &Buffer, _Container &Container, size_t BagSize, bool Equipment) {

	// Get bag count
	int BagCount = Buffer.Read<int32_t>();
	Container.clear();

	// Read each bag
	for(int i = 0; i < BagCount; i++) {
		_Bag Bag;
		Bag.Equipment = Equipment;
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
	_Item *Item = Stats.CreateItem(ID, Level, Quality, glm::vec2(0), false);

	return Item;
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

// Delete slot item
void _Slot::DeleteItem() const {
	delete Bag->Slots[Index];
	Bag->Slots[Index] = nullptr;
}
