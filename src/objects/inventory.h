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
#pragma once

// Libraries
#include <vector>
#include <cstddef>

// Forward Declarations
class _Item;
namespace ae {
	class _Buffer;
}

enum EquipmentType : size_t {
	MAINHAND,
	OFFHAND,
	MELEE,
	ARMOR,
	RELIC,
	COUNT,
};

enum class BagType : size_t {
	OUTFIT,
	BACKPACK,
	COUNT,
};

// Bags contain multiple slots
struct _Bag {

	_Bag();
	int GetItemCount() const;

	std::vector<_Item *> Slots;
	std::vector<int> TypeCount;
	int HighestType{0};
	bool Equipment{false};
};

// Contains bag and index
struct _Slot {

	_Slot() { }
	_Slot(_Bag *Bag, size_t Index) : Bag(Bag), Index(Index) { }

	bool operator==(const _Slot &Slot) const { return this->Bag == Slot.Bag && this->Index == Slot.Index; }
	bool operator!=(const _Slot &Slot) const { return !(*this == Slot); }

	void Reset() { Bag = nullptr; Index = (size_t)-1; }

	bool IsValidIndex() const { return Bag && Index < Bag->Slots.size(); }
	bool IsHandIndex() const { return Index == EquipmentType::MAINHAND || Index == EquipmentType::OFFHAND; }
	bool IsEquipmentSlot() const { return Bag->Equipment; }

	_Item *GetItem() const { return Bag->Slots[Index]; }
	void SetItem(_Item *Item) const { Bag->Slots[Index] = Item; }
	void RemoveItem() const { Bag->Slots[Index] = nullptr; }
	void DeleteItem() const;

	_Bag *Bag{nullptr};
	size_t Index{(size_t)-1};
};

// Containers hold multiple bags
typedef std::vector<_Bag> _Container;

// Classes
class _Inventory  {

	public:

		_Inventory();
		~_Inventory();

		void Serialize(ae::_Buffer &Buffer);
		void Unserialize(ae::_Buffer &Buffer);

		void AddBag(BagType Type);

		int GetItemCount(BagType Type, size_t BagIndex);
		void UpdateTypeCount();
		const char *GetBackpackIcon(size_t BagIndex);
		size_t GetSuitableBackpackBag(const _Item *Item, size_t StartIndex);

		std::vector<_Container> Containers;

	private:

		void SerializeContainer(ae::_Buffer &Buffer, const _Container &Container) const;
		void UnserializeContainer(ae::_Buffer &Buffer, _Container &Container, size_t BagSize, bool Equipment);
		_Item *UnserializeItem(ae::_Buffer &Buffer);

};
