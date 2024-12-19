/*
 ==============================================================================
    Copyright (c) 2019-2023 Foleys Finest Audio - Daniel Walz
    All rights reserved.

    **BSD 3-Clause License**

    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

 ==============================================================================

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
    OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
    OF THE POSSIBILITY OF SUCH DAMAGE.
 ==============================================================================
 */

#include "foleys_MagicGUIBuilder.h"

#include "../Helpers/foleys_DefaultGuiTrees.h"
#include "../Layout/foleys_Container.h"
#include "../Layout/foleys_RootItem.h"
#include "../LookAndFeels/foleys_JuceLookAndFeels.h"
#include "../LookAndFeels/foleys_LookAndFeel.h"
#include "../LookAndFeels/foleys_Skeuomorphic.h"

#if FOLEYS_SHOW_GUI_EDITOR_PALLETTE
#include "../Editor/foleys_ToolBox.h"
#include "../Editor/foleys_StylePropertyComponent.h"
#endif

namespace foleys
{


MagicGUIBuilder::MagicGUIBuilder (MagicGUIState& state, std::unique_ptr<Stylesheet> customStylesheet) : magicState (state)
{
    if (customStylesheet)
        stylesheet.reset (customStylesheet.release());
    else
        stylesheet = std::make_unique<Stylesheet> (*this);

    updateStylesheet();
    getConfigTree().addListener (this);
    getEditorTree().addListener (this);
}

MagicGUIBuilder::~MagicGUIBuilder()
{
    getConfigTree().removeListener (this);
    getEditorTree().removeListener (this);
    
    masterReference.clear();
}

Stylesheet& MagicGUIBuilder::getStylesheet()
{
    return *stylesheet.get ();
}

juce::ValueTree& MagicGUIBuilder::getConfigTree()
{
    return magicState.getGuiTree();
}

juce::ValueTree& MagicGUIBuilder::getEditorTree()
{
    return magicState.getEditorTree();
}

juce::ValueTree MagicGUIBuilder::getGuiRootNode()
{
    return getConfigTree().getOrCreateChildWithName (IDs::view, &undo);
}

std::unique_ptr<GuiItem> MagicGUIBuilder::createGuiItem (const juce::ValueTree& node)
{
    // in case a gui item calls updateSelectedNode which would lead to infinite loop
    juce::ScopedValueSetter svs{ blockSelectedNodeUpdates, true };
    
    if (node.getType() == IDs::view)
    {
        auto item = (node == getGuiRootNode()) ? createRootItem (node) : createContainer (node);
        item->updateInternal();
        item->createSubComponents();
        return item;
    }

    auto factory = factories.find (node.getType());
    if (factory != factories.end())
    {
        auto item = factory->second (*this, node);
        item->init ();
        item->updateInternal();
        return item;
    }

    DBG ("No GUI factory for: " << node.getType().toString());
    return {};
}

void MagicGUIBuilder::updateStylesheet()
{
    auto stylesNode = getConfigTree().getOrCreateChildWithName (IDs::styles, &undo);
    if (stylesNode.getNumChildren() == 0)
        stylesNode.appendChild (DefaultGuiTrees::createDefaultStylesheet(), &undo);

    auto selectedName = stylesNode.getProperty (IDs::selected, {}).toString();
    if (selectedName.isNotEmpty())
    {
        auto style = stylesNode.getChildWithProperty (IDs::name, selectedName);
        stylesheet->setStyle (style);
    }
    else
    {
        stylesheet->setStyle (stylesNode.getChild (0));
    }

    stylesheet->updateStyleClasses();
    stylesheet->updateValidRanges();
}

void MagicGUIBuilder::clearGUI()
{
    auto guiNode = getConfigTree().getOrCreateChildWithName (IDs::view, &undo);
    guiNode.removeAllChildren (&undo);
    guiNode.removeAllProperties (&undo);

    updateComponents();
}

void MagicGUIBuilder::showOverlayDialog (std::unique_ptr<juce::Component> dialog)
{
    if (parent == nullptr)
        return;

    overlayDialog = std::move (dialog);
    parent->addAndMakeVisible (overlayDialog.get());

    parent->resized();
}

void MagicGUIBuilder::closeOverlayDialog()
{
    overlayDialog.reset();
}

void MagicGUIBuilder::createGUI (juce::Component& parentToUse)
{
    parent = &parentToUse;

    updateComponents();
}

void MagicGUIBuilder::updateComponents()
{
    if (parent == nullptr)
        return;

    updateStylesheet();

    root = createGuiItem (getGuiRootNode());
    parent->addAndMakeVisible (root.get());

    auto width = getGuiRootNode().getProperty (IDs::width, 600);
    auto height = getGuiRootNode().getProperty (IDs::height, 800);

    parent->setSize (width, height);

    if (root.get() != nullptr)
        root->setEditMode (editMode);

    listeners.call ([&] (Listener& l) { l.stateWasReloaded(); });
}

void MagicGUIBuilder::updateLayout (juce::Rectangle<int> bounds)
{
    if (parent == nullptr)
        return;

    if (root.get() != nullptr)
    {
        if (!stylesheet->setMediaSize (bounds.getWidth(), bounds.getHeight()))
        {
            stylesheet->updateValidRanges();
            root->updateInternal();
        }

        if (root->getBounds() == bounds)
            root->updateLayout();
        else
            root->setBounds (bounds);
    }

    if (overlayDialog)
    {
        if (overlayDialog->getBounds() == bounds)
            overlayDialog->resized();
        else
            overlayDialog->setBounds (bounds);
    }

    parent->repaint();
}

void MagicGUIBuilder::updateColours()
{
    if (root)
        root->updateColours();
}

GuiItem* MagicGUIBuilder::findGuiItemWithId (const juce::String& name)
{
    if (root)
        return root->findGuiItemWithId (name);

    return nullptr;
}

GuiItem* MagicGUIBuilder::findGuiItem (const juce::ValueTree& node)
{
    if (node.isValid() && root)
        return root->findGuiItem (node);

    return nullptr;
}

GuiItem* MagicGUIBuilder::findGuiItemWithProperty (const juce::Identifier& property, const juce::var& value)
{
    if (root)
        return root->findGuiItemWithProperty (property, value);

    return nullptr;
}

juce::ValueTree MagicGUIBuilder::findNodeWithProperty (const juce::Identifier& property, const juce::var& value)
{
    if (auto item = findGuiItemWithProperty (property, value))
        return item->getNode ();

    return {};
}

void MagicGUIBuilder::registerFactory (juce::Identifier type, std::unique_ptr<GuiItem> (*factory) (MagicGUIBuilder& builder, const juce::ValueTree&))
{
    if (factories.find (type) != factories.cend())
    {
        // You tried to add two factories with the same type name!
        // That cannot work, the second factory will be ignored.
        jassertfalse;
        return;
    }

    factories[type] = factory;
    
    auto temp = factory (*this, juce::ValueTree (type));
    jassert (temp);

    defaultProperties[type] = temp->getSettableProperties();
}

juce::StringArray MagicGUIBuilder::getFactoryNames() const
{
    juce::StringArray names { IDs::view.toString() };

    names.ensureStorageAllocated (int (factories.size()));
    for (const auto& f: factories)
        names.add (f.first.toString());

    return names;
}

std::unique_ptr<GuiItem> MagicGUIBuilder::createRootItem (const juce::ValueTree& node)
{
    return std::make_unique<RootItem> (*this, node);
}

std::unique_ptr<GuiItem> MagicGUIBuilder::createContainer (const juce::ValueTree& node)
{
    return std::make_unique<Container> (*this, node);
}

void MagicGUIBuilder::registerLookAndFeel (juce::String name, std::unique_ptr<juce::LookAndFeel> lookAndFeel)
{
    stylesheet->registerLookAndFeel (name, std::move (lookAndFeel));
}

void MagicGUIBuilder::registerJUCELookAndFeels()
{
    stylesheet->registerLookAndFeel ("LookAndFeel_V1", std::make_unique<juce::LookAndFeel_V1>());
    stylesheet->registerLookAndFeel ("LookAndFeel_V2", std::make_unique<JuceLookAndFeel_V2>());
    stylesheet->registerLookAndFeel ("LookAndFeel_V3", std::make_unique<JuceLookAndFeel_V3>());
    stylesheet->registerLookAndFeel ("LookAndFeel_V4", std::make_unique<JuceLookAndFeel_V4>());
    stylesheet->registerLookAndFeel ("FoleysFinest", std::make_unique<LookAndFeel>());
    stylesheet->registerLookAndFeel ("Skeuomorphic", std::make_unique<Skeuomorphic>());
}

juce::var MagicGUIBuilder::getStyleProperty (const juce::Identifier& name, const juce::ValueTree& node) const
{
    if (auto value = stylesheet->getStyleProperty (name, node); ! value.isVoid ())
        return value;

    // if (auto defaults = defaultProperties.find (node.getType()); defaults != defaultProperties.end ())
    //     for (auto property : defaults->second)
    //         if (property.name == name)
    //             return property.defaultValue;

    return {};
}

void MagicGUIBuilder::removeStyleClassReferences (juce::ValueTree tree, const juce::String& name)
{
    if (tree.hasProperty (IDs::styleClass))
    {
        const auto separator = " ";
        auto       strings   = juce::StringArray::fromTokens (tree.getProperty (IDs::styleClass).toString(), separator, "");
        strings.removeEmptyStrings (true);
        strings.removeString (name);
        tree.setProperty (IDs::styleClass, strings.joinIntoString (separator), &undo);
    }

    for (auto child: tree)
        removeStyleClassReferences (child, name);
}

juce::StringArray MagicGUIBuilder::getColourNames (juce::Identifier type)
{
    juce::ValueTree node (type);
    if (auto item = createGuiItem (node))
        return item->getColourNames();

    return {};
}

juce::PropertyComponent * MagicGUIBuilder::createStylePropertyComponent(SettableProperty property, juce::ValueTree node)
{
#if FOLEYS_SHOW_GUI_EDITOR_PALLETTE
    return StylePropertyComponent::createComponent (*this, property, node);
#else
    jassertfalse;
    return nullptr;
#endif
}

std::function<void (juce::ComboBox&)> MagicGUIBuilder::createChoicesMenuLambda (juce::StringArray choices) const
{
    return [choices] (juce::ComboBox& combo)
    {
        int index = 0;
        for (auto& choice: choices)
            combo.addItem (choice, ++index);
    };
}

std::function<void (juce::ComboBox&)> MagicGUIBuilder::createParameterMenuLambda() const
{
    return [this] (juce::ComboBox& combo) { *combo.getRootMenu() = magicState.createParameterMenu(); };
}

std::function<void (juce::ComboBox&)> MagicGUIBuilder::createPropertiesMenuLambda() const
{
    return [this] (juce::ComboBox& combo) { magicState.populatePropertiesMenu (combo); };
}

std::function<void (juce::ComboBox&)> MagicGUIBuilder::createTriggerMenuLambda() const
{
    return [this] (juce::ComboBox& combo) { *combo.getRootMenu() = magicState.createTriggerMenu(); };
}

juce::var MagicGUIBuilder::getPropertyDefaultValue (juce::Identifier property, juce::Identifier type) const
{
    if (! type.isNull ())
        if (auto defaults = defaultProperties.find (type); defaults != defaultProperties.end ())
            for (auto pDefault : defaults->second)
                if (pDefault.name == property)
                    return pDefault.defaultValue;
                    
    // flexbox
    if (property == IDs::flexDirection)
        return IDs::flexDirRow;
    if (property == IDs::flexWrap)
        return IDs::flexNoWrap;
    if (property == IDs::flexAlignContent)
        return IDs::flexStretch;
    if (property == IDs::flexAlignItems)
        return IDs::flexStretch;
    if (property == IDs::flexJustifyContent)
        return IDs::flexStart;
    if (property == IDs::flexAlignSelf)
        return IDs::flexStretch;
    if (property == IDs::flexOrder)
        return 0;
    if (property == IDs::flexGrow)
        return 1.0;
    if (property == IDs::flexShrink)
        return 1.0;
    if (property == IDs::minWidth)
        return 10.0;
    if (property == IDs::minHeight)
        return 10.0;
    if (property == IDs::display)
        return IDs::flexbox;

    // caption
    if (property == IDs::captionPlacement)
        return "centred-top";

    // look and feel
    if (property == IDs::lookAndFeel)
        return "FoleysFinest";

    // font
    if (property == juce::Identifier ("font-size"))
        return 12.0;

    // // decorator
    if (property == IDs::backgroundColour)
        return juce::Colours::darkgrey.toString();
    
    if (property == IDs::borderColour || property == IDs::captionColour)
        return juce::Colours::silver.toString();

    return {};
}

RadioButtonManager& MagicGUIBuilder::getRadioButtonManager()
{
    return radioButtonManager;
}

void MagicGUIBuilder::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (root.get() != nullptr)
        root->updateInternal();

