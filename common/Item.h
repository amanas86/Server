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

// @merth notes:
// These classes could be optimized with database reads/writes by storing
// a status flag indicating how object needs to interact with database

#ifndef __ITEM_H
#define __ITEM_H

class ItemInst;				// Item belonging to a client (contains info on item, dye, augments, charges, etc)
class ManagedCursor;		// Cursor management class - replaces ItemInstQueue
class InventoryLimits;		// Client-based limits class
class ItemInstQueue;		// Queue of ItemInst objects (i.e., cursor)
class Inventory;			// Character inventory
class ItemParse;			// Parses item packets
class EvoItemInst;			// Extended item inst, for use with scaling/evolving items
class EvolveInfo;			// Stores information about an evolving item family

#include <string>
#include <vector>
#include <map>
#include <list>
#include "../common/eq_packet_structs.h"
#include "../common/eq_constants.h"
#include "../common/item_struct.h"
#include "../common/clientversions.h"

// Helper typedefs
typedef std::list<ItemInst*>::const_iterator					iter_queue;
typedef std::map<int16, ItemInst*>::const_iterator			iter_inst;
typedef std::map<uint8, ItemInst*>::const_iterator			iter_contents;

namespace ItemField {
	enum {
		source=0,
#define F(x) x,
#include "item_fieldlist.h"
#undef F
		updated
	};
};

// Depricated #defines -U
// Indexing positions to the beginning slot_id's for a bucket of slots
#define IDX_EQUIP		0
#define IDX_CURSOR_BAG	331
#define IDX_INV			22
#define IDX_INV_BAG		251
#define IDX_TRIBUTE		400
#define IDX_BANK		2000
#define IDX_BANK_BAG	2031
#define IDX_SHBANK		2500
#define IDX_SHBANK_BAG	2531
#define IDX_TRADE		3000
#define IDX_TRADE_BAG	3031
#define IDX_TRADESKILL	4000
#define MAX_ITEMS_PER_BAG 10

// Specifies usage type for item inside ItemInst
enum ItemUseType
{
	ItemUseNormal,
	ItemUseWorldContainer
};

typedef enum {
	byFlagIgnore,	//do not consider this flag
	byFlagSet,		//apply action if the flag is set
	byFlagNotSet	//apply action if the flag is NOT set
} byFlagSetting;

// Left in situ until deemed no longer needed (currently used for legacy scripting) -U
//FatherNitwit: location bits for searching specific
//places with HasItem() and HasItemByUse()
enum {
	invWhereWorn		= 0x01,
	invWherePersonal	= 0x02,	//in the character's inventory
	invWhereBank		= 0x04,
	invWhereSharedBank	= 0x08,
	invWhereTrading		= 0x10,
	invWhereCursor		= 0x20
};

enum InvWhereLocation
{
	InvWhereNull			= 0x00000000,
	InvWhereEquipment		= 0x00000001, // worn equipment
	InvWherePersonal		= 0x00000002, // carried equipment
	InvWhereCursor			= 0x00000004, // visible cursor only
	InvWherePossessions		= 0x00000007,
	InvWhereBank			= 0x00000008,
	InvWhereSharedBank		= 0x00000010,
	InvWhereTrade			= 0x00000020,
	InvWhereWorld			= 0x00000040,
	InvWhereLimbo			= 0x00000080,
	InvWhereTribute			= 0x00000100,
	InvWhereTrophyTribute	= 0x00000200,
	InvWhereDeleted			= 0x00000400, // delete buyback?
	InvWhereRealEstate		= 0x00000800,
	InvWhereAltStorage		= 0x00001000, // tokenized bag storage?
	InvWhereArchived		= 0x00002000, // tokenized bag storage?
	InvWhereMail			= 0x00004000,
	InvWhereKrono			= 0x00008000,
	InvWhereOther			= 0x00010000,
	InvWhereAll				= 0xFFFFFFFF
};

class ManagedCursor
{
public:
	~ManagedCursor();

private:

};
 
class InventoryLimits
{
public:
	~InventoryLimits();

	static bool		SetServerInventoryLimits(InventoryLimits &limits);
	static bool		SetMobInventoryLimits(InventoryLimits &limits);
	static bool		SetClientInventoryLimits(InventoryLimits &limits, EQClientVersion client_version = EQClientUnknown);
	
	void			ResetInventoryLimits();
	inline bool		IsLimitsSet() const { return m_limits_set; }

	inline int16	GetSlotTypeSize(int16 slot_type)	const { return (slot_type >= SLOTTYPE_START && slot_type < SlotType_Count) ? m_slottypesize[slot_type] : 0; }
	inline const int16	operator[](int16 slot_type)		const { return GetSlotTypeSize(slot_type); }

	inline int16	GetEquipmentStart()					const { return m_equipmentstart; }
	inline int16	GetEquipmentEnd()					const { return m_equipmentend; }
	inline uint32	GetEquipmentBitMask()				const { return m_equipmentbitmask; }
	inline int16	GetPersonalStart()					const { return m_personalstart; }
	inline int16	GetPersonalEnd()					const { return m_personalend; }
	inline uint32	GetPersonalBitMask()				const { return m_personalbitmask; }

	inline uint8	GetBandolierSlotsMax()				const { return m_bandolierslotsmax; }
	inline uint8	GetPotionBeltSlotsMax()				const { return m_potionbeltslotsmax; }
	inline uint8	GetBagSlotsMax()					const { return m_bagslotsmax; }
	inline uint8	GetAugmentsMax()					const { return m_augmentsmax; }

private:
	int16			m_slottypesize[SlotType_Count];

	int16			m_equipmentstart;
	int16			m_equipmentend;
	uint32			m_equipmentbitmask;
	int16			m_personalstart;
	int16			m_personalend;
	uint32			m_personalbitmask;

	uint8			m_bandolierslotsmax;
	uint8			m_potionbeltslotsmax;
	uint8			m_bagslotsmax;
	uint8			m_augmentsmax;

	bool			m_limits_set;
};

// Depricated class -U
// ########################################
// Class: Queue
//	Queue that allows a read-only iterator
class ItemInstQueue
{
public:
	~ItemInstQueue();
	/////////////////////////
	// Public Methods
	/////////////////////////

	inline iter_queue begin()	{ return m_list.begin(); }
	inline iter_queue end()		{ return m_list.end(); }

	void push(ItemInst* inst);
	void push_front(ItemInst* inst);
	ItemInst* pop();
	ItemInst* peek_front() const;
	inline int size()		{ return static_cast<int>(m_list.size()); }

protected:
	/////////////////////////
	// Protected Members
	/////////////////////////

	std::list<ItemInst*> m_list;

};

// ########################################
// Class: Inventory
//	Character inventory
class Inventory
{
	friend class ItemInst;
public:
	///////////////////////////////
	// Public Methods
	///////////////////////////////

	virtual ~Inventory();

	// Retrieve a writeable item at specified slot
	ItemInst* GetItem(int16 slot_id) const;
	ItemInst* GetItem(int16 slot_id, uint8 bagidx) const;

	inline iter_queue cursor_begin()	{ return m_cursor.begin(); }
	inline iter_queue cursor_end()		{ return m_cursor.end(); }
	inline bool CursorEmpty()		{ return (m_cursor.size() == 0); }

	// Retrieve a read-only item from inventory
	inline const ItemInst* operator[](int16 slot_id) const { return GetItem(slot_id); }

	// Add item to inventory
	int16 PutItem(int16 slot_id, const ItemInst& inst);

	// Add item to cursor queue
	int16 PushCursor(const ItemInst& inst);

	// Swap items in inventory
	bool SwapItem(int16 slot_a, int16 slot_b);

	// Remove item from inventory
	bool DeleteItem(int16 slot_id, uint8 quantity=0);

