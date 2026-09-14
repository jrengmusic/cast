namespace jam
{
/*____________________________________________________________________________*/

std::unique_ptr<juce::Button> createAddButton() { return std::make_unique<AddOneButton>(); }

/*____________________________________________________________________________*/

RemovableRow::RemovableRow (juce::StringRef newID)
{
    setComponentID (newID);
    removeButton = std::make_unique<CloseOneButton>();
    removeButton->onClick = [this]
    {
        if (onRemoveButtonClicked)
            onRemoveButtonClicked();
    };

    addAndMakeVisible (removeButton.get());
}

void RemovableRow::resized()
{
    auto area { getLocalBounds().reduced (inset) };
    removeButton->setBounds (area.removeFromLeft (area.getHeight()));

    area.removeFromLeft (inset);
    layoutContentArea (area);
}

void RemovableRow::setInset (int newInset)
{
    inset = newInset;
    resized();
}

void RemovableRow::setRowHeight (int newRowHeight)
{
    rowHeight = newRowHeight;
    resized();
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
