/**
 * @file jam_Function.h
 * @brief Type-safe, type-erased callable containers — Array, Map, and MapType.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct Function
 * @brief Type-safe container for callable entities using composition pattern.
 *
 * @details Provides compile-time type-safe storage and retrieval of callables.
 *          Uses composition (not inheritance) for clean, predictable behavior.
 *          Supports both return values and output parameters for zero-copy patterns.
 *
 * @par Example: Basic callback map
 * @code{.cpp}
 * Function::Map<std::string, void> callbacks;
 * callbacks.add<int>("setValue", [](int v) { std::cout << v; });
 * callbacks.get("setValue", 42);  // Prints "42"
 * @endcode
 *
 * @par Example: Zero-copy pattern with output parameters
 * @code{.cpp}
 * Function::Map<std::string, void> getters;
 * getters.add<int, std::vector<double>&>("getData",
 *     [](int size, std::vector<double>& output) {
 *         output.resize(size);
 *         // Fill output...
 *     });
 * std::vector<double> buffer;  // Reusable buffer
 * getters.get("getData", 100, buffer);  // Zero allocations
 * @endcode
 *
 * @par Signature contract
 * Storage type-erases the callable behind Function::Common; get() recovers it by
 * casting to `Function::Element<ReturnType, Args...>`. get() is deduce-only —
 * never pass explicit template arguments at the call site. Args deduce from the
 * forwarded arguments exactly as any forwarding-reference parameter pack does: a
 * named lvalue deduces `T&`, a prvalue deduces `T`, a const lvalue deduces
 * `const T&`. Registrations declare deduction reality: the `add<Ts...>` list at
 * the matching add() call is written to reproduce, in order and including
 * reference/cv-qualification, exactly what the map's get() call sites will
 * naturally deduce:
 * @code{.cpp}
 * bindTo.add<BaseType*&, juce::Component*&> (componentID,
 *     [] (BaseType* obj, juce::Component* p) { ... });
 *
 * BaseType* obj = ...;
 * juce::Component* child = ...;
 * bindTo.get (componentID, obj, child);// named lvalues deduce BaseType*&, juce::Component*&
 * @endcode
 * Because Common carries no template parameters, this deduction match cannot be
 * enforced by the compiler. A mismatch is only caught at runtime: dynamic_cast
 * verifies the registered signature, an assertion fires in debug builds, and in
 * release builds the null result crashes at the call site rather than invoking
 * through the wrong signature.
 */
struct Function
{
    /**
     * @struct Common
     * @brief Base class for callable elements in Function.
     *
     * @details Acts as a common interface for all callable elements.
     */
    struct Common
    {
        /** @brief Virtual destructor for Common. */
        virtual ~Common() = default;
    };

    /**
     * @brief A callable element that contains a std::function (composition pattern).
     *
     * @tparam ReturnType The return type of the callable element.
     * @tparam Args The argument types for the callable element (as specified, preserving references).
     */
    template <typename ReturnType, typename... Args>
    struct Element : Common
    {
        std::function<ReturnType (Args...)> func;

        /**
         * @brief Constructor for Element.
         *
         * @tparam FunctionType The type of the function to wrap.
         * @param newFunction The function to wrap as a callable element.
         */
        template <typename FunctionType>
        Element (FunctionType&& newFunction)
            : func (std::forward<FunctionType> (newFunction))
        {
        }

        /**
         * @brief Call operator to invoke the stored function.
         */
        ReturnType operator() (Args... args) const { return func (std::forward<Args> (args)...); }
    };

    //==============================================================================
    /**
     * @struct Array
     * @brief A container for callable elements, implemented as an array with safety enhancements.
     *
     * @tparam ReturnType The return type of the callable elements in the array.
     */
    template <typename ReturnType>
    struct Array : jam::Array<std::unique_ptr<Function::Common>>
    {
        /** @brief Default constructor for Array. */
        Array() = default;
        // explicitly move-only
        Array (Array&&) noexcept = default;
        Array& operator= (Array&&) noexcept = default;
        Array (const Array&) = delete;
        Array& operator= (const Array&) = delete;

