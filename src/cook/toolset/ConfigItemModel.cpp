#include "cook/toolset/ConfigItemModel.hpp"
#include "cook/toolset/ConfigItem.hpp"
#include "cook/toolset/ToolManager.hpp"
#include <QUuid>
#include <utils/qtcassert.h>

namespace cook { namespace toolset {

ConfigItemModel::ConfigItemModel(const Core::Id & type_id, QObject *parent)
    : Utils::TreeModel<Utils::TreeItem, Utils::TreeItem, ConfigItem>(parent),
      type_id_(type_id)
{
    ToolManager * mgr = ToolManager::instance();
    factory_ = mgr->factory(type_id_);
    default_item_id_ = mgr->default_tool_id(type_id);

    setHeader({ tr("Name"), tr("Location"), tr("Version") });
    rootItem()->appendChild(new Utils::StaticTreeItem(tr("Auto-detected")));
    rootItem()->appendChild(new Utils::StaticTreeItem(tr("Manual")));

    // add the existing item
    for(const Tool * tool: mgr->registered_tools())
        if(tool->type_id() == type_id_)
            add_tool(tool);

    // connect the signals
    connect(mgr, &ToolManager::tool_removed, this, &ConfigItemModel::remove_tool);
    connect(mgr, &ToolManager::tool_added, this, [this, mgr](const Core::Id & id) { add_tool(mgr->find_registered_tool(id)); });
    connect(mgr, &ToolManager::default_tool_changed, this, [this, mgr]() { set_default_item_id(mgr->default_tool_id(type_id_)); });
}

ConfigItem * ConfigItemModel::add_tool(const Tool * tool)
{
    QTC_ASSERT(tool, return nullptr);
    QTC_ASSERT(factory_, return nullptr);

    if(tool->type_id() != type_id_)
        return nullptr;

    // already an item with this id?
    ConfigItem * item = find_item(tool->id());
    if(item)
        return item;

    item = new ConfigItem(factory_, tool);
    (tool->is_auto_detected() ? auto_group_item() : manual_group_item())->appendChild(item);

    return item;
}

ConfigItem * ConfigItemModel::add_tool(const QString & displayName, const QFileInfo & executable)
{
    QTC_ASSERT(factory_, return nullptr);
    ConfigItem * item = new ConfigItem(factory_);
    item->tool().set_display_name(displayName);
    item->tool().set_exec_file(executable);
    manual_group_item()->appendChild(item);

    return item;
}

void ConfigItemModel::update_tool(const Core::Id &id, const QString &display_name, const QFileInfo &executable)
{
    ConfigItem * item = find_item(id);
    if (item == nullptr)
        return;

    item->tool().set_display_name(display_name);
    item->tool().set_exec_file(executable);

    if(item->has_changed())
        item->update();
}

void ConfigItemModel::remove_tool(const Core::Id & id)
{
    ConfigItem * item = find_item(id);
    if( item == nullptr)
        return;

    // add to be removed on apply
    to_remove_.append(id);

    // remove from the model
    destroyItem(item);
}

ConfigItem * ConfigItemModel::find_item(const Core::Id & id) const
{
    return findItemAtLevel<2>( [id](ConfigItem * item){ return item->tool().id() == id; });
}

ConfigItem * ConfigItemModel::find_item(const QModelIndex & index) const
{
    return itemForIndexAtLevel<2>(index);
}

void ConfigItemModel::apply()
{
    ToolManager * mgr = ToolManager::instance();

    QList<ConfigItem*> to_update;

    // apply all the changes to the manager
    forItemsAtLevel<2>( [this, mgr, &to_update](ConfigItem * item)
    {
        if(item->has_changed())
        {
            // new item?
            const Tool * tool = mgr->update_or_register_tool(item->tool());
            QTC_ASSERT(tool, return);
            item->update_original_tool(tool);

            to_update << item;
        }
    });

    // remove the tools that need to be removed
    for(const Core::Id & id : to_remove_)
        mgr->deregister_tool(id);
    to_remove_.clear();

    // update the default item (if changed)
    if(default_item_id() != mgr->default_tool_id(type_id_))
        mgr->set_default_tool(default_item_id());


    // update the items
    for(ConfigItem * item : to_update)
        item->update();
}

Utils::TreeItem * ConfigItemModel::auto_group_item() const
{
    return rootItem()->childAt(0);
}

Utils::TreeItem * ConfigItemModel::manual_group_item() const
{
    return rootItem()->childAt(1);
}

Core::Id ConfigItemModel::default_item_id() const
{
    return default_item_id_;
}

void ConfigItemModel::update_(const Core::Id &id)
{
    ConfigItem * item = find_item(id);
    if (item && item->has_changed())
        item->update();
}

void ConfigItemModel::set_default_item_id(const Core::Id &id)
{
    if (id == default_item_id_)
        return;

    Core::Id old_id = default_item_id_;
    default_item_id_ = id;

    update_(old_id);
    update_(id);
}

} }


