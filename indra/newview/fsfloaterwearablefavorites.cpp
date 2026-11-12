/**
 * @file fsfloaterwearablefavorites.cpp
 * @brief Class for the favorite wearables floater
 *
 * $LicenseInfo:firstyear=2018&license=viewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 * Copyright (c) 2018 Ansariel Hiller @ Second Life
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The Phoenix Firestorm Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
 * http://www.firestormviewer.org
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "fsfloaterwearablefavorites.h"
#include "fscommon.h"
#include "llappearancemgr.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llfiltereditor.h"
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "llmenugl.h"
#include "llmenubutton.h"
#include "llnotificationsutil.h"
#include "lltoggleablemenu.h"
#include "llviewercontrol.h"        // for gSavedSettings
#include "llviewerinventory.h"
#include "llviewermenu.h"           // for gMenuHolder
#include "rlvactions.h"
#include "rlvlocks.h"
#include <memory>
#include <set>
#include <utility>

// Hello Kokua! Have fun yoinking! ;)

#define FS_WEARABLE_FAVORITES_FOLDER "#Wearable Favorites"

static LLDefaultChildRegistry::Register<FSWearableFavoritesItemsList> r("fs_wearable_favorites_items_list");

FSWearableFavoritesItemsList::FSWearableFavoritesItemsList(const Params& p)
:   LLWearableItemsList(p)
{
}

bool FSWearableFavoritesItemsList::handleDragAndDrop(S32 x, S32 y, MASK mask,
                                                  bool drop,
                                                  EDragAndDropType cargo_type,
                                                  void* cargo_data,
                                                  EAcceptance* accept,
                                                  std::string& tooltip_msg)
{
    // Scroll folder view if needed.  Never accepts a drag or drop.
    *accept = ACCEPT_NO;
    autoScroll(x, y);

    if (cargo_type == DAD_BODYPART || cargo_type == DAD_CLOTHING || cargo_type == DAD_OBJECT)
    {
        if (drop)
        {
            LLInventoryItem* item = (LLInventoryItem*)cargo_data;
            if (!mDADSignal.empty())
            {
                mDADSignal(item->getUUID());
            }
        }
        else
        {
            *accept = ACCEPT_YES_SINGLE;
        }
    }

    return true;
}

LLUUID FSFloaterWearableFavorites::sFolderID = LLUUID();

FSFloaterWearableFavorites::FSFloaterWearableFavorites(const LLSD& key)
    : LLFloater(key)
    , mCategoriesObserver(new LLInventoryCategoriesObserver())
{
}

FSFloaterWearableFavorites::~FSFloaterWearableFavorites()
{
    if (gInventory.containsObserver(mCategoriesObserver))
    {
        gInventory.removeObserver(mCategoriesObserver);
    }
    delete mCategoriesObserver;

    if (mDADCallbackConnection.connected())
    {
        mDADCallbackConnection.disconnect();
    }

    if (mOptionsMenuHandle.get())
    {
        mOptionsMenuHandle.get()->die();
    }
}

//virtual
bool FSFloaterWearableFavorites::postBuild()
{
    mItemsList = getChild<FSWearableFavoritesItemsList>("favorites_list");
    mItemsList->setNoFilteredItemsMsg(getString("search_no_items"));
    mItemsList->setDoubleClickCallback(boost::bind(&FSFloaterWearableFavorites::onDoubleClick, this));

    mRemoveItemBtn = getChild<LLButton>("remove_btn");
    mRemoveItemBtn->setCommitCallback(boost::bind(&FSFloaterWearableFavorites::handleRemove, this));

    getChild<LLButton>("new_folder_btn")->setCommitCallback(boost::bind(&FSFloaterWearableFavorites::onNewFolder, this));

    mRenameFolderBtn = getChild<LLButton>("rename_folder_btn");
    mRenameFolderBtn->setCommitCallback(boost::bind(&FSFloaterWearableFavorites::onRenameFolder, this));

    mDeleteFolderBtn = getChild<LLButton>("delete_folder_btn");
    mDeleteFolderBtn->setCommitCallback(boost::bind(&FSFloaterWearableFavorites::onDeleteFolder, this));

    mFolderCombo = getChild<LLComboBox>("folder_combo");
    mFolderCombo->setCommitCallback(boost::bind(&FSFloaterWearableFavorites::onFolderChanged, this));

    mFilterEditor = getChild<LLFilterEditor>("wearable_filter_input");
    mFilterEditor->setCommitCallback(boost::bind(&FSFloaterWearableFavorites::onFilterEdit, this, _2));

    // Create menus.
    LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
    LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

    registrar.add("FavWearables.Action",                boost::bind(&FSFloaterWearableFavorites::onOptionsMenuItemClicked, this, _2));
    enable_registrar.add("FavWearables.CheckAction",    boost::bind(&FSFloaterWearableFavorites::onOptionsMenuItemChecked, this, _2));

    mOptionsButton = getChild<LLMenuButton>("options_btn");

    if (LLToggleableMenu* options_menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_fs_wearable_favorites.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance()); options_menu)
    {
        mOptionsMenuHandle = options_menu->getHandle();
        mOptionsButton->setMenu(options_menu, LLMenuButton::MP_BOTTOM_LEFT);
    }

    return true;
}

//virtual
void FSFloaterWearableFavorites::onOpen(const LLSD& /*info*/)
{
    if (!mInitialized)
    {
        if (sFolderID.isNull())
        {
            const LLHandle<LLFloater> handle = getHandle();
            initCategory([handle](const LLUUID&)
            {
                if (auto* self = static_cast<FSFloaterWearableFavorites*>(handle.get()))
                {
                    self->initialize();
                }
            });
        }
        else
        {
            initialize();
        }
    }
}

