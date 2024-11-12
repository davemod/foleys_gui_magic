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

#pragma once

#include "foleys_GUITreeEditor.h"
#include "foleys_Palette.h"
#include "foleys_PropertiesEditor.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace foleys
{

class MagicGUIBuilder;
class ToolBoxBase : public juce::Component
{
public:
    enum ColourIds {
        backgroundColourId         = 0x90000001,
        outlineColourId            = 0x90000002,
        textColourId               = 0x90000003,
        disabledTextColourId       = 0x90000004,
        removeButtonColourId       = 0x90000005,
        selectedBackgroundColourId = 0x90000006
    };

    ToolBoxBase();
    virtual ~ToolBoxBase () = default;
    virtual void setNodeToEdit (juce::ValueTree node) = 0;
    virtual void stateWasReloaded () = 0;
};

class ToolBoxContentBase
{
public:
    virtual ~ToolBoxContentBase() = default;
    
    virtual void setNodeToEdit (juce::ValueTree node) = 0;
    virtual void setSelectedNode (const juce::ValueTree& node) = 0;
};

/**
 The Toolbox defines a floating window, that allows live editing of the currently loaded GUI.
 */
class ToolBox
  : public ToolBoxBase
  , public juce::DragAndDropContainer
  , public juce::KeyListener
  , private juce::MultiTimer
  , private foleys::MagicGUIBuilder::Listener
{
public:

    /** first: parent to use, second: run in window */
    using Properties = std::pair<juce::WeakReference<juce::Component>, bool>;

    /**
     Create a ToolBox floating window to edit the currently shown GUI.
     The window will float attached to the edited window.

     @param properties a pair with the window to attach to and a boolean to run toolbox in window
     @param builder is the builder instance that manages the GUI
     */
    ToolBox (const Properties& properties, MagicGUIBuilder& builder);
    ~ToolBox() override;

    enum PositionOption
    {
        left,
        right,
        detached
    };

    enum Layout
    {
        StretchableLayout = 0,
        TabbedLayout
    };

    virtual void loadDialog();
    virtual void saveDialog();

    virtual void loadGUI (const juce::File& file);
    virtual bool saveGUI (const juce::File& file);
    
    /** updates the layout to use either tabs or a stretchable layout */
    void setLayout (const Layout& layout);
    Layout getLayout () const;

    void setSelectedNode (const juce::ValueTree& node);
    void setNodeToEdit (juce::ValueTree node) override;

    void setToolboxPosition (PositionOption position);

    bool keyPressed (const juce::KeyPress& key) override;
    bool keyPressed (const juce::KeyPress& key, juce::Component* originalComponent) override;

    void selectedItem (const juce::ValueTree& node) override;

    static juce::PropertiesFile::Options getApplicationPropertyStorage();

protected:
    enum Timers : int
    {
        WindowDrag = 1,
        AutoSave
    };

    static juce::String   positionOptionToString (PositionOption option);
    static PositionOption positionOptionFromString (const juce::String& text);

    static std::unique_ptr<juce::FileFilter> getFileFilter();

    juce::Component::SafePointer<juce::Component> parent;

    MagicGUIBuilder&            builder;
    juce::UndoManager&          undo;
    juce::ApplicationProperties appProperties;

    juce::TextButton fileMenu { TRANS ("File...") };
    juce::TextButton viewMenu { TRANS ("View...") };

    juce::TextButton undoButton { TRANS ("Undo") };

    juce::TextButton editSwitch { TRANS ("Edit") };

    PositionOption positionOption { left };

    Layout layout { StretchableLayout };

    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };

    GUITreeEditor    treeEditor { builder };
    PropertiesEditor propertiesEditor { builder };
    Palette          palette { builder };

    juce::StretchableLayoutManager    resizeManager;
    juce::StretchableLayoutResizerBar resizer1 { &resizeManager, 1, false };
    juce::StretchableLayoutResizerBar resizer3 { &resizeManager, 3, false };

    std::unique_ptr<juce::FileBrowserComponent> fileBrowser;
    juce::File                                  lastLocation;
    juce::File                                  autoSaveFile;

    void                           updateToolboxPosition();
    juce::ResizableCornerComponent resizeCorner { this, nullptr };
    juce::ComponentDragger         componentDragger;

    void updateLayout ();

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

    void stateWasReloaded() override;
    void setLastLocation (juce::File file);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback (int timer) override;

    void guiItemDropped ([[maybe_unused]] const juce::ValueTree& node, [[maybe_unused]] juce::ValueTree& droppedOnto) override { }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToolBox)
};

}  // namespace foleys
