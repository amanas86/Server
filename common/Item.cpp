/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2003 EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "debug.h"
#include "StringUtil.h"
#include "Item.h"
#include "database.h"
#include "misc.h"
#include "races.h"
#include "shareddb.h"
#include "classes.h"

#include <limits.h>

#include <sstream>
#include <iostream>

const int16 ServerInventoryLimits[SlotType_Count] =
{
	SIZE_POSSESSIONS,	SIZE_BANK,					SIZE_SHAREDBANK,
	SIZE_TRADE,			SIZE_WORLD,					SIZE_LIMBO,
	SIZE_TRIBUTE,		SIZE_TROPHYTRIBUTE,			SIZE_GUILDTRIBUTE,
	SIZE_MERCHANT,		SIZE_DELETED,				SIZE_CORPSE,
	SIZE_BAZAAR,		SIZE_INSPECT,				SIZE_REALESTATE,
	SIZE_VIEWMODPC,		SIZE_VIEWMODBANK,			SIZE_VIEWMODSHAREDBANK,
	SIZE_VIEWMODLIMBO,	SIZE_ALTSTORAGE,			SIZE_ARCHIVED,
	SIZE_MAIL,			SIZE_GUILDTROPHYTRIBUTE,	SIZE_KRONO,
	SIZE_OTHER
};

int32 NextItemInstSerialNumber = 1;

static inline int32 GetNextItemInstSerialNumber()
{
	// The Bazaar relies on each item a client has up for Trade having a unique
	// identifier. This 'SerialNumber' is sent in Serialized item packets and
	// is used in Bazaar packets to identify the item a player is buying or inspecting.
	//
	// E.g. A trader may have 3 Five dose cloudy potions, each with a different number of remaining charges
	// up for sale with different prices.
	//
	// NextItemInstSerialNumber is the next one to hand out.
	//
	// It is very unlikely to reach 2,147,483,647. Maybe we should call abort(), rather than wrapping back to 1.
	
	if(NextItemInstSerialNumber >= INT_MAX) { NextItemInstSerialNumber = 1; }
	else { NextItemInstSerialNumber++; }

	return NextItemInstSerialNumber;
}


/*
 Class: ManagedCursor ######################################################################
	<description>
 ###########################################################################################
*/
ManagedCursor::~ManagedCursor()
{
	// Will probably apply methods to 'Inventory' class to avoid additional coding and handlers
}
// ###########################################################################################


/*
 Class: ItemInstQueue ######################################################################
	DEPRECATED
 ###########################################################################################
*/
ItemInstQueue::~ItemInstQueue() {
	iter_queue cur,end;
	cur = m_list.begin();
	end = m_list.end();
	for(; cur != end; cur++) {
		ItemInst *tmp = * cur;
		safe_delete(tmp);
	}
	m_list.clear();
}

// Put item onto back of queue
void ItemInstQueue::push(ItemInst* inst)
{
	m_list.push_back(inst);
}

// Put item onto front of queue
void ItemInstQueue::push_front(ItemInst* inst)
{
	m_list.push_front(inst);
}

// Remove item from front of queue
ItemInst* ItemInstQueue::pop()
{
	if (m_list.size() == 0)
		return nullptr;

	ItemInst* inst = m_list.front();
	m_list.pop_front();
	return inst;
}

// Look at item at front of queue
ItemInst* ItemInstQueue::peek_front() const
{
	return (m_list.size()==0) ? nullptr : m_list.front();
}
// ###########################################################################################


/*
 Class: Inventory ######################################################################
	<description>
 #######################################################################################
*/
// Class deconstructor
Inventory::~Inventory()
{
	std::map<int16, ItemInst*>::iterator cur, end;

	cur = m_worn.begin();
	end = m_worn.end();
	for(; cur != end; cur++)
	{
		ItemInst *tmp = cur->second;
		safe_delete(tmp);
	}

	m_worn.clear();

	cur = m_inv.begin();
	end = m_inv.end();
	for(; cur != end; cur++)
	{
		ItemInst *tmp = cur->second;
		safe_delete(tmp);
	}

	m_inv.clear();

	cur = m_bank.begin();
	end = m_bank.end();
	for(; cur != end; cur++)
	{
		ItemInst *tmp = cur->second;
		safe_delete(tmp);
	}

	m_bank.clear();

	cur = m_shbank.begin();
	end = m_shbank.end();
	for(; cur != end; cur++)
	{
		ItemInst *tmp = cur->second;
		safe_delete(tmp);
	}

	m_shbank.clear();

	cur = m_trade.begin();
	end = m_trade.end();
	for(; cur != end; cur++)
	{
		ItemInst *tmp = cur->second;
		safe_delete(tmp);
	}

	m_trade.clear();
}

// Retrieve item at specified slot; returns false if item not found
ItemInst* Inventory::GetItem(int16 slot_id) const
{
	_CP(Inventory_GetItem);
	ItemInst* result = nullptr;

	// Cursor
	if(slot_id == SLOT_CURSOR)
	{
		result = m_cursor.peek_front();
	}

	// Non bag slots
	else if(slot_id >= 3000 && slot_id <= 3007)
	{
		// Trade slots
		result = _GetItem(m_trade, slot_id);
	}
	else if(slot_id >= 2500 && slot_id <= 2501)
	{
		// Shared Bank slots
		result = _GetItem(m_shbank, slot_id);
	}
	else if(slot_id >= 2000 && slot_id <= 2023)
	{
		// Bank slots
		result = _GetItem(m_bank, slot_id);
	}
	else if((slot_id >= 22 && slot_id <= 29))
	{
		// Personal inventory slots
		result = _GetItem(m_inv, slot_id);
	}
	else if((slot_id >= 0 && slot_id <= 21) || (slot_id >= 400 && slot_id <= 404) || (slot_id == 9999))
	{
		// Equippable slots (on body)
		result = _GetItem(m_worn, slot_id);
	}

	// Inner bag slots
	else if(slot_id >= 3031 && slot_id <= 3110)
	{
		// Trade bag slots
		ItemInst* inst = _GetItem(m_trade, Inventory::CalcSlotId(slot_id));

		if (inst && inst->IsType(ItemClassContainer))
		{
			result = inst->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	else if(slot_id >= 2531 && slot_id <= 2550)
	{
		// Shared Bank bag slots
		ItemInst* inst = _GetItem(m_shbank, Inventory::CalcSlotId(slot_id));

		if (inst && inst->IsType(ItemClassContainer))
		{
			result = inst->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	else if(slot_id >= 2031 && slot_id <= 2270)
	{
		// Bank bag slots
		ItemInst* inst = _GetItem(m_bank, Inventory::CalcSlotId(slot_id));

		if (inst && inst->IsType(ItemClassContainer))
		{
			result = inst->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	else if(slot_id >= 331 && slot_id <= 340)
	{
		// Cursor bag slots
		ItemInst* inst = m_cursor.peek_front();

		if (inst && inst->IsType(ItemClassContainer))
		{
			result = inst->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	else if(slot_id >= 251 && slot_id <= 330)
	{
		// Personal inventory bag slots
		ItemInst* inst = _GetItem(m_inv, Inventory::CalcSlotId(slot_id));

		if (inst && inst->IsType(ItemClassContainer))
		{
			result = inst->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}

	return result;
}

// Retrieve item at specified position within bag
ItemInst* Inventory::GetItem(int16 slot_id, uint8 bagidx) const
{
	return GetItem(Inventory::CalcSlotId(slot_id, bagidx));
}

int16 Inventory::PushCursor(const ItemInst& inst)
{
	m_cursor.push(inst.Clone());
	return SLOT_CURSOR;
}

// Put an item snto specified slot
int16 Inventory::PutItem(int16 slot_id, const ItemInst& inst)
{
	// Clean up item already in slot (if exists)
	DeleteItem(slot_id);

	// User is effectively deleting the item
	// in the slot, why hold a null ptr in map<>?
	if(!inst) { return slot_id; }

	// Delegate to internal method
	return _PutItem(slot_id, inst.Clone());
}

// Swap items in inventory
bool Inventory::SwapItem(int16 slot_a, int16 slot_b)
{
	// Temp holding areas for a and b
	ItemInst* inst_a = GetItem(slot_a);
	ItemInst* inst_b = GetItem(slot_b);

	InventorySlot_Struct placeholder; // placeholder for ItemInst->IsSlotAllowed() check

	if(inst_a && !inst_a->IsSlotAllowed(placeholder /*slot_b*/)) { return false; }
	if(inst_b && !inst_b->IsSlotAllowed(placeholder /*slot_a*/)) { return false; }

	_PutItem(slot_a, inst_b); // Copy b->a
	_PutItem(slot_b, inst_a); // Copy a->b

	return true;
}

// Checks that user has at least 'quantity' number of items in a given inventory slot
// Returns first slot it was found in, or SLOT_INVALID if not found

//This function has a flaw in that it only returns the last stack that it looked at
//when quantity is greater than 1 and not all of quantity can be found in 1 stack.
int16 Inventory::HasItem(uint32 item_id, uint8 quantity, uint8 where)
{
	_CP(Inventory_HasItem);
	int16 slot_id = SLOT_INVALID;

	//Altered by Father Nitwit to support a specification of
	//where to search, with a default value to maintain compatibility

	// Check each inventory bucket
	if(where & invWhereWorn)
	{
		slot_id = _HasItem(m_worn, item_id, quantity);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWherePersonal)
	{
		slot_id = _HasItem(m_inv, item_id, quantity);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWhereBank)
	{
		slot_id = _HasItem(m_bank, item_id, quantity);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWhereSharedBank)
	{
		slot_id = _HasItem(m_shbank, item_id, quantity);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWhereTrading)
	{
		slot_id = _HasItem(m_trade, item_id, quantity);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWhereCursor)
	{
		// Check cursor queue
		slot_id = _HasItem(m_cursor, item_id, quantity);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	return slot_id;
}

//this function has the same quantity flaw mentioned above in HasItem()
int16 Inventory::HasItemByUse(uint8 use, uint8 quantity, uint8 where)
{
	int16 slot_id = SLOT_INVALID;

	// Check each inventory bucket
	if(where & invWhereWorn)
	{
		slot_id = _HasItemByUse(m_worn, use, quantity);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWherePersonal)
	{
		slot_id = _HasItemByUse(m_inv, use, quantity);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWhereBank)
	{
		slot_id = _HasItemByUse(m_bank, use, quantity);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWhereSharedBank)
	{
		slot_id = _HasItemByUse(m_shbank, use, quantity);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWhereTrading)
	{
		slot_id = _HasItemByUse(m_trade, use, quantity);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWhereCursor)
	{
		// Check cursor queue
		slot_id = _HasItemByUse(m_cursor, use, quantity);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	return slot_id;
}

int16 Inventory::HasItemByLoreGroup(uint32 loregroup, uint8 where)
{
	int16 slot_id = SLOT_INVALID;

	// Check each inventory bucket
	if(where & invWhereWorn)
	{
		slot_id = _HasItemByLoreGroup(m_worn, loregroup);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWherePersonal)
	{
		slot_id = _HasItemByLoreGroup(m_inv, loregroup);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWhereBank)
	{
		slot_id = _HasItemByLoreGroup(m_bank, loregroup);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWhereSharedBank)
	{
		slot_id = _HasItemByLoreGroup(m_shbank, loregroup);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWhereTrading)
	{
		slot_id = _HasItemByLoreGroup(m_trade, loregroup);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	if(where & invWhereCursor)
	{
		// Check cursor queue
		slot_id = _HasItemByLoreGroup(m_cursor, loregroup);

		if(slot_id != SLOT_INVALID) { return slot_id; }
	}

	return slot_id;
}

// Since we don't return an actual location with this method, we should truncate this code to use
// one outer and one inner loop since the actual slot will be found eventually
bool Inventory::HasSpaceForItem(const Item_Struct *ItemToTry, int16 Quantity)
{
	if(ItemToTry->Stackable)
	{
		for(int16 i = 22; i <= 29; i++)
		{
			ItemInst* InvItem = GetItem(i);

			if(InvItem && (InvItem->GetItem()->ID == ItemToTry->ID) && (InvItem->GetCharges() < InvItem->GetItem()->StackSize))
			{
				int ChargeSlotsLeft = InvItem->GetItem()->StackSize - InvItem->GetCharges();

				if(Quantity <= ChargeSlotsLeft) { return true; }

				Quantity -= ChargeSlotsLeft;
			}

			if(InvItem && InvItem->IsType(ItemClassContainer))
			{
				int16 BaseSlotID = Inventory::CalcSlotId(i, 0);
				uint8 BagSize=InvItem->GetItem()->BagSlots;
				for(uint8 BagSlot = 0; BagSlot < BagSize; BagSlot++)
				{
					InvItem = GetItem(BaseSlotID + BagSlot);

					if(InvItem && (InvItem->GetItem()->ID == ItemToTry->ID) &&
						(InvItem->GetCharges() < InvItem->GetItem()->StackSize))
					{
						int ChargeSlotsLeft = InvItem->GetItem()->StackSize - InvItem->GetCharges();

						if(Quantity <= ChargeSlotsLeft) { return true; }

						Quantity -= ChargeSlotsLeft;
					}
				}
			}
		}
	}

	for(int16 i = 22; i <= 29; i++)
	{
		ItemInst* InvItem = GetItem(i);

		if (!InvItem)
		{
			if(!ItemToTry->Stackable)
			{
				if(Quantity == 1) { return true; }
				else { Quantity--; }
			}
			else
			{
				if(Quantity <= ItemToTry->StackSize) { return true; }
				else { Quantity -= ItemToTry->StackSize; }
			}
		}
		else if(InvItem->IsType(ItemClassContainer) && CanItemFitInContainer(ItemToTry, InvItem->GetItem()))
		{
			int16 BaseSlotID = Inventory::CalcSlotId(i, 0);
			uint8 BagSize = InvItem->GetItem()->BagSlots;
			for(uint8 BagSlot = 0; BagSlot < BagSize; BagSlot++)
			{
				InvItem = GetItem(BaseSlotID + BagSlot);

				if(!InvItem)
				{
					if(!ItemToTry->Stackable)
					{
						if(Quantity == 1) { return true; }
						else { Quantity--; }
					}
					else
					{
						if(Quantity <= ItemToTry->StackSize) { return true; }
						else { Quantity -= ItemToTry->StackSize; }
					}
				}
			}
		}
	}

	return false;
}

// Remove item from inventory (with memory delete)
bool Inventory::DeleteItem(int16 slot_id, uint8 quantity)
{
	// Pop item out of inventory map (or queue)
	ItemInst* item_to_delete = PopItem(slot_id);

	// Determine if object should be fully deleted, or
	// just a quantity of charges of the item can be deleted
	if(item_to_delete && (quantity > 0))
	{
		item_to_delete->SetCharges(item_to_delete->GetCharges() - quantity);

		// If there are no charges left on the item,
		if(item_to_delete->GetCharges() <= 0)
		{
			// If the item is stackable (e.g arrows), or
			// the item is not stackable, and is not a charged item, or is expendable, delete it
			if(item_to_delete->IsStackable() ||
				(!item_to_delete->IsStackable() &&
				((item_to_delete->GetItem()->MaxCharges == 0) || item_to_delete->IsExpendable())))
			{
				// Item can now be destroyed
				safe_delete(item_to_delete);
				return true;
			}
		}

		// Charges still exist, or it is a charged item that is not expendable. Put back into inventory
		_PutItem(slot_id, item_to_delete);
		return false;
	}

	safe_delete(item_to_delete);
	return true;
}

// Checks All items in a bag for No Drop
bool Inventory::CheckNoDrop(int16 slot_id)
{
	ItemInst* inst = GetItem(slot_id);

	if(!inst) { return false; }

	if(!inst->GetItem()->NoDrop) { return true; }

	if(inst->GetItem()->ItemClass == 1)
	{
		for(uint8 i = 0; i < 10; i++)
		{
			ItemInst* bagitem = GetItem(Inventory::CalcSlotId(slot_id, i));

			if(bagitem && !bagitem->GetItem()->NoDrop) { return true; }
		}
	}

	return false;
}

// Remove item from bucket without memory delete
// Returns item pointer if full delete was successful
ItemInst* Inventory::PopItem(int16 slot_id)
{
	ItemInst* p = nullptr;

	if(slot_id == SLOT_CURSOR)
	{ // Cursor
		p = m_cursor.pop();
	}
	else if((slot_id >= 0 && slot_id <= 21) || (slot_id >= 400 && slot_id <= 404) || (slot_id == 9999))
	{ // Worn slots
		p = m_worn[slot_id];
		m_worn.erase(slot_id);
	}
	else if((slot_id >= 22 && slot_id <= 29))
	{
		p = m_inv[slot_id];
		m_inv.erase(slot_id);
	}
	else if(slot_id >= 2000 && slot_id <= 2023)
	{ // Bank slots
		p = m_bank[slot_id];
		m_bank.erase(slot_id);
	}
	else if(slot_id >= 2500 && slot_id <= 2501)
	{ // Shared bank slots
		p = m_shbank[slot_id];
		m_shbank.erase(slot_id);
	}
	else if(slot_id >= 3000 && slot_id <= 3007)
	{ // Trade window slots
		p = m_trade[slot_id];
		m_trade.erase(slot_id);
	}
	else
	{
		// Is slot inside bag?
		ItemInst* baginst = GetItem(Inventory::CalcSlotId(slot_id));

		if(baginst != nullptr && baginst->IsType(ItemClassContainer))
		{
			p = baginst->PopItem(Inventory::CalcBagIdx(slot_id));
		}
	}

	// Return pointer that needs to be deleted (or otherwise managed)
	return p;
}

// Locate an available inventory slot
// Returns slot_id when there's one available, else SLOT_INVALID
int16 Inventory::FindFreeSlot(bool for_bag, bool try_cursor, uint8 min_size, bool is_arrow)
{
	// Check basic inventory
	for(int16 i=22; i<=29; i++)
	{
		// Found available slot in personal inventory
		if(!GetItem(i)) { return i; }
	}

	if(!for_bag)
	{
		for (int16 i = 22; i <= 29; i++)
		{
			const ItemInst* inst = GetItem(i);

			if(inst && inst->IsType(ItemClassContainer)
				&& inst->GetItem()->BagSize >= min_size)
			{
				if(inst->GetItem()->BagType == bagTypeQuiver && inst->GetItem()->ItemType != ItemTypeArrow)
				{
					continue;
				}

				int16 base_slot_id = Inventory::CalcSlotId(i, 0);

				uint8 slots = inst->GetItem()->BagSlots;
				uint8 j;
				for(j = 0; j < slots; j++)
				{
					if(!GetItem(base_slot_id + j))
						// Found available slot within bag
						return (base_slot_id + j);
				}
			}
		}
	}

	// Always room on cursor (it's a queue)
	// (we may wish to cap this in the future)
	if(try_cursor) { return SLOT_CURSOR; }

	// Note on cursor: We may want to be a little lenient with try_cursor using the following:
	// cursor (Slot_Cursor) >> limbo (to Max) >> deleted (to Max), and then delete any items above this.
	// If they can't clear their cursors before filling up limbo and deleted, I have no sympathy...

	// No available slots
	return SLOT_INVALID;
}

void Inventory::dumpBagContents(ItemInst *inst, iter_inst *it)
{
	iter_contents itb;

	if(!inst || !inst->IsType(ItemClassContainer)) { return; }

	// Go through bag, if bag
	for(itb = inst->_begin(); itb != inst->_end(); itb++)
	{
		ItemInst* baginst = itb->second;

		if(!baginst || !baginst->GetItem()) { continue; }

		std::string subSlot;
		StringFormat(subSlot,"	Slot %d: %s (%d)", Inventory::CalcSlotId((*it)->first, itb->first),
			baginst->GetItem()->Name, (baginst->GetCharges() <= 0) ? 1 : baginst->GetCharges());
		std::cout << subSlot << std::endl;
	}
}

void Inventory::dumpItemCollection(const std::map<int16, ItemInst*> &collection)
{
	iter_inst it;
	iter_contents itb;
	ItemInst* inst = nullptr;
	
	for(it = collection.begin(); it != collection.end(); it++)
	{
		inst = it->second;
		it->first;

		if(!inst || !inst->GetItem()) { continue; }
		
		std::string slot;
		StringFormat(slot, "Slot %d: %s (%d)", it->first, it->second->GetItem()->Name, (inst->GetCharges() <= 0) ? 1 : inst->GetCharges());
		std::cout << slot << std::endl;		

		dumpBagContents(inst, &it);
	}
}

void Inventory::dumpWornItems()
{
	std::cout << "Worn items:" << std::endl;
	dumpItemCollection(m_worn);
}

void Inventory::dumpInventory()
{
	std::cout << "Inventory items:" << std::endl;
	dumpItemCollection(m_inv);
}

void Inventory::dumpBankItems()
{
	std::cout << "Bank items:" << std::endl;
	dumpItemCollection(m_bank);
}

void Inventory::dumpSharedBankItems()
{
	std::cout << "Shared Bank items:" << std::endl;
	dumpItemCollection(m_shbank);
}

void Inventory::dumpEntireInventory()
{
	dumpWornItems();
	dumpInventory();
	dumpBankItems();
	dumpSharedBankItems();
	
	std::cout << std::endl;
}

// Internal Method: Retrieves item within an inventory bucket
ItemInst* Inventory::_GetItem(const std::map<int16, ItemInst*>& bucket, int16 slot_id) const
{
	iter_inst it = bucket.find(slot_id);

	if(it != bucket.end()) { return it->second; }

	// Not found!
	return nullptr;
}

// Internal Method: "put" item into bucket, without regard for what is currently in bucket
// Assumes item has already been allocated
int16 Inventory::_PutItem(int16 slot_id, ItemInst* inst)
{
	// If putting a nullptr into slot, we need to remove slot without memory delete
	if(inst == nullptr)
	{
		//Why do we not delete the poped item here????
		PopItem(slot_id);
		return slot_id;
	}

	int16 result = SLOT_INVALID;

	if(slot_id == SLOT_CURSOR)
	{ // Cursor
		// Replace current item on cursor, if exists
		m_cursor.pop(); // no memory delete, clients of this function know what they are doing
		m_cursor.push_front(inst);
		result = slot_id;
	}
	else if((slot_id >= 0 && slot_id <= 21) || (slot_id >= 400 && slot_id <= 404) || (slot_id == 9999))
	{ // Worn slots
		m_worn[slot_id] = inst;
		result = slot_id;
	}
	else if((slot_id >= 22 && slot_id <= 29))
	{
		m_inv[slot_id] = inst;
		result = slot_id;
	}
	else if(slot_id >= 2000 && slot_id <= 2023)
	{ // Bank slots
		m_bank[slot_id] = inst;
		result = slot_id;
	}
	else if(slot_id >= 2500 && slot_id <= 2501)
	{ // Shared bank slots
		m_shbank[slot_id] = inst;
		result = slot_id;
	}
	else if(slot_id >= 3000 && slot_id <= 3007)
	{ // Trade window slots
		m_trade[slot_id] = inst;
		result = slot_id;
	}
	else
	{
		// Slot must be within a bag
		ItemInst* baginst = GetItem(Inventory::CalcSlotId(slot_id)); // Get parent bag

		if(baginst && baginst->IsType(ItemClassContainer))
		{
			baginst->_PutItem(Inventory::CalcBagIdx(slot_id), inst);
			result = slot_id;
		}
	}

	if(result == SLOT_INVALID)
	{
		LogFile->write(EQEMuLog::Error, "Inventory::_PutItem: Invalid slot_id specified (%i)", slot_id);
		safe_delete(inst); // Slot not found, clean up
	}

	return result;
}

// Internal Method: Checks an inventory bucket for a particular item
int16 Inventory::_HasItem(std::map<int16, ItemInst*>& bucket, uint32 item_id, uint8 quantity)
{
	iter_inst it;
	iter_contents itb;
	ItemInst* inst = nullptr;
	uint8 quantity_found = 0;

	// Check item: After failed checks, check bag contents (if bag)
	for(it = bucket.begin(); it != bucket.end(); it++)
	{
		inst = it->second;

		if(inst)
		{
			if(inst->GetID() == item_id)
			{
				quantity_found += (inst->GetCharges() <= 0) ? 1 : inst->GetCharges();

				if(quantity_found >= quantity) { return it->first; }
			}

			for(int i = 0; i < MAX_AUGMENT_SLOTS; i++)
			{
				if(inst->GetAugmentItemID(i) == item_id && quantity <= 1) { return SLOT_AUGMENT; } // Only one augment per slot.
			}
		}

		// Go through bag, if bag
		if(inst && inst->IsType(ItemClassContainer))
		{
			for(itb = inst->_begin(); itb != inst->_end(); itb++)
			{
				ItemInst* baginst = itb->second;

				if(baginst->GetID() == item_id)
				{
					quantity_found += (baginst->GetCharges() <= 0) ? 1 : baginst->GetCharges();

					if(quantity_found >= quantity) { return Inventory::CalcSlotId(it->first, itb->first); }
				}

				for(int i = 0; i < MAX_AUGMENT_SLOTS; i++)
				{
					if(baginst->GetAugmentItemID(i) == item_id && quantity <= 1) { return SLOT_AUGMENT; } // Only one augment per slot.
				}
			}
		}
	}

	// Not found
	return SLOT_INVALID;
}

// Internal Method: Checks an inventory queue type bucket for a particular item
int16 Inventory::_HasItem(ItemInstQueue& iqueue, uint32 item_id, uint8 quantity)
{
	iter_queue it;
	iter_contents itb;
	uint8 quantity_found = 0;

	// Read-only iteration of queue
	for(it = iqueue.begin(); it != iqueue.end(); it++)
	{
		ItemInst* inst = *it;

		if(inst)
		{
			if(inst->GetID() == item_id)
			{
				quantity_found += (inst->GetCharges() <= 0) ? 1 : inst->GetCharges();

				if(quantity_found >= quantity) { return SLOT_CURSOR; }
			}

			for(int i = 0; i < MAX_AUGMENT_SLOTS; i++)
			{
				if(inst->GetAugmentItemID(i) == item_id && quantity <= 1) { return SLOT_AUGMENT; } // Only one augment per slot.
			}
		}

		// Go through bag, if bag
		if(inst && inst->IsType(ItemClassContainer))
		{
			for(itb = inst->_begin(); itb != inst->_end(); itb++)
			{
				ItemInst* baginst = itb->second;

				if(baginst->GetID() == item_id)
				{
					quantity_found += (baginst->GetCharges() <= 0) ? 1 : baginst->GetCharges();

					if(quantity_found >= quantity) { return Inventory::CalcSlotId(SLOT_CURSOR, itb->first); }
				}

				for(int i = 0; i < MAX_AUGMENT_SLOTS; i++)
				{
					if(baginst->GetAugmentItemID(i) == item_id && quantity <= 1) { return SLOT_AUGMENT; } // Only one augment per slot.
				}
			}
		}
	}

	// Not found
	return SLOT_INVALID;
}

// Internal Method: Checks an inventory bucket for a particular item
int16 Inventory::_HasItemByUse(std::map<int16, ItemInst*>& bucket, uint8 use, uint8 quantity)
{
	iter_inst it;
	iter_contents itb;
	ItemInst* inst = nullptr;
	uint8 quantity_found = 0;

	// Check item: After failed checks, check bag contents (if bag)
	for(it=bucket.begin(); it!=bucket.end(); it++)
	{
		inst = it->second;

		if(inst && inst->IsType(ItemClassCommon) && inst->GetItem()->ItemType == use)
		{
			quantity_found += (inst->GetCharges() <= 0) ? 1 : inst->GetCharges();

			if(quantity_found >= quantity) { return it->first; }
		}

		// Go through bag, if bag
		if(inst && inst->IsType(ItemClassContainer))
		{
			for(itb = inst->_begin(); itb != inst->_end(); itb++)
			{
				ItemInst* baginst = itb->second;

				if(baginst && baginst->IsType(ItemClassCommon) && baginst->GetItem()->ItemType == use)
				{
					quantity_found += (baginst->GetCharges() <= 0) ? 1 : baginst->GetCharges();

					if(quantity_found >= quantity) { return Inventory::CalcSlotId(it->first, itb->first); }
				}
			}
		}
	}

	// Not found
	return SLOT_INVALID;
}

// Internal Method: Checks an inventory queue type bucket for a particular item
int16 Inventory::_HasItemByUse(ItemInstQueue& iqueue, uint8 use, uint8 quantity)
{
	iter_queue it;
	iter_contents itb;
	uint8 quantity_found = 0;

	// Read-only iteration of queue
	for(it = iqueue.begin(); it != iqueue.end(); it++)
	{
		ItemInst* inst = *it;

		if(inst && inst->IsType(ItemClassCommon) && inst->GetItem()->ItemType == use)
		{
			quantity_found += (inst->GetCharges() <= 0) ? 1 : inst->GetCharges();

			if(quantity_found >= quantity) { return SLOT_CURSOR; }
		}

		// Go through bag, if bag
		if(inst && inst->IsType(ItemClassContainer))
		{
			for(itb = inst->_begin(); itb != inst->_end(); itb++)
			{
				ItemInst* baginst = itb->second;

				if(baginst && baginst->IsType(ItemClassCommon) && baginst->GetItem()->ItemType == use)
				{
					quantity_found += (baginst->GetCharges() <= 0) ? 1 : baginst->GetCharges();

					if(quantity_found >= quantity) { return Inventory::CalcSlotId(SLOT_CURSOR, itb->first); }
				}
			}
		}
	}

	// Not found
	return SLOT_INVALID;
}

int16 Inventory::_HasItemByLoreGroup(std::map<int16, ItemInst*>& bucket, uint32 loregroup)
{
	iter_inst it;
	iter_contents itb;
	ItemInst* inst = nullptr;

	// Check item: After failed checks, check bag contents (if bag)
	for(it = bucket.begin(); it != bucket.end(); it++)
	{
		inst = it->second;

		if(inst)
		{
			if(inst->GetItem()->LoreGroup == loregroup) { return it->first; }

			ItemInst* Aug;
			for(int i = 0; i < MAX_AUGMENT_SLOTS; i++)
			{
				Aug = inst->GetAugment(i);

				if(Aug && Aug->GetItem()->LoreGroup == loregroup) { return SLOT_AUGMENT; } // Only one augment per slot.
			}
		}

		// Go through bag, if bag
		if(inst && inst->IsType(ItemClassContainer))
		{
			for(itb = inst->_begin(); itb != inst->_end(); itb++)
			{
				ItemInst* baginst = itb->second;

				if(baginst && baginst->IsType(ItemClassCommon)&& baginst->GetItem()->LoreGroup == loregroup)
				{
					return Inventory::CalcSlotId(it->first, itb->first);
				}

				ItemInst* Aug2;
				for(int i = 0; i < MAX_AUGMENT_SLOTS; i++)
				{
					Aug2 = baginst->GetAugment(i);

					if(Aug2 && Aug2->GetItem()->LoreGroup == loregroup) { return SLOT_AUGMENT; } // Only one augment per slot.
				}
			}
		}
	}

	// Not found
	return SLOT_INVALID;
}

// Internal Method: Checks an inventory queue type bucket for a particular item
int16 Inventory::_HasItemByLoreGroup(ItemInstQueue& iqueue, uint32 loregroup)
{
	iter_queue it;
	iter_contents itb;

	// Read-only iteration of queue
	for(it = iqueue.begin(); it != iqueue.end(); it++)
	{
		ItemInst* inst = *it;

		if(inst)
		{
			if(inst->GetItem()->LoreGroup == loregroup) { return SLOT_CURSOR; }

			ItemInst* Aug;
			for(int i = 0; i < MAX_AUGMENT_SLOTS; i++)
			{
				Aug = inst->GetAugment(i);

				if(Aug && Aug->GetItem()->LoreGroup == loregroup) { return SLOT_AUGMENT; } // Only one augment per slot.
			}
		}

		// Go through bag, if bag
		if(inst && inst->IsType(ItemClassContainer))
		{
			for(itb = inst->_begin(); itb != inst->_end(); itb++)
			{
				ItemInst* baginst = itb->second;

				if(baginst && baginst->IsType(ItemClassCommon)&& baginst->GetItem()->LoreGroup == loregroup)
				{
					return Inventory::CalcSlotId(SLOT_CURSOR, itb->first);
				}

				ItemInst* Aug2;
				for(int i = 0; i < MAX_AUGMENT_SLOTS; i++)
				{
					Aug2 = baginst->GetAugment(i);

					if(Aug2 && Aug2->GetItem()->LoreGroup == loregroup) { return SLOT_AUGMENT; } // Only one augment per slot.
				}
			}
		}
	}

	// Not found
	return SLOT_INVALID;
}

void Inventory::InvalidateSlotStruct(InventorySlot_Struct &is_struct)
{
	is_struct.slottype	= SLOTTYPE_INVALID;
	is_struct.unknown02	= 0;
	is_struct.mainslot	= MAINSLOT_INVALID;
	is_struct.subslot	= SUBSLOT_INVALID;
	is_struct.augslot	= AUGSLOT_INVALID;
	is_struct.unknown01	= 0;
}

bool Inventory::IsValidServerSlotStruct(InventorySlot_Struct &is_struct)
{
	if(&is_struct == nullptr) { return false; }

	if(IsDeleteRequest(is_struct)) { return false; }

	// in-work

	return false;
}

bool Inventory::IsValidMobSlotStruct(InventorySlot_Struct &is_struct)
{
	if(&is_struct == nullptr) { return false; }

	if(IsDeleteRequest(is_struct)) { return true; }

	// in-work

	return false;
}

bool Inventory::IsValidClientSlotStruct(InventorySlot_Struct &is_struct)
{
	if(&is_struct == nullptr) { return false; }

	if(IsDeleteRequest(is_struct)) { return true; }

	// in-work

	return false;
}

// This is only valid for clients and mobs. This position does not exist on the server
bool Inventory::IsDeleteRequest(InventorySlot_Struct &is_struct)
{
	return (is_struct.slottype == SLOTTYPE_INVALID &&
		is_struct.mainslot == MAINSLOT_INVALID &&
		is_struct.subslot == SUBSLOT_INVALID &&
		is_struct.augslot == AUGSLOT_INVALID);
}

// Calculate slot_id for an item within a bag
int16 Inventory::CalcSlotId(int16 bagslot_id, uint8 bagidx)
{
	if(!Inventory::SupportsContainers(bagslot_id)) { return SLOT_INVALID; }

	int16 slot_id = SLOT_INVALID;

	if(bagslot_id == SLOT_CURSOR || bagslot_id == 8000) // Cursor
	{
		slot_id = IDX_CURSOR_BAG + bagidx;
	}
	else if(bagslot_id >= 22 && bagslot_id <= 29) // Inventory slots
	{
		slot_id = IDX_INV_BAG + (bagslot_id - 22) * MAX_ITEMS_PER_BAG + bagidx;
	}
	else if(bagslot_id >= 2000 && bagslot_id <= 2023) // Bank slots
	{
		slot_id = IDX_BANK_BAG + (bagslot_id - 2000) * MAX_ITEMS_PER_BAG + bagidx;
	}
	else if(bagslot_id >= 2500 && bagslot_id <= 2501) // Shared bank slots
	{
		slot_id = IDX_SHBANK_BAG + (bagslot_id - 2500) * MAX_ITEMS_PER_BAG + bagidx;
	}
	else if(bagslot_id >= 3000 && bagslot_id <= 3007) // Trade window slots
	{
		slot_id = IDX_TRADE_BAG + (bagslot_id - 3000) * MAX_ITEMS_PER_BAG + bagidx;
	}

	return slot_id;
}

// Opposite of above: Get parent bag slot_id from a slot inside of bag
int16 Inventory::CalcSlotId(int16 slot_id)
{
	int16 parent_slot_id = SLOT_INVALID;

	if(slot_id >= 251 && slot_id <= 330)
	{
		parent_slot_id = IDX_INV + (slot_id - 251) / MAX_ITEMS_PER_BAG;
	}
	else if(slot_id >= 331 && slot_id <= 340)
	{
		parent_slot_id = SLOT_CURSOR;
	}
	else if(slot_id >= 2000 && slot_id <= 2023)
	{
		parent_slot_id = IDX_BANK + (slot_id - 2000) / MAX_ITEMS_PER_BAG;
	}
	else if(slot_id >= 2031 && slot_id <= 2270)
	{
		parent_slot_id = IDX_BANK + (slot_id - 2031) / MAX_ITEMS_PER_BAG;
	}
	else if(slot_id >= 2531 && slot_id <= 2550)
	{
		parent_slot_id = IDX_SHBANK + (slot_id - 2531) / MAX_ITEMS_PER_BAG;
	}
	else if(slot_id >= 3100 && slot_id <= 3179)
	{
		parent_slot_id = IDX_TRADE + (slot_id - 3100) / MAX_ITEMS_PER_BAG;
	}

	return parent_slot_id;
}

uint8 Inventory::CalcBagIdx(int16 slot_id)
{
	uint8 index = 0;

	if(slot_id >= 251 && slot_id <= 330)
	{
		index = (slot_id - 251) % MAX_ITEMS_PER_BAG;
	}
	else if(slot_id >= 331 && slot_id <= 340)
	{
		index = (slot_id - 331) % MAX_ITEMS_PER_BAG;
	}
	else if(slot_id >= 2000 && slot_id <= 2023)
	{
		index = (slot_id - 2000) % MAX_ITEMS_PER_BAG;
	}
	else if(slot_id >= 2031 && slot_id <= 2270)
	{
		index = (slot_id - 2031) % MAX_ITEMS_PER_BAG;
	}
	else if(slot_id >= 2531 && slot_id <= 2550)
	{
		index = (slot_id - 2531) % MAX_ITEMS_PER_BAG;
	}
	else if(slot_id >= 3100 && slot_id <= 3179)
	{
		index = (slot_id - 3100) % MAX_ITEMS_PER_BAG;
	}
	else if(slot_id >= 4000 && slot_id <= 4009)
	{
		index = (slot_id - 4000) % MAX_ITEMS_PER_BAG;
	}

	return index;
}

int16 Inventory::CalcSlotFromMaterial(uint8 material)
{
	switch(material)
	{
		case MATERIAL_HEAD:			{ return SLOT_HEAD; }
		case MATERIAL_CHEST:		{ return SLOT_CHEST; }
		case MATERIAL_ARMS:			{ return SLOT_ARMS; }
		case MATERIAL_BRACER:		{ return SLOT_BRACER01; } // there's 2 bracers, only one bracer material
		case MATERIAL_HANDS:		{ return SLOT_HANDS; }
		case MATERIAL_LEGS:			{ return SLOT_LEGS; }
		case MATERIAL_FEET:			{ return SLOT_FEET; }
		case MATERIAL_PRIMARY:		{ return SLOT_PRIMARY; }
		case MATERIAL_SECONDARY:	{ return SLOT_SECONDARY; }
		default: { return -1; }
	}
}

uint8 Inventory::CalcMaterialFromSlot(int16 equipslot)
{
	switch(equipslot)
	{
		case SLOT_HEAD:			{ return MATERIAL_HEAD; }
		case SLOT_CHEST:		{ return MATERIAL_CHEST; }
		case SLOT_ARMS:			{ return MATERIAL_ARMS; }
		case SLOT_BRACER01:
		case SLOT_BRACER02:		{ return MATERIAL_BRACER; }
		case SLOT_HANDS:		{ return MATERIAL_HANDS; }
		case SLOT_LEGS:			{ return MATERIAL_LEGS; }
		case SLOT_FEET:			{ return MATERIAL_FEET; }
		case SLOT_PRIMARY:		{ return MATERIAL_PRIMARY; }
		case SLOT_SECONDARY:	{ return MATERIAL_SECONDARY; }
		default: { return 0xFF; }
	}
}

// Test whether a given slot can support a container item
bool Inventory::SupportsContainers(int16 slot_id)
{
	return ((slot_id >= 22 && slot_id <= 30) ||		// Personal inventory slots
		(slot_id >= 2000 && slot_id <= 2023) ||		// Bank slots
		(slot_id >= 2500 && slot_id <= 2501) ||		// Shared bank slots
		(slot_id == SLOT_CURSOR) ||					// Cursor
		(slot_id >= 3000 && slot_id <= 3007));		// Trade window
}

bool Inventory::CanItemFitInContainer(const Item_Struct *ItemToTry, const Item_Struct *Container)
{
	if(!ItemToTry || !Container) { return false; }

	if(ItemToTry->Size > Container->BagSize) { return false; }

	if((Container->BagType == bagTypeQuiver) && (ItemToTry->ItemType != ItemTypeArrow)) { return false; }

	if((Container->BagType == bagTypeBandolier) && (ItemToTry->ItemType != ItemTypeThrowingv2)) { return false; }

	return true;
}
// #######################################################################################


/*
 Class: InventoryLimits ######################################################################
	<description>
 #############################################################################################
*/
InventoryLimits::~InventoryLimits()
{
	memset(this, 0, sizeof(InventoryLimits));
}

bool InventoryLimits::SetServerInventoryLimits(InventoryLimits &limits)
{
	if(limits.m_limits_set) { return false; }
	
	memcpy(limits.m_slottypesize, ServerInventoryLimits, sizeof(InventoryLimits.m_slottypesize));
	/*
	limits.m_slottypesize[SlotType_Possessions]			= SIZE_POSSESSIONS;
	limits.m_slottypesize[SlotType_Bank]				= SIZE_BANK;
	limits.m_slottypesize[SlotType_SharedBank]			= SIZE_SHAREDBANK;
	limits.m_slottypesize[SlotType_Trade]				= SIZE_TRADE;
	limits.m_slottypesize[SlotType_World]				= SIZE_WORLD;
	limits.m_slottypesize[SlotType_Limbo]				= SIZE_LIMBO;
	limits.m_slottypesize[SlotType_Tribute]				= SIZE_TRIBUTE;
	limits.m_slottypesize[SlotType_TrophyTribute]		= SIZE_TROPHYTRIBUTE;
	limits.m_slottypesize[SlotType_GuildTribute]		= SIZE_GUILDTRIBUTE;
	limits.m_slottypesize[SlotType_Merchant]			= SIZE_MERCHANT;
	limits.m_slottypesize[SlotType_Deleted]				= SIZE_DELETED;
	limits.m_slottypesize[SlotType_Corpse]				= SIZE_CORPSE;
	limits.m_slottypesize[SlotType_Bazaar]				= SIZE_BAZAAR;
	limits.m_slottypesize[SlotType_Inspect]				= SIZE_INSPECT;
	limits.m_slottypesize[SlotType_RealEstate]			= SIZE_REALESTATE;
	limits.m_slottypesize[SlotType_ViewMODPC]			= SIZE_VIEWMODPC;
	limits.m_slottypesize[SlotType_ViewMODBank]			= SIZE_VIEWMODBANK;
	limits.m_slottypesize[SlotType_ViewMODSharedBank]	= SIZE_VIEWMODSHAREDBANK;
	limits.m_slottypesize[SlotType_ViewMODLimbo]		= SIZE_VIEWMODLIMBO;
	limits.m_slottypesize[SlotType_AltStorage]			= SIZE_ALTSTORAGE;
	limits.m_slottypesize[SlotType_Archived]			= SIZE_ARCHIVED;
	limits.m_slottypesize[SlotType_Mail]				= SIZE_MAIL;
	limits.m_slottypesize[SlotType_GuildTrophyTribute]	= SIZE_GUILDTROPHYTRIBUTE;
	limits.m_slottypesize[SlotType_Krono]				= SIZE_KRONO;
	limits.m_slottypesize[SlotType_Other]				= SIZE_OTHER;
	*/

	limits.m_equipmentstart								= EQUIPMENT_START;
	limits.m_equipmentend								= EQUIPMENT_END;
	limits.m_equipmentbitmask							= EQUIPMENT_BITMASK;
	limits.m_personalstart								= PERSONAL_START;
	limits.m_personalend								= PERSONAL_END;
	limits.m_personalbitmask							= PERSONAL_BITMASK;

	limits.m_bandolierslotsmax							= MAX_BANDOLIERSLOTS;
	limits.m_potionbeltslotsmax							= MAX_POTIONBELTSLOTS;
	limits.m_bagslotsmax								= MAX_BAGSLOTS;
	limits.m_augmentsmax								= MAX_AUGMENTS;

	limits.m_limits_set = true;

	return true;
}

bool InventoryLimits::SetMobInventoryLimits(InventoryLimits &limits)
{
	if(limits.m_limits_set) { return false; }

	SetServerInventoryLimits(limits);
	
	// Non-Client slot type sizes (Set slot types unused by NPC to SIZE_UNUSED)
	limits.m_slottypesize[SlotType_Bank]				= SIZE_UNUSED;
	limits.m_slottypesize[SlotType_SharedBank]			= SIZE_UNUSED;
	limits.m_slottypesize[SlotType_World]				= SIZE_UNUSED;
	limits.m_slottypesize[SlotType_Limbo]				= SIZE_UNUSED;
	limits.m_slottypesize[SlotType_Deleted]				= SIZE_UNUSED;
	limits.m_slottypesize[SlotType_RealEstate]			= SIZE_UNUSED;
	limits.m_slottypesize[SlotType_ViewMODPC]			= SIZE_UNUSED;
	limits.m_slottypesize[SlotType_ViewMODBank]			= SIZE_UNUSED;
	limits.m_slottypesize[SlotType_ViewMODSharedBank]	= SIZE_UNUSED;
	limits.m_slottypesize[SlotType_ViewMODLimbo]		= SIZE_UNUSED;
	limits.m_slottypesize[SlotType_AltStorage]			= SIZE_UNUSED;
	limits.m_slottypesize[SlotType_Archived]			= SIZE_UNUSED;
	limits.m_slottypesize[SlotType_Mail]				= SIZE_UNUSED;
	limits.m_slottypesize[SlotType_Krono]				= SIZE_UNUSED;
	limits.m_slottypesize[SlotType_Other]				= SIZE_UNUSED;

	limits.m_limits_set = true;

	return true;
}

bool InventoryLimits::SetClientInventoryLimits(InventoryLimits &limits, EQClientVersion client_version)
{
	if(limits.m_limits_set) { return false; }
	
	SetServerInventoryLimits(limits);
	
	// Client slot type sizes (Set slot types unused by client to SIZE_UNUSED)

	// Add new clients in descending order
	if(client_version < EQClientRoF)
	{
		limits.m_slottypesize[SlotType_Possessions]			= SIZE_POSSESSIONS_PRE_ROF;
		limits.m_slottypesize[SlotType_Corpse]				= SIZE_CORPSE_PRE_ROF;
		limits.m_slottypesize[SlotType_Bazaar]				= SIZE_BAZAAR_PRE_ROF;
			
		limits.m_personalend								= PERSONAL_END_PRE_ROF;
		limits.m_personalbitmask							= PERSONAL_BITMASK_PRE_ROF;

		limits.m_bagslotsmax								= MAX_BAGSLOTS_PRE_ROF;
		limits.m_augmentsmax								= MAX_AUGMENTS_PRE_ROF;
	}

	if(client_version < EQClientUnderfoot)
	{

	}

	if(client_version < EQClientSoD)
	{

	}

	if(client_version < EQClientSoF)
	{
		limits.m_slottypesize[SlotType_Possessions]			= SIZE_POSSESSIONS_PRE_SOF;
		limits.m_slottypesize[SlotType_Corpse]				= SIZE_CORPSE_PRE_SOF;
		limits.m_slottypesize[SlotType_Bank]				= SIZE_BANK_PRE_SOF;

		limits.m_equipmentbitmask							= EQUIPMENT_BITMASK_PRE_SOF;
	}

	if(client_version < EQClientTitanium)
	{
		limits.m_slottypesize[SlotType_Possessions]			= SIZE_POSSESSIONS_PRE_TI;
		limits.m_slottypesize[SlotType_Corpse]				= SIZE_CORPSE_PRE_TI;

		limits.m_equipmentbitmask							= EQUIPMENT_BITMASK_PRE_TI;
	}

	if(client_version < EQClient62)
	{
		// If we got here, we screwed the pooch somehow...
		// TODO: log error message
	}

	limits.m_limits_set = true;

	return true;
}

void InventoryLimits::ResetInventoryLimits()
{
	memset(this->m_slottypesize, 0, sizeof(InventoryLimits.m_slottypesize));
	
	m_equipmentstart	= 0;
	m_equipmentend		= 0;
	m_equipmentbitmask	= 0;
	m_personalstart		= 0;
	m_personalend		= 0;
	m_personalbitmask	= 0;

	m_bandolierslotsmax		= 0;
	m_potionbeltslotsmax	= 0;
	m_bagslotsmax			= 0;
	m_augmentsmax			= 0;

	m_limits_set = false;
}
// #############################################################################################


/*
 Class: ItemInst ######################################################################
	Base class for an instance of an item. An item instance encapsulates item data
	and data specific to an item instance (includes dye, augments, charges, etc)
 ######################################################################################
*/
// Class constructors
ItemInst::ItemInst(const Item_Struct* item, int16 charges)
{
	m_use_type		= ItemUseNormal;
	m_item			= item;
	m_charges		= charges;
	m_price			= 0;
	m_instnodrop	= false;
	m_merchantslot	= 0;

	if(m_item && m_item->ItemClass == ItemClassCommon) { m_color = m_item->Color; }
	else { m_color = 0; }

	m_merchantcount	= 1;
	m_serialnumber	= GetNextItemInstSerialNumber();
}

ItemInst::ItemInst(SharedDatabase *db, uint32 item_id, int16 charges)
{
	m_use_type		= ItemUseNormal;
	m_item			= db->GetItem(item_id);
	m_charges		= charges;
	m_price			= 0;
	m_merchantslot	= 0;
	m_instnodrop	= false;

	if(m_item && m_item->ItemClass == ItemClassCommon) { m_color = m_item->Color; }
	else { m_color = 0; }

	m_merchantcount	= 1;
	m_serialnumber	= GetNextItemInstSerialNumber();
}

ItemInst::ItemInst(const ItemInst& copy)
{
	m_use_type		= copy.m_use_type;
	m_item			= copy.m_item;
	m_charges		= copy.m_charges;
	m_price			= copy.m_price;
	m_color			= copy.m_color;
	m_merchantslot	= copy.m_merchantslot;
	m_currentslot	= copy.m_currentslot;
	m_instnodrop	= copy.m_instnodrop;
	m_merchantcount	= copy.m_merchantcount;

	iter_contents cc_iter;
	for(cc_iter = copy.m_contents.begin(); cc_iter != copy.m_contents.end(); cc_iter++) {
		ItemInst* inst_old = cc_iter->second;
		ItemInst* inst_new = nullptr;

		if(inst_old) { inst_new = inst_old->Clone(); }

		if(inst_new != nullptr) { m_contents[cc_iter->first] = inst_new; }
	}

	std::map<std::string, std::string>::const_iterator ccd_iter;
	for(ccd_iter = copy.m_customdata.begin(); ccd_iter != copy.m_customdata.end(); ccd_iter++)
	{
		m_customdata[ccd_iter->first] = ccd_iter->second;
	}

	m_serialnumber	= copy.m_serialnumber;
	m_customdata	= copy.m_customdata;
}

// Class deconstructor
ItemInst::~ItemInst()
{
	Clear();
}

// ItemInst methods
ItemInst* ItemInst::Clone() const
{
	// Clone a type of ItemInst object: c++ doesn't allow a polymorphic
	// copy constructor, so we have to resort to a polymorphic Clone()
	
	// Pseudo-polymorphic copy constructor
	return new ItemInst(*this);
}

bool ItemInst::IsType(ItemClass item_class) const
{
	if((m_use_type == ItemUseWorldContainer) && (item_class == ItemClassContainer)) { return true; }

	if(m_item) { return (m_item->ItemClass == item_class); }

	return false;
}

bool ItemInst::IsStackable() const
{
	if(m_item) { return m_item->Stackable; }

	return false;
}

bool ItemInst::IsEquipable(uint16 race_id, uint16 class_id) const
{
	if(!m_item || (m_item->Slots == 0)) { return false; }

	return m_item->IsEquipable(race_id, class_id);
}

bool ItemInst::IsEquipable(InventorySlot_Struct is_struct) const
{
	if(!m_item) { return false; }

	if(is_struct.slottype == SlotType_Possessions)
	{
		if(is_struct.subslot == SUBSLOT_INVALID && is_struct.augslot == AUGSLOT_INVALID)
		{
			if(is_struct.mainslot >= EQUIPMENT_START && is_struct.mainslot <= EQUIPMENT_END)
			{
				if(m_item->Slots & (1 << is_struct.mainslot)) { return true; }
			}
		}
	}

	return false;
}

bool ItemInst::IsWeapon() const
{
	if(!m_item || m_item->ItemClass != ItemClassCommon) { return false; }

	if(m_item->ItemType == ItemTypeArrow && m_item->Damage != 0) { return true; }
	else { return ((m_item->Damage != 0) && (m_item->Delay != 0)); }
}

bool ItemInst::IsAmmo() const
{
	if(!m_item) { return false; }

	return ((m_item->ItemType == ItemTypeArrow) ||
		(m_item->ItemType == ItemTypeThrowing) ||
		(m_item->ItemType == ItemTypeThrowingv2));
}

bool ItemInst::IsAugmentable() const
{
	if(!m_item) { return false; }
	
	for(uint8 aug_index = AUGSLOT_START; aug_index < MAX_AUGMENTS; aug_index++)
	{
		if(m_item->AugSlotType[aug_index]) { return true; }
	}

	return false;
}

bool ItemInst::IsSlotAllowed(InventorySlot_Struct is_struct) const
{
	if(!m_item) { return false; }
	else if(Inventory::SupportsContainers(0 /* placeholder for 'is_struct' */)) { return true; }
	else if(IsEquipable(is_struct)) { return true; }
	else if(is_struct.augslot == AUGSLOT_INVALID)
	{
		if(is_struct.slottype == SlotType_Possessions)
		{
			if(is_struct.mainslot >= PERSONAL_START && is_struct.mainslot < ServerInventoryLimits[SlotType_Possessions])
			{
				// The use of 'm_item->BagSlots' will prevent bag overloading - the use of illegal bag slots
				// ..otherwise, if there are issues, use '< MAX_BAGSLOTS' (same for below)

				if(is_struct.subslot >= SUBSLOT_INVALID && is_struct.subslot < m_item->BagSlots) { return true; }
			}
		}
		else if(is_struct.slottype > SlotType_Possessions && is_struct.slottype < SlotType_Count)
		{	
			if(is_struct.mainslot >= MAINSLOT_START && is_struct.mainslot < ServerInventoryLimits[is_struct.slottype])
			{
				if(is_struct.subslot >= SUBSLOT_INVALID && is_struct.subslot < m_item->BagSlots) { return true; }
			}
		}
	}

	return false;
}

bool ItemInst::IsNoneEmptyContainer()
{
	if(!m_item || m_item->ItemClass != ItemClassContainer) { return false; }

	for(int16 sub_slot = SUBSLOT_START; sub_slot < m_item->BagSlots; sub_slot++)
	{
		if(GetItem(sub_slot)) { return true; }
	}

	return false;
}

bool ItemInst::AvailableWearSlot(uint32 aug_wear_slots) const
{
	if(!m_item || m_item->ItemClass != ItemClassCommon) { return false; }

	uint32 wear_index;
	for(wear_index = EQUIPMENT_START; wear_index <= EQUIPMENT_END; wear_index++)
	{
		if(m_item->Slots & (1 << wear_index))
		{
			if(aug_wear_slots & (1 << wear_index)) { break; }
		}
	}

	return ((wear_index <= EQUIPMENT_END) ? true : false);
}

bool ItemInst::IsAugmented()
{
	if(!m_item || m_item->ItemClass != ItemClassCommon) { return false; }
	
	for(int16 aug_slot = AUGSLOT_START; aug_slot < MAX_AUGMENTS; aug_slot++)
	{
		if(GetAugmentItemID(aug_slot)) { return true; }
	}

	return false;
}

uint8 ItemInst::GetTotalItemCount() const
{
	uint8 item_count = 1;

	if(!m_item || m_item->ItemClass != ItemClassContainer) { return item_count; }

	for(int16 sub_slot = SUBSLOT_START; sub_slot < m_item->BagSlots; sub_slot++)
	{
		if(GetItem(sub_slot)) { item_count++; }
	}

	return item_count;
}

void ItemInst::Clear()
{
	iter_contents cur, end;
	cur = m_contents.begin();
	end = m_contents.end();
	for(; cur != end; cur++)
	{
		ItemInst* inst = cur->second;
		safe_delete(inst);
	}

	m_contents.clear();
}

void ItemInst::ClearByFlags(byFlagSetting is_nodrop, byFlagSetting is_norent)
{
	iter_contents cur, end, del;
	cur = m_contents.begin();
	end = m_contents.end();
	for(; cur != end;)
	{
		ItemInst* inst = cur->second;
		const Item_Struct* item = inst->GetItem();
		del = cur;
		cur++;

		if(!item) { continue; }

		switch(is_nodrop)
		{
			case byFlagSet:
			{
				if (item->NoDrop == 0)
				{
					safe_delete(inst);
					m_contents.erase(del->first);
					continue;
				}
			}
			case byFlagNotSet:
			{
				if (item->NoDrop != 0)
				{
					safe_delete(inst);
					m_contents.erase(del->first);
					continue;
				}
			}
			default: { break; }
		}

		switch(is_norent)
		{
			case byFlagSet:
			{
				if (item->NoRent == 0)
				{
					safe_delete(inst);
					m_contents.erase(del->first);
					continue;
				}
			}
			case byFlagNotSet:
			{
				if (item->NoRent != 0)
				{
					safe_delete(inst);
					m_contents.erase(del->first);
					continue;
				}
			}
			default: { break; }
		}
	}
}

std::string ItemInst::GetCustomDataString() const
{
	std::string ret_val;
	std::map<std::string, std::string>::const_iterator cd_iter = m_customdata.begin();
	while(cd_iter != m_customdata.end())
	{
		if(ret_val.length() > 0) { ret_val += "^"; }

		ret_val += cd_iter->first;
		ret_val += "^";
		ret_val += cd_iter->second;
		cd_iter++;

		if(ret_val.length() > 0) { ret_val += "^"; }
	}

	return ret_val;
}

std::string ItemInst::GetCustomData(std::string identifier)
{
	std::map<std::string, std::string>::const_iterator cd_iter = m_customdata.find(identifier);

	if(cd_iter != m_customdata.end()) { return cd_iter->second; }

	return "";
}

void ItemInst::SetCustomData(std::string identifier, std::string value)
{
	DeleteCustomData(identifier);
	m_customdata[identifier] = value;
}

void ItemInst::SetCustomData(std::string identifier, int value)
{
	DeleteCustomData(identifier);
	std::stringstream ss;
	ss << value;
	m_customdata[identifier] = ss.str();
}

void ItemInst::SetCustomData(std::string identifier, float value)
{
	DeleteCustomData(identifier);
	std::stringstream ss;
	ss << value;
	m_customdata[identifier] = ss.str();
}

void ItemInst::SetCustomData(std::string identifier, bool value)
{
	DeleteCustomData(identifier);
	std::stringstream ss;
	ss << value;
	m_customdata[identifier] = ss.str();
}

void ItemInst::DeleteCustomData(std::string identifier)
{
	std::map<std::string, std::string>::iterator cd_iter = m_customdata.find(identifier);

	if(cd_iter != m_customdata.end()) { m_customdata.erase(cd_iter); }
}

// Container (bag) methods
int16 ItemInst::FirstOpenSlot() const
{
	int16 sub_slot;
	for(sub_slot = SUBSLOT_START; sub_slot < m_item->BagSlots; sub_slot++)
	{
		if(!GetItem(sub_slot)) { break; }
	}

	return ((sub_slot < m_item->BagSlots) ? sub_slot : SUBSLOT_INVALID);
}

uint32 ItemInst::GetItemID(int16 sub_slot) const
{
	const ItemInst *item;
	uint32 item_id = 0;

	if((item = GetItem(sub_slot)) != nullptr) { item_id = item->GetItem()->ID; }

	return item_id;
}

ItemInst* ItemInst::GetItem(int16 sub_slot) const
{
	if(sub_slot & MCONTENTS_OOR) { return nullptr; }
	
	iter_contents it = m_contents.find((uint8)sub_slot);

	if(it != m_contents.end())
	{
		ItemInst* inst = it->second;
		return inst;
	}

	return nullptr;
}

void ItemInst::PutItem(int16 sub_slot, const ItemInst& inst)
{
	if(sub_slot & MCONTENTS_OOR) { return; }

	DeleteItem(sub_slot);

	_PutItem(sub_slot, inst.Clone());
}

void ItemInst::PutItem(SharedDatabase *db, int16 sub_slot, uint32 item_id)
{
	if(sub_slot & MCONTENTS_OOR) { return; }
	
	if(item_id != 0 && sub_slot < m_item->BagSlots)
	{
		const ItemInst* bag_item = db->CreateItem(item_id);

		if(bag_item)
		{
			PutAugment(sub_slot, *bag_item);
			safe_delete(bag_item);
		}
	}
}

ItemInst* ItemInst::PopItem(int16 sub_slot)
{
	if(sub_slot & MCONTENTS_OOR) { return nullptr; }
	
	iter_contents c_iter = m_contents.find((uint8)sub_slot);

	if(c_iter != m_contents.end())
	{
		ItemInst* inst = c_iter->second;
		m_contents.erase((uint8)sub_slot);
		return inst;
	}

	return nullptr;
}

void ItemInst::DeleteItem(int16 sub_slot)
{
	if(sub_slot & MCONTENTS_OOR) { return; }
	
	ItemInst* inst = PopItem((uint8)sub_slot);
	safe_delete(inst);
}

// Augment methods
int16 ItemInst::AvailableAugmentSlot(int32 aug_type) const
{
	if(!m_item || m_item->ItemClass != ItemClassCommon) { return AUGSLOT_INVALID; }

	int16 aug_slot;
	for(aug_slot = AUGSLOT_START; aug_slot < MAX_AUGMENTS; aug_slot++)
	{
		if(!GetItem(aug_slot))
		{
			if(aug_type == -1 ||
				(m_item->AugSlotType[(uint8)aug_slot] && ((1 << (m_item->AugSlotType[(uint8)aug_slot] - 1)) & aug_type)))
			{
				break;
			}
		}
	}

	return ((aug_slot < MAX_AUGMENTS) ? aug_slot : AUGSLOT_INVALID);
}

uint32 ItemInst::GetAugmentItemID(int16 aug_slot) const
{
	uint32 item_id = 0;
	
	if(aug_slot & MCONTENTS_OOR) { return item_id; }
	
	if(m_item && m_item->ItemClass == ItemClassCommon) { return GetItemID(aug_slot); }

	return item_id;
}

ItemInst* ItemInst::GetAugment(int16 aug_slot) const
{
	if(aug_slot & MCONTENTS_OOR) { return nullptr; }
	
	if(m_item && m_item->ItemClass == ItemClassCommon) { return GetItem(aug_slot); }

	return nullptr;
}

void ItemInst::PutAugment(int16 aug_slot, const ItemInst& augment)
{
	if(aug_slot & MCONTENTS_OOR) { return; }
	
	if(m_item && m_item->ItemClass == ItemClassCommon) { PutItem(aug_slot, augment); }
}

void ItemInst::PutAugment(SharedDatabase *db, int16 aug_slot, uint32 item_id)
{
	if(aug_slot & MCONTENTS_OOR) { return; }
	
	if(item_id != 0)
	{
		const ItemInst* aug = db->CreateItem(item_id);

		if(aug)
		{
			PutAugment(aug_slot, *aug);
			safe_delete(aug);
		}
	}
}

ItemInst* ItemInst::RemoveAugment(int16 aug_slot)
{
	if(aug_slot & MCONTENTS_OOR) { return nullptr; }
	
	if(m_item && m_item->ItemClass == ItemClassCommon) { return PopItem(aug_slot); }

	return nullptr;
}

void ItemInst::DeleteAugment(int16 aug_slot)
{
	if(aug_slot & MCONTENTS_OOR) { return; }

	if(m_item && m_item->ItemClass == ItemClassCommon) { DeleteItem(aug_slot); }
}

// Internal container methods
void ItemInst::_PutItem(int16 contents_index, ItemInst* inst)
 {
	if(contents_index & MCONTENTS_OOR) { return; }
	
	m_contents[(uint8)contents_index] = inst;
 }
// ######################################################################################


/*
 Class: EvoItemInst ######################################################################
	<description>
 #########################################################################################
*/
// Methods for EvoItemInst, the extended ItemInst for evolving/scaling items
// Copy constructors
EvoItemInst::EvoItemInst(const EvoItemInst &copy)
{
	m_use_type		= copy.m_use_type;
	m_item			= copy.m_item;
	m_charges		= copy.m_charges;
	m_price			= copy.m_price;
	m_color			= copy.m_color;
	m_merchantslot	= copy.m_merchantslot;
	m_currentslot	= copy.m_currentslot;
	m_instnodrop	= copy.m_instnodrop;
	m_merchantcount	= copy.m_merchantcount;

	// Copy container contents
	iter_contents it;
	for(it = copy.m_contents.begin(); it != copy.m_contents.end(); it++)
	{
		ItemInst* inst_old = it->second;
		ItemInst* inst_new = nullptr;

		if(inst_old) { inst_new = inst_old->Clone(); }

		if(inst_new != nullptr) { m_contents[it->first] = inst_new; }
	}

	std::map<std::string, std::string>::const_iterator iter;
	for(iter = copy.m_customdata.begin(); iter != copy.m_customdata.end(); iter++)
	{
		m_customdata[iter->first] = iter->second;
	}

	m_serialnumber	= copy.m_serialnumber;
	m_exp			= copy.m_exp;
	m_evolveLvl		= copy.m_evolveLvl;
	m_activated		= copy.m_activated;
	m_evolveInfo	= nullptr;

	if(copy.m_scaledItem) { m_scaledItem = new Item_Struct(*copy.m_scaledItem); }
	else { m_scaledItem = nullptr; }
}

EvoItemInst::EvoItemInst(const ItemInst &basecopy)
{
	EvoItemInst* copy = (EvoItemInst*)&basecopy;

	m_use_type		= copy->m_use_type;
	m_item			= copy->m_item;
	m_charges		= copy->m_charges;
	m_price			= copy->m_price;
	m_color			= copy->m_color;
	m_merchantslot	= copy->m_merchantslot;
	m_currentslot	= copy->m_currentslot;
	m_instnodrop	= copy->m_instnodrop;
	m_merchantcount	= copy->m_merchantcount;

	// Copy container contents
	iter_contents it;
	for(it = copy->m_contents.begin(); it != copy->m_contents.end(); it++)
	{
		ItemInst* inst_old = it->second;
		ItemInst* inst_new = nullptr;

		if(inst_old) { inst_new = inst_old->Clone(); }

		if(inst_new != nullptr) { m_contents[it->first] = inst_new; }
	}

	std::map<std::string, std::string>::const_iterator iter;
	for(iter = copy->m_customdata.begin(); iter != copy->m_customdata.end(); iter++)
	{
		m_customdata[iter->first] = iter->second;
	}

	m_serialnumber	= copy->m_serialnumber;
	m_exp			= 0;
	m_evolveLvl		= 0;
	m_activated		= false;
	m_evolveInfo	= nullptr;
	m_scaledItem	= nullptr;
}

EvoItemInst::EvoItemInst(const Item_Struct* item, int16 charges)
{
	m_use_type		= ItemUseNormal;
	m_item			= item;
	m_charges		= charges;
	m_price			= 0;
	m_instnodrop	= false;
	m_merchantslot	= 0;

	if(m_item && m_item->ItemClass == ItemClassCommon) { m_color = m_item->Color; }
	else { m_color = 0; }

	m_merchantcount	= 1;
	m_serialnumber	= GetNextItemInstSerialNumber();
	m_exp			= 0;
	m_evolveLvl		= 0;
	m_activated		= false;
	m_evolveInfo	= nullptr;
	m_scaledItem	= nullptr;
}

EvoItemInst::~EvoItemInst()
{
	safe_delete(m_scaledItem);
}

EvoItemInst* EvoItemInst::Clone() const
{
	return new EvoItemInst(*this);
}

const Item_Struct* EvoItemInst::GetItem() const
{
	if(!m_scaledItem) { return m_item; }
	else { return m_scaledItem; }
}

const Item_Struct* EvoItemInst::GetUnscaledItem() const
{
	return m_item;
}

void EvoItemInst::Initialize(SharedDatabase *db)
{
	// if there's no actual item, don't do anything
	if(!m_item) { return; }

	// initialize scaling items
	if(m_item->CharmFileID != 0)
	{
		m_evolveLvl = -1;
		this->ScaleItem();
	}
	// initialize evolving items
	else if((db) && m_item->LoreGroup >= 1000 && m_item->LoreGroup != -1)
	{
		// not complete yet
	}
}

void EvoItemInst::ScaleItem()
{
	// free memory from any previously scaled item data
	safe_delete(m_scaledItem);

	m_scaledItem		= new Item_Struct(*m_item);
	float Mult			= (float)(GetExp()) / 10000;	// scaling is determined by exp, with 10,000 being full stats

	m_scaledItem->AStr	= (int8)((float)m_item->AStr * Mult);
	m_scaledItem->ASta	= (int8)((float)m_item->ASta * Mult);
	m_scaledItem->AAgi	= (int8)((float)m_item->AAgi * Mult);
	m_scaledItem->ADex	= (int8)((float)m_item->ADex * Mult);
	m_scaledItem->AInt	= (int8)((float)m_item->AInt * Mult);
	m_scaledItem->AWis	= (int8)((float)m_item->AWis * Mult);
	m_scaledItem->ACha	= (int8)((float)m_item->ACha * Mult);

	m_scaledItem->MR	= (int8)((float)m_item->MR * Mult);
	m_scaledItem->PR	= (int8)((float)m_item->PR * Mult);
	m_scaledItem->DR	= (int8)((float)m_item->DR * Mult);
	m_scaledItem->CR	= (int8)((float)m_item->CR * Mult);
	m_scaledItem->FR	= (int8)((float)m_item->FR * Mult);

	m_scaledItem->HP	= (int32)((float)m_item->HP * Mult);
	m_scaledItem->Mana	= (int32)((float)m_item->Mana * Mult);
	m_scaledItem->AC	= (int32)((float)m_item->AC * Mult);

	m_scaledItem->SkillModValue	= (int32)((float)m_item->SkillModValue * Mult);
	m_scaledItem->BaneDmgAmt	= (int8)((float)m_item->BaneDmgAmt * Mult);
	m_scaledItem->BardValue		= (int32)((float)m_item->BardValue * Mult);
	m_scaledItem->ElemDmgAmt	= (uint8)((float)m_item->ElemDmgAmt * Mult);
	m_scaledItem->Damage		= (uint32)((float)m_item->Damage * Mult);

	m_scaledItem->CombatEffects	= (int8)((float)m_item->CombatEffects * Mult);
	m_scaledItem->Shielding		= (int8)((float)m_item->Shielding * Mult);
	m_scaledItem->StunResist	= (int8)((float)m_item->StunResist * Mult);
	m_scaledItem->StrikeThrough	= (int8)((float)m_item->StrikeThrough * Mult);
	m_scaledItem->ExtraDmgAmt	= (uint32)((float)m_item->ExtraDmgAmt * Mult);
	m_scaledItem->SpellShield	= (int8)((float)m_item->SpellShield * Mult);
	m_scaledItem->Avoidance		= (int8)((float)m_item->Avoidance * Mult);
	m_scaledItem->Accuracy		= (int8)((float)m_item->Accuracy * Mult);

	m_scaledItem->FactionAmt1	= (int32)((float)m_item->FactionAmt1 * Mult);
	m_scaledItem->FactionAmt2	= (int32)((float)m_item->FactionAmt2 * Mult);
	m_scaledItem->FactionAmt3	= (int32)((float)m_item->FactionAmt3 * Mult);
	m_scaledItem->FactionAmt4	= (int32)((float)m_item->FactionAmt4 * Mult);

	m_scaledItem->Endur				= (uint32)((float)m_item->Endur * Mult);
	m_scaledItem->DotShielding		= (uint32)((float)m_item->DotShielding * Mult);
	m_scaledItem->Attack			= (uint32)((float)m_item->Attack * Mult);
	m_scaledItem->Regen				= (uint32)((float)m_item->Regen * Mult);
	m_scaledItem->ManaRegen			= (uint32)((float)m_item->ManaRegen * Mult);
	m_scaledItem->EnduranceRegen	= (uint32)((float)m_item->EnduranceRegen * Mult);
	m_scaledItem->Haste				= (uint32)((float)m_item->Haste * Mult);
	m_scaledItem->DamageShield		= (uint32)((float)m_item->DamageShield * Mult);

	m_scaledItem->CharmFileID = 0;	// this stops the client from trying to scale the item itself.
}

bool EvoItemInst::EvolveOnAllKills() const
{
	return (m_evolveInfo && m_evolveInfo->AllKills);
}

int8 EvoItemInst::GetMaxEvolveLvl() const
{
	if(m_evolveInfo) { return m_evolveInfo->MaxLvl; }
	else { return 0; }
}

uint32 EvoItemInst::GetKillsNeeded(uint8 currentlevel)
{
	uint32 kills = -1;	// default to -1 (max uint32 value) because this value is usually divided by, so we don't want to ever return zero.
	
	if(m_evolveInfo)
	{
		if(currentlevel != m_evolveInfo->MaxLvl) { kills = m_evolveInfo->LvlKills[currentlevel - 1]; }
	}

	if(kills == 0) { kills = -1; }

	return kills;
}

EvolveInfo::EvolveInfo()
{
	// nothing here yet
}

EvolveInfo::EvolveInfo(uint32 first, uint8 max, bool allkills, uint32 L2, uint32 L3, uint32 L4, uint32 L5, uint32 L6, uint32 L7, uint32 L8, uint32 L9, uint32 L10)
{
	FirstItem	= first;
	MaxLvl		= max;
	AllKills	= allkills;
	LvlKills[0]	= L2;
	LvlKills[1]	= L3;
	LvlKills[2]	= L4;
	LvlKills[3]	= L5;
	LvlKills[4]	= L6;
	LvlKills[5]	= L7;
	LvlKills[6]	= L8;
	LvlKills[7]	= L9;
	LvlKills[8]	= L10;
}
// ######################################################################################


/*
 Structure: Item_Struct ######################################################################
	<description>
 #############################################################################################
*/
bool Item_Struct::IsEquipable(uint16 race_id, uint16 class_id) const
{
	bool IsRace = false;
	bool IsClass = false;

	uint32 Classes_ = Classes;
	uint32 Races_ = Races;

	uint32 Race_ = GetArrayRace(race_id);

	for(int CurrentClass = 1; CurrentClass <= PLAYER_CLASS_COUNT; ++CurrentClass)
	{
		if(Classes_ % 2 == 1)
		{
			if(CurrentClass == class_id)
			{
					IsClass = true;
					break;
			}
		}

		Classes_ >>= 1;
	}

	Race_ = (Race_ == 18 ? 16 : Race_);

	for(unsigned int CurrentRace = 1; CurrentRace <= PLAYER_RACE_COUNT; ++CurrentRace)
	{
		if(Races_ % 2 == 1)
		{
			if(CurrentRace == Race_)
			{
				IsRace = true;
				break;
			}
		}

		Races_ >>= 1;
	}

	return (IsRace && IsClass);
}
// #############################################################################################