        /** @brief Destructor for Array. */
        ~Array() = default;

        /**
         * @brief Adds a new callable element to the array.
         *
         * @tparam Args The argument types for the callable element (preserving references).
         * @tparam FunctionType The type of the function to add.
         * @param newFunction The function to add as a callable element.
         */
        template <typename... Args, typename FunctionType>
        void add (FunctionType&& newFunction)
        {
            using CanonElem = Function::Element<ReturnType, Args...>;
            auto ptr = std::make_unique<CanonElem> (std::forward<FunctionType> (newFunction));
            this->jam::Array<std::unique_ptr<Function::Common>>::add (std::move (ptr));
        }

        /**
         * @brief Retrieves and calls a callable element from the array with safety checks.
         *
         * @tparam Args Deduced only — never supplied explicitly at the call site.
         *         Must reproduce, in order and including reference/cv-qualification,
         *         the Args the element was registered with via add(); call sites are
         *         written so the natural deduction (named lvalue -> `T&`, prvalue ->
         *         `T`, const lvalue -> `const T&`) lands on that exact signature.
         * @param index The index of the callable element in the array.
         * @param args The arguments to pass to the callable element.
         * @return The result of calling the callable element.
         *
         * @throws std::out_of_range If the index is not within the array bounds.
         * @note The stored element is checked against Args via dynamic_cast; an
         *       assertion fires on mismatch in debug builds, while release builds
         *       crash at the call site on the resulting null pointer rather than
         *       failing to build.
         */
        template <typename... Args>
        ReturnType get (int index, Args&&... args)
        {
            using CanonElem = Function::Element<ReturnType, Args...>;
            auto* elem { dynamic_cast<CanonElem*> (this->at (index).get()) };
            assert (elem != nullptr
                    and "Function::Array::get() - Signature mismatch. Was this function registered with a different signature via add()?");

            return (*elem) (std::forward<Args> (args)...);
        }

        /**
         * @brief Retrieves and calls a callable element from the array (const overload).
         *
         * @tparam Args Deduced only — never supplied explicitly at the call site.
         *         Must reproduce, in order and including reference/cv-qualification,
         *         the Args the element was registered with via add(); call sites are
         *         written so the natural deduction (named lvalue -> `T&`, prvalue ->
         *         `T`, const lvalue -> `const T&`) lands on that exact signature.
         * @param index The index of the callable element in the array.
         * @param args The arguments to forward to the callable element.
         * @return The result of invoking the stored callable.
         *
         * @details
         * This overload allows invocation on a const-qualified array.
         *
         * @throws std::out_of_range If the index is not within the array bounds.
         * @note The stored element is checked against Args via dynamic_cast; an
         *       assertion fires on mismatch in debug builds, while release builds
         *       crash at the call site on the resulting null pointer rather than
         *       failing to build.
         */
        template <typename... Args>
        ReturnType get (int index, Args&&... args) const
        {
            using CanonElem = Function::Element<ReturnType, Args...>;
            auto* elem { dynamic_cast<CanonElem*> (this->at (index).get()) };
            assert (elem != nullptr
                    and "Function::Array::get() const - Signature mismatch. Was this function registered with a different signature via add()?");

            return (*elem) (std::forward<Args> (args)...);
        }
    };

    //==============================================================================
    /**
     * @struct Map
     * @brief A container for callable elements, implemented as a map with enhanced safety.
     *
     * @tparam KeyType The type of the keys in the map.
     * @tparam ReturnType The return type of the callable elements in the map.
     */
    template <typename KeyType, typename ReturnType>
    struct Map : jam::HashMap<KeyType, std::unique_ptr<Function::Common>>
    {
        /** @brief Default constructor for Map. */
        Map() = default;
        Map (Map&&) noexcept = default;
        Map& operator= (Map&&) noexcept = default;
        Map (const Map&) = delete;
        Map& operator= (const Map&) = delete;

