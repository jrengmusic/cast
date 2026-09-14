#pragma once

namespace jam
{

class ButtonLogo final : public juce::Button
{
public:
    ButtonLogo()
        : juce::Button ({}) {}

    ~ButtonLogo() = default;

    void paintButton (juce::Graphics& g,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override
    {
        auto area { getLocalBounds().reduced (edgeIndent).toFloat() };
        float lineThickness { 3.0f };

        jam::Logo logo;

        if (shouldDrawButtonAsDown)
        {
            logo.drawStroke (g, area, jam::Logo::default3(), lineThickness);
        }
        else if (shouldDrawButtonAsHighlighted)
        {
            logo.drawFill (g, area);
        }
        else
        {
            const juce::Colour colour { findColour (juce::TextButton::textColourOffId) };

            logo.drawStroke (g, area, { colour, colour.darker (0.5f), colour.darker (1.0f) }, lineThickness);
        }
    }

private:
    int edgeIndent { 2 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonLogo)
};

} // namespace jam
