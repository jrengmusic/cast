/** @file jam_VulkanOrbitCamera.h
 *  @brief Pure math+interaction turnkey orbit camera for the OBJ mesh render
 *         path. No vk:: handles — glm only.
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief Turnkey arcball-orbit camera — perspective + view + normal matrix,
 *  fed by drag deltas. Shadertoy-independent: this is NOT iMouse —
 *  it consumes the SAME mouseDrag() events jam::VulkanShaderComponent
 *  already intercepts, at that same call site, via its own addOrbitDelta()
 *  below, entirely separate from that component's iMouse sign-encoded state.
 *
 *  @par The AABB-fit seam
 *  computeAutoFitModelMatrix() is a STATIC, pure function of a VulkanMesh's AABB —
 *  called exactly ONCE, by jam::VulkanShaderInstance::build() (which owns
 *  the real jam::VulkanMesh and its AABB), to bake a per-VulkanShader MODEL matrix
 *  once at build time. This is deliberately NOT an instance method threading
 *  the AABB into a live orbiting VulkanOrbitCamera: jam::VulkanShaderComponent (the
 *  class that owns the INTERACTIVE VulkanOrbitCamera instance orbit/view/projection
 *  math below feeds) never sees a jam::VulkanMesh or its AABB at all — the
 *  component-to-execution seam has no channel for it (render() only carries a
 *  VulkanShader + opacity/resolutionScale/mouse/camera, never a built VulkanShaderInstance
 *  or its VulkanMesh). Baking the auto-fit into the MODEL matrix instead means every
 *  mesh, regardless of its real-world size, already renders normalized into a
 *  comfortable unit-ish cube around the origin by the time ANY camera ever
 *  sees it — so the interactive VulkanOrbitCamera below needs no AABB backchannel:
 *  it always orbits the same fixed target at the same fixed distance,
 *  correct for every mesh because the model matrix already did the fitting.
 *  (the accepted fallback — the alternative, threading the AABB back from
 *  VulkanShaderInstance to VulkanShaderComponent
 *  post-build, demands a new cross-layer channel outside this task's locked
 *  scope.)
 *
 *  @par Why getNormalMatrix() needs no model term
 *  computeAutoFitModelMatrix()'s own model matrix is translate-then-uniform-
 *  scale ONLY (see its own doc comment) — never rotation, never non-uniform
 *  scale. The textbook normal-matrix correction (inverse-transpose) of a
 *  uniform-scale-only matrix is itself a uniform scalar multiple of identity,
 *  which cannot change any normal's DIRECTION (only its length, discarded by
 *  the fragment shader's own normalize()) — so folding the model term into
 *  the normal matrix below would be mathematically inert. getNormalMatrix()
 *  therefore computes the inverse-transpose of the VIEW matrix's own
 *  upper-3x3 alone, exactly as this class's own header instruction specifies
 *  ("normal matrix (inverse-transpose upper-3x3)"), and is already the
 *  complete, correct answer given the model matrix's own restricted shape.
 *
 *  Zoom rides the mouse wheel — addZoomDelta() below, its own forwarding
 *  seam mirroring addOrbitDelta()'s.
 */
class VulkanOrbitCamera
{
public:
    /** @brief Default distance from the orbit target to the eye — distance's
     *  own starting value and reset() target. The model matrix already
     *  normalizes every mesh to fit within autoFitTargetRadius (see
     *  computeAutoFitModelMatrix()'s doc comment), so this one distance
     *  frames any mesh comfortably regardless of its real-world size, before
     *  addZoomDelta() ever moves it away. */
    static constexpr float defaultDistance { 3.0f };

    /** @brief Initial yaw, radians (0) — at yaw = 0, pitch = 0,
     *  getViewMatrix()'s own eyeDirection is (0, 0, 1), placing the eye at
     *  (0, 0, +defaultDistance) looking toward the origin, i.e. toward -Z:
     *  the standard OBJ/glTF authoring convention treats -Z as a mesh's own
     *  front-facing direction, so this unoffset placement already looks at
     *  the mesh's front — the Sascha Willems-standard right-handed-lookAt +
     *  Y-flipped-projection Vulkan/glm camera (see jam_vulkan.h's own
     *  GLM_FORCE_LEFT_HANDED doc comment for why right-handed, not left, is
     *  the correct chirality here). No yaw offset is needed. */
    static constexpr float defaultYawRadians { 0.0f };

    /** @brief Initial pitch, radians (0) — level with the orbit target,
     *  matching defaultYawRadians's own unoffset placement above (this
     *  class's own doc comment for why (0, 0) already looks at the mesh's
     *  front). */
    static constexpr float defaultPitchRadians { 0.0f };

    /** @brief Vertical field of view, radians (45 degrees) — a comfortable,
     *  non-distorting default for a turnkey preview camera. */
    static constexpr float fieldOfViewRadians { 0.7853981634f }; // glm::radians (45.0f), evaluated at compile time

    /** @brief Near/far clip planes — generous bounds around defaultDistance
     *  and autoFitTargetRadius (computeAutoFitModelMatrix()'s own doc
     *  comment), never tight enough to clip a turnkey-framed mesh. */
    static constexpr float nearPlane { 0.05f };
    static constexpr float farPlane { 100.0f };

    /** @brief Radians of orbit rotation per pixel of drag delta — addOrbitDelta()'s
     *  own pixel-to-radians conversion factor. */
    static constexpr float orbitSensitivity { 0.01f };

    /** @brief Pitch is clamped to +/- this bound (just short of +/- 90 degrees)
     *  to avoid the arcball's own gimbal flip at the poles. */
    static constexpr float pitchLimitRadians { 1.553343034f }; // glm::radians (89.0f), evaluated at compile time

    /** @brief The target radius computeAutoFitModelMatrix() normalizes every
     *  mesh's own largest AABB half-extent to — see that method's own doc
     *  comment for the exact formula. */
    static constexpr float autoFitTargetRadius { 1.0f };

    /** @brief Multiplicative conversion factor from one wheel event's own
     *  delta (juce::MouseWheelDetails::deltaY's own units) into the exponent
     *  addZoomDelta() scales distance by — the zoom-equivalent of
     *  orbitSensitivity's own pixel-to-radians conversion factor above,
     *  chosen so a single wheel step moves distance a noticeable but gentle
     *  amount rather than a jarring jump. */
    static constexpr float zoomSensitivity { 1.0f };

    /** @brief distance is never allowed to close nearer than this. Every
     *  mesh is auto-fit (computeAutoFitModelMatrix()'s own doc comment) so
     *  its largest half-extent sits at autoFitTargetRadius (1.0) from the
     *  orbit target — the eye must stay outside that radius or it enters the
     *  mesh's own bounding volume, so this bound sits 0.2 past it (1.2),
     *  comfortably clear of nearPlane (0.05) so the near clip plane never
     *  crosses the mesh's own surface even at the closest permitted zoom. */
    static constexpr float minimumDistance { 1.2f };

    /** @brief distance is never allowed to widen past this — far short of
     *  farPlane (100), leaving generous headroom so the auto-fit mesh
     *  (largest half-extent at autoFitTargetRadius == 1) never approaches
     *  the far clip plane, while still keeping the mesh a visibly framed
     *  subject rather than a vanishing dot at the widest permitted zoom. */
    static constexpr float maximumDistance { 30.0f };

    VulkanOrbitCamera() = default;

    /** @brief Computes a MODEL matrix that recenters @p aabbMin/@p aabbMax's
     *  own object-space box onto the origin and uniformly scales it so its
     *  largest half-extent equals autoFitTargetRadius — jam::
     *  VulkanShaderInstance::build()'s own ONE call site (see this class's own doc
     *  comment for the full seam rationale).
     *
     *  Formula: center = (min + max) / 2; halfExtent = (max - min) / 2;
     *  scale = autoFitTargetRadius / max (halfExtent.x, halfExtent.y,
     *  halfExtent.z), or 1.0 when that largest half-extent is degenerately
     *  small (a single-point or zero-volume mesh — graceful fallback, never a
     *  divide-by-near-zero blow-up); model = Scale (scale) * Translate
     *  (-center), so a point p transforms as scale * (p - center) — center
     *  first, then scale, matching how the two glm calls compose applied to
     *  a column vector (M * p == Scale * (Translate * p)).
     *  @param aabbMin  Object-space AABB minimum corner (jam::VulkanMesh::getAabbMin()).
     *  @param aabbMax  Object-space AABB maximum corner (jam::VulkanMesh::getAabbMax()).
     *  @return The auto-fit model matrix.
     */
    static glm::mat4 computeAutoFitModelMatrix (const float aabbMin[3], const float aabbMax[3]) noexcept
    {
        const glm::vec3 minCorner { aabbMin[0], aabbMin[1], aabbMin[2] };
        const glm::vec3 maxCorner { aabbMax[0], aabbMax[1], aabbMax[2] };

        const glm::vec3 center { (minCorner + maxCorner) * 0.5f };
        const glm::vec3 halfExtent { (maxCorner - minCorner) * 0.5f };
        const float largestHalfExtent { std::max (halfExtent.x, std::max (halfExtent.y, halfExtent.z)) };

        static constexpr float degenerateExtentEpsilon { 1.0e-6f };
        const float scale { largestHalfExtent > degenerateExtentEpsilon
            ? autoFitTargetRadius / largestHalfExtent : 1.0f };

        return glm::scale (glm::mat4 { 1.0f }, glm::vec3 { scale })
             * glm::translate (glm::mat4 { 1.0f }, -center);
    }

    /** @brief Feeds one arcball-drag delta (device pixels, this frame's
     *  motion since the last call) into yaw/pitch — jam::
     *  VulkanShaderComponent::mouseDrag()'s own call site, riding the SAME
     *  mouseDrag() event its iMouse tracking already intercepts (this
     *  class's own doc comment). Pitch is clamped away from the poles;
     *  yaw wraps freely (glm::lookAt() itself needs no wrap normalization).
     *  @param deltaXPixels  Horizontal drag delta since the last call.
     *  @param deltaYPixels  Vertical drag delta since the last call.
     */
    void addOrbitDelta (float deltaXPixels, float deltaYPixels) noexcept
    {
        yawRadians += deltaXPixels * orbitSensitivity;
        pitchRadians = std::clamp (pitchRadians + deltaYPixels * orbitSensitivity,
                                   -pitchLimitRadians, pitchLimitRadians);
    }

    /** @brief Feeds one mouse-wheel event's own delta into distance —
     *  jam::VulkanShaderComponent::mouseWheelMove()'s own call site, the
     *  zoom-equivalent forwarding seam addOrbitDelta() above already
     *  establishes for drag. Exponential (scale-invariant) response:
     *  distance *= exp(-wheelDelta * zoomSensitivity), so a wheel step feels
     *  the same proportional amount whether the camera is currently close
     *  or far, unlike a fixed linear increment. Clamped to
     *  [minimumDistance, maximumDistance] (see each constant's own doc
     *  comment for the derivation) — the wheel can never zoom the eye inside
     *  the auto-fit mesh's own radius or out past a comfortably framed
     *  vanishing point.
     *  @param wheelDelta  This wheel event's own delta (juce::
     *                     MouseWheelDetails::deltaY's own convention) —
     *                     positive contracts distance (zooms in), negative
     *                     widens it (zooms out).
     */
    void addZoomDelta (float wheelDelta) noexcept
    {
        distance = std::clamp (distance * std::exp (-wheelDelta * zoomSensitivity),
                                minimumDistance, maximumDistance);
    }

    /** @brief Restores yaw, pitch, and distance to their own default
     *  constants (defaultYawRadians, defaultPitchRadians, defaultDistance)
     *  — jam::VulkanShaderComponent::mouseUp()'s own call site, the
     *  middle-click camera-reset gesture's forwarding seam. */
    void reset() noexcept
    {
        yawRadians = defaultYawRadians;
        pitchRadians = defaultPitchRadians;
        distance = defaultDistance;
    }

    /** @brief Returns the current VIEW matrix — glm::lookAt() from the
     *  current yaw/pitch-derived eye position, at the current distance
     *  (defaultDistance until addZoomDelta() or reset() change it), orbiting
     *  the origin (the auto-fit model matrix's own target — see class doc
     *  comment), up = +Y. */
    glm::mat4 getViewMatrix() const noexcept
    {
        const glm::vec3 eyeDirection {
            std::cos (pitchRadians) * std::sin (yawRadians),
            std::sin (pitchRadians),
            std::cos (pitchRadians) * std::cos (yawRadians)
        };

        static constexpr glm::vec3 orbitTarget { 0.0f, 0.0f, 0.0f };
        static constexpr glm::vec3 upAxis { 0.0f, 1.0f, 0.0f };

        return glm::lookAt (orbitTarget + eyeDirection * distance, orbitTarget, upAxis);
    }

    /** @brief Returns the current PROJECTION matrix — glm::perspective() at
     *  fieldOfViewRadians/nearPlane/farPlane (GLM_FORCE_DEPTH_ZERO_TO_ONE
     *  already configured globally, right-handed — glm's own default,
     *  jam_vulkan.h's own GLM_FORCE_LEFT_HANDED doc comment), with a manual
     *  Y-flip: glm never flips Vulkan's own clip-space Y convention (Vulkan
     *  NDC +Y points down; every glm perspective/lookAt convention assumes
     *  +Y up), so row[1][1] is negated here — the one correction every
     *  Vulkan-hpp renderer using glm's right-handed perspective family must
     *  apply itself (this is the ONLY axis correction in this class's whole
     *  view+projection chain — see jam_vulkan.h's own doc comment for why a
     *  second, left-handedness-induced correction on top of this one was the
     *  mirrored-render bug's root cause).
     *  @param aspectRatio  Viewport width / height.
     */
    glm::mat4 getProjectionMatrix (float aspectRatio) const noexcept
    {
        glm::mat4 projection { glm::perspective (fieldOfViewRadians, aspectRatio, nearPlane, farPlane) };
        projection[1][1] *= -1.0f;

        return projection;
    }

    /** @brief Returns the current NORMAL matrix — inverse-transpose of the
     *  VIEW matrix's own upper-3x3 (see class doc comment for why no MODEL
     *  term is needed here, given computeAutoFitModelMatrix()'s own
     *  uniform-scale-only shape). */
    glm::mat4 getNormalMatrix() const noexcept
    {
        return glm::mat4 { glm::inverseTranspose (glm::mat3 { getViewMatrix() }) };
    }

private:
    /** @brief Current horizontal orbit angle, radians — see addOrbitDelta().
     *  Starts at defaultYawRadians (own doc comment above), not 0 — the
     *  front-facing default orientation. */
    float yawRadians { defaultYawRadians };

    /** @brief Current vertical orbit angle, radians, clamped to
     *  +/- pitchLimitRadians — see addOrbitDelta(). Starts at
     *  defaultPitchRadians. */
    float pitchRadians { defaultPitchRadians };

    /** @brief Current distance from the orbit target to the eye — see
     *  addZoomDelta(). Starts at defaultDistance. */
    float distance { defaultDistance };
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam