#include "Presets.h"

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state) : apvts (state) {}

juce::File PresetManager::getUserPresetDirectory()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("NightShift").getChildFile ("User Presets");
    if (! dir.exists())
        dir.createDirectory();
    return dir;
}

juce::StringArray PresetManager::getFolders() const
{
    juce::StringArray folders;
    for (auto& p : getFactoryPresets())
        folders.addIfNotAlreadyThere (p.folder);
    if (folders.contains ("Tech House Pro"))   // keep the house folder at the end, after all label styles
    {
        folders.removeString ("Tech House Pro");
        folders.add ("Tech House Pro");
    }
    folders.add (userFolderName);
    return folders;
}

juce::StringArray PresetManager::getPresetsInFolder (const juce::String& folder) const
{
    juce::StringArray names;
    if (folder == userFolderName)
    {
        for (auto& f : getUserPresetDirectory().findChildFiles (juce::File::findFiles, false, juce::String ("*") + fileExtension))
            names.add (f.getFileNameWithoutExtension());
        names.sortNatural();
    }
    else
    {
        for (auto& p : getFactoryPresets())
            if (p.folder == folder)
                names.add (p.name);
    }
    return names;
}

void PresetManager::initPatch()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());
}

bool PresetManager::loadFactory (int index)
{
    const auto& bank = getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) bank.size()))
        return false;

    initPatch();
    const auto& preset = bank[(size_t) index];
    for (const auto& [id, value] : preset.values)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
        else
            jassertfalse;   // unknown parameter id in factory bank
    }
    currentFolder = preset.folder;
    currentName = preset.name;
    currentFactory = index;
    return true;
}

bool PresetManager::load (const juce::String& folder, const juce::String& name)
{
    if (folder == userFolderName)
    {
        auto file = getUserPresetDirectory().getChildFile (name + fileExtension);
        auto xml = juce::XmlDocument::parse (file);
        if (xml == nullptr)
            return false;

        auto tree = juce::ValueTree::fromXml (*xml);
        if (! tree.isValid() || ! tree.hasType (apvts.state.getType()))
            return false;

        initPatch();   // parameters missing from older files fall back to defaults
        for (auto* p : apvts.processor.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            {
                auto child = tree.getChildWithProperty ("id", rp->getParameterID());
                if (child.isValid() && child.hasProperty ("value"))
                    rp->setValueNotifyingHost (rp->convertTo0to1 ((float) child.getProperty ("value")));
            }
        currentFolder = folder;
        currentName = name;
        currentFactory = -1;
        return true;
    }

    const auto& bank = getFactoryPresets();
    for (int i = 0; i < (int) bank.size(); ++i)
        if (bank[(size_t) i].folder == folder && bank[(size_t) i].name == name)
            return loadFactory (i);
    return false;
}

bool PresetManager::saveUserPreset (const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty())
        return false;

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml == nullptr)
        return false;

    auto file = getUserPresetDirectory().getChildFile (name + fileExtension);
    if (! xml->writeTo (file))
        return false;

    currentFolder = userFolderName;
    currentName = name;
    currentFactory = -1;
    return true;
}

void PresetManager::step (int delta)
{
    juce::Array<std::pair<juce::String, juce::String>> all;
    for (auto& folder : getFolders())
        for (auto& name : getPresetsInFolder (folder))
            all.add ({ folder, name });

    if (all.isEmpty())
        return;

    int idx = 0;
    for (int i = 0; i < all.size(); ++i)
        if (all[i].first == currentFolder && all[i].second == currentName) { idx = i; break; }

    idx = (idx + delta + all.size()) % all.size();
    load (all[idx].first, all[idx].second);
}

juce::StringArray PresetManager::validateFactoryPresets() const
{
    juce::StringArray errors;
    juce::StringArray seen;
    for (auto& p : getFactoryPresets())
    {
        const auto key = p.folder + "/" + p.name;
        if (p.name != "Init" && p.category == "Other") errors.add ("Missing type prefix: " + key);
        if (seen.contains (key)) errors.add ("Duplicate preset: " + key);
        seen.add (key);

        juce::StringArray ids;
        for (const auto& [id, value] : p.values)
        {
            if (ids.contains (id)) errors.add (key + ": duplicate id " + id);
            ids.add (id);

            auto* param = apvts.getParameter (id);
            if (param == nullptr) { errors.add (key + ": unknown id " + id); continue; }

            const auto range = param->getNormalisableRange();
            if (value < range.start - 1.0e-4f || value > range.end + 1.0e-4f)
                errors.add (key + ": " + id + " = " + juce::String (value) + " out of range ["
                            + juce::String (range.start) + ", " + juce::String (range.end) + "]");
        }
    }
    return errors;
}
