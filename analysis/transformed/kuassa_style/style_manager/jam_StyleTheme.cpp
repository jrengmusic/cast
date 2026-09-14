namespace jam
{
/*____________________________________________________________________________*/

StyleTheme::StyleTheme (StyleManager& styleManager, juce::StringRef defaultAppearance)
    : style (styleManager)
{
    juce::LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypeface (getCommonFont().getTypeface());

    setAppearance (defaultAppearance);
}

void StyleTheme::setAppearance (juce::StringRef lightOrDark)
{
    style.setAppearance (*this, lightOrDark);
}

void StyleTheme::setFontRasterization()
{
#if JUCE_MODULE_AVAILABLE_jam_vulkan
    // Hard default — ALWAYS applied, regardless of the StyleSheet override
    // below — so this glyph atlas never rasterizes through
    // Id::FontRasterizerBackend::edgeTable's thin aliased identity default
    // (jam_GlyphAtlas.h's own "used before any setRasterization() call"
    // doc comment on its `backend` member).
    auto* atlas { jam::GlyphAtlas::getInstance() };
    jassert (atlas != nullptr);

    atlas->setRasterization (map::FontRasterizerBackend::freetype, defaultFontGamma, defaultFontContrast);

    // StyleSheet override — the first @font-face record's rasterizer/gamma/
    // contrast custom properties, read through hasFontsStyle()/
    // getFontsStyleString() (jam_StyleManager.h). rasterizer absent
    // (StyleSheet silent) leaves the hard default above untouched — never
    // overwritten with a partially-resolved value.

    if (style.hasFontsStyle (Id::rasterizer))
    {
        auto* backends { map::FontRasterizerBackend::getInstance() };
        const auto backendName { style.getFontsStyleString (Id::rasterizer) };

        // Per-key independence — gamma/contrast each fall back to THIS atlas's
        // own hard default independently of one another (either may be
        // present), gated individually through hasFontsStyle() before the
        // getter is called.
        const auto newGamma { style.hasFontsStyle (Id::gamma) ? style.getFontsStyle<float> (Id::gamma)
                                                               : defaultFontGamma };
        const auto newContrast { style.hasFontsStyle (Id::contrast)
                                     ? style.getFontsStyle<float> (Id::contrast)
                                     : defaultFontContrast };

        atlas->setRasterization (backends->contains (backendName)
                                     ? static_cast<map::FontRasterizerBackend::value> (backends->get (backendName))
                                     : map::FontRasterizerBackend::freetype,
                                 newGamma,
                                 newContrast);
    }
#endif
}

void StyleTheme::setEmbolden()
{
#if JUCE_MODULE_AVAILABLE_jam_vulkan
    auto* atlas { jam::GlyphAtlas::getInstance() };
    jassert (atlas != nullptr);

    // embolden is a boolean STRING attribute ("true"/"false"), never an int —
    // matched against jam::Format::fromBoolean(true)'s own "true" spelling
    // case-insensitively; anything else (including "1") resolves to false.
    // Absent key keeps GlyphAtlas::embolden's own hard default (false,
    // jam_GlyphAtlas.h).
    const auto newEmbolden { style.hasFontsStyle (Id::embolden)
                                 ? style.getFontsStyleString (Id::embolden).equalsIgnoreCase (jam::Format::fromBoolean (true))
                                 : false };

    atlas->setEmbolden (newEmbolden);
#endif
}
//==============================================================================
const juce::FontOptions StyleTheme::getCommonFont()
{
    return style.getFont (Id::common);
}

float StyleTheme::getWindowOpacity() const noexcept
{
    return style.getWindowStyle<float> (Id::opacity);
}

float StyleTheme::getWindowBlur() const noexcept
{
    return style.getWindowStyle<float> (Id::blur);
}

int StyleTheme::getWindowPadding() const noexcept
{
    return style.getWindowStyle<int> (Id::padding);
}

int StyleTheme::getWindowClose() const noexcept
{
    return style.getWindowStyle<int> (Id::close);
}

juce::String StyleTheme::getWindowBackendString() const noexcept
{
#if JUCE_MAC
    return style.getWindowStyleString (Id::mac);
#elif JUCE_WINDOWS
    return style.getWindowStyleString (Id::win);
#else
    return {};
#endif
}

juce::Font StyleTheme::getComboBoxFont (juce::ComboBox&)
{
    return getCommonFont();
}

juce::Font StyleTheme::getPopupMenuFont()
{
    return getCommonFont();
}

void StyleTheme::getIdealPopupMenuItemSize (const juce::String& text,
                                            bool isSeparator,
                                            int standardMenuItemHeight,
                                            int& idealWidth,
                                            int& idealHeight)
{
    juce::LookAndFeel_V4::getIdealPopupMenuItemSize (
        text, isSeparator, standardMenuItemHeight, idealWidth, idealHeight);

    static constexpr float menuItemHeightScale { 1.8f };
    if (not isSeparator)
        idealHeight = juce::jmax (idealHeight, juce::roundToInt (getPopupMenuFont().getHeight() * menuItemHeightScale));
}

juce::Font StyleTheme::getTextButtonFont (juce::TextButton&, int)
{
    return getCommonFont();
}

juce::Font StyleTheme::getLabelFont (juce::Label& label)
{
    if (label.getProperties().contains (Id::content) or label.getProperties().contains (Id::fontFamily))
        return label.getFont();

    return getCommonFont();
}

juce::BorderSize<int> StyleTheme::getLabelBorderSize (juce::Label& label)
{
    if (label.getProperties().contains (Id::content))
        return { 0, 0, 0, 0 };

    return label.getBorderSize();
}

juce::Font
StyleTheme::getMenuBarFont (juce::MenuBarComponent& menuBar, int itemIndex, const juce::String& itemText)
{
    return getCommonFont();
}

//==============================================================================
juce::Path StyleTheme::getTickShape (float height)
{
    const char* const right { "M19 12L31 24L19 36" };
    juce::Path path { juce::Drawable::parseSVGPath (right) };
    path.scaleToFit (0, 0, height * 2.0f, height, true);

    return path;
}

void StyleTheme::drawPopupMenuSectionHeader (juce::Graphics& g,
                                             const juce::Rectangle<int>& area,
                                             const juce::String& sectionName)
{
    auto r = area.reduced (style.getMetrics<int> (Id::inset));
    juce::Font font { style.getFont (Id::bold) };
    g.setFont (font);
    g.setColour (findColour (juce::PopupMenu::headerTextColourId));

    g.drawFittedText (sectionName, r.getX(), r.getY(), r.getWidth(), r.getHeight(), juce::Justification::centred, 1);
}

void StyleTheme::drawDocumentWindowTitleBar (juce::DocumentWindow&,
                                             juce::Graphics&,
                                             int,
                                             int,
                                             int,
                                             int,
                                             const juce::Image*,
                                             bool)
{
}

juce::Button* StyleTheme::createDocumentWindowButton (int buttonType)
{
#if JUCE_MODULE_AVAILABLE_jam_gui
    switch (buttonType)
    {
        case juce::DocumentWindow::minimiseButton:
            return std::make_unique<jam::ButtonSVG> ("windowMinimize").release();

        case juce::DocumentWindow::maximiseButton:
            return std::make_unique<jam::ButtonSVG> ("windowMaximise").release();

        case juce::DocumentWindow::closeButton:
            return std::make_unique<jam::ButtonSVG> ("windowClose").release();
    }

    jassertfalse;
    return nullptr;
#else
    return LookAndFeel_V4::createDocumentWindowButton (buttonType);
#endif
}

void StyleTheme::positionDocumentWindowButtons (juce::DocumentWindow&,
                                                int titleBarX,
                                                int titleBarY,
                                                int titleBarW,
                                                int titleBarH,
                                                juce::Button* minimiseButton,
                                                juce::Button* maximiseButton,
                                                juce::Button* closeButton,
                                                bool positionTitleBarButtonsOnLeft)
{
    constexpr int buttonInset { 2 };

    const int buttonW = titleBarH - titleBarH / 8;

    int x = positionTitleBarButtonsOnLeft ? titleBarX + 4 : titleBarX + titleBarW - buttonW - buttonW / 4;

    if (closeButton != nullptr)
    {
        closeButton->setBounds (juce::Rectangle<int> (x, titleBarY, buttonW, titleBarH).reduced (buttonInset));
        x += positionTitleBarButtonsOnLeft ? buttonW : -(buttonW + buttonW / 4);
    }

    if (positionTitleBarButtonsOnLeft)
        std::swap (minimiseButton, maximiseButton);

    if (maximiseButton != nullptr)
    {
        maximiseButton->setBounds (juce::Rectangle<int> (x, titleBarY, buttonW, titleBarH).reduced (buttonInset));
        x += positionTitleBarButtonsOnLeft ? buttonW : -buttonW;
    }

    if (minimiseButton != nullptr)
        minimiseButton->setBounds (juce::Rectangle<int> (x, titleBarY, buttonW, titleBarH).reduced (buttonInset));
}

void StyleTheme::drawPopupMenuBackgroundWithOptions (juce::Graphics& g,
                                                     int width,
                                                     int height,
                                                     const juce::PopupMenu::Options&)
{
#if JUCE_WINDOWS && JUCE_MODULE_AVAILABLE_jam_gui
    if (not jam::BackgroundBlur::isEnabled())
        g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
#elif JUCE_WINDOWS
    juce::ignoreUnused (width, height);
    g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
#else
    juce::ignoreUnused (g, width, height);
#endif
}

void StyleTheme::drawPopupMenuSectionHeaderWithOptions (juce::Graphics& g,
                                                        const juce::Rectangle<int>& area,
                                                        const juce::String& sectionName,
                                                        const juce::PopupMenu::Options&)
{
    drawPopupMenuSectionHeader (g, area, sectionName);
}

void StyleTheme::preparePopupMenuWindow (juce::Component& newWindow)
{
#if JUCE_MODULE_AVAILABLE_jam_gui
    newWindow.setOpaque (false);

    auto safeComponent { juce::Component::SafePointer<juce::Component> (&newWindow) };

    juce::MessageManager::callAsync (
        [this, safeComponent]
        {
            if (safeComponent != nullptr)
            {
                const auto opacity { getWindowOpacity() };
                const auto baseColour {
                    safeComponent->findColour (juce::PopupMenu::backgroundColourId).withAlpha (opacity)
                };
                const auto blur { getWindowBlur() };

                jam::StyleWindow::setMenu (safeComponent.getComponent(), baseColour);
                const auto backend { jam::BackgroundBlur::fromString (getWindowBackendString()) };
                jam::BackgroundBlur::enable (safeComponent.getComponent(), backend, blur, baseColour);
            }
        });
#else
    juce::LookAndFeel_V4::preparePopupMenuWindow (newWindow);
#endif
}

void StyleTheme::prepareWindow (juce::Component& window)
{
#if JUCE_MODULE_AVAILABLE_jam_gui
    const auto opacity { getWindowOpacity() };
    const auto blur { getWindowBlur() };
    const auto colour { this->LookAndFeel::findColour (juce::ResizableWindow::backgroundColourId).withAlpha (opacity) };
    const auto backend { jam::BackgroundBlur::fromString (getWindowBackendString()) };

    auto* jamWindow { dynamic_cast<jam::Window*> (&window) };
    jassert (jamWindow != nullptr);

    jamWindow->setStyle (colour, blur, backend);
#else
    juce::ignoreUnused (window);
#endif
}

int StyleTheme::getMenuWindowFlags()
{
#if JUCE_WINDOWS
    return 0;
#else
    return LookAndFeel_V4::getMenuWindowFlags();
#endif
}

void StyleTheme::drawMenuSeparator (juce::Graphics& g, const juce::Rectangle<int>& area)
{
    auto r = area.reduced (5, 0);
    r.removeFromTop (juce::roundToInt (((float) r.getHeight() * 0.5f) - 0.5f));

    g.setColour (findColour (juce::PopupMenu::textColourId).withAlpha (0.3f));
    g.fillRect (r.removeFromTop (1));
}

void StyleTheme::drawPopupMenuItem (juce::Graphics& g,
                                    const juce::Rectangle<int>& area,
                                    const bool isSeparator,
                                    const bool isActive,
                                    const bool isHighlighted,
                                    const bool isTicked,
                                    const bool hasSubMenu,
                                    const juce::String& text,
                                    const juce::String& shortcutKeyText,
                                    const juce::Drawable* icon,
                                    const juce::Colour* const textColourToUse)
{
    if (isSeparator)
    {
        drawMenuSeparator (g, area);
    }
    else
    {
        auto textColour = (textColourToUse == nullptr ? findColour (juce::PopupMenu::textColourId) : *textColourToUse);

        auto r = area.reduced (1);

        if (isHighlighted and isActive)
        {
            g.setColour (findColour (juce::PopupMenu::highlightedBackgroundColourId));
            g.fillRect (r);

            g.setColour (findColour (juce::PopupMenu::highlightedTextColourId));
        }
        else
        {
            g.setColour (textColour.withMultipliedAlpha (isActive ? 1.0f : 0.5f));

            if (isTicked)
            {
                g.setColour (findColour (juce::PopupMenu::headerTextColourId));
            }
        }

        r.reduce (juce::jmin (5, area.getWidth() / 12), 0);

        auto font = getPopupMenuFont();

        auto maxFontHeight = (float) r.getHeight();

        g.setFont (font);

        auto iconArea = r.removeFromLeft (juce::roundToInt (maxFontHeight)).toFloat();

        if (icon != nullptr)
        {
            icon->drawWithin (
                g, iconArea, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
            r.removeFromLeft (juce::roundToInt (maxFontHeight * 0.5f));
        }
        else if (isTicked)
        {
            auto tick { getTickShape (1.0f) };
            auto stroke { juce::PathStrokeType (2.0f) };
            auto delta { iconArea.getWidth() / 3 };
            g.strokePath (tick, stroke, tick.getTransformToScaleToFit (iconArea.reduced (delta).toFloat(), true));
        }

        if (hasSubMenu)
        {
            auto arrowH = 0.6f * getPopupMenuFont().getAscent();

            auto x = static_cast<float> (r.removeFromRight ((int) arrowH).getX());
            auto halfH = static_cast<float> (r.getCentreY());

            juce::Path path;
            path.startNewSubPath (x, halfH - arrowH * 0.5f);
            path.lineTo (x + arrowH * 0.6f, halfH);
            path.lineTo (x, halfH + arrowH * 0.5f);

            g.strokePath (path, juce::PathStrokeType (2.0f));
        }

        r.removeFromRight (3);
        g.drawFittedText (text, r, juce::Justification::centredLeft, 1);

        if (shortcutKeyText.isNotEmpty())
        {
            auto f2 = font.withPointHeight (font.getHeightInPoints() * 0.75f);
            f2.setHorizontalScale (0.95f);
            g.setFont (f2);

            g.drawText (shortcutKeyText, r, juce::Justification::centredRight, true);
        }
    }
}

void StyleTheme::drawComboBox (juce::Graphics& g,
                               int width,
                               int height,
                               bool isButtonDown,
                               int buttonX,
                               int buttonY,
                               int buttonW,
                               int buttonH,
                               juce::ComboBox& box)
{
#if JUCE_MODULE_AVAILABLE_jam_gui
    auto cornerSize = box.findParentComponentOfClass<juce::ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
    juce::Rectangle<int> boxBounds (0, 0, width, height);

    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);

    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (boxBounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);

    const auto& arrowPath { box.isPopupActive() ? jam::SelectorArrows::open() : jam::SelectorArrows::closed() };
    auto inset { box.isPopupActive() ? 4 : 5 };
    auto offset { box.isPopupActive() ? -4 : 0 };
    auto arrowArea { boxBounds.removeFromRight (height).translated (offset, 0).reduced (inset).toFloat() };
    juce::Path p { juce::Drawable::parseSVGPath (arrowPath) };
    auto stroke { juce::PathStrokeType (1.5f) };
    float alpha { (box.isEnabled() ? 0.9f : 0.2f) * (box.isMouseOver() ? 1.0f : 0.5f) };
    g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha (alpha));
    g.strokePath (p, stroke, p.getTransformToScaleToFit (arrowArea, true));
#else
    juce::ignoreUnused (isButtonDown, buttonX, buttonY, buttonW, buttonH);
    juce::LookAndFeel_V4::drawComboBox (g, width, height, false, 0, 0, 0, 0, box);
#endif
}