        /** @brief Default destructor for Map. */
        ~Map() = default;

        /**
         * @brief Adds a new callable element to the map.
         *
         * @tparam Args The argument types for the callable element (preserving references).
         * @tparam FunctionType The type of the function to add.
         * @param key The key for the callable element.
         * @param newFunction The function to add as a callable element.
         */
        template <typename... Args, typename FunctionType>
        void add (KeyType key, FunctionType&& newFunction)
        {
            using CanonElem = Function::Element<ReturnType, Args...>;
            auto ptr = std::make_unique<CanonElem> (std::forward<FunctionType> (newFunction));
            this->insert ({ key, std::move (ptr) });
        }

        template <typename... Args, typename FunctionType, typename IdentifierType,
                  std::enable_if_t<std::is_same_v<IdentifierType, juce::Identifier>
                                   and std::is_same_v<KeyType, juce::String>, int> = 0>
        void add (const IdentifierType& key, FunctionType&& newFunction)
        {
            add<Args...> (key.toString(), std::forward<FunctionType> (newFunction));
        }

        /**
         * @brief Retrieves and calls a callable element from the map with safety checks.
         *
         * @tparam Args Deduced only — never supplied explicitly at the call site.
         *         Must reproduce, in order and including reference/cv-qualification,
         *         the Args the element was registered with via add(); call sites are
         *         written so the natural deduction (named lvalue -> `T&`, prvalue ->
         *         `T`, const lvalue -> `const T&`) lands on that exact signature.
         * @param key The key of the callable element in the map.
         * @param args The arguments to pass to the callable element.
         * @return The result of calling the callable element.
         *
         * @throws std::out_of_range If the key is not present in the map.
         * @note The stored element is checked against Args via dynamic_cast; an
         *       assertion fires on mismatch in debug builds, while release builds
         *       crash at the call site on the resulting null pointer rather than
         *       failing to build.
         */
        template <typename... Args>
        ReturnType get (const KeyType& key, Args&&... args)
        {
            using CanonElem = Function::Element<ReturnType, Args...>;
            auto* elem { dynamic_cast<CanonElem*> (this->at (key).get()) };
            assert (elem != nullptr
                    and "Function::Map::get() - Signature mismatch. Was this function registered with a different signature via add()?");

            return (*elem) (std::forward<Args> (args)...);
        }

        template <typename... Args, typename IdentifierType,
                  std::enable_if_t<std::is_same_v<IdentifierType, juce::Identifier>
                                   and std::is_same_v<KeyType, juce::String>, int> = 0>
        ReturnType get (const IdentifierType& key, Args&&... args)
        {
            return get (key.toString(), std::forward<Args> (args)...);
        }

        /**
         * @brief Retrieves and calls a callable element from the map (const overload).
         *
         * @tparam Args Deduced only — never supplied explicitly at the call site.
         *         Must reproduce, in order and including reference/cv-qualification,
         *         the Args the element was registered with via add(); call sites are
         *         written so the natural deduction (named lvalue -> `T&`, prvalue ->
         *         `T`, const lvalue -> `const T&`) lands on that exact signature.
         * @param key The key of the callable element in the map.
         * @param args The arguments to forward to the callable element.
         * @return The result of invoking the stored callable.
         *
         * @details
         * This overload allows invocation on a const-qualified map.
         *
         * @throws std::out_of_range If the key is not present in the map.
         * @note The stored element is checked against Args via dynamic_cast; an
         *       assertion fires on mismatch in debug builds, while release builds
         *       crash at the call site on the resulting null pointer rather than
         *       failing to build.
         */
        template <typename... Args>
        ReturnType get (const KeyType& key, Args&&... args) const
        {
            using CanonElem = Function::Element<ReturnType, Args...>;
            auto* elem { dynamic_cast<CanonElem*> (this->at (key).get()) };
            assert (elem != nullptr
                    and "Function::Map::get() const - Signature mismatch. Was this function registered with a different signature via add()?");

            return (*elem) (std::forward<Args> (args)...);
        }

        template <typename... Args, typename IdentifierType,
                  std::enable_if_t<std::is_same_v<IdentifierType, juce::Identifier>
                                   and std::is_same_v<KeyType, juce::String>, int> = 0>
        ReturnType get (const IdentifierType& key, Args&&... args) const
        {
            return get (key.toString(), std::forward<Args> (args)...);
        }

        /**
         * @brief Checks if the map contains a given key.
         *
         * @param key The key to look for in the map.
         * @return True if the key is found, false otherwise.
         */
        bool contains (const KeyType& key) const noexcept
        {
            return this->find (key) != this->end();
        }

        template <typename IdentifierType,
                  std::enable_if_t<std::is_same_v<IdentifierType, juce::Identifier>
                                   and std::is_same_v<KeyType, juce::String>, int> = 0>
        bool contains (const IdentifierType& key) const noexcept
        {
            return contains (key.toString());
        }
    };

    //==============================================================================
    /**
     * @struct TypeEntry
     * @brief Value type for MapType — pairs a std::type_index with a callable.
     */
    struct TypeEntry
    {
        std::type_index type;
        std::unique_ptr<Function::Common> callable;
    };

    //==============================================================================
    /**
     * @struct MapType
     * @brief A type-aware callable map that preserves the registering type identity.
     *
     * Identical interface to Function::Map but each entry carries a std::type_index
     * captured at registration time from the ObjectType template parameter.
     * This enables runtime type comparison without dynamic_cast.
     *
     * @tparam KeyType The type of the keys in the map.
     * @tparam ReturnType The return type of the callable elements in the map.
     *
     * @example
     * @code
     * Function::MapType<juce::String, void> configure;
     * configure.add<MyWidget, juce::Component*> ("key", [] (juce::Component* c) { ... });
     * configure.typeOf ("key");  // returns typeid (MyWidget)
     * @endcode
     */
    template <typename KeyType, typename ReturnType>
    struct MapType : jam::HashMap<KeyType, Function::TypeEntry>
    {
        /** @brief Default constructor for MapType. */
        MapType() = default;
        MapType (MapType&&) noexcept = default;
        MapType& operator= (MapType&&) noexcept = default;
        MapType (const MapType&) = delete;
        MapType& operator= (const MapType&) = delete;

        /** @brief Default destructor for MapType. */
        ~MapType() = default;

        /**
         * @brief Adds a callable with type identity to the map.
         *
         * @tparam ObjectType The type to associate with this entry.
         * @tparam Args The argument types for the callable element (preserving references).
         * @tparam FunctionType The type of the function to add.
         * @param key The key for the callable element.
         * @param newFunction The function to add.
         */
        template <typename ObjectType, typename... Args, typename FunctionType>
        void add (KeyType key, FunctionType&& newFunction)
        {
            using CanonElem = Function::Element<ReturnType, Args...>;
            auto ptr = std::make_unique<CanonElem> (std::forward<FunctionType> (newFunction));
            this->insert ({
                key, TypeEntry { std::type_index (typeid (ObjectType)), std::move (ptr) }
            });
        }

        template <typename ObjectType, typename... Args, typename FunctionType, typename IdentifierType,
                  std::enable_if_t<std::is_same_v<IdentifierType, juce::Identifier>
                                   and std::is_same_v<KeyType, juce::String>, int> = 0>
        void add (const IdentifierType& key, FunctionType&& newFunction)
        {
            add<ObjectType, Args...> (key.toString(), std::forward<FunctionType> (newFunction));
        }

        /**
         * @brief Retrieves and calls a callable element from the map with safety checks.
         *
         * @tparam Args Deduced only — never supplied explicitly at the call site.
         *         Must reproduce, in order and including reference/cv-qualification,
         *         the Args the element was registered with via add(); call sites are
         *         written so the natural deduction (named lvalue -> `T&`, prvalue ->
         *         `T`, const lvalue -> `const T&`) lands on that exact signature.
         * @param key The key of the callable element in the map.
         * @param args The arguments to pass to the callable element.
         * @return The result of calling the callable element.
         *
         * @throws std::out_of_range If the key is not present in the map.
         * @note The stored element is checked against Args via dynamic_cast; an
         *       assertion fires on mismatch in debug builds, while release builds
         *       crash at the call site on the resulting null pointer rather than
         *       failing to build.
         */
        template <typename... Args>
        ReturnType get (const KeyType& key, Args&&... args)
        {
            using CanonElem = Function::Element<ReturnType, Args...>;
            auto* elem { dynamic_cast<CanonElem*> (this->at (key).callable.get()) };
            assert (elem != nullptr
                    and "Function::MapType::get() - Signature mismatch. Was this function registered with a different signature via add()?");

            return (*elem) (std::forward<Args> (args)...);
        }

        template <typename... Args, typename IdentifierType,
                  std::enable_if_t<std::is_same_v<IdentifierType, juce::Identifier>
                                   and std::is_same_v<KeyType, juce::String>, int> = 0>
        ReturnType get (const IdentifierType& key, Args&&... args)
        {
            return get (key.toString(), std::forward<Args> (args)...);
        }

        /**
         * @brief Retrieves and calls a callable element from the map (const overload).
         *
         * @tparam Args Deduced only — never supplied explicitly at the call site.
         *         Must reproduce, in order and including reference/cv-qualification,
         *         the Args the element was registered with via add(); call sites are
         *         written so the natural deduction (named lvalue -> `T&`, prvalue ->
         *         `T`, const lvalue -> `const T&`) lands on that exact signature.
         * @param key The key of the callable element in the map.
         * @param args The arguments to forward to the callable element.
         * @return The result of invoking the stored callable.
         *
         * @details
         * This overload allows invocation on a const-qualified map.
         *
         * @throws std::out_of_range If the key is not present in the map.
         * @note The stored element is checked against Args via dynamic_cast; an
         *       assertion fires on mismatch in debug builds, while release builds
         *       crash at the call site on the resulting null pointer rather than
         *       failing to build.
         */
        template <typename... Args>
        ReturnType get (const KeyType& key, Args&&... args) const
        {
            using CanonElem = Function::Element<ReturnType, Args...>;
            auto* elem { dynamic_cast<CanonElem*> (this->at (key).callable.get()) };
            assert (elem != nullptr
                    and "Function::MapType::get() const - Signature mismatch. Was this function registered with a different signature via add()?");

            return (*elem) (std::forward<Args> (args)...);
        }

        template <typename... Args, typename IdentifierType,
                  std::enable_if_t<std::is_same_v<IdentifierType, juce::Identifier>
                                   and std::is_same_v<KeyType, juce::String>, int> = 0>
        ReturnType get (const IdentifierType& key, Args&&... args) const
        {
            return get (key.toString(), std::forward<Args> (args)...);
        }

        /**
         * @brief Checks if the map contains a given key.
         *
         * @param key The key to look for in the map.
         * @return True if the key is found, false otherwise.
         */
        bool contains (const KeyType& key) const noexcept
        {
            return this->find (key) != this->end();
        }

        template <typename IdentifierType,
                  std::enable_if_t<std::is_same_v<IdentifierType, juce::Identifier>
                                   and std::is_same_v<KeyType, juce::String>, int> = 0>
        bool contains (const IdentifierType& key) const noexcept
        {
            return contains (key.toString());
        }

        /**
         * @brief Returns the type_index associated with a key.
         *
         * @param key The key to look up.
         * @return The std::type_index stored at registration time.
         *
         * @throws std::out_of_range If the key is not present in the map.
         */
        std::type_index typeOf (const KeyType& key) const
        {
            return this->at (key).type;
        }

        template <typename IdentifierType,
                  std::enable_if_t<std::is_same_v<IdentifierType, juce::Identifier>
                                   and std::is_same_v<KeyType, juce::String>, int> = 0>
        std::type_index typeOf (const IdentifierType& key) const
        {
            return typeOf (key.toString());
        }
    };
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