void FSFloaterWearableFavorites::initialize()
{
    if (mInitialized || !gInventory.getCategory(sFolderID))
    {
        return;
    }

    const LLUUID cof = gInventory.findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
    if (!gInventory.getCategory(cof))
    {
        return;
    }

    gInventory.addObserver(mCategoriesObserver);
    mCategoriesObserver->addCategory(cof, [this]() { mInventoryDirty = true; });
    mItemsList->setSortOrder((LLWearableItemsList::ESortOrder)gSavedSettings.getU32("FSWearableFavoritesSortOrder"));
    mDADCallbackConnection = mItemsList->setDADCallback(boost::bind(&FSFloaterWearableFavorites::onItemDAD, this, _1));
    mSelectedFolderID = sFolderID;
    mInitialized = true;
    refreshFolders();
}

void FSFloaterWearableFavorites::draw()
{
    if (mInventoryDirty)
    {
        refreshFolders();
    }

    LLFloater::draw();

    mRemoveItemBtn->setEnabled(mItemsList->numSelected() > 0);
    const auto* folder = getFavoriteSubfolder(mSelectedFolderID);
    mRenameFolderBtn->setEnabled(folder && (!RlvActions::isRlvEnabled() ||
        gRlvFolderLocks.canRenameFolder(mSelectedFolderID)));
    mDeleteFolderBtn->setEnabled(folder && !gInventory.getProtectedCategories().count(mSelectedFolderID) &&
        (!RlvActions::isRlvEnabled() || gRlvFolderLocks.canRemoveFolder(mSelectedFolderID)));
}

//virtual
bool FSFloaterWearableFavorites::handleKeyHere(KEY key, MASK mask)
{
    if (FSCommon::isFilterEditorKeyCombo(key, mask))
    {
        mFilterEditor->setFocus(true);
        return true;
    }

    return LLFloater::handleKeyHere(key, mask);
}

// static
std::optional<LLUUID> FSFloaterWearableFavorites::getWearableFavoritesFolderID()
{
    if (LLUUID fs_root_cat_id = gInventory.findCategoryByName(ROOT_FIRESTORM_FOLDER); !fs_root_cat_id.isNull())
    {
        LLInventoryModel::item_array_t* items;
        LLInventoryModel::cat_array_t* cats;
        gInventory.getDirectDescendentsOf(fs_root_cat_id, cats, items);
        if (cats)
        {
            for (const auto& cat : *cats)
            {
                if (cat->getName() == FS_WEARABLE_FAVORITES_FOLDER)
                {
                    return cat->getUUID();
                }
            }
        }
    }

    return std::nullopt;
}

// static
void FSFloaterWearableFavorites::initCategory(inventory_func_type callback)
{
    if (!gInventory.isInventoryUsable())
    {
        LL_WARNS() << "Cannot initialize Favorite Wearables inventory folder - inventory is not usable!" << LL_ENDL;
        return;
    }

    if (auto fs_favs_id = getWearableFavoritesFolderID(); fs_favs_id.has_value())
    {
        sFolderID = fs_favs_id.value();
        callback(sFolderID);
    }
    else
    {
        LLUUID fs_root_cat_id = gInventory.findCategoryByName(ROOT_FIRESTORM_FOLDER);
        if (fs_root_cat_id.isNull())
        {
            gInventory.createNewCategory(gInventory.getRootFolderID(), LLFolderType::FT_NONE, ROOT_FIRESTORM_FOLDER, [callback](const LLUUID& new_cat_id)
            {
                gInventory.createNewCategory(new_cat_id, LLFolderType::FT_NONE, FS_WEARABLE_FAVORITES_FOLDER, [callback](const LLUUID& new_cat_id)
                {
                    FSFloaterWearableFavorites::sFolderID = new_cat_id;
                    callback(new_cat_id);
                });
            });
        }
        else
        {
            gInventory.createNewCategory(fs_root_cat_id, LLFolderType::FT_NONE, FS_WEARABLE_FAVORITES_FOLDER, [callback](const LLUUID& new_cat_id)
            {
                FSFloaterWearableFavorites::sFolderID = new_cat_id;
                callback(new_cat_id);
            });
        }
    }
}

//static
LLUUID FSFloaterWearableFavorites::getFavoritesFolder()
{
    if (!sFolderID.isNull())
    {
        if (auto fs_favs_id = getWearableFavoritesFolderID(); fs_favs_id.has_value())
        {
            sFolderID = fs_favs_id.value();
        }
    }

    return sFolderID;
}

LLViewerInventoryCategory* FSFloaterWearableFavorites::getFavoriteSubfolder(const LLUUID& id)
{
    LLViewerInventoryCategory* folder = gInventory.getCategory(id);
    if (folder && sFolderID.notNull() && id != sFolderID &&
        folder->getParentUUID() == sFolderID &&
        folder->getPreferredType() == LLFolderType::FT_NONE)
    {
        return folder;
    }
    return nullptr;
}

void FSFloaterWearableFavorites::refreshFolders()
{
    mInventoryDirty = false;
    LLInventoryModel::cat_array_t categories;
    LLInventoryModel::item_array_t items;
    gInventory.collectDescendents(sFolderID, categories, items, LLInventoryModel::EXCLUDE_TRASH);

    // Change the set of observed folders outside the observer callback.
    uuid_set_t current_folders;
    auto observe = [this, &current_folders](LLViewerInventoryCategory* folder)
    {
        const LLUUID id = folder->getUUID();
        if (mObservedFolders.count(id) || mCategoriesObserver->addCategory(id, [this]() { mInventoryDirty = true; }))
        {
            current_folders.insert(id);
            if (!mObservedFolders.count(id))
            {
                folder->fetch();
            }
        }
    };
    if (auto* folder = gInventory.getCategory(sFolderID))
    {
        observe(folder);
    }
    for (LLViewerInventoryCategory* folder : categories)
    {
        observe(folder);
    }
    for (const LLUUID& id : mObservedFolders)
    {
        if (!current_folders.count(id))
        {
            mCategoriesObserver->removeCategory(id);
        }
    }
    mObservedFolders.swap(current_folders);

    LLInventoryModel::cat_array_t folders = getFavoriteSubfolders();
    mFolderCombo->clearRows();
    mFolderCombo->add(getString("root_folder_label"), LLSD(sFolderID));
    for (const auto& folder : folders)
    {
        mFolderCombo->add(folder->getName(), LLSD(folder->getUUID()));
    }
    if (!mFolderCombo->setSelectedByValue(LLSD(mSelectedFolderID), true))
    {
        mSelectedFolderID = sFolderID;
        mFolderCombo->setSelectedByValue(LLSD(sFolderID), true);
    }

    updateList();
}

LLInventoryModel::cat_array_t FSFloaterWearableFavorites::getFavoriteSubfolders()
{
    LLInventoryModel::cat_array_t folders;
    if (sFolderID.isNull())
    {
        return folders;
    }
    LLInventoryModel::cat_array_t* cats = nullptr;
    LLInventoryModel::item_array_t* items = nullptr;
    gInventory.getDirectDescendentsOf(sFolderID, cats, items);
    if (cats)
    {
        for (const auto& folder : *cats)
        {
            if (folder && getFavoriteSubfolder(folder->getUUID()))
            {
                folders.push_back(folder);
            }
        }
        std::sort(folders.begin(), folders.end(), [](const auto& lhs, const auto& rhs)
        {
            return LLStringUtil::compareDict(lhs->getName(), rhs->getName()) < 0;
        });
    }
    return folders;
}

void FSFloaterWearableFavorites::updateList()
{
    if (mSelectedFolderID == sFolderID)
    {
        LLInventoryModel::cat_array_t folders;
        LLInventoryModel::item_array_t items;
        LLFindOutfitItems collector;
        gInventory.collectDescendentsIf(sFolderID, folders, items, LLInventoryModel::EXCLUDE_TRASH, collector);

        // Prefer root links when the same item also appears in a subfolder.
        std::stable_partition(items.begin(), items.end(), [](const auto& item)
        {
            return item->getParentUUID() == sFolderID;
        });
        uuid_set_t seen;
        LLInventoryModel::item_array_t unique_items;
        for (const auto& item : items)
        {
            if (seen.insert(item->getLinkedUUID()).second)
            {
                unique_items.push_back(item);
            }
        }
        mItemsList->refreshList(unique_items);
    }
    else
    {
        mItemsList->updateList(mSelectedFolderID);
    }

    if (gInventory.isCategoryComplete(mSelectedFolderID))
    {
        mItemsList->setNoItemsCommentText(getString("empty_list"));
    }
}

static bool folder_contains_favorite(const LLUUID& folder_id, const LLUUID& linked_id)
{
    LLInventoryModel::cat_array_t* cats = nullptr;
    LLInventoryModel::item_array_t* items = nullptr;
    gInventory.getDirectDescendentsOf(folder_id, cats, items);
    return items && std::any_of(items->begin(), items->end(), [&linked_id](const auto& item)
    {
        return item && item->getLinkedUUID() == linked_id;
    });
}

static bool is_favorite_link(const LLViewerInventoryItem* item, const LLUUID& root)
{
    return item && item->getActualType() == LLAssetType::AT_LINK && root.notNull() &&
        gInventory.isObjectDescendentOf(item->getUUID(), root);
}

bool FSFloaterWearableFavorites::canMoveFavoriteLink(const LLUUID& id)
{
    return is_favorite_link(gInventory.getItem(id), sFolderID);
}

