/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZILLA_GFX_COMPOSITOR_H
#define MOZILLA_GFX_COMPOSITOR_H

#include "mozilla/gfx/Rect.h"
#include "mozilla/gfx/Matrix.h"
#include "gfxMatrix.h"
#include "Layers.h"
#include "mozilla/layers/TextureHost.h"


/**
 * Different elements of a web pages are rendered into separate "layers" before
 * they are flattened into the final image that is brought to the screen.
 * See Layers.h for more informations about layers and why we use retained
 * structures.
 * Most of the documentation for layers is directly in the source code in the
 * form of doc comments. An overview can also be found in the the wiki:
 * https://wiki.mozilla.org/Gecko:Overview#Graphics
 *
 *
 * # Main interfaces and abstractions
 *
 *  - Layer, ShadowableLayer and ShadowLayer
 *    (see Layers.h and ipc/ShadowLayers.h)
 *  - CompositableClient and CompositableHost
 *    (client/CompositableClient.h composite/CompositableHost.h)
 *  - TextureClient and TextureHost
 *    (client/TextureClient.h composite/TextureHost.h)
 *  - TextureSource
 *    (composite/TextureHost.h)
 *  - Forwarders
 *    (ipc/CompositableForwarder.h ipc/ShadowLayers.h)
 *  - Compositor
 *    (this file)
 *  - IPDL protocols
 *    (.ipdl files under the gfx/layers/ipc directory)
 *
 * The *Client and Shadowable* classes are always used on the content thread.
 * Forwarders are always used on the content thread.
 * The *Host and Shadow* classes are always used on the compositor thread.
 * Compositors, TextureSource, and Effects are always used on the compositor
 * thread.
 * Most enums and constants are declared in LayersTypes.h and CompositorTypes.h
 *
 *
 * # Texture transfer
 *
 * Most layer classes own a Compositable plus some extra information like
 * transforms and clip rects. They are platform independent.
 * Compositable classes manipulate Texture objects and are reponsible for
 * things like tiling, buffer rotation or double buffering. Compositables
 * are also platform-independent. Examples of compositable classes are:
 *  - ImageClient
 *  - CanvasClient
 *  - ContentHost
 *  - etc.
 * Texture classes (TextureClient and TextureHost) are thin abstractions over
 * platform-dependent texture memory. They are maniplulated by compositables
 * and don't know about buffer rotations and such. The purposes of TextureClient
 * and TextureHost are to synchronize, serialize and deserialize texture data.
 * TextureHosts provide access to TextureSources that are views on the
 * Texture data providing the necessary api for Compositor backend to composite
 * them.
 * 
 * Compositable and Texture clients and hosts are created using factory methods.
 * They should only be created by using their constructor in exceptional
 * circumstances. The factory methods are located:
 *    TextureClient       - CompositableClient::CreateTextureClient
 *    TextureHost         - TextureHost::CreateTextureHost, which calls a
 *                          platform-specific function, e.g., CreateTextureHostOGL
 *    CompositableClient  - in the appropriate subclass, e.g.,
 *                          CanvasClient::CreateCanvasClient
 *    CompositableHost    - CompositableHost::Create
 *
 *
 * # IPDL
 *
 * If off-main-thread compositing (OMTC) is enabled, compositing is performed
 * in a dedicated thread. In some setups compositing happens in a dedicated
 * process. Documentation may refer to either the compositor thread or the
 * compositor process.
 * See explanations in ShadowLayers.h
 *
 *
 * # Backend implementations
 *
 * Compositor backends like OpenGL or flavours of D3D live in their own directory
 * under gfx/layers/. To add a new backend, implement at least the following
 * interfaces:
 * - Compositor (ex. CompositorOGL)
 * - TextureHost (ex. TextureImageTextureHost)
 * Depending on the type of data that needs to be serialized, you may need to
 * add specific TextureClient implementations.
 */

class gfxContext;
class nsIWidget;

namespace mozilla {
namespace gfx {
class DrawTarget;
}

namespace layers {

struct Effect;
struct EffectChain;
class Image;
class ISurfaceAllocator;

enum SurfaceInitMode
{
  INIT_MODE_NONE,
  INIT_MODE_CLEAR,
  INIT_MODE_COPY
};

/**
 * Common interface for compositor backends.
 *
 * Compositor provides a cross-platform interface to a set of operations for
 * compositing quads. Compositor knows nothing about the layer tree. It must be
 * told everything about each composited quad - contents, location, transform,
 * opacity, etc.
 *
 * In theory it should be possible for different widgets to use the same
 * compositor. In practice, we use one compositor per window.
 *
 * # Usage
 *
 * For an example of a user of Compositor, see LayerManagerComposite
 *
 * Initialisation: create a Compositor object, call Initialize().
 *
 * Destruction: destroy any resources associated with the compositor, call
 * Destroy(), delete the Compositor object.
 *
 * Composition:
 *  call BeginFrame,
 *  for each quad to be composited:
 *    call MakeCurrent if necessary (not necessary if no other context has been
 *      made current),
 *    take care of any texture upload required to composite the quad, this step
 *      is backend-dependent,
 *    construct an EffectChain for the quad,
 *    call DrawQuad,
 *  call EndFrame.
 * If the user has to stop compositing at any point before EndFrame, call
 * AbortFrame.
 * If the compositor is usually used for compositing but compositing is
 * temporarily done without the compositor, call EndFrameForExternalComposition
 * after compositing each frame so the compositor can remain internally
 * consistent.
 *
 * By default, the compositor will render to the screen, to render to a target,
 * call SetTargetContext or SetRenderTarget, the latter with a target created
 * by CreateRenderTarget or CreateRenderTargetFromSource.
 *
 * The target and viewport methods can be called before any DrawQuad call and
 * affect any subsequent DrawQuad calls. 
 */
class Compositor : public RefCounted<Compositor>
{
public:
  Compositor()
    : mCompositorID(0)
  {
    MOZ_COUNT_CTOR(Compositor);
  }
  virtual ~Compositor() {
    MOZ_COUNT_DTOR(Compositor);
  }

  virtual bool Initialize() = 0;
  virtual void Destroy() = 0;

  /**
   * Request a texture host identifier that may be used for creating textures
   * across process or thread boundaries that are compatible with this
   * compositor.
   */
  virtual TextureFactoryIdentifier
    GetTextureFactoryIdentifier() = 0;

  /**
   * Properties of the compositor.
   */
  virtual bool CanUseCanvasLayerForSize(const gfxIntSize &aSize) = 0;
  virtual int32_t GetMaxTextureSize() const = 0;

  /**
   * Set the target for rendering. Results will have been written to aTarget by
   * the time that EndFrame returns.
   *
   * If this method is not used, or we pass in nullptr, we target the compositor's
   * usual swap chain and render to the screen.
   */
  virtual void SetTargetContext(gfxContext *aTarget) = 0;

  /**
   * Make this compositor's rendering context the current context for the
   * underlying graphics API. This may be a global operation, depending on the
   * API. Our context will remain the current one until someone else changes it.
   *
   * Clients of the compositor should call this at the start of the compositing
   * process, it might be required by texture uploads etc.
   *
   * If aForce is true, then we will (re-)set our context on the underlying API
   * even if it is already the current context.
   */
  virtual void MakeCurrent(bool aForce = false) = 0;

  /**
   * Modifies the TextureIdentifier in aInfo to a more reliable kind. For use by
   * when creating texture hosts/clients. If the desired texture cannot be
   * created, this method allows the compositor to suggest a less desirable, but
   * more reliable kind of texture.
   */
  virtual void FallbackTextureInfo(TextureInfo& aInfo) {}

  /**
   * This creates a Surface that can be used as a rendering target by this
   * compositor.
   */
  virtual TemporaryRef<CompositingRenderTarget>
  CreateRenderTarget(const gfx::IntRect &aRect, SurfaceInitMode aInit) = 0;

  /**
   * Creates a Surface that can be used as a rendering target by this
   * compositor, and initializes the surface by copying from aSource.
   * If aSource is null, then the current screen buffer is used as source.
   */
  virtual TemporaryRef<CompositingRenderTarget>
  CreateRenderTargetFromSource(const gfx::IntRect &aRect,
                               const CompositingRenderTarget* aSource) = 0;

  /**
   * Sets the given surface as the target for subsequent calls to DrawQuad.
   * Passing nullptr as aSurface sets the screen as the target.
   */
  virtual void SetRenderTarget(CompositingRenderTarget *aSurface) = 0;

  /**
   * Returns the current target for rendering. Will return null if we are
   * rendering to the screen.
   */
  virtual CompositingRenderTarget*
  GetCurrentRenderTarget() = 0;

  /**
   * Mostly the compositor will pull the size from a widget and this method will
   * be ignored, but compositor implementations are free to use it if they like.
   */
  virtual void SetRenderTargetSize(int aWidth, int aHeight) = 0;

  /**
   * Tell the compositor to actually draw a quad. What to do draw and how it is
   * drawn is specified by aEffectChain. aRect is the quad to draw, in user space.
   * aTransform transforms from user space to screen space. aOffset is the
   * offset of the render target from 0,0 of the screen. If texture coords are
   * required, these will be in the primary effect in the effect chain.
   */
  virtual void DrawQuad(const gfx::Rect &aRect, const gfx::Rect *aClipRect,
                        const EffectChain &aEffectChain,
                        gfx::Float aOpacity, const gfx::Matrix4x4 &aTransform,
                        const gfx::Point &aOffset) = 0;

  /**
   * Start a new frame.
   * If aClipRectIn is null, sets *aClipRectOut to the screen dimensions. 
   */
  virtual void BeginFrame(const gfx::Rect *aClipRectIn,
                          const gfxMatrix& aTransform,
                          const gfx::Rect& aRenderBounds,
                          gfx::Rect *aClipRectOut = nullptr) = 0;

  /**
   * Flush the current frame to the screen and tidy up.
   */
  virtual void EndFrame(const gfxMatrix& aTransform) = 0;

  /**
   * Post-rendering stuff if the rendering is done outside of this Compositor
   * e.g., by Composer2D
   */
  virtual void EndFrameForExternalComposition(const gfxMatrix& aTransform) = 0;

  /**
   * Tidy up if BeginFrame has been called, but EndFrame won't be.
   */
  virtual void AbortFrame() = 0;

  /**
   * Setup the viewport and projection matrix for rendering
   * to a window of the given dimensions.
   */
  virtual void PrepareViewport(int aWidth, int aHeight,
                               const gfxMatrix& aWorldTransform) = 0;

  /**
   * The compositor maintains (or appears to maintain) a viewport stack. There
   * is an implicit current viewport. SaveViewport saves the current viewport
   * to the stack. RestoreViewport pops a viewport from the stack, makes that
   * the current viewport, and returns a rectangle which defines that viewport.
   */
  virtual void SaveViewport() = 0;
  virtual gfx::IntRect RestoreViewport() = 0;

  /**
   * Whether textures created by this compositor can receive partial updates.
   */
  virtual bool SupportsPartialTextureUpdate() = 0;

#ifdef MOZ_DUMP_PAINTING
  virtual const char* Name() const = 0;
#endif // MOZ_DUMP_PAINTING


  /**
   * Each Compositor has a unique ID.
   * This ID is used to keep references to each Compositor in a map accessed
   * from the compositor thread only, so that async compositables can find
   * the right compositor parent and schedule compositing even if the compositor
   * changed.
   */
  uint32_t GetCompositorID() const
  {
    return mCompositorID;
  }
  void SetCompositorID(uint32_t aID)
  {
    NS_ASSERTION(mCompositorID==0, "The compositor ID must be set only once.");
    mCompositorID = aID;
  }

  /**
   * Notify the compositor that a layers transaction has occured. This is only
   * used for FPS information at the moment.
   * XXX: surely there is a better way to do this?
   */
  virtual void NotifyLayersTransaction() = 0;

  /**
   * Notify the compositor that composition is being paused. This allows the
   * compositor to temporarily release any resources.
   * Between calling Pause and Resume, compositing may fail.
   */
  virtual void Pause() {}
  /**
   * Notify the compositor that composition is being resumed. The compositor
   * regain any resources it requires for compositing.
   * Returns true if succeeded.
   */
  virtual bool Resume() { return true; }

  // XXX I expect we will want to move mWidget into this class and implement
  // these methods properly.
  virtual nsIWidget* GetWidget() const { return nullptr; }
  virtual nsIntSize* GetWidgetSize() { return nullptr; }

  /**
   * We enforce that there can only be one Compositor backend type off the main
   * thread at the same time. The backend type in use can be checked with this
   * static method. We need this for creating texture clients/hosts etc. when we
   * don't have a reference to a Compositor.
   */
  static LayersBackend GetBackend();
protected:
  uint32_t mCompositorID;
  static LayersBackend sBackend;
};


}
}

#endif /* MOZILLA_GFX_COMPOSITOR_H */
