namespace jam
{
/*____________________________________________________________________________*/
DebugModelMonitor::ModelMonitorComponent::ModelMonitorComponent()
    : layoutResizer (&layout, 1, false)
{
    layout.setItemLayout (0, -0.1, -0.9, -0.6);
    layout.setItemLayout (1, 5, 5, 5);
    layout.setItemLayout (2, -0.1, -0.9, -0.1);

    setSize (600, 600);
    addAndMakeVisible (treeView);
    addAndMakeVisible (propertyEditor);
    addAndMakeVisible (layoutResizer);

    treeView.setDefaultOpenness (true);
}

DebugModelMonitor::ModelMonitorComponent::~ModelMonitorComponent() { treeView.setRootItem (nullptr); }

static constexpr int contentInset { 10 };

void DebugModelMonitor::ModelMonitorComponent::resized()
{
    auto const bounds { getLocalBounds().reduced (contentInset) };
    juce::Component* comps[] = { &treeView, &layoutResizer, &propertyEditor };
    layout.layOutComponents (comps, 3, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), true, true);
}

void DebugModelMonitor::ModelMonitorComponent::setTree (juce::ValueTree newTree)
{
    if (not newTree.isValid())
    {
        treeView.setRootItem (nullptr);
    }
    else if (tree != newTree)
    {
        tree = newTree;
        rootItem = std::make_unique<Item> (tree, formats);
        treeView.setRootItem (rootItem.get());
    }
}

/*____________________________________________________________________________*/

DebugModelMonitor::ModelMonitorComponent::PropertyEditor::PropertyEditor() { noEditValue = "not editable"; }

void DebugModelMonitor::ModelMonitorComponent::PropertyEditor::setSource (juce::ValueTree& newSource)
{
    clear();

    tree = newSource;

    const int maxChars { 200 };

    juce::Array<juce::PropertyComponent*> pc;

    for (int i = 0; i < tree.getNumProperties(); ++i)
    {
        const juce::Identifier name = tree.getPropertyName (i).toString();
        juce::Value v = tree.getPropertyAsValue (name, nullptr);
        juce::TextPropertyComponent* tpc;

        if (v.getValue().isObject())
        {
            tpc = new juce::TextPropertyComponent (noEditValue, name.toString(), maxChars, false);
            tpc->setEnabled (false);
        }
        else
        {
            tpc = new juce::TextPropertyComponent (v, name.toString(), maxChars, false);
        }

        pc.add (tpc);
    }

    addProperties (pc);
}

/*____________________________________________________________________________*/

DebugModelMonitor::ModelMonitorComponent::Item::Item (
    juce::ValueTree tree,
    const jam::HashMap<juce::Identifier,
                       jam::HashMap<juce::Identifier, std::function<juce::String (const juce::var&)>>>& formats)
    : t (tree)
    , formats (formats)
{
    t.addListener (this);
}

DebugModelMonitor::ModelMonitorComponent::Item::~Item() { clearSubItems(); }

bool DebugModelMonitor::ModelMonitorComponent::Item::mightContainSubItems() { return t.getNumChildren() > 0; }

void DebugModelMonitor::ModelMonitorComponent::Item::itemOpennessChanged (bool isNowOpen)
{
    if (isNowOpen)
        updateSubItems();
}

void DebugModelMonitor::ModelMonitorComponent::Item::updateSubItems()
{
    std::unique_ptr<juce::XmlElement> opennessState = getOpennessState();
    clearSubItems();
    int children = t.getNumChildren();

    for (int i = 0; i < children; ++i)
        addSubItem (new Item (t.getChild (i), formats));

    if (opennessState.get() != nullptr)
        restoreOpennessState (*opennessState.get());
}

