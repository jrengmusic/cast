/**
 * @file jam_PluginEditorLayout.h
 * @brief Loads, validates, and joins every layout document the plugin
 *        editor needs -- component.md, the view layout, HTML panels, CSS,
 *        and SVG art -- and seeds the default user settings.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class PluginEditorLayout
 * @brief Owns and validates the plugin's layout documents, and readies the
 *        model's persisted parameter state before the editor is built.
 *
 * Every binary-data document is parsed once at construction, joined to
 * component.md by addComponents() (rect element <-> component row, matched
 * on the SVG rect id's leading number), and validated against getRules().
 * isReady() additionally seeds the default settings file and populates the
 * model's persisted-parameter subtree from it.
 */
class PluginEditorLayout : public Document::Validator
{
public:
    /**
     * @brief Parses every binary-data layout document and joins component.md
     *        to the named view layout.
     * @param binaryDataFetcher   Fetcher supplying font resources from binary data.
     * @param viewLayoutFilename  Filename of the view-layout document (EditorLayout.svg or .md).
     */
    PluginEditorLayout (ResourceFetcher binaryDataFetcher, const juce::String& viewLayoutFilename)
        : fonts { binaryDataFetcher }
        , documents { std::in_place, viewLayoutFilename }
    {
    }

    /** Font resources fetched from binary data. */
    ResourceFetchers fonts;

    /**
     * @brief Seeds the default settings file, then validates every layout
     *        document and populates the model's persisted-parameter subtree.
     * @param model  Audio model whose persisted-parameter subtree is populated.
     * @return `true` when every document is valid, the settings file is
     *         valid, and the parameter subtree was populated.
     */
    bool isReady (AudioModel& model) const noexcept
    {
        loadDefaultSettings();

        return isDocumentsValid() and isSettingsValid() and populateTree (model);
    }