void StyleTheme::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.fillAll (label.findColour (juce::Label::backgroundColourId));

    if (not label.isBeingEdited())
    {
        auto alpha { label.isEnabled() ? 1.0f : 0.75f };
        const juce::Font font { getLabelFont (label) };

        g.setColour (label.findColour (juce::Label::textColourId).withMultipliedAlpha (alpha));
        g.setFont (font);

        auto textArea { getLabelBorderSize (label).subtractedFrom (label.getLocalBounds()) };

        if (label.getText().containsChar (Chars::newline))
            g.drawFittedText (
                label.getText(), textArea, label.getJustificationType(), textArea.getHeight() / static_cast<int> (font.getHeight()));
        else
            g.drawText (label.getText(), textArea, label.getJustificationType());
        g.setColour (label.findColour (juce::Label::outlineColourId).withMultipliedAlpha (alpha));
    }
    else if (label.isEnabled())
    {
        g.setColour (label.findColour (juce::Label::outlineColourId));
    }

    g.drawRect (label.getLocalBounds());
}

void StyleTheme::drawComboBoxTextWhenNothingSelected (juce::Graphics& g,
                                                       juce::ComboBox& box,
                                                       juce::Label& label)
{
    g.setColour (findColour (juce::ComboBox::textColourId).withMultipliedAlpha (0.5f));

    auto font = label.getLookAndFeel().getLabelFont (label);

    g.setFont (font);

    auto textArea = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds()).reduced (label.getHeight(), 0);

    g.drawText (box.getTextWhenNothingSelected(), textArea, label.getJustificationType());
}

