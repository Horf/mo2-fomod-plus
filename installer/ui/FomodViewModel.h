﻿#ifndef DIALOGSTATEMANAGER_H
#define DIALOGSTATEMANAGER_H
#include <imoinfo.h>
#include <string>

#include "../lib/ConditionTester.h"
#include "../lib/FlagMap.h"
#include "lib/FileInstaller.h"
#include "xml/FomodInfoFile.h"

template <typename T>
using shared_ptr_list = std::vector<std::shared_ptr<T> >;

/*
--------------------------------------------------------------------------------
                                Plugins
--------------------------------------------------------------------------------
*/
class PluginViewModel {
public:
    PluginViewModel(const std::shared_ptr<Plugin>& plugin_, const bool selected, bool, const int index)
        : ownIndex(index), selected(selected), enabled(true), plugin(plugin_) {}

    void setSelected(const bool selected) { this->selected = selected; }
    void setEnabled(const bool enabled) { this->enabled = enabled; }
    [[nodiscard]] std::string getName() const { return plugin->name; }
    [[nodiscard]] std::string getDescription() const { return plugin->description; }
    [[nodiscard]] std::string getImagePath() const { return plugin->image.path; }
    [[nodiscard]] bool isSelected() const { return selected; }
    [[nodiscard]] bool isEnabled() const { return enabled; }
    [[nodiscard]] std::vector<ConditionFlag> getConditionFlags() const { return plugin->conditionFlags.flags; }
    int getOwnIndex() const { return ownIndex; }
    PluginTypeEnum getCurrentPluginType() const { return currentPluginType; }
    void setCurrentPluginType(const PluginTypeEnum type) { currentPluginType = type; }

    friend class FomodViewModel;
    friend class FileInstaller;
    friend class ConditionTester;

protected:
    [[nodiscard]] std::shared_ptr<Plugin> getPlugin() const { return plugin; }

private:
    int ownIndex;
    bool selected;
    bool enabled;
    PluginTypeEnum currentPluginType = PluginTypeEnum::UNKNOWN;
    std::shared_ptr<Plugin> plugin;
};

/*
--------------------------------------------------------------------------------
                                Groups
--------------------------------------------------------------------------------
*/
class GroupViewModel {
public:
    GroupViewModel(const std::shared_ptr<Group>& group_, const shared_ptr_list<PluginViewModel>& plugins,
        const int index, const int stepIndex)
        : plugins(plugins), group(group_), ownIndex(index), stepIndex(stepIndex) {}

    void addPlugin(const std::shared_ptr<PluginViewModel>& plugin) { plugins.emplace_back(plugin); }

    [[nodiscard]] std::string getName() const { return group->name; }
    [[nodiscard]] GroupTypeEnum getType() const { return group->type; }
    [[nodiscard]] shared_ptr_list<PluginViewModel> getPlugins() const { return plugins; }
    [[nodiscard]] int getOwnIndex() const { return ownIndex; }
    [[nodiscard]] int getStepIndex() const { return stepIndex; }

private:
    shared_ptr_list<PluginViewModel> plugins;
    std::shared_ptr<Group> group;
    int ownIndex;
    int stepIndex;
};

/*
--------------------------------------------------------------------------------
                                Steps
--------------------------------------------------------------------------------
*/
class StepViewModel {
public:
    StepViewModel(const std::shared_ptr<InstallStep>& installStep_, const shared_ptr_list<GroupViewModel>& groups,
        const int index)
        : installStep(installStep_), groups(groups), ownIndex(index) {}

    [[nodiscard]] CompositeDependency& getVisibilityConditions() const { return installStep->visible; }
    [[nodiscard]] std::string getName() const { return installStep->name; }
    [[nodiscard]] const shared_ptr_list<GroupViewModel>& getGroups() const { return groups; }
    [[nodiscard]] int getOwnIndex() const { return ownIndex; }
    [[nodiscard]] bool getHasVisited() const { return visited; }
    void setVisited(const bool visited) { this->visited = visited; }

private:
    bool visited{false};
    std::shared_ptr<InstallStep> installStep;
    shared_ptr_list<GroupViewModel> groups;
    int ownIndex;
};

/*
--------------------------------------------------------------------------------
                                Info
--------------------------------------------------------------------------------
*/
class InfoViewModel {
public:
    explicit InfoViewModel(const std::unique_ptr<FomodInfoFile>& infoFile)
    {
        if (infoFile) {
            // Copy the necessary members from FomodInfoFile to InfoViewModel
            mName    = infoFile->getName();
            mVersion = infoFile->getVersion();
            mAuthor  = infoFile->getAuthor();
            mWebsite = infoFile->getWebsite();
        }
    }

    // Accessor methods
    [[nodiscard]] std::string getName() const { return mName; }
    [[nodiscard]] std::string getVersion() const { return mVersion; }
    [[nodiscard]] std::string getAuthor() const { return mAuthor; }
    [[nodiscard]] std::string getWebsite() const { return mWebsite; }

private:
    std::string mName;
    std::string mVersion;
    std::string mAuthor;
    std::string mWebsite;
};