    /**
     * @brief Returns the validation rules applied to every loaded layout document.
     * @return Rules keyed by name: `rect` (every component.md row has exactly
     *         one `rect` cell), `z` (row order matches the `z` column),
     *         `primary` (every `primary` reference resolves to a row),
     *         `component` (every numbered SVG rect has a joined component
     *         row), and `li` (every list-cell item is declared in the row's
     *         `type` list and appears at most once).
     */
    const Rules& getRules() const override
    {
        static const Rules rules {
            []
            {
                Rules rules;

                rules.add<const Document&> (Id::rect.toString(),
                    [] (const Document& document) -> juce::Result
                    {
                        if (const auto* markdown { dynamic_cast<const MarkdownDocument*> (&document) })
                        {
                            jam::Strings failures;

                            for (auto* table : markdown->getTables (Id::component))
                                for (auto* row : markdown->getTableRows (*table))
                                {
                                    int count { 0 };
                                    row->applyToProperties (
                                        [&count] (const juce::Identifier& key, const Document::Value&)
                                        {
                                            if (key == Id::rect)
                                                ++count;
                                        });

                                    if (count != 1)
                                        failures.add (MarkdownValidator::getLocation (*table, *row, Id::rect.toString()));
                                }

                            if (failures.size() > 0)
                                return juce::Result::fail (failures.joinIntoString (
                                    juce::String::charToString (Chars::newline), 0, -1));
                        }

                        return juce::Result::ok();
                    });

                rules.add<const Document&> (Id::z.toString(),
                    [] (const Document& document) -> juce::Result
                    {
                        if (const auto* markdown { dynamic_cast<const MarkdownDocument*> (&document) })
                        {
                            jam::Strings failures;

                            for (auto* table : markdown->getTables (Id::component))
                            {
                                int ordinal { 0 };

                                for (auto* row : markdown->getTableRows (*table))
                                {
                                    if (Format::getNumber (markdown->getTableValueView (*row, Id::z)) != ordinal)
                                        failures.add (MarkdownValidator::getLocation (*table, *row, Id::z.toString()));

                                    ++ordinal;
                                }
                            }

                            if (failures.size() > 0)
                                return juce::Result::fail (failures.joinIntoString (
                                    juce::String::charToString (Chars::newline), 0, -1));
                        }

                        return juce::Result::ok();
                    });

                rules.add<const Document&> (Id::primary.toString(),
                    [] (const Document& document) -> juce::Result
                    {
                        if (const auto* markdown { dynamic_cast<const MarkdownDocument*> (&document) })
                        {
                            jam::Strings failures;

                            for (auto* table : markdown->getTables (Id::component))
                                for (auto* row : markdown->getTableRows (*table))
                                {
                                    const auto view { markdown->getTableValueView (*row, Id::primary) };

                                    if (not view.empty())
                                        if (markdown->getTableRow (*table, juce::Identifier (juce::String (Format::getNumber (view)))) == nullptr)
                                            failures.add (MarkdownValidator::getLocation (*table, *row, Id::primary.toString()));
                                }

                            if (failures.size() > 0)
                                return juce::Result::fail (failures.joinIntoString (
                                    juce::String::charToString (Chars::newline), 0, -1));
                        }

                        return juce::Result::ok();
                    });

                rules.add<const Document&> (Id::component.toString(),
                    [] (const Document& document) -> juce::Result
                    {
                        if (const auto* xml { dynamic_cast<const XmlDocument*> (&document) })
                        {
                            jam::Strings failures;

                            auto apply = [&failures] (auto& self, Document::Element& element) -> void
                            {
                                if (element.isTag (Id::rect))
                                {
                                    const auto id { Svg::getElementId (element) };

                                    if (id.isNotEmpty() and Document::isDigit (id[0]))
                                        if (not element.contains (Id::component))
                                            failures.add (id);
                                }

                                for (auto* child : element)
                                    self (self, *child);
                            };

                            apply (apply, *xml->root);

                            if (failures.size() > 0)
                                return juce::Result::fail (failures.joinIntoString (
                                    juce::String::charToString (Chars::newline), 0, -1));
                        }

                        return juce::Result::ok();
                    });

                rules.add<const Document&> (Id::li.toString(),
                    [] (const Document& document) -> juce::Result
                    {
                        if (const auto* markdown { dynamic_cast<const MarkdownDocument*> (&document) })
                        {
                            jam::Strings failures;

                            for (auto* table : markdown->getTables (Id::component))
                                for (auto* row : markdown->getTableRows (*table))
                                    if (auto* typeList { markdown->getTableList (*row, Id::type) })
                                        for (auto* cell : *row)
                                            if (auto* quote { markdown->getBlockquote (*cell) })
                                                if (auto* list { markdown->getList (*quote) })
                                                {
                                                    jam::Strings seen;

                                                    for (auto* item : *list)
                                                    {
                                                        const auto itemId { item->id.toString() };

                                                        if (seen.contains (itemId, false))
                                                            failures.add (MarkdownValidator::getLocation (*table, *row, cell->id.toString()));

                                                        seen.add (itemId);

                                                        if (cell->id != Id::type and markdown->getListItem (*typeList, item->id) == nullptr)
                                                            failures.add (MarkdownValidator::getLocation (*table, *row, cell->id.toString()));
                                                    }
                                                }

                            if (failures.size() > 0)
                                return juce::Result::fail (failures.joinIntoString (
                                    juce::String::charToString (Chars::newline), 0, -1));
                        }

                        return juce::Result::ok();
                    });

                return rules;
            }()
        };

        return rules;
    }

private:
    /**
     * @class LayoutDocuments
     * @brief Shared store of every parsed binary-data layout document,
     *        keyed by filename identifier.
     */
    struct LayoutDocuments : SharedDocuments
    {
        /**
         * @brief Parses every binary-data resource with a known layout
         *        extension, then joins component.md to the named view layout.
         * @param viewLayoutFilename  Filename of the view-layout document (EditorLayout.svg or .md).
         */
        explicit LayoutDocuments (const juce::String& viewLayoutFilename)
        {
            BinaryData::forEachResource (&LayoutDocuments::parseResource);

            const juce::Identifier viewLayout { viewLayoutFilename };
            jassert (contains (viewLayout));

            addComponents (*at (viewLayout), MarkdownDocument::getOrCreate (juce::Identifier { files::componentLayout }));
        }

        /** @return The markdown document loaded or created for `name`. */
        static const Document& parseMarkdown (const juce::Identifier& name) { return MarkdownDocument::getOrCreate (name); }
        /** @return The HTML document loaded or created for `name`. */
        static const Document& parseHtml (const juce::Identifier& name) { return Html::getOrCreate (name); }
        /** @return The CSS document loaded or created for `name`. */
        static const Document& parseCss (const juce::Identifier& name) { return Css::getOrCreate (name); }
        /** @return The XML/SVG document loaded or created for `name`. */
        static const Document& parseXml (const juce::Identifier& name) { return Xml::getOrCreate (name); }

        /** @return The parser function for each supported file extension (md, html, css, svg). */
        static const jam::HashMap<juce::String, const Document& (*) (const juce::Identifier&)>& getDocumentParsers()
        {
            static const jam::HashMap<juce::String, const Document& (*) (const juce::Identifier&)> documentParsers
            {
                { Extensions::md,   parseMarkdown },
                { Extensions::html, parseHtml },
                { Extensions::css,  parseCss },
                { Extensions::svg,  parseXml },
            };

            return documentParsers;
        }

        /**
         * @brief Parses and validates one binary-data resource, when its
         *        extension is supported.
         * @param originalFilename  Binary-data resource filename.
         */
        static void parseResource (const char* originalFilename, const void*, int)
        {
            const juce::String filename { originalFilename };
            const auto extension { getFileExtension (filename) };

            if (getDocumentParsers().contains (extension))
            {
                const auto name { juce::Identifier { filename } };
                const auto& document { getDocumentParsers().at (extension) (name) };
                const auto result { getDocumentValidators().get (extension, document) };
                jassert (result.wasOk());
#if JUCE_DEBUG
                if (not result.wasOk())
                    debug::Log::write (result.getErrorMessage());
#endif
            }
        }

        /** @return The validator function for each supported file extension (md, css, html, svg). */
        static const jam::Function::Map<juce::String, juce::Result>& getDocumentValidators()
        {
            static const jam::Function::Map<juce::String, juce::Result> documentValidators {
                []
                {
                    jam::Function::Map<juce::String, juce::Result> documentValidators;

                    documentValidators.add<const Document&> (Extensions::md,
                        [] (const Document& document)
                        {
                            static const MarkdownValidator validator;
                            return validator.isValid (document);
                        });

                    documentValidators.add<const Document&> (Extensions::css,
                        [] (const Document& document)
                        {
                            static const CssValidator validator;
                            return validator.isValid (document);
                        });

                    documentValidators.add<const Document&> (Extensions::html,
                        [] (const Document& document)
                        {
                            static const HtmlValidator validator;
                            return validator.isValid (document);
                        });

                    documentValidators.add<const Document&> (Extensions::svg,
                        [] (const Document& document)
                        {
                            static const XmlValidator validator;
                            return validator.isValid (document);
                        });

                    return documentValidators;
                }()
            };

            return documentValidators;
        }

        /** Length of the leading dot in juce::File::getFileExtension()'s result. */
        static constexpr int extensionDotLength { 1 };

        /** @return `file`'s extension, without the leading dot. */
        static juce::String getFileExtension (const juce::String& file)
        {
            return juce::File::createFileWithoutCheckingPath (file).getFileExtension().substring (extensionDotLength);
        }

        /**
         * @brief Joins every numbered SVG rect element in `viewLayout` to
         *        its component.md row, matched on the rect id's leading
         *        number, cross-linking each side to the other.
         * @param viewLayout   Parsed view-layout document (EditorLayout.svg or .md).
         * @param components   Parsed component.md document.
         */
        static void addComponents (Document& viewLayout, const MarkdownDocument& components)
        {
            for (auto* table : components.getTables (Id::component))
            {
                auto apply = [&components, table] (auto& self, Document::Element& element) -> void
                {
                    const auto id { Svg::getElementId (element) };

                    if (element.isTag (Id::rect) and id.isNotEmpty())
                    {
                        const auto number { id.getIntValue() };

                        if (auto* row { components.getTableRow (*table, juce::Identifier (juce::String (number))) })
                        {
                            element.add<Document::Element*> (Id::component, row);
                            row->add<Document::Element*> (Id::rect, &element);
                        }
                    }

                    for (auto* child : element)
                        self (self, *child);
                };

                apply (apply, *viewLayout.root);
            }
        }
    };