void StyleTheme::fillTextEditorBackground (juce::Graphics& g,
                                           int width,
                                           int height,
                                           juce::TextEditor& textEditor)
{
    juce::Rectangle<int> boxBounds { 0, 0, width, height };
    float lineThickness { 1.0f };
    float cornerSize { 3.0f };
    boxBounds.reduce (lineThickness, lineThickness);

    if (dynamic_cast<juce::AlertWindow*> (textEditor.getParentComponent()) != nullptr)
    {
        g.setColour (textEditor.findColour (juce::TextEditor::backgroundColourId));
        g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);

        g.setColour (textEditor.findColour (juce::TextEditor::outlineColourId));
        g.drawHorizontalLine (height - 1, 0.0f, static_cast<float> (width));
    }
    else
    {
        g.setColour (textEditor.findColour (juce::TextEditor::backgroundColourId));
        g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);
    }
}

void StyleTheme::drawTextEditorOutline (juce::Graphics& g,
                                        int width,
                                        int height,
                                        juce::TextEditor& textEditor)
{
    juce::Rectangle<int> boxBounds { 0, 0, width, height };
    float cornerSize { 3.0f };
    float lineThickness { 1.0f };
    boxBounds.reduce (lineThickness, lineThickness);

    if (dynamic_cast<juce::AlertWindow*> (textEditor.getParentComponent()) == nullptr)
    {
        if (textEditor.isEnabled())
        {
            if (textEditor.hasKeyboardFocus (true) and not textEditor.isReadOnly())
            {
                g.setColour (textEditor.findColour (juce::TextEditor::focusedOutlineColourId));
                g.drawRoundedRectangle (boxBounds.toFloat(), cornerSize, lineThickness);
            }
            else
            {
                g.setColour (textEditor.findColour (juce::TextEditor::outlineColourId));
                g.drawRoundedRectangle (boxBounds.toFloat(), cornerSize, lineThickness);
            }
        }
    }
}