void FSFloaterWearableFavorites::assignFavoriteLinks(const LLUUID& folder_id, const uuid_vec_t& ids, bool move)
{
    const LLUUID root = sFolderID;
    if (!(getFavoriteSubfolder(folder_id) || (folder_id == root && gInventory.getCategory(root))) ||
        (RlvActions::isRlvEnabled() && gRlvFolderLocks.isLockedFolder(folder_id, RLV_LOCK_ADD)))
    {
        return;
    }

    // Inventory replies are asynchronous. Treat repeated drops or menu clicks
    // for the same item and destination as one request until it completes.
    using link_key_t = std::pair<LLUUID, LLUUID>;
    struct PendingLinks
    {
        std::set<link_key_t> links;
        uuid_set_t moves;
    };
    static const auto pending = std::make_shared<PendingLinks>();
    for (const LLUUID& id : ids)
    {
        const auto* item = gInventory.getItem(id);
        if (!item || (move && (!is_favorite_link(item, root) || item->getParentUUID() == folder_id ||
            pending->moves.count(id) || (RlvActions::isRlvEnabled() && !gRlvFolderLocks.canMoveItem(id, folder_id)))))
        {
            continue;
        }
        const LLUUID linked_id = item->getLinkedUUID();
        const auto* original = gInventory.getItem(linked_id);
        if (!original)
        {
            continue;
        }
        const LLAssetType::EType type = original->getType();
        if (type != LLAssetType::AT_CLOTHING && type != LLAssetType::AT_BODYPART &&
            type != LLAssetType::AT_OBJECT && type != LLAssetType::AT_GESTURE)
        {
            continue;
        }

        LLPointer<LLInventoryCallback> move_completion;
        if (move)
        {
            pending->moves.insert(id);
            move_completion = new LLBoostFuncInventoryCallback(no_op_inventory_func, [state = pending, id]()
            {
                state->moves.erase(id);
            });
        }
        const LLUUID source_folder = item->getParentUUID();
        auto remove_source = [root, folder_id, source_folder, id, linked_id, move_completion]()
        {
            const auto* source = gInventory.getItem(id);
            if (sFolderID == root && getFavoriteSubfolder(folder_id) &&
                is_favorite_link(source, root) && source->getParentUUID() == source_folder &&
                source->getLinkedUUID() == linked_id && folder_contains_favorite(folder_id, linked_id) &&
                (!RlvActions::isRlvEnabled() || gRlvFolderLocks.canMoveItem(id, folder_id)))
            {
                remove_inventory_item(id, move_completion);
            }
        };

        if (folder_contains_favorite(folder_id, linked_id))
        {
            if (move)
            {
                remove_source();
            }
            continue;
        }

        const link_key_t key(folder_id, linked_id);
        if (!pending->links.insert(key).second)
        {
            continue;
        }
        LLPointer<LLInventoryCallback> callback = new LLBoostFuncInventoryCallback(
            [move, remove_source](const LLUUID& new_id)
            {
                // Keep the source until the destination link is confirmed.
                if (move && new_id.notNull())
                {
                    remove_source();
                }
            },
            [state = pending, key]()
            {
                state->links.erase(key);
            });
        link_inventory_object(folder_id, linked_id, callback);
    }
}

void FSFloaterWearableFavorites::onItemDAD(const LLUUID& item_id)
{
    assignFavoriteLinks(mSelectedFolderID, {item_id}, false);
}

void FSFloaterWearableFavorites::handleRemove()
{
    uuid_vec_t selected_ids;
    mItemsList->getSelectedUUIDs(selected_ids);

    if (mSelectedFolderID == sFolderID)
    {
        uuid_set_t selected_targets;
        for (const LLUUID& id : selected_ids)
        {
            const auto* item = gInventory.getItem(id);
            if (is_favorite_link(item, sFolderID))
            {
                selected_targets.insert(item->getLinkedUUID());
            }
        }
        LLInventoryModel::cat_array_t folders;
        LLInventoryModel::item_array_t items;
        gInventory.collectDescendents(sFolderID, folders, items, LLInventoryModel::EXCLUDE_TRASH);
        selected_ids.clear();
        for (const auto& item : items)
        {
            if (item && selected_targets.count(item->getLinkedUUID()))
            {
                selected_ids.push_back(item->getUUID());
            }
        }
    }

    for (const LLUUID& id : selected_ids)
    {
        if (is_favorite_link(gInventory.getItem(id), sFolderID) &&
            (!RlvActions::isRlvEnabled() || gRlvFolderLocks.canRemoveItem(id)))
        {
            remove_inventory_item(id, nullptr);
        }
    }
}

void FSFloaterWearableFavorites::onFilterEdit(const std::string& search_string)
{
    mItemsList->setFilterSubString(search_string, true);
    mItemsList->setNoItemsCommentText(getString("empty_list"));
    mItemsList->rearrange();
}

void FSFloaterWearableFavorites::onFolderChanged()
{
    const LLUUID id = mFolderCombo->getSelectedValue().asUUID();
    if (id != mSelectedFolderID && (id == sFolderID || getFavoriteSubfolder(id)))
    {
        mSelectedFolderID = id;
        updateList();
    }
}

static std::string favorite_folder_name(const LLSD& response)
{
    std::string name = response["message"].asString();
    const bool invalid = std::any_of(name.begin(), name.end(), [](unsigned char ch)
    {
        return ch < 32 || ch > 126 || ch == '/' || ch == '\\' || ch == '|';
    });
    LLInventoryObject::correctInventoryName(name);
    if (invalid || name.empty())
    {
        LLNotificationsUtil::add("FSWearableFavoritesFolderInvalidName");
        return {};
    }
    return name;
}

