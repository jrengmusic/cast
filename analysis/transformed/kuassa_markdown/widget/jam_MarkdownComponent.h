#pragma once

namespace jam
{
/*____________________________________________________________________________*/

class MarkdownComponent : public juce::Component
{
public:
    enum ColourIds
    {
        textColourId                   = map::ColourId::markdownTextColourId,
        headingColourId                = map::ColourId::markdownHeadingColourId,
        codeBackgroundColourId         = map::ColourId::markdownCodeBackgroundColourId,
        codeTextColourId               = map::ColourId::markdownCodeTextColourId,
        quoteBarColourId               = map::ColourId::markdownQuoteBarColourId,
        linkColourId                   = map::ColourId::markdownLinkColourId,
        ruleColourId                   = map::ColourId::markdownRuleColourId,
        tableLineColourId              = map::ColourId::markdownTableLineColourId,
        tableHeaderColourId            = map::ColourId::markdownTableHeaderColourId,
        checkboxColourId               = map::ColourId::markdownCheckboxColourId,
        tableHeaderBackgroundColourId  = map::ColourId::markdownTableHeaderBackgroundColourId,
        syntaxKeywordColourId          = map::ColourId::markdownSyntaxKeywordColourId,
        syntaxStringColourId           = map::ColourId::markdownSyntaxStringColourId,
        syntaxCommentColourId          = map::ColourId::markdownSyntaxCommentColourId,
        syntaxNumberColourId           = map::ColourId::markdownSyntaxNumberColourId,
        syntaxPunctuationColourId      = map::ColourId::markdownSyntaxPunctuationColourId,
        quoteBackgroundColourId        = map::ColourId::markdownQuoteBackgroundColourId,
        codeBlockBackgroundColourId    = map::ColourId::markdownCodeBlockBackgroundColourId,
    };

    ~MarkdownComponent() override = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MarkdownComponent)
};

/*____________________________________________________________________________*/
}