void DebugModelMonitor::ModelMonitorComponent::Item::paintItem (juce::Graphics& g, int w, int h)
{
    auto& laf = getOwnerView()->getLookAndFeel();
    juce::FontOptions const font { juce::FontOptions (12.0f).withKerningFactor (0.05f) };

    const float padding { 20.0f };

    const juce::String typeName = t.getType().toString();
    const float nameWidth = juce::TextLayout::getStringWidth (font, typeName);
    const float propertyX = padding + nameWidth;

    g.setFont (font);
    g.setColour (laf.findColour (juce::PropertyComponent::labelTextColourId));
    g.drawText (typeName, 0, 0, w, h, juce::Justification::left, false);

    juce::AttributedString property;

    const juce::Colour idColour { laf.findColour (jam::StyleDebug::propertyIdentifierColourId) };
    const juce::Colour numberColour { laf.findColour (jam::StyleDebug::propertyNumberColourId) };
    const juce::Colour stringColour { laf.findColour (jam::StyleDebug::propertyStringColourId) };
    const juce::Colour eqColour { laf.findColour (juce::PropertyComponent::labelTextColourId) };

    for (int i = 0; i < t.getNumProperties(); ++i)
    {
        const juce::Identifier name = t.getPropertyName (i);
        juce::String propertyValue = t.getProperty (name).toString();

        if (formats.contains (t.getType()) and formats.at (t.getType()).contains (name))
        {
            const auto& typeFormats = formats.at (t.getType());
            propertyValue = typeFormats.at (name) (t.getProperty (name));
        }
        else if (name == Id::bounds)
        {
            propertyValue = jam::Bounds { t.getProperty (name) }.toRectangle().toString();
        }

        // property name
        property.append (" " + name.toString(), idColour);

        // equals sign
        property.append ("=", eqColour);

        // decide value colour
        juce::Colour valueColour { stringColour };
        {
            auto trimmed = propertyValue.trim();
            bool hasDigit = trimmed.containsAnyOf ("0123456789");
            bool numericChars = trimmed.removeCharacters ("0123456789.-+eE").isEmpty();
            if (hasDigit and numericChars)
                valueColour = numberColour;
        }

        property.append (propertyValue, valueColour);
    }

    juce::Rectangle<float> propArea { propertyX, 0.0f, static_cast<float> (w - propertyX), static_cast<float> (h) };
    property.setFont (font);
    property.draw (g, propArea);
}

void DebugModelMonitor::ModelMonitorComponent::Item::itemSelectionChanged (bool isNowSelected)
{
    if (isNowSelected)
    {
        t.removeListener (this);
        static_cast<ModelMonitorComponent*> (getOwnerView()->getParentComponent())->propertyEditor.setSource (t);
        t.addListener (this);
    }
}

/* Enormous list of ValueTree::Listener options... */
void DebugModelMonitor::ModelMonitorComponent::Item::valueTreePropertyChanged (
    juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property)
{
#if JUCE_MODULE_AVAILABLE_juce_audio_processors
    const juce::MessageManagerLock mmLock;
#endif

    if (t == treeWhosePropertyHasChanged)
    {
        t.removeListener (this);
        repaintItem();
        t.addListener (this);
    }
}

void DebugModelMonitor::ModelMonitorComponent::Item::valueTreeChildAdded (juce::ValueTree& parentTree,
                                                                          juce::ValueTree& childWhichHasBeenAdded)
{
    if (parentTree == t)
        updateSubItems();

    treeHasChanged();
}

void DebugModelMonitor::ModelMonitorComponent::Item::valueTreeChildRemoved (juce::ValueTree& parentTree,
                                                                            juce::ValueTree& childWhichHasBeenRemoved,
                                                                            int)
{
    if (parentTree == t)
        updateSubItems();

    treeHasChanged();
}
void DebugModelMonitor::ModelMonitorComponent::Item::valueTreeChildOrderChanged (
    juce::ValueTree& parentTreeWhoseChildrenHaveMoved,
    int,
    int)
{
    if (parentTreeWhoseChildrenHaveMoved == t)
        updateSubItems();

    treeHasChanged();
}
void DebugModelMonitor::ModelMonitorComponent::Item::valueTreeParentChanged (juce::ValueTree& treeWhoseParentHasChanged)
{
    treeHasChanged();
}
void DebugModelMonitor::ModelMonitorComponent::Item::valueTreeRedirected (juce::ValueTree& treeWhichHasBeenChanged)
{
    if (treeWhichHasBeenChanged == t)
        updateSubItems();

    treeHasChanged();
}

/* Works only if the ValueTree isn't updated between calls to getUniqueName. */
juce::String DebugModelMonitor::ModelMonitorComponent::Item::getUniqueName() const
{
    if (not t.getParent().isValid())
        return "1";

    return juce::String (t.getParent().indexOf (t));
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