    root->resized();
}

void MagicGUIBuilder::valueTreeRedirected (juce::ValueTree& treeWhichHasBeenChanged)
{
    juce::ignoreUnused (treeWhichHasBeenChanged);
    updateComponents();
}

void MagicGUIBuilder::valueTreePropertyChanged(juce::ValueTree & tree, const juce::Identifier & property)
{
    if (tree == magicState.getEditorTree ())
    {
        if (property == IDs::editModeEnabled)
            setEditMode (tree[property]);
    }
}

MagicGUIState& MagicGUIBuilder::getMagicState()
{
    return magicState;
}

juce::UndoManager& MagicGUIBuilder::getUndoManager()
{
    return undo;
}

void MagicGUIBuilder::setEditMode (bool shouldEdit)
{
    editMode = shouldEdit;

    if (parent == nullptr)
        return;

    if (root.get() != nullptr)
        root->setEditMode (shouldEdit);

    if (shouldEdit == false)
        setSelectedNode (juce::ValueTree());

    listeners.call (&Listener::editModeToggled, editMode);
    
    getMagicState ().getEditorTree ().setProperty (IDs::editModeEnabled, editMode, nullptr);

    parent->repaint();
}

bool MagicGUIBuilder::isEditModeOn() const
{
    return editMode;
}

