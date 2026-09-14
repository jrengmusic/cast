namespace jam
{
/*____________________________________________________________________________*/

static constexpr int panelCount { 2 };
static constexpr int sandboxPanelCount { 3 };
static constexpr float uiScaleStep { 0.1f };

void ViewEditor::setAppearance()
{
    ViewManager::setAppearance (this, model.getAppearance());
}

void ViewEditor::setBypassed()
{
    ViewManager::setBypassed (this, model);
}

Size<int> ViewEditor::getUISize (float scale) const noexcept
{
    return Size<int> { getWidth(), getHeight() } * scale;
}

float ViewEditor::getUserAreaScale() const noexcept
{
    constexpr float maxScale { 0.8f };
    const auto bounds { getUISize() };
    return getScreenProportionScale (bounds, maxScale);
}

float ViewEditor::getScreenProportionScale (Size<int> componentSize, float maxScale) const noexcept
{
    const auto& displays { juce::Desktop::getInstance().getDisplays() };
    float result { maxScale };

    if (auto* primaryDisplay { displays.getPrimaryDisplay() })
    {
        const Size<int> screen { primaryDisplay->userBounds.getWidth(), primaryDisplay->userBounds.getHeight() };
        result = Size<int>::getRelativeScale (screen, componentSize, maxScale);
    }

    return result;
}

float ViewEditor::getScaleFactor (const juce::String& val) noexcept
{
    return getScaleFactor (map::UIScaleMap::getInstance()->get (val));
}

float ViewEditor::getScaleFactor (int key) noexcept
{
    return 1.0f - (static_cast<size_t> (map::UIScaleMap::getInstance()->get().size()) - static_cast<size_t> (key)) * uiScaleStep;
}

//==============================================================================
float ViewEditor::getUIScaleFactor (AudioModel& audioModel) const noexcept
{
    return getScaleFactor (audioModel.getUIScale())
           * getUserAreaScale() / audioModel.getDesktopScale();
}

Size<int> ViewEditor::getUISize (AudioModel& audioModel) const noexcept
{
    auto bounds { getUISize (getUIScaleFactor (audioModel)) };
    const auto [width, height] { bounds };
    bounds = Size<int> { width, height + (isAUSandboxHost() ? sandboxPanelCount : panelCount) * audioModel.getUIPanelHeight() };
    return bounds;
}

juce::Rectangle<int> ViewEditor::getViewBounds (AudioModel& audioModel) const noexcept
{
    const auto bounds { getUISize() };
    const int y { juce::roundToInt (audioModel.getUIPanelHeight() / getUIScaleFactor (audioModel)) };
    const auto [width, height] { bounds };
    return { 0, y, width, height };
}

/**_____________________________END_OF_NAMESPACE______________________________*/
} /** namespace jam */
