/*
 * (c) Credland Technical Limited.
 * MIT License
 *
 * JCF_DEBUG - Debugging helpers for JUCE.  Demo application.
 *
 * Don't forget to install the VisualStudio or Xcode debug scripts as
 * well.  These ensure that your IDEs debugger displays the contents
 * of ValueTrees, Strings and Arrays in a useful way!
 *
 *
 * Credland Technical Limited provide a range of consultancy and contract
 * services including:
 * - JUCE software development and support
 * - information security consultancy
 *
 * Contact via http://www.credland.net/
 */

/**
 * @file jam_DebugModelMonitor.h
 * @brief jam::DebugModelMonitor — desktop window for viewing/editing a ValueTree's property fields.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Display a separate desktop window for viewing and editing a value
 *        tree's property fields.
 *
 * Instantiate a DebugModelMonitor instance, then call
 * DebugModelMonitor::setSource(ValueTree&) and it'll display your tree.
 *
 * For example:
 * @code
 * monitor = new DebugModelMonitor();
 * monitor->setSource(myTree);
 * @endcode
 *
 * @note This code isn't pretty - it's for debugging, not production use!
 */
class DebugModelMonitor : public jam::Window
{
public:
    /** @brief Constructs the monitor window with no ValueTree attached yet. */
    DebugModelMonitor()
        : jam::Window (
              []
              {
                  auto desktop { juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userBounds.toNearestInt() };
                  auto component { std::make_unique<ModelMonitorComponent>() };
                  component->setSize (desktop.getWidth() / 3, desktop.getHeight() * 2 / 3);
                  return component.release();
              }(),
              "Model Monitor",
              true,
              false)
    {
        setLookAndFeel (&jam::StyleDebug::getShared());
        setVisible (true);
        setWindowButtons (false);
    }

    /**
     * @brief Constructs the monitor window and immediately shows @p tree.
     * @param tree  The ValueTree to display.
     */
    explicit DebugModelMonitor (juce::ValueTree& tree)
        : DebugModelMonitor()
    {
        setSource (tree);
    }

    /** @brief Detaches the displayed ValueTree before destruction. */
    ~DebugModelMonitor() override { main().setTree (juce::ValueTree()); }

    /**
     * @brief Shows a particular ValueTree in the editor.
     *
     * If you attach all the ValueTrees in your program to a common root,
     * you'll be able to view the whole thing in one editor.
     *
     * @param treeToShow  The ValueTree to display.
     */
    void setSource (juce::ValueTree& treeToShow) { main().setTree (treeToShow); }

    /**
     * @brief Installs per-tag, per-property display formatters from @p validators.
     * @tparam ValidatorsType  Nested-map validators container type.
     * @param  validators      Validators whose non-empty `format` callbacks are installed.
     */
    template<typename ValidatorsType>
    void setFormats (const ValidatorsType& validators)
    {
        auto& mainComponent { main() };

        for (const auto& [tag, group] : validators)
        {
            for (const auto& [property, validator] : group)
            {
                if (validator.format)
                {
                    auto [tagEntry, inserted] = mainComponent.formats.try_emplace (tag);
                    auto& [tagKey, tagFormats] = *tagEntry;
                    tagFormats.addOrReplace (property, validator.format);
                }
            }
        }
    }

private:
    //==============================================================================

    /** @brief Content component pairing a TreeView of ValueTree nodes with a PropertyEditor panel. */
    class ModelMonitorComponent : public juce::Component
    {
    public:
        /** @brief Property panel showing the selected ValueTree node's properties, read-only. */
        class PropertyEditor : public juce::PropertyPanel
        {
        public:
            /** @brief Constructs an empty PropertyEditor. */
            PropertyEditor();
            /** @brief Rebuilds the displayed property rows from @p newSource. */
            void setSource (juce::ValueTree& newSource);

        private:
            juce::Value noEditValue;
            juce::ValueTree tree;

            //==============================================================================
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PropertyEditor)
        };

        //==============================================================================

        /** @brief TreeViewItem mirroring one ValueTree node, live-updating via ValueTree::Listener. */
        class Item
            : public juce::TreeViewItem
            , public juce::ValueTree::Listener
        {
        public:
            /**
             * @brief Constructs an Item wrapping @p tree, using @p formats for display formatting.
             * @param tree     The ValueTree node this item represents.
             * @param formats  Per-tag, per-property display formatters (referenced, not copied).
             */
            Item (juce::ValueTree tree,
                  const jam::HashMap<
                      juce::Identifier,
                      jam::HashMap<juce::Identifier, std::function<juce::String (const juce::var&)>>>& formats);
            ~Item();
            bool mightContainSubItems();
            void itemOpennessChanged (bool isNowOpen);
            void updateSubItems();
            void paintItem (juce::Graphics& g, int w, int h);
            void itemSelectionChanged (bool isNowSelected);
            /* Enormous list of ValueTree::Listener options... */
            void
            valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property);
            void valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded);
            void valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int);
            void valueTreeChildOrderChanged (juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int, int);
            void valueTreeParentChanged (juce::ValueTree& treeWhoseParentHasChanged);
            void valueTreeRedirected (juce::ValueTree& treeWhichHasBeenChanged);
            /* Works only if the ValueTree isn't updated between calls to getUniqueName. */
            juce::String getUniqueName() const;

        private:
            juce::ValueTree t;
            juce::Array<juce::Identifier> currentProperties;
            const jam::HashMap<juce::Identifier,
                               jam::HashMap<juce::Identifier, std::function<juce::String (const juce::var&)>>>&
                formats;
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Item)
        };

        //==============================================================================

        ModelMonitorComponent();
        ~ModelMonitorComponent() override;
        void resized() override;
        void setTree (juce::ValueTree newTree);

    public:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModelMonitorComponent)

        std::unique_ptr<Item> rootItem;
        juce::ValueTree tree;
        juce::TreeView treeView;
        PropertyEditor propertyEditor;
        juce::StretchableLayoutManager layout;
        juce::StretchableLayoutResizerBar layoutResizer;
        jam::HashMap<juce::Identifier,
                     jam::HashMap<juce::Identifier, std::function<juce::String (const juce::var&)>>>
            formats;
    };

    ModelMonitorComponent& main() { return *dynamic_cast<ModelMonitorComponent*> (getContentComponent()); }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DebugModelMonitor)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