/*
--------------------------------------------------------------------------------
                               View Model
--------------------------------------------------------------------------------
*/
enum class NEXT_OP { NEXT, INSTALL };

class FomodViewModel {
public:
    FomodViewModel(
        MOBase::IOrganizer* organizer,
        std::unique_ptr<ModuleConfiguration> fomodFile,
        std::unique_ptr<FomodInfoFile> infoFile);

    static std::shared_ptr<FomodViewModel> create(
        MOBase::IOrganizer* organizer,
        std::unique_ptr<ModuleConfiguration> fomodFile,
        std::unique_ptr<FomodInfoFile> infoFile);

    void forEachGroup(
        const std::function<void(const std::shared_ptr<GroupViewModel>&)>& callback)
    const;

    void forEachPlugin(
        const std::function<void(const std::shared_ptr<GroupViewModel>&, const std::shared_ptr<PluginViewModel>&)>&
        callback)
    const;

    void forEachFuturePlugin(
        int fromStepIndex, const std::function<void(const std::shared_ptr<GroupViewModel>&, const std::shared_ptr<
            PluginViewModel>&)> &callback)
    const;

    void selectFromJson(nlohmann::json json) const;

    [[nodiscard]] const std::shared_ptr<PluginViewModel>& getFirstPluginForActiveStep() const;

    // Steps
    [[nodiscard]] shared_ptr_list<StepViewModel> getSteps() const { return mSteps; }
    [[nodiscard]] const std::shared_ptr<StepViewModel>& getActiveStep() const { return mActiveStep; }
    [[nodiscard]] int getCurrentStepIndex() const { return mCurrentStepIndex; }
    [[deprecated]] void setCurrentStepIndex(const int index) { mCurrentStepIndex = index; }

    void updateVisibleSteps() const;

    void rebuildConditionFlagsForStep(int stepIndex) const;

    void preinstall(const std::shared_ptr<MOBase::IFileTree>& tree, const QString& fomodPath);

    std::shared_ptr<FileInstaller> getFileInstaller() { return mFileInstaller; }

    std::string getDisplayImage() const;

    // Plugins
    [[nodiscard]] std::shared_ptr<PluginViewModel> getActivePlugin() const { return mActivePlugin; }

    // Info
    [[nodiscard]] std::shared_ptr<InfoViewModel> getInfoViewModel() const { return mInfoViewModel; }

    // Interactions
    void stepBack();

    void stepForward();

    bool isLastVisibleStep() const;

    void togglePlugin(const std::shared_ptr<GroupViewModel>&, const std::shared_ptr<PluginViewModel>& plugin,
        bool selected) const;

    void setActivePlugin(const std::shared_ptr<PluginViewModel>& plugin) const { mActivePlugin = plugin; }

private:
    Logger& log = Logger::getInstance();
    MOBase::IOrganizer* mOrganizer = nullptr;
    std::unique_ptr<ModuleConfiguration> mFomodFile;
    std::unique_ptr<FomodInfoFile> mInfoFile;
    std::shared_ptr<FlagMap> mFlags{ nullptr };
    ConditionTester mConditionTester;
    std::shared_ptr<InfoViewModel> mInfoViewModel;
    std::vector<std::shared_ptr<StepViewModel> > mSteps;
    mutable std::shared_ptr<PluginViewModel> mActivePlugin{ nullptr };
    mutable std::shared_ptr<StepViewModel> mActiveStep{ nullptr };
    mutable std::vector<int> mVisibleStepIndices;
    std::shared_ptr<FileInstaller> mFileInstaller{ nullptr };
    bool mInitialized{ false };

    void createStepViewModels();

    void setFlagForPluginState(const std::shared_ptr<PluginViewModel>& plugin, bool selected) const;

    static void createNonePluginForGroup(const std::shared_ptr<GroupViewModel>& group) ;

    void processPlugin(const std::shared_ptr<GroupViewModel>& group,
        const std::shared_ptr<PluginViewModel>& plugin) const;

    void enforceRadioGroupConstraints(const std::shared_ptr<GroupViewModel>& group) const;

    void enforceSelectAllConstraint(const std::shared_ptr<GroupViewModel>& groupViewModel) const;

    void enforceSelectAtLeastOneConstraint(const std::shared_ptr<GroupViewModel>& groupViewModel) const;

    void enforceGroupConstraints() const;

    void processPluginConditions(int fromStepIndex) const;

    // Indices
    int mCurrentStepIndex{ 0 };

    void logMessage(LogLevel level, const std::string& message) const
    {
        log.logMessage(level, "[VIEWMODEL] " + message);
    };

    std::string toString();

    bool isRadioLike(const std::shared_ptr<GroupViewModel>& group) const;
};


#endif //DIALOGSTATEMANAGER_H