	// Checks All items in a bag for No Drop
	bool CheckNoDrop(int16 slot_id);

	// Remove item from inventory (and take control of memory)
	ItemInst* PopItem(int16 slot_id);

	// Check whether item exists in inventory
	// where argument specifies OR'd list of invWhere constants to look
	int16 HasItem(uint32 item_id, uint8 quantity=0, uint8 where=0xFF);

	// Check whether there is space for the specified number of the specified item.
	bool HasSpaceForItem(const Item_Struct *ItemToTry, int16 Quantity);

	// Check whether item exists in inventory
	// where argument specifies OR'd list of invWhere constants to look
	int16 HasItemByUse(uint8 use, uint8 quantity=0, uint8 where=0xFF);

	// Check whether item exists in inventory
	// where argument specifies OR'd list of invWhere constants to look
	int16 HasItemByLoreGroup(uint32 loregroup, uint8 where=0xFF);

	// Locate an available inventory slot
	int16 FindFreeSlot(bool for_bag, bool try_cursor, uint8 min_size = 0, bool is_arrow = false);

	// Calculate slot_id for an item within a bag
	static int16 CalcSlotId(int16 slot_id); // Calc parent bag's slot_id
	static int16 CalcSlotId(int16 bagslot_id, uint8 bagidx); // Calc slot_id for item inside bag
	static uint8 CalcBagIdx(int16 slot_id); // Calc bagidx for slot_id
	static int16 CalcSlotFromMaterial(uint8 material);
	static uint8 CalcMaterialFromSlot(int16 equipslot);

	static bool CanItemFitInContainer(const Item_Struct *ItemToTry, const Item_Struct *Container);

	// Test whether a given slot can support a container item
	static bool SupportsContainers(int16 slot_id);

	void dumpEntireInventory();
	void dumpWornItems();
	void dumpInventory();
	void dumpBankItems();
	void dumpSharedBankItems();

	void SetCustomItemData(uint32 character_id, int16 slot_id, std::string identifier, std::string value);
	void SetCustomItemData(uint32 character_id, int16 slot_id, std::string identifier, int value);
	void SetCustomItemData(uint32 character_id, int16 slot_id, std::string identifier, float value);
	void SetCustomItemData(uint32 character_id, int16 slot_id, std::string identifier, bool value);
	std::string GetCustomItemData(int16 slot_id, std::string identifier);

	// Inventory helpers
	static void InvalidateSlotStruct(InventorySlot_Struct &is_struct);
	static bool IsValidServerSlotStruct(InventorySlot_Struct &is_struct);
	static bool IsValidMobSlotStruct(InventorySlot_Struct &is_struct);
	static bool IsValidClientSlotStruct(InventorySlot_Struct &is_struct);
	static bool IsDeleteRequest(InventorySlot_Struct &is_struct);

	// InventoryLimits Accessors
	inline InventoryLimits& GetLimits()				{ return m_limits; }
	inline const InventoryLimits& GetLimits() const	{ return m_limits; }

	inline bool IsLimitsSet()						{ return m_limits.IsLimitsSet(); }

protected:
	///////////////////////////////
	// Protected Methods
	///////////////////////////////

	void dumpItemCollection(const std::map<int16, ItemInst*> &collection);
	void dumpBagContents(ItemInst *inst, iter_inst *it);

	// Retrieves item within an inventory bucket
	ItemInst* _GetItem(const std::map<int16, ItemInst*>& bucket, int16 slot_id) const;

	// Private "put" item into bucket, without regard for what is currently in bucket
	int16 _PutItem(int16 slot_id, ItemInst* inst);