void StyleTheme::drawToggleButton (juce::Graphics& g,
                                   juce::ToggleButton& button,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown)
{
    auto tickWidth = juce::jmin (15.0f, (float) button.getHeight() * 0.75f) * 1.1f;

    drawTickBox (g,
                 button,
                 4.0f,
                 ((float) button.getHeight() - tickWidth) * 0.5f,
                 tickWidth,
                 tickWidth,
                 button.getToggleState(),
                 button.isEnabled(),
                 shouldDrawButtonAsHighlighted,
                 shouldDrawButtonAsDown);

    g.setColour (button.findColour (juce::ToggleButton::textColourId));
    g.setFont (getCommonFont());

    if (not button.isEnabled())
        g.setOpacity (0.5f);

    g.drawFittedText (button.getButtonText(),
                      button.getLocalBounds().withTrimmedLeft (jam::toInt (tickWidth) + 10).withTrimmedRight (2),
                      juce::Justification::centredLeft,
                      10);
}

juce::Path StyleTheme::getConnectedEdgePath (const juce::Rectangle<float>& bounds,
                                             float cornerSize,
                                             bool flatOnLeft,
                                             bool flatOnRight,
                                             bool flatOnTop,
                                             bool flatOnBottom)
{
    juce::Path path;
    path.addRoundedRectangle (bounds.getX(),
                              bounds.getY(),
                              bounds.getWidth(),
                              bounds.getHeight(),
                              cornerSize,
                              cornerSize,
                              not(flatOnLeft or flatOnTop),
                              not(flatOnRight or flatOnTop),
                              not(flatOnLeft or flatOnBottom),
                              not(flatOnRight or flatOnBottom));

    return path;
}