    /** Shared store of every parsed binary-data layout document. */
    SharedInstance<LayoutDocuments> documents;

    /**
     * @brief Validates every parsed layout document against getRules(),
     *        logging each failure in debug builds.
     * @return `true` when every document is valid.
     */
    bool isDocumentsValid() const noexcept
    {
        for (auto& [name, document] : *documents)
        {
            if (const auto result { this->isValid (*document) }; not result.wasOk())
            {
#if JUCE_DEBUG
                debug::Log::write (result.getErrorMessage());
#endif
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Creates the user's settings XML file from the binary default
     *        template, when it does not already exist, substituting the
     *        product tag and default-preset-directory placeholders with the
     *        product's valid ID and the default presets directory.
     */
    void loadDefaultSettings() const
    {
        auto* parameterManager { ParameterManager::getInstance() };

        if (auto raw { BinaryData::getString (files::defaultSettings) }; raw.isNotEmpty())
        {
            auto productTag { Format::toValidID (parameterManager->getProductName(), true) };
            raw = Format::replaceholder (raw, Id::product, productTag, juce::String::charToString (Chars::at));
            raw = Format::replaceholder (raw, Id::defaultPreset, parameterManager->getDefaultPresetsDirectory().getFullPathName(), juce::String::charToString (Chars::at));

            if (const auto document { Xml::parse (raw) }; not document.root->id.isNull())
                parameterManager->getOrCreateUserSettings (raw, document);
        }
    }

    /** @return `true` when the user's settings file is valid. */
    bool isSettingsValid() const noexcept
    {
        return ParameterManager::getInstance()->isSettingsValid();
    }

    /**
     * @brief Appends one `param` child, carrying `id` and `value`, for every
     *        named setting element that declares a non-empty value.
     * @param parameter        Parameter subtree the `param` children are appended to.
     * @param settingsElement  Persisted `\<settings\>` element to read named values from.
     */
    static void addParameters (juce::ValueTree& parameter, const Document::Element& settingsElement)
    {
        for (auto* child : settingsElement)
        {
            auto name { child->id.toString() };

            if (child->contains (Id::value) and child->get<juce::String> (Id::value) != nullptr)
            {
                auto value { *child->get<juce::String> (Id::value) };

                if (name.isNotEmpty() and value.isNotEmpty())
                {
                    auto param { juce::ValueTree (Id::toType (Id::param)) };
                    param.setProperty (Id::id, name, nullptr);
                    param.setProperty (Id::value, value, nullptr);

                    parameter.appendChild (param, nullptr);
                }
            }
        }
    }

    /**
     * @brief Populates the model's persisted-parameter subtree from the
     *        user's settings file, when the subtree is still empty.
     * @param model  Audio model whose persisted-parameter subtree is populated.
     * @return `true` when the parameter subtree is valid.
     */
    bool populateTree (AudioModel& model) const noexcept
    {
        auto parameter { model.state.getOrCreateChildWithName (Id::toTag (Id::parameter), nullptr) };

        if (parameter.getNumChildren() == 0)
        {
            if (auto settingsFile { ParameterManager::getInstance()->getUserSettings() }; settingsFile.existsAsFile())
            {
                if (const auto document { Xml::parse (settingsFile.loadFileAsString()) };
                    not document.root->id.isNull())
                {
                    if (const auto* settingsElement { document.root->getChildByID (Id::toTag (Id::settings)) })
                    {
                        addParameters (parameter, *settingsElement);
                    }
                }
            }
        }

        return parameter.isValid();
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
