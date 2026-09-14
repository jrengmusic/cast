namespace jam
{
/*____________________________________________________________________________*/

void ViewManager::applyLayout (juce::Component* view, float emUnit)
{
    if (view->getProperties().contains (Id::body))
    {
        if (jam::Component::hasProperty (view->getProperties(), Id::display, Id::flex))
        {
            applyFlexLayout (view, emUnit);
        }
        else
        {
            auto applyChildLayout = [&emUnit] (auto& self, juce::Component* parent) -> void
            {
                for (auto* child : parent->getChildren())
                {
                    auto& properties { child->getProperties() };

                    const auto isFlex { jam::Component::hasProperty (properties, Id::display, Id::flex) };

                    const auto isAbsolute { jam::Component::hasProperty (properties, Id::position, Id::absolute) };

                    if (isFlex)
                        applyFlexLayout (child, emUnit);
                    else if (not isAbsolute)
                        self (self, child);
                }
            };

            applyChildLayout (applyChildLayout, view);
        }
    }
}

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
