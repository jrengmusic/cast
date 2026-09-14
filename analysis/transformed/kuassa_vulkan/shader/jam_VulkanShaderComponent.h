/** @file jam_VulkanShaderComponent.h
 *  @brief Self-managed component rendering a compiled jam::VulkanShader through
 *         its own repaint timer.
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief Dumb, self-managed component that paints a compiled jam::VulkanShader.
 *
 *  Owns the installed VulkanShader and its own repaint timer. Carries no config or
 *  frame-state knowledge of its own -- the engine stamps time/frame uniforms at
 *  record time (jam::render()) -- every value it needs arrives
 *  explicitly through setShader(), invoked by the owner on every relevant
 *  change (initial load and hot-reload alike, one code path).
 *
 *  Transparent -- paint() only emits the installed shader when one exists;
 *  otherwise it draws nothing, letting whatever paints beneath it show through.
 *
 *  Intercepts mouse clicks (not child interception) to maintain iMouse per
 *  Shadertoy's own sign-encoding convention (see VulkanShaderInstance::
 *  stampUniforms()'s doc comment, jam_VulkanShaderInstance.h, for the exact
 *  encoding this component reproduces). mouseState is transient per-component
 *  render state -- a calculation input synced from live interaction, not
 *  machinery state (BLESSED Stateless's DSP-parameter carve-out).
 *
 *  @par Configured button routing (mouseDown()/mouseDrag()/mouseUp()/
 *  mouseWheelMove() below)
 *  setMouseConfig() resolves which physical button drives iMouse, which
 *  drives this component's own orbit-capture, and which resets the camera
 *  on a plain click -- map::MouseButton::none for any of the
 *  three disables that binding, and mouseInteractionEnabled false (also set
 *  via setMouseConfig()) disables every branch below regardless of button
 *  choice. The click-vs-drag distinction that decides reset-vs-orbit for
 *  the configured buttons is mouseUp()'s own concern below. Wheel drives
 *  zoom, unchanged -- fixed, never button-configurable (Id::
 *  MouseButton has no wheel entry).
 *
 *  @par One set of overrides, two delivery paths
 *  mouseDown()/mouseDrag()/mouseUp()/mouseWheelMove() below are this
 *  component's own juce::Component overrides -- JUCE calls them directly
 *  when this component itself is the actual mouse target, and ALSO calls
 *  them as juce::MouseListener callbacks whenever an owner registers this
 *  component as a deep mouse listener over its own subtree (e.g. a host
 *  application view's own ctor: addMouseListener (&background, true)) -- the seam such an
 *  owner uses to feed camera's own orbit/reset/zoom input from elsewhere in
 *  its tree, without stealing the event from whichever component actually
 *  owns it -- see each method's own doc comment. The full gesture state
 *  machine (lastDragPosition, lastOrbitDragPosition, resetButtonDragged
 *  below) is owned entirely here, alongside camera itself and hasMesh()'s
 *  own gating -- the gesture state belongs to the object that owns what it
 *  drives, never the deep-listener owner.
 */