void FSFloaterWearableFavorites::onNewFolder()
{
    if (!gInventory.getCategory(sFolderID))
    {
        return;
    }

    const LLUUID root = sFolderID;
    const LLHandle<LLFloater> handle = getHandle();
    LLNotificationsUtil::add("FSWearableFavoritesNewFolder", LLSD(), LLSD(),
        [handle, root](const LLSD& notification, const LLSD& response)
        {
            if (handle.isDead() || LLNotificationsUtil::getSelectedOption(notification, response) != 0 ||
                sFolderID != root || !gInventory.getCategory(root) ||
                (RlvActions::isRlvEnabled() && gRlvFolderLocks.isLockedFolder(root, RLV_LOCK_ADD)))
            {
                return false;
            }
            const std::string name = favorite_folder_name(response);
            if (name.empty())
            {
                return false;
            }
            gInventory.createNewCategory(root, LLFolderType::FT_NONE, name, [handle, root](const LLUUID& id)
            {
                auto* self = static_cast<FSFloaterWearableFavorites*>(handle.get());
                if (self && sFolderID == root && getFavoriteSubfolder(id))
                {
                    self->mSelectedFolderID = id;
                    self->mInventoryDirty = true;
                }
            });
            return false;
        });
}

void FSFloaterWearableFavorites::onRenameFolder()
{
    const LLUUID id = mSelectedFolderID;
    const auto* folder = getFavoriteSubfolder(id);
    if (!folder || (RlvActions::isRlvEnabled() && !gRlvFolderLocks.canRenameFolder(id)))
    {
        return;
    }

    LLSD args;
    args["FOLDER_NAME"] = folder->getName();
    const LLHandle<LLFloater> handle = getHandle();
    LLNotificationsUtil::add("FSWearableFavoritesRenameFolder", args, LLSD(),
        [id, handle](const LLSD& notification, const LLSD& response)
        {
            if (LLNotificationsUtil::getSelectedOption(notification, response) != 0 || !getFavoriteSubfolder(id) ||
                (RlvActions::isRlvEnabled() && !gRlvFolderLocks.canRenameFolder(id)))
            {
                return false;
            }
            const std::string name = favorite_folder_name(response);
            if (!name.empty())
            {
                LLSD updates;
                updates["name"] = name;
                auto callback = new LLBoostFuncInventoryCallback([handle](const LLUUID&)
                {
                    if (auto* self = static_cast<FSFloaterWearableFavorites*>(handle.get()))
                    {
                        self->mInventoryDirty = true;
                    }
                });
                update_inventory_category(id, updates, callback);
            }
            return false;
        });
}

static bool can_delete_favorite_folder(const LLUUID& id)
{
    const auto& protected_folders = gInventory.getProtectedCategories();
    if (!gInventory.isCategoryComplete(id) || protected_folders.count(id) ||
        (RlvActions::isRlvEnabled() && !gRlvFolderLocks.canRemoveFolder(id)))
    {
        return false;
    }
    LLInventoryModel::cat_array_t folders;
    LLInventoryModel::item_array_t items;
    gInventory.collectDescendents(id, folders, items, LLInventoryModel::EXCLUDE_TRASH);
    return std::all_of(folders.begin(), folders.end(), [&protected_folders](const auto& folder)
    {
        return folder && gInventory.isCategoryComplete(folder->getUUID()) &&
            folder->getPreferredType() == LLFolderType::FT_NONE && !protected_folders.count(folder->getUUID());
    }) && std::all_of(items.begin(), items.end(), [](const auto& item)
    {
        return item && item->getIsLinkType();
    });
}

void FSFloaterWearableFavorites::onDeleteFolder()
{
    const LLUUID id = mSelectedFolderID;
    const auto* folder = getFavoriteSubfolder(id);
    if (!folder)
    {
        return;
    }
    if (!can_delete_favorite_folder(id))
    {
        LLNotificationsUtil::add("FSWearableFavoritesFolderCannotDelete");
        return;
    }

    LLSD args;
    args["FOLDER_NAME"] = folder->getName();
    const LLHandle<LLFloater> handle = getHandle();
    LLNotificationsUtil::add("FSWearableFavoritesDeleteFolder", args, LLSD(),
        [id, handle](const LLSD& notification, const LLSD& response)
        {
            if (LLNotificationsUtil::getSelectedOption(notification, response) == 0 && getFavoriteSubfolder(id))
            {
                if (can_delete_favorite_folder(id))
                {
                    auto callback = new LLBoostFuncInventoryCallback([handle](const LLUUID&)
                    {
                        if (auto* self = static_cast<FSFloaterWearableFavorites*>(handle.get()))
                        {
                            self->mInventoryDirty = true;
                        }
                    });
                    remove_inventory_category(id, callback);
                }
                else
                {
                    LLNotificationsUtil::add("FSWearableFavoritesFolderCannotDelete");
                }
            }
            return false;
        });
}

void FSFloaterWearableFavorites::onOptionsMenuItemClicked(const LLSD& userdata)
{
    const std::string action = userdata.asString();

    if (action == "sort_by_name")
    {
        mItemsList->setSortOrder(LLWearableItemsList::E_SORT_BY_NAME);
        gSavedSettings.setU32("FSWearableFavoritesSortOrder", LLWearableItemsList::E_SORT_BY_NAME);
    }
    else if (action == "sort_by_most_recent")
    {
        mItemsList->setSortOrder(LLWearableItemsList::E_SORT_BY_MOST_RECENT);
        gSavedSettings.setU32("FSWearableFavoritesSortOrder", LLWearableItemsList::E_SORT_BY_MOST_RECENT);
    }
    else if (action == "sort_by_type_name")
    {
        mItemsList->setSortOrder(LLWearableItemsList::E_SORT_BY_TYPE_NAME);
        gSavedSettings.setU32("FSWearableFavoritesSortOrder", LLWearableItemsList::E_SORT_BY_TYPE_NAME);
    }
}

bool FSFloaterWearableFavorites::onOptionsMenuItemChecked(const LLSD& userdata)
{
    const std::string action = userdata.asString();

    if (action == "sort_by_name")
    {
        return mItemsList->getSortOrder() == LLWearableItemsList::E_SORT_BY_NAME;
    }
    else if (action == "sort_by_most_recent")
    {
        return mItemsList->getSortOrder() == LLWearableItemsList::E_SORT_BY_MOST_RECENT;
    }
    else if (action == "sort_by_type_name")
    {
        return mItemsList->getSortOrder() == LLWearableItemsList::E_SORT_BY_TYPE_NAME;
    }

    return false;
}

void FSFloaterWearableFavorites::onDoubleClick()
{
    if (LLUUID selected_item_id = mItemsList->getSelectedUUID(); selected_item_id.notNull())
    {
        uuid_vec_t ids;
        ids.push_back(selected_item_id);
        LLViewerInventoryItem* item = gInventory.getItem(selected_item_id);
        if (!item)
        {
            return;
        }

        if (get_is_item_worn(selected_item_id))
        {
            if ((item->getType() == LLAssetType::AT_CLOTHING && (!RlvActions::isRlvEnabled() || gRlvWearableLocks.canRemove(item))) ||
                ((item->getType() == LLAssetType::AT_OBJECT) && (!RlvActions::isRlvEnabled() || gRlvAttachmentLocks.canDetach(item))))
            {
                LLAppearanceMgr::instance().removeItemsFromAvatar(ids);
            }
        }
        else
        {
            if (item->getType() == LLAssetType::AT_BODYPART && (!RlvActions::isRlvEnabled() || (gRlvWearableLocks.canWear(item) & RLV_WEAR_REPLACE) == RLV_WEAR_REPLACE))
            {
                wear_multiple(ids, true);
            }
            else if (item->getType() == LLAssetType::AT_CLOTHING && LLAppearanceMgr::instance().canAddWearables(ids) && (!RlvActions::isRlvEnabled() || (gRlvWearableLocks.canWear(item) & RLV_WEAR_ADD) == RLV_WEAR_ADD))
            {
                wear_multiple(ids, false);
            }
            else if (item->getType() == LLAssetType::AT_OBJECT && LLAppearanceMgr::instance().canAddWearables(ids) && (!RlvActions::isRlvEnabled() || (gRlvAttachmentLocks.canAttach(item) & RLV_WEAR_ADD) == RLV_WEAR_ADD))
            {
                wear_multiple(ids, false);
            }
        }
    }
}