	// Checks an inventory bucket for a particular item
	int16 _HasItem(std::map<int16, ItemInst*>& bucket, uint32 item_id, uint8 quantity);
	int16 _HasItem(ItemInstQueue& iqueue, uint32 item_id, uint8 quantity);
	int16 _HasItemByUse(std::map<int16, ItemInst*>& bucket, uint8 use, uint8 quantity);
	int16 _HasItemByUse(ItemInstQueue& iqueue, uint8 use, uint8 quantity);
	int16 _HasItemByLoreGroup(std::map<int16, ItemInst*>& bucket, uint32 loregroup);
	int16 _HasItemByLoreGroup(ItemInstQueue& iqueue, uint32 loregroup);


	// Player inventory
	std::map<int16, ItemInst*>	m_worn;		// Items worn by character
	std::map<int16, ItemInst*>	m_inv;		// Items in character personal inventory
	//std::map<int16, ItemInst*>	m_bank;		// Items in character bank
	std::map<int16, ItemInst*>	m_shbank;	// Items in character shared bank
	//std::map<int16, ItemInst*>	m_trade;	// Items in a trade session
	ItemInstQueue			m_cursor;	// Items on cursor: FIFO

	std::map<int16, ItemInst*>	m_possessions;			// type 0 - combined equipment, personal and cursor
	std::map<int16, ItemInst*>	m_bank;					// type 1
	std::map<int16, ItemInst*>	m_sharedbank;			// type 2
	std::map<int16, ItemInst*>	m_trade;				// type 3
	std::map<int16, ItemInst*>	m_world;				// type 4
	std::map<int16, ItemInst*>	m_limbo;				// type 5 - aka cursor buffer
	std::map<int16, ItemInst*>	m_tribute;				// type 6
	std::map<int16, ItemInst*>	m_trophytribute;		// type 7 - not yet implemented
	std::map<int16, ItemInst*>	m_guildtribute;			// type 8 - not yet implemented, generated from guild entity?
	std::map<int16, ItemInst*>	m_merchant;				// type 9 - generated from target?
	std::map<int16, ItemInst*>	m_deleted;				// type 10 - unknown, possibly deleted item storage?
	std::map<int16, ItemInst*>	m_corpse;				// type 11 - generated from target?
	std::map<int16, ItemInst*>	m_bazaar;				// type 12 - generated from target?
	std::map<int16, ItemInst*>	m_inspect;				// type 13 - generated from target?
	std::map<int16, ItemInst*>	m_realestate;			// type 14 - not yet implemented
	std::map<int16, ItemInst*>	m_viewmodpc;			// type 15 - unknown, possibly gm-related?
	std::map<int16, ItemInst*>	m_viewmodbank;			// type 16 - unknown, possibly gm-related?
	std::map<int16, ItemInst*>	m_viewmodsharedbank;	// type 17 - unknown, possibly gm-related?
	std::map<int16, ItemInst*>	m_viewmodlimbo;			// type 18 - unknown, possibly gm-related?
	std::map<int16, ItemInst*>	m_altstorage;			// type 19 - unknown
	std::map<int16, ItemInst*>	m_archived;				// type 20 - unknown, possibly deleted item or tokenized bag storage?
	std::map<int16, ItemInst*>	m_mail;					// type 21 - not yet implemented
	std::map<int16, ItemInst*>	m_guildtrophytribute;	// type 22 - not yet implemented, generated from guild entity?
	std::map<int16, ItemInst*>	m_krono;				// type 23
	std::map<int16, ItemInst*>	m_other;				// type 24 - unknown

	InventoryLimits			m_limits;				// client-based inventory limits
};

class SharedDatabase;

/*
 Class: ItemInst ######################################################################
	Base class for an instance of an item. An item instance encapsulates item data
	and data specific to an item instance (includes dye, augments, charges, etc)
 ######################################################################################
*/
class ItemInst
{
public:
	// Class constructors
	ItemInst(const Item_Struct* item = nullptr, int16 charges = 0);
	ItemInst(SharedDatabase *db, uint32 item_id, int16 charges = 0);
	ItemInst(ItemUseType use_type)
	{
		m_use_type		= use_type;
		m_item			= nullptr;
		m_charges		= 0;
		m_price			= 0;
		m_instnodrop	= false;
		m_merchantslot	= 0;
		m_color			= 0;
	}
	ItemInst(const ItemInst& copy);


	// Class deconstructors
	virtual ~ItemInst();


	// ItemInst property accessors
	virtual const Item_Struct* GetItem() const { return m_item; }
	void SetItem(const Item_Struct* item) { m_item = item; }

	int16 GetCharges() const { return m_charges; }
	void SetCharges(int16 charges) { m_charges = charges; }

	uint32 GetColor() const { return m_color; }
	void SetColor(uint32 color) { m_color = color; }

	bool IsInstNoDrop() const { return m_instnodrop; }
	void SetInstNoDrop(bool instnodrop_flag) { m_instnodrop = instnodrop_flag; }

	uint32 GetMerchantSlot() const { return m_merchantslot; }
	void SetMerchantSlot(uint32 merchant_slot) { m_merchantslot = merchant_slot; }

	int32 GetMerchantCount() const { return m_merchantcount; }
	void SetMerchantCount(int32 merchant_count) { m_merchantcount = merchant_count; }

	uint32 GetPrice() const { return m_price; }
	void SetPrice(uint32 price) { m_price = price; }

	int16 GetCurrentSlot() const { return m_currentslot; }
	void SetCurrentSlot(int16 current_slot) { m_currentslot = current_slot; }

	inline int32 GetSerialNumber() const { return m_serialnumber; }
	inline void SetSerialNumber(int32 serial_number) { m_serialnumber = serial_number; }


	// Item_Struct property accessors
	const uint32 GetID() const { return (m_item ? m_item->ID : 0); }
	const uint32 GetItemScriptID() const { return (m_item ? m_item->ScriptFileID : 0); }
	inline int32 GetAugmentType() const { return (m_item ? m_item->AugType : 0); }


	// ItemInst methods
	virtual ItemInst* Clone() const;

	virtual bool IsType(ItemClass item_class) const;
	virtual bool IsStackable() const;
	virtual bool IsEquipable(uint16 race_id, uint16 class_id) const;
	virtual bool IsEquipable(InventorySlot_Struct is_struct) const;
	virtual bool IsWeapon() const;
	virtual bool IsAmmo() const;

	bool IsAugmentable() const;

	inline bool IsExpendable() const
	{
		return (m_item ? ((m_item->Click.Type == ET_Expendable ) || (m_item->ItemType == ItemTypePotion)) : false);
	}

	virtual bool IsEvolving() const { return false; }
	virtual bool IsScaling() const { return false; }

	bool IsSlotAllowed(InventorySlot_Struct is_struct) const;
	bool IsNoneEmptyContainer();
	
	bool AvailableWearSlot(uint32 aug_wear_slots) const;
	bool IsAugmented();

	uint8 GetTotalItemCount() const;

	void Clear();
	void ClearByFlags(byFlagSetting is_nodrop, byFlagSetting is_norent);

	std::map<uint8, ItemInst*>* GetContents() { return &m_contents; }

	std::string GetCustomDataString() const;
	std::string GetCustomData(std::string identifier);
	void SetCustomData(std::string identifier, std::string value);
	void SetCustomData(std::string identifier, int value);
	void SetCustomData(std::string identifier, float value);
	void SetCustomData(std::string identifier, bool value);
	void DeleteCustomData(std::string identifier);

	std::string Serialize(InventorySlot_Struct is_struct) const
	{
		InternalSerializedItem_Struct isi_struct;
		isi_struct.slot_id = 0 /* placeholder for 'is_struct' */;
		isi_struct.inst = (const void *)this;
		std::string ser;
		ser.assign((char *)&isi_struct, sizeof(InternalSerializedItem_Struct));
		return ser;
	}


	// Container (bag) methods
	int16 FirstOpenSlot() const;

	virtual uint32 GetItemID(int16 sub_slot) const;

	ItemInst* GetItem(int16 sub_slot) const;
	void PutItem(int16 sub_slot, const ItemInst& inst);
	void PutItem(SharedDatabase *db, int16 sub_slot, uint32 item_id);

	ItemInst* PopItem(int16 sub_slot);
	void DeleteItem(int16 sub_slot);
	

	// Augment methods
	int16 AvailableAugmentSlot(int32 aug_type) const;	

	virtual uint32 GetAugmentItemID(int16 aug_slot) const;

	ItemInst* GetAugment(int16 aug_slot) const;
	void PutAugment(int16 aug_slot, const ItemInst& inst);
	void PutAugment(SharedDatabase *db, int16 aug_slot, uint32 item_id);

	ItemInst* RemoveAugment(int16 aug_slot);
	void DeleteAugment(int16 aug_slot);


	// ItemInst operators
	operator bool() const { return (m_item != nullptr); }
	bool operator==(const ItemInst& right) const { return (m_item ? (this->m_item == right.m_item) : false); }
	bool operator!=(const ItemInst& right) const { return (m_item ? (this->m_item != right.m_item) : false); }

	inline const ItemInst* operator[](int16 sub_slot) const { return GetItem(sub_slot); } // Container op


protected:
	iter_contents _begin() { return m_contents.begin(); }
	iter_contents _end() { return m_contents.end(); }

	friend class Inventory;

	void _PutItem(int16 contents_index, ItemInst* inst);

	const Item_Struct* m_item;
	ItemUseType m_use_type;

	int16 m_charges;
	uint32 m_color;
	bool m_instnodrop;

	uint32 m_merchantslot;
	int32 m_merchantcount;
	uint32 m_price;
	int16 m_currentslot;
	int32 m_serialnumber;

	std::map<std::string, std::string> m_customdata;
	std::map<uint8, ItemInst*> m_contents;
};


/*
 Class: EvoItemInst ######################################################################
	<description>
 #########################################################################################
*/
class EvoItemInst: public ItemInst {
public:
	// constructor and destructor
	EvoItemInst(const EvoItemInst& copy);
	EvoItemInst(const ItemInst& copy);
	EvoItemInst(const Item_Struct* item = nullptr, int16 charges = 0);
	~EvoItemInst();

	// accessors... a lot of these are for evolving items (not complete yet)
	bool IsScaling() const				{ return (m_evolveLvl == -1); }
	bool IsEvolving() const				{ return (m_evolveLvl >= 1); }
	uint32 GetExp() const				{ return m_exp; }
	void SetExp(uint32 exp)				{ m_exp = exp; }
	void AddExp(uint32 exp)				{ m_exp += exp; }
	bool IsActivated()					{ return m_activated; }
	void SetActivated(bool activated)	{ m_activated = activated; }
	int8 GetEvolveLvl() const			{ return m_evolveLvl; }

	EvoItemInst* Clone() const;
	const Item_Struct* GetItem() const;
	const Item_Struct* GetUnscaledItem() const;
	void Initialize(SharedDatabase *db = nullptr);
	void ScaleItem();
	bool EvolveOnAllKills() const;
	int8 GetMaxEvolveLvl() const;
	uint32 GetKillsNeeded(uint8 currentlevel);


private:
	uint32				m_exp;
	int8				m_evolveLvl;
	bool				m_activated;
	Item_Struct*		m_scaledItem;
	const EvolveInfo*	m_evolveInfo;
};

class EvolveInfo {
public:
	friend class EvoItemInst;
	//temporary
	uint16				LvlKills[9];
	uint32				FirstItem;
	uint8				MaxLvl;
	bool				AllKills;

	EvolveInfo();
	EvolveInfo(uint32 first, uint8 max, bool allkills, uint32 L2, uint32 L3, uint32 L4, uint32 L5, uint32 L6, uint32 L7, uint32 L8, uint32 L9, uint32 L10);
	~EvolveInfo();
};

#endif // #define __ITEM_H