class VulkanShaderComponent
    : public juce::Component
    , private juce::Timer
{
public:
    /** @brief Constructs the component: transparent, non-opaque, intercepting
     *  its own mouse clicks (not its children's) to feed iMouse.
     */
    VulkanShaderComponent()
    {
        setOpaque (false);
        setInterceptsMouseClicks (true, false);
    }

    /** @brief Configured iMouse button only: captures the click-start position
     *  into mouseState -- current position (.xy) and click-start position
     *  (abs(.zw)) both set to the Y-flipped event position, button-held/
     *  click-start-frame signs applied. Configured orbit button only: records
     *  @p e's own position as this drag's own starting point for camera's own
     *  addOrbitDelta() feed (mouseDrag()'s own doc comment) — entirely
     *  separate from the iMouse state above (this class's own doc comment:
     *  camera is NOT iMouse). Both gated on mouseInteractionEnabled
     *  (setMouseConfig()'s own doc comment) -- disabled, neither branch runs.
     *  Unconditionally, every call, regardless of mouseInteractionEnabled or
     *  button: captures @p e's own position as lastOrbitDragPosition below,
     *  the starting point for the next mouseDrag() delta along the
     *  deep-listener-forwarded path (this class's own doc comment). On a
     *  configured resetButtonConfig down (gated on mouseInteractionEnabled),
     *  clears resetButtonDragged so mouseUp() below can tell a click of that
     *  button from a drag of it.
     *  @param e  The mouse-down event, in this component's own coordinates.
     */
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (mouseInteractionEnabled and isDown (e.mods, imouseButtonConfig))
        {
            const float flippedY { static_cast<float> (getHeight()) - e.position.y };

            mouseState.at (0) = e.position.x;
            mouseState.at (1) = flippedY;
            mouseState.at (2) = e.position.x * buttonHeldSign;
            mouseState.at (3) = flippedY * clickStartFrameSign;
        }

        if (mouseInteractionEnabled and isDown (e.mods, orbitButtonConfig))
            lastDragPosition = e.position;

        lastOrbitDragPosition = e.position;

        if (mouseInteractionEnabled and isDown (e.mods, resetButtonConfig))
            resetButtonDragged = false;
    }

    /** @brief Configured iMouse button only: updates the current position
     *  (.xy) to the Y-flipped event position while the button remains held;
     *  click-start magnitude (abs(.zw)) is unchanged, only .w's sign moves off
     *  the click-start frame. Configured orbit button only: feeds camera's
     *  own addOrbitDelta() with this event's own motion since the last
     *  mouseDown()/mouseDrag() call — jam::VulkanOrbitCamera's own
     *  arcball-orbit input, riding this SAME mouseDrag() event
     *  (jam_VulkanOrbitCamera.h's own class doc comment for why this is NOT
     *  iMouse). Both gated on mouseInteractionEnabled, same as mouseDown()
     *  above. Independently, gated on mouseInteractionEnabled and hasMesh()
     *  below (the active background shader's own mesh declaration): on a
     *  configured orbitButtonConfig down, feeds camera's own addOrbitDelta()
     *  a SECOND time with this event's own motion since the last
     *  mouseDown()/mouseDrag() call, tracked via lastOrbitDragPosition below
     *  (the deep-listener-forwarded path's own delta tracking, kept separate
     *  from lastDragPosition above -- see lastOrbitDragPosition's own doc
     *  comment for why). Whenever @p e's own button is resetButtonConfig
     *  (whether or not it is ALSO the orbit button), marks resetButtonDragged
     *  so the matching mouseUp() knows this was a drag of the reset button,
     *  not a click -- tracked independently of the orbit branch above since
     *  orbitButtonConfig and resetButtonConfig may differ.
     *  lastOrbitDragPosition below is updated unconditionally on every call.
     *  @param e  The mouse-drag event, in this component's own coordinates.
     */
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (mouseInteractionEnabled and isDown (e.mods, imouseButtonConfig))
        {
            const float flippedY { static_cast<float> (getHeight()) - e.position.y };

            mouseState.at (0) = e.position.x;
            mouseState.at (1) = flippedY;
            mouseState.at (3) = std::abs (mouseState.at (3)) * subsequentFrameSign;
        }

        if (mouseInteractionEnabled and isDown (e.mods, orbitButtonConfig))
        {
            camera.addOrbitDelta (
                e.position.x - lastDragPosition.x, e.position.y - lastDragPosition.y);
            lastDragPosition = e.position;
        }

        if (mouseInteractionEnabled)
        {
            if (hasMesh() and isDown (e.mods, orbitButtonConfig))
                camera.addOrbitDelta (
                    e.position.x - lastOrbitDragPosition.x, e.position.y - lastOrbitDragPosition.y);

            // Tracked independently of the orbit branch above -- orbit and
            // reset may be configured to different buttons, so a drag
            // of the reset button alone (not the orbit
            // button) must still disqualify mouseUp()'s own click-reset
            // below.
            if (isDown (e.mods, resetButtonConfig))
                resetButtonDragged = true;
        }

        lastOrbitDragPosition = e.position;
    }

    /** @brief Configured iMouse button only: marks the button released -- .xy
     *  stays frozen at its last dragged position (Shadertoy convention --
     *  never zeroed here), .z's sign flips to released while its magnitude
     *  (click-start X) is preserved, .w stays on the not-click-start-frame
     *  sign. @p e's own mods reflects the button state just BEFORE this
     *  release (juce::MouseEvent::mods's own doc comment, juce_MouseEvent.h:
     *  "When used for mouse-up events, this will indicate the state of the
     *  mouse buttons just before they were released, so that you can tell
     *  which button they let go of."), so isDown() here is checking "this
     *  mouseUp is the configured iMouse button's own release." Gated on
     *  mouseInteractionEnabled, same as mouseDown() above.
     *  Independently, configured resetButtonConfig only (same @p e's own
     *  mods reasoning, checking that button's own release instead): resolves
     *  the click-vs-drag distinction -- when resetButtonDragged was never set
     *  by mouseDrag() above (a click of this button, no intervening drag of
     *  the SAME button), resets camera to its own defaults via
     *  camera.reset(); a preceding drag of this same button already orbited
     *  the camera via mouseDrag() above, so no reset fires on that release.
     *  Gated on mouseInteractionEnabled and hasMesh() (same gate as
     *  mouseDown()/mouseDrag() above).
     *  @param e  The mouse-up event, in this component's own coordinates.
     */
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (mouseInteractionEnabled and isDown (e.mods, imouseButtonConfig))
        {
            mouseState.at (2) = std::abs (mouseState.at (2)) * buttonReleasedSign;
            mouseState.at (3) = std::abs (mouseState.at (3)) * subsequentFrameSign;
        }

        if (mouseInteractionEnabled and hasMesh()
            and isDown (e.mods, resetButtonConfig) and not resetButtonDragged)
            camera.reset();
    }

    /** @brief Configures mouse-driven interaction -- resolved values only,
     *  forwarded by the owner from its own resolved mouse-interaction config
     *  — the same "resolved value only" contract as setShader()'s
     *  own opacity/resolutionScale/frameRate parameters, never a config
     *  string re-parsed here). @p enabled false disables every branch in
     *  mouseDown()/mouseDrag()/mouseUp()/mouseWheelMove() above -- ONE gate
     *  for both the direct-event and deep-listener-forwarded paths,
     *  regardless of button.
     *  @param enabled       False disables every branch in mouseDown()/
     *                       mouseDrag()/mouseUp()/mouseWheelMove() above,
     *                       regardless of button.
     *  @param imouseButton  Physical button whose mouseDown()/mouseDrag()/
     *                       mouseUp() above stamps mouseState
     *                       (map::MouseButton::none disables
     *                       iMouse stamping the same way enabled=false does).
     *  @param orbitButton   Physical button this component's own
     *                       mouseDown()/mouseDrag() above captures twice --
     *                       once via lastDragPosition (direct-event path),
     *                       once via lastOrbitDragPosition (deep-listener-
     *                       forwarded path) -- (map::MouseButton::
     *                       none disables both the same way
     *                       enabled=false does).
     *  @param resetButton   Physical button mouseDown()/mouseDrag()/mouseUp()
     *                       above track for the click-vs-drag distinction
     *                       that resets camera on a plain click of this
     *                       button (map::MouseButton::none
     *                       disables the reset gesture the same way
     *                       enabled=false does).
     */
    void setMouseConfig (bool enabled,
                         map::MouseButton::value imouseButton,
                         map::MouseButton::value orbitButton,
                         map::MouseButton::value resetButton) noexcept
    {
        mouseInteractionEnabled = enabled;
        imouseButtonConfig = imouseButton;
        orbitButtonConfig = orbitButton;
        resetButtonConfig = resetButton;
    }

    /** @brief Installs (or, with nullptr, clears) the currently rendered shader.
     *
     *  Non-null: installs newShader, stores opacity/resolutionScale, (re)starts
     *  the repaint timer at frameRateHz, and repaints. Null: discards any
     *  installed shader, stops the repaint timer, and repaints once to clear
     *  the last painted frame.
     *  @param newShader        The compiled shader to install, or nullptr to clear.
     *  @param opacity          Overall blend opacity, [0, 1] — stored, applied at
     *                          every paint() until the next setShader()/setParams() call.
     *  @param resolutionScale  Intermediate-pass extent fraction, [0, 1] — stored,
     *                          applied the same way.
     *  @param frameRateHz      Repaint timer rate in Hz, applied only when
     *                          newShader is non-null.
     */
    void setShader (std::unique_ptr<VulkanShader> newShader,
                    float opacity,
                    float resolutionScale,
                    int frameRateHz)
    {
        currentOpacity = opacity;
        currentResolutionScale = resolutionScale;

        if (newShader != nullptr)
        {
            shader = std::move (newShader);
            startTimerHz (frameRateHz);
        }
        else
        {
            shader.reset();
            stopTimer();
        }

        repaint();
    }

    /** @brief Cheap parameter-only update — no recompile, no shader replace.
     *  Updates the stored opacity/resolutionScale, retimes the repaint timer
     *  to frameRateHz, and repaints. Assert-free, positive-check simple: a
     *  no-op on the installed shader itself when none is currently installed
     *  (still updates the stored values and retimes, harmlessly, for whenever
     *  a shader is next installed).
     *  @param opacity          New overall blend opacity, [0, 1].
     *  @param resolutionScale  New intermediate-pass extent fraction, [0, 1].
     *  @param frameRateHz      New repaint timer rate in Hz.
     */
    void setParams (float opacity, float resolutionScale, int frameRateHz)
    {
        currentOpacity = opacity;
        currentResolutionScale = resolutionScale;
        startTimerHz (frameRateHz);
        repaint();
    }

    /** @brief Emits the installed shader, if one exists, through g.
     *  @param g  JUCE graphics context for this paint pass.
     */
    void paint (juce::Graphics& g) override
    {
        if (shader != nullptr)
            render (g, *shader, currentOpacity, currentResolutionScale, mouseState, camera);
    }

    /** @brief Forwards @p details' own vertical delta to camera's own
     *  addZoomDelta(), gated on mouseInteractionEnabled and hasMesh() below
     *  (the active background shader's own mesh declaration -- the
     *  data-side signal a mesh-backed orbit camera exists to feed at all).
     *  Zoom's own trigger (the wheel) is fixed, never button-configurable --
     *  only mouseInteractionEnabled gates it.
     *  @param details  The wheel movement's own delta/reversed/smooth/inertial state.
     */
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& details) override
    {
        if (mouseInteractionEnabled and hasMesh())
            camera.addZoomDelta (details.deltaY);
    }