void StyleTheme::drawButtonBackground (juce::Graphics& g,
                                       juce::Button& button,
                                       const juce::Colour& backgroundColour,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown)
{
    if (not button.getProperties().contains (Id::groupButton))
    {
        float cornerSize { 3.0f };
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f, 0.5f);

        auto baseColour = backgroundColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
                              .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);

        if (shouldDrawButtonAsDown or shouldDrawButtonAsHighlighted)
            baseColour = baseColour.contrasting (shouldDrawButtonAsDown ? 0.2f : 0.05f);

        const bool flatOnLeft = button.isConnectedOnLeft();
        const bool flatOnRight = button.isConnectedOnRight();
        const bool flatOnTop = button.isConnectedOnTop();
        const bool flatOnBottom = button.isConnectedOnBottom();

        // choose the drawing area (slightly different for standalone vs connected)
        auto area = (flatOnLeft or flatOnRight or flatOnTop or flatOnBottom)
                        ? bounds
                        : button.getLocalBounds().toFloat().reduced (0.5f);

        // build gradient once
        juce::Colour base = baseColour;
        if (shouldDrawButtonAsDown)
            base = base.darker (0.2f);
        else if (shouldDrawButtonAsHighlighted)
            base = base.brighter (0.1f);

        juce::ColourGradient grad (
            base, area.getCentreX(), area.getY(), base, area.getCentreX(), area.getBottom(), false);

        grad.addColour (0.0, base.brighter (0.5f));
        grad.addColour (0.06, base);
        grad.addColour (0.95, base);
        grad.addColour (0.99, base.darker (1.0f));

        g.setGradientFill (grad);

        if (flatOnLeft or flatOnRight or flatOnTop or flatOnBottom)
        {
            const auto path { getConnectedEdgePath (bounds, cornerSize, flatOnLeft, flatOnRight, flatOnTop, flatOnBottom) };

            g.fillPath (path);
            g.setColour (base.darker (1.0f));
            g.strokePath (path, juce::PathStrokeType (0.5f));
        }
        else
        {
            g.fillRoundedRectangle (area, cornerSize);
            g.setColour (base.darker (1.0f));
            g.drawRoundedRectangle (area, cornerSize, 0.5f);
        }
    }
}

juce::Button* StyleTheme::createSliderButton (juce::Slider&, bool isIncrement)
{
    auto* button { new juce::TextButton (juce::String::charToString (isIncrement ? Chars::plus : Chars::dash)) };
    button->getProperties().set (Id::isIncrement, isIncrement);
    return button;
}

void StyleTheme::drawGroupButtonText (juce::Graphics& g, juce::TextButton& button)
{
    g.setFont (getTextButtonFont (button, button.getHeight()));

    auto colour { button.findColour (button.getToggleState() ? juce::TextButton::textColourOnId
                                                             : juce::TextButton::textColourOffId) };

    if (not button.isEnabled())
        colour = colour.withMultipliedAlpha (0.5f);

    g.setColour (colour);
    g.drawFittedText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 1);
}

#if JUCE_MODULE_AVAILABLE_jam_gui
void StyleTheme::drawSliderButtonText (juce::Graphics& g,
                                       juce::TextButton& button,
                                       bool isMouseOver,
                                       bool isButtonDown)
{
    auto colourMode { jam::ButtonSVG::ColourMode::toggle };
    auto pathStyle { isMouseOver ? jam::Svg::PathStyle::fill : jam::Svg::PathStyle::stroke };
    auto strokeType { juce::PathStrokeType (
        1.2f, juce::PathStrokeType::beveled, juce::PathStrokeType::rounded) };
    int edgeIndent { isMouseOver ? 2 : 4 };

    static const juce::String incDecUpImages[3] {
        BinaryData::getString (files::incDecUpNormal),
        BinaryData::getString (files::incDecUpOver),
        BinaryData::getString (files::incDecUpDown),
    };
    static const char* const incDecUpRawImages[3] {
        incDecUpImages[0].toRawUTF8(),
        incDecUpImages[1].toRawUTF8(),
        incDecUpImages[2].toRawUTF8(),
    };

    static const juce::String incDecDownImages[3] {
        BinaryData::getString (files::incDecDownNormal),
        BinaryData::getString (files::incDecDownOver),
        BinaryData::getString (files::incDecDownDown),
    };
    static const char* const incDecDownRawImages[3] {
        incDecDownImages[0].toRawUTF8(),
        incDecDownImages[1].toRawUTF8(),
        incDecDownImages[2].toRawUTF8(),
    };

    if (button.getProperties()[Id::isIncrement])
        jam::ButtonSVG::paint (g,
                                  isMouseOver,
                                  isButtonDown,
                                  button,
                                  incDecUpRawImages,
                                  3,
                                  colourMode,
                                  pathStyle,
                                  strokeType,
                                  edgeIndent);
    else
        jam::ButtonSVG::paint (g,
                                  isMouseOver,
                                  isButtonDown,
                                  button,
                                  incDecDownRawImages,
                                  3,
                                  colourMode,
                                  pathStyle,
                                  strokeType,
                                  edgeIndent);
}
#endif

void StyleTheme::drawButtonText (juce::Graphics& g,
                                 juce::TextButton& button,
                                 bool isMouseOver,
                                 bool isButtonDown)
{
    auto* parent { button.getParentComponent() };

    if (button.getProperties().contains (Id::groupButton))
    {
        drawGroupButtonText (g, button);
    }
    else
    {
        auto* slider { parent != nullptr ? dynamic_cast<juce::Slider*> (parent) : nullptr };

        if (slider != nullptr and slider->getSliderStyle() == juce::Slider::IncDecButtons)
        {
#if JUCE_MODULE_AVAILABLE_jam_gui
            drawSliderButtonText (g, button, isMouseOver, isButtonDown);
#else
            LookAndFeel_V4::drawButtonText (g, button, isMouseOver, isButtonDown);
#endif
        }
        else
        {
            LookAndFeel_V4::drawButtonText (g, button, isMouseOver, isButtonDown);
        }
    }
}

void StyleTheme::drawTickBox (juce::Graphics& g,
                              juce::Component& component,
                              float x,
                              float y,
                              float w,
                              float h,
                              const bool ticked,
                              [[maybe_unused]] const bool isEnabled,
                              [[maybe_unused]] const bool shouldDrawButtonAsHighlighted,
                              [[maybe_unused]] const bool shouldDrawButtonAsDown)
{
    juce::Rectangle<float> tickBounds (x, y, w, h);

    g.setColour (component.findColour (juce::ToggleButton::tickDisabledColourId));
    g.drawRoundedRectangle (tickBounds, 4.0f, 1.0f);

    if (ticked)
    {
        static const char* const tickPath { "M43 11L16.875 37L5 25.1818" };
        juce::Path tick { juce::Drawable::parseSVGPath (tickPath) };
        g.setColour (component.findColour (juce::ToggleButton::tickColourId));
        float delta { 2.0f };
        tickBounds.translate (delta, -(2 * delta));
        tick.applyTransform (tick.getTransformToScaleToFit (tickBounds.reduced (1.0f), false));
        auto stroke { juce::PathStrokeType (2.5f) };
        g.strokePath (tick, stroke);
    }
}

//==============================================================================
std::array<juce::Point<int>, 5> StyleTheme::getTabOutline (juce::TabbedButtonBar::Orientation orientation,
                                                            const juce::Rectangle<int>& tab,
                                                            bool toggled)
{
    // Five points tracing one tab's outline, in draw order (see getTabOutline() header doc).
    using TabOutline = std::array<juce::Point<int>, 5>;

    static constexpr std::array<TabOutline (*) (const juce::Rectangle<int>&, bool), 4> tabOutlines
    {
        [] (const juce::Rectangle<int>& t, bool isToggled) -> TabOutline
        {
            int cut { isToggled ? 0 : t.getHeight() / 3 };
            return { t.getBottomLeft(), t.getTopLeft(), t.getTopRight().translated (-cut, 0),
                     t.getTopRight().translated (0, cut), t.getBottomRight() };
        },
        [] (const juce::Rectangle<int>& t, bool isToggled) -> TabOutline
        {
            int cut { isToggled ? 0 : t.getHeight() / 3 };
            return { t.getTopRight(), t.getBottomRight(), t.getBottomLeft().translated (cut, 0),
                     t.getBottomLeft().translated (0, -cut), t.getTopLeft() };
        },
        [] (const juce::Rectangle<int>& t, bool isToggled) -> TabOutline
        {
            int cut { isToggled ? 0 : t.getWidth() / 3 };
            return { t.getBottomRight(), t.getBottomLeft(), t.getTopLeft().translated (0, cut),
                     t.getTopLeft().translated (cut, 0), t.getTopRight() };
        },
        [] (const juce::Rectangle<int>& t, bool isToggled) -> TabOutline
        {
            int cut { isToggled ? 0 : t.getWidth() / 3 };
            return { t.getTopLeft(), t.getTopRight(), t.getBottomRight().translated (0, -cut),
                     t.getBottomRight().translated (-cut, 0), t.getBottomLeft() };
        }
    };

    return tabOutlines.at (static_cast<size_t> (orientation)) (tab, toggled);
}

void StyleTheme::applyTabTransform (juce::Graphics& g,
                                    juce::TabbedButtonBar::Orientation orientation,
                                    const juce::Rectangle<float>& area)
{
    juce::AffineTransform t;

    switch (orientation)
    {
        case juce::TabbedButtonBar::TabsAtLeft:
            t = t.rotated (juce::MathConstants<float>::pi * -0.5f).translated (area.getX(), area.getBottom());
            break;
        case juce::TabbedButtonBar::TabsAtRight:
            t = t.rotated (juce::MathConstants<float>::pi * 0.5f).translated (area.getRight(), area.getY());
            break;
        case juce::TabbedButtonBar::TabsAtTop:
        case juce::TabbedButtonBar::TabsAtBottom:
            t = t.translated (area.getX(), area.getY());
            break;
        default:
            jassertfalse;
            break;
    }

    g.addTransform (t);
}

void StyleTheme::drawTabButton (juce::TabBarButton& button,
                                juce::Graphics& g,
                                bool isMouseOver,
                                bool isMouseDown)
{
    const juce::Rectangle<int> tab (button.getActiveArea());

    juce::Path tabFill;
    const auto [p1, p2, p3, p4, p5] { getTabOutline (button.getTabbedButtonBar().getOrientation(), tab, button.getToggleState()) };

    tabFill.startNewSubPath (p1.x, p1.y);
    tabFill.lineTo (p2.x, p2.y);
    tabFill.lineTo (p3.x, p3.y);
    tabFill.lineTo (p4.x, p4.y);
    tabFill.lineTo (p5.x, p5.y);
    tabFill.closeSubPath();

    const juce::Colour bg (button.getTabBackgroundColour());

    if (button.getToggleState())
    {
        g.setColour (bg);
    }
    else
    {
        g.setColour (bg.darker (1.0f));
    }

    g.fillPath (tabFill);

    juce::Path tabStroke;
    tabStroke.startNewSubPath (p1.x, p1.y);
    tabStroke.lineTo (p2.x, p2.y);
    tabStroke.lineTo (p3.x, p3.y);
    tabStroke.lineTo (p4.x, p4.y);
    tabStroke.lineTo (p5.x, p5.y);

    g.setColour (button.findColour (juce::TabbedButtonBar::tabOutlineColourId));
    g.strokePath (tabStroke,
                  juce::PathStrokeType (
                      2.0f, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded));

    const float alpha = button.isEnabled() ? ((isMouseOver or isMouseDown) ? 1.0f : 0.8f) : 0.3f;

    juce::Colour col (bg.contrasting().withMultipliedAlpha (alpha));

    if (juce::TabbedButtonBar* bar = button.findParentComponentOfClass<juce::TabbedButtonBar>())
    {
        juce::TabbedButtonBar::ColourIds colID =
            button.isFrontTab() ? juce::TabbedButtonBar::frontTextColourId : juce::TabbedButtonBar::tabTextColourId;

        if (bar->isColourSpecified (colID))
            col = bar->findColour (colID);
        else if (isColourSpecified (colID))
            col = findColour (colID);
    }

    const juce::Rectangle<float> area (button.getTextArea().toFloat());

    float length = area.getWidth();
    float depth = area.getHeight();

    if (button.getTabbedButtonBar().isVertical())
        std::swap (length, depth);

    juce::TextLayout textLayout;
    createTabTextLayout (button, length, depth, col, textLayout);

    applyTabTransform (g, button.getTabbedButtonBar().getOrientation(), area);
    textLayout.draw (g, juce::Rectangle<float> (length, depth));
}

int StyleTheme::getTabButtonBestWidth (juce::TabBarButton& button, int tabDepth)
{
    int width =
        juce::GlyphArrangement::getStringWidthInt (withDefaultMetrics (getCommonFont()), button.getButtonText().trim())
        + getTabButtonOverlap (tabDepth) * 2;

    if (auto* extraComponent = button.getExtraComponent())
        width += button.getTabbedButtonBar().isVertical() ? extraComponent->getHeight() : extraComponent->getWidth();

    return juce::jlimit (tabDepth * 2, tabDepth * 8, width);
}

void StyleTheme::drawButtonGroupTrack (juce::Graphics& g, juce::Component& group)
{
#if JUCE_MODULE_AVAILABLE_jam_gui
    auto bounds { group.getLocalBounds().toFloat() };
    constexpr float cornerSize { 4.0f };
    constexpr float stroke { 1.0f };

    g.setColour (group.findColour (ButtonGroup::trackFillColourId));
    g.fillRoundedRectangle (bounds.reduced (stroke), cornerSize);
    g.setColour (group.findColour (ButtonGroup::trackOutlineColourId));
    g.drawRoundedRectangle (bounds.reduced (stroke), cornerSize, stroke);
#else
    juce::ignoreUnused (g, group);
#endif
}

void StyleTheme::drawButtonGroupSlidingIndicator (juce::Graphics& g, juce::Component& indicator)
{
#if JUCE_MODULE_AVAILABLE_jam_gui
    auto bounds { indicator.getLocalBounds().toFloat().reduced (3.0f, 3.0f) };
    constexpr float cornerSize { 3.0f };
    constexpr float stroke { 1.0f };

    g.setColour (indicator.findColour (ButtonGroup::indicatorFillColourId));
    g.fillRoundedRectangle (bounds, cornerSize);
    g.setColour (indicator.findColour (ButtonGroup::indicatorOutlineColourId));
    g.drawRoundedRectangle (bounds, cornerSize, stroke);
#else
    juce::ignoreUnused (g, indicator);
#endif
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