void MagicGUIBuilder::setSelectedNode (const juce::ValueTree& node)
{
    if (selectedNode != node)
    {
        if (auto* item = findGuiItem (selectedNode))
            item->setDraggable (false);

        selectedNode = node;

        listeners.call ([node] (Listener& l) { l.selectedItem (node); });

        if (auto* item = findGuiItem (selectedNode); item && ! item->isRoot ())
            item->setDraggable (true);

        if (parent != nullptr)
            parent->repaint();
    }
}

const juce::ValueTree& MagicGUIBuilder::getSelectedNode() const
{
    return selectedNode;
}

void MagicGUIBuilder::updateSelectedNode() 
{
    if (blockSelectedNodeUpdates)
        return;
    
    listeners.call ([&] (Listener& l) { l.selectedItem (selectedNode); });
}

void MagicGUIBuilder::draggedItemOnto (juce::ValueTree dragged, juce::ValueTree target, juce::Point<int> targetPos, int index)
{
    if (dragged == target)
        return;

    setEditMode (true);
    
    undo.beginNewTransaction();

    if (targetPos.x > 0 && targetPos.y > 0)
    {
        dragged.setProperty (IDs::posX, targetPos.x, &undo);
        dragged.setProperty (IDs::posY, targetPos.y, &undo);
    }
    
    auto targetParent  = target.getParent();
    auto draggedParent = dragged.getParent();

    if (draggedParent.isValid())
        draggedParent.removeChild (dragged, &undo);

    if (target.getType() == IDs::view)
        target.addChild (dragged, index, &undo);
    else
    {
        if (targetParent.isValid() != false && index < 0)
            index = targetParent.indexOf (target);
        
        // update position
        const auto posX = (int)dragged.getProperty (IDs::posX, 0);
        const auto posY = (int)dragged.getProperty (IDs::posY, 0);

        const auto targetX = (int)target.getProperty (IDs::posX, 0);
        const auto targetY = (int)target.getProperty (IDs::posY, 0);

        dragged.setProperty (IDs::posX, posX + targetX, &undo);
        dragged.setProperty (IDs::posY, posY + targetY, &undo);

        targetParent.addChild (dragged, index, &undo);
    }
}

bool MagicGUIBuilder::canNodeBeDeleted (juce::ValueTree node)
{
    return node.getType () != IDs::magic;
}

#if FOLEYS_SHOW_GUI_EDITOR_PALLETTE

void MagicGUIBuilder::attachToolboxToWindow (juce::Component& window)
{
    juce::Component::SafePointer<juce::Component> reference (&window);

    juce::MessageManager::callAsync (
      [&, reference]
      {
          if (reference != nullptr)
          {
              magicToolBox = std::make_unique<ToolBox> (std::make_pair (reference->getTopLevelComponent(), true), *this);
              magicToolBox->setLastLocation (magicState.getResourcesFolder());
          }
      });
}

#endif


}  // namespace foleys