private:
    /** @brief Repaint timer callback -- runs only while shader is installed
     *  (started/stopped by setShader(), never idle when no shader exists).
     */
    void timerCallback() override { repaint(); }

    /** @brief Returns true if the currently installed shader declares a mesh
     *  -- VulkanShader::meshPath non-empty, the data-side signal mouseDrag()/
     *  mouseUp()/mouseWheelMove() above gate the mesh-only
     *  orbit/reset/zoom gestures on -- the active shader's own mesh
     *  declaration, distinct from jam::VulkanShaderInstance::hasMesh(),
     *  the GPU-side equivalent read only from inside the render() call this
     *  component's own paint() reaches. False when no shader is installed at
     *  all. */
    bool hasMesh() const noexcept { return shader != nullptr and shader->meshPath.isNotEmpty(); }

    /** @brief Returns true when @p button is currently held per @p mods --
     *  map::MouseButton::none never matches (the "disabled binding"
     *  sentinel). Single source of truth for button-agnostic mouse dispatch
     *  shared by mouseDown()/mouseDrag()/mouseUp() above.
     *  @param mods    Modifier keys from the mouse event under test.
     *  @param button  Configured button to test @p mods against.
     */
    static bool isDown (const juce::ModifierKeys& mods, map::MouseButton::value button) noexcept
    {
        if (button == map::MouseButton::left) return mods.isLeftButtonDown();
        if (button == map::MouseButton::middle) return mods.isMiddleButtonDown();
        if (button == map::MouseButton::right) return mods.isRightButtonDown();

        return false;
    }

    //==============================================================================
    /** @brief The currently installed shader, or nullptr when none is installed. */
    std::unique_ptr<VulkanShader> shader;

    /** @brief Overall blend opacity, [0, 1] — stamped into every paint() draw
     *  regardless of whether shader was installed by setShader() or updated
     *  by the cheaper setParams(). */
    float currentOpacity { 1.0f };

    /** @brief Intermediate-pass extent fraction, [0, 1] — see currentOpacity's
     *  doc comment; same update/read pattern. */
    float currentResolutionScale { 1.0f };

    //==============================================================================
    /** @brief Sign applied to .z (via multiplication) while the mouse button
     *  is held -- positive, per Shadertoy's own iMouse convention. */
    static constexpr float buttonHeldSign { 1.0f };

    /** @brief Sign applied to .z once the mouse button is released --
     *  negative, per Shadertoy's own iMouse convention. */
    static constexpr float buttonReleasedSign { -1.0f };

    /** @brief Sign applied to .w on the exact frame a click starts --
     *  positive, per Shadertoy's own iMouse convention. */
    static constexpr float clickStartFrameSign { 1.0f };

    /** @brief Sign applied to .w on every frame after the click-start frame
     *  (dragging or released) -- negative, per Shadertoy's own iMouse
     *  convention. */
    static constexpr float subsequentFrameSign { -1.0f };

    /** @brief This execution's sign-encoded iMouse value -- transient
     *  per-component render state fed to render() every paint() (calculation
     *  input synced from live interaction, not machinery state; see this
     *  class's own doc comment). .xy is the current position while a button
     *  is held (frozen at its last value once released); abs(.zw) is the
     *  click-start position; sign(.z) encodes held/released
     *  (buttonHeldSign/buttonReleasedSign); sign(.w) encodes click-start-frame
     *  vs. every frame after (clickStartFrameSign/subsequentFrameSign). All
     *  four components are zero until the first mouseDown(). */
    std::array<float, 4> mouseState { 0.0f, 0.0f, 0.0f, 0.0f };

    //==============================================================================
    /** @brief The last configured-orbit-button mouseDown()/mouseDrag() event
     *  position — camera's own addOrbitDelta() feed needs a per-frame DELTA,
     *  unlike mouseState's own absolute-position convention, so this
     *  component tracks the previous event's position itself. Updated only
     *  when orbitButtonConfig is held -- see lastOrbitDragPosition below for
     *  the separate, unconditionally-updated field mouseDown()/mouseDrag()
     *  above use instead (the deep-listener owner's own former forwarded-path
     *  update semantics, preserved exactly here). */
    juce::Point<float> lastDragPosition { 0.0f, 0.0f };

    /** @brief This component's own interactive orbit camera — unconditionally
     *  owned and always fed to render(), same "always supplied, read only if
     *  the shader cares" contract as mouseState above (jam::
     *  VulkanOrbitCamera.h's own class doc comment: this is NOT iMouse, a
     *  meshless shader's own render() call simply never reads it). Fed twice
     *  by mouseDrag() above -- once via lastDragPosition (this component's
     *  own former direct-event path), once via lastOrbitDragPosition (this
     *  component's own former deep-listener-forwarded path, hasMesh()-gated)
     *  -- ONE camera, two feed calls per drag event now that both paths
     *  share the same override (see this class's own doc comment). */
    VulkanOrbitCamera camera;

    /** @brief The last mouseDown()/mouseDrag() event position, updated
     *  unconditionally on every call regardless of mouseInteractionEnabled
     *  or button (the deep-listener owner's own former forwarded-path update
     *  semantics, preserved exactly here). Kept separate from lastDragPosition above
     *  rather than shared: that field only updates when orbitButtonConfig is
     *  held, an update rule this field never followed even before the two
     *  paths shared one override (this class's own doc comment). */
    juce::Point<float> lastOrbitDragPosition { 0.0f, 0.0f };

    /** @brief Whether a drag of resetButtonConfig below occurred since the
     *  last mouseDown() of that SAME button -- cleared there, set by
     *  mouseDrag() on any motion of that button, read by mouseUp() to
     *  distinguish a click of that button (reset the camera) from a drag of
     *  it (already orbited, no reset). Button-agnostic -- scoped to
     *  whichever button graphics.mouse.reset currently names rather than a
     *  hardcoded middle button. */
    bool resetButtonDragged { false };

    //==============================================================================
    /** @brief Master gate for mouseDown()/mouseDrag()/mouseUp()/
     *  mouseWheelMove() above -- set via setMouseConfig(). False disables
     *  every branch in both the direct-event and deep-listener-forwarded
     *  paths regardless of button. Defaults to today's pre-config behaviour
     *  (always on) so a caller that never invokes setMouseConfig() sees
     *  unchanged behaviour. */
    bool mouseInteractionEnabled { true };

    /** @brief Physical button mouseDown()/mouseDrag()/mouseUp() above tests
     *  for iMouse stamping — set via setMouseConfig(). Defaults to
     *  map::MouseButton::left, matching this class's own
     *  pre-config hardcoded convention (Shadertoy's own primary-button rule). */
    map::MouseButton::value imouseButtonConfig { map::MouseButton::left };

    /** @brief Physical button mouseDown()/mouseDrag() above tests for
     *  orbit-capture (both the lastDragPosition and lastOrbitDragPosition
     *  feeds above) — set via setMouseConfig(). Defaults to Id::
     *  MouseButton::middle, matching this class's own pre-config
     *  hardcoded convention. */
    map::MouseButton::value orbitButtonConfig { map::MouseButton::middle };

    /** @brief Physical button mouseDown()/mouseDrag()/mouseUp() above test
     *  for the click-vs-drag reset gesture -- set via setMouseConfig().
     *  Defaults to map::MouseButton::middle, matching
     *  orbitButtonConfig's own default above (the two configured buttons may
     *  be the same -- click-vs-drag disambiguation -- or differ). */
    map::MouseButton::value resetButtonConfig { map::MouseButton::middle };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanShaderComponent)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam