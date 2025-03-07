/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 *           (C) 2004 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef RenderObject_h
#define RenderObject_h

#include "CachedImageClient.h"
#include "DocumentStyleSheetCollection.h"
#include "Element.h"
#include "FloatQuad.h"
#include "LayoutRect.h"
#include "PaintPhase.h"
#include "RenderObjectChildList.h"
#include "RenderStyle.h"
#include "ScrollBehavior.h"
#include "StyleInheritedData.h"
#include "TextAffinity.h"
#include <wtf/HashSet.h>

namespace WebCore {

class AffineTransform;
class AnimationController;
class Cursor;
class Document;
class HitTestLocation;
class HitTestResult;
class InlineBox;
class Path;
class Position;
class PseudoStyleRequest;
class RenderBoxModelObject;
class RenderInline;
class RenderBlock;
class RenderFlowThread;
class RenderGeometryMap;
class RenderLayer;
class RenderLayerModelObject;
class RenderNamedFlowThread;
class RenderTheme;
class TransformState;
class VisiblePosition;
#if ENABLE(SVG)
class RenderSVGResourceContainer;
#endif

struct PaintInfo;

enum CursorDirective {
    SetCursorBasedOnStyle,
    SetCursor,
    DoNotSetCursor
};

enum HitTestFilter {
    HitTestAll,
    HitTestSelf,
    HitTestDescendants
};

enum HitTestAction {
    HitTestBlockBackground,
    HitTestChildBlockBackground,
    HitTestChildBlockBackgrounds,
    HitTestFloat,
    HitTestForeground
};

// Sides used when drawing borders and outlines. The values should run clockwise from top.
enum BoxSide {
    BSTop,
    BSRight,
    BSBottom,
    BSLeft
};

enum MarkingBehavior {
    MarkOnlyThis,
    MarkContainingBlockChain,
};

enum MapCoordinatesMode {
    IsFixed = 1 << 0,
    UseTransforms = 1 << 1,
    ApplyContainerFlip = 1 << 2
};
typedef unsigned MapCoordinatesFlags;

const int caretWidth = 1;

#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(DRAGGABLE_REGION)
struct AnnotatedRegionValue {
    bool operator==(const AnnotatedRegionValue& o) const
    {
#if ENABLE(DASHBOARD_SUPPORT)
        return type == o.type && bounds == o.bounds && clip == o.clip && label == o.label;
#else // ENABLE(DRAGGABLE_REGION)
        return draggable == o.draggable && bounds == o.bounds;
#endif
    }
    bool operator!=(const AnnotatedRegionValue& o) const
    {
        return !(*this == o);
    }

    LayoutRect bounds;
#if ENABLE(DASHBOARD_SUPPORT)
    String label;
    LayoutRect clip;
    int type;
#else // ENABLE(DRAGGABLE_REGION)
    bool draggable;
#endif
};
#endif

typedef WTF::HashSet<const RenderObject*> RenderObjectAncestorLineboxDirtySet;

#ifndef NDEBUG
const int showTreeCharacterOffset = 39;
#endif

// Base class for all rendering tree objects.
class RenderObject : public CachedImageClient {
    friend class RenderBlock;
    friend class RenderLayer;
    friend class RenderObjectChildList;
public:
    // Anonymous objects should pass the document as their node, and they will then automatically be
    // marked as anonymous in the constructor.
    explicit RenderObject(Node*);
    virtual ~RenderObject();

    RenderTheme* theme() const;

    virtual const char* renderName() const = 0;

    RenderObject* parent() const { return m_parent; }
    bool isDescendantOf(const RenderObject*) const;

    RenderObject* previousSibling() const { return m_previous; }
    RenderObject* nextSibling() const { return m_next; }

    // FIXME: These should be renamed slowFirstChild, slowLastChild, etc.
    // to discourage their use. The virtualChildren() call inside these
    // can be slow for hot code paths.
    // Currently, some subclasses like RenderBlock, override these NON-virtual
    // functions to make these fast when we already have a more specific pointer type.
    RenderObject* firstChild() const
    {
        if (const RenderObjectChildList* children = virtualChildren())
            return children->firstChild();
        return 0;
    }
    RenderObject* lastChild() const
    {
        if (const RenderObjectChildList* children = virtualChildren())
            return children->lastChild();
        return 0;
    }

    virtual RenderObjectChildList* virtualChildren() { return 0; }
    virtual const RenderObjectChildList* virtualChildren() const { return 0; }

    RenderObject* nextInPreOrder() const;
    RenderObject* nextInPreOrder(const RenderObject* stayWithin) const;
    RenderObject* nextInPreOrderAfterChildren() const;
    RenderObject* nextInPreOrderAfterChildren(const RenderObject* stayWithin) const;
    RenderObject* previousInPreOrder() const;
    RenderObject* previousInPreOrder(const RenderObject* stayWithin) const;
    RenderObject* childAt(unsigned) const;

    RenderObject* firstLeafChild() const;
    RenderObject* lastLeafChild() const;

    // The following six functions are used when the render tree hierarchy changes to make sure layers get
    // properly added and removed.  Since containership can be implemented by any subclass, and since a hierarchy
    // can contain a mixture of boxes and other object types, these functions need to be in the base class.
    RenderLayer* enclosingLayer() const;
    void addLayers(RenderLayer* parentLayer);
    void removeLayers(RenderLayer* parentLayer);
    void moveLayers(RenderLayer* oldParent, RenderLayer* newParent);
    RenderLayer* findNextLayer(RenderLayer* parentLayer, RenderObject* startPoint, bool checkParent = true);

    // Scrolling is a RenderBox concept, however some code just cares about recursively scrolling our enclosing ScrollableArea(s).
    bool scrollRectToVisible(const LayoutRect&, const ScrollAlignment& alignX = ScrollAlignment::alignCenterIfNeeded, const ScrollAlignment& alignY = ScrollAlignment::alignCenterIfNeeded);

    // Convenience function for getting to the nearest enclosing box of a RenderObject.
    RenderBox* enclosingBox() const;
    RenderBoxModelObject* enclosingBoxModelObject() const;

    // Function to return our enclosing flow thread if we are contained inside one. This
    // function follows the containing block chain.
    RenderFlowThread* flowThreadContainingBlock() const
    {
        if (flowThreadState() == NotInsideFlowThread)
            return 0;
        return locateFlowThreadContainingBlock();
    }

    RenderNamedFlowThread* renderNamedFlowThreadWrapper() const;

    virtual bool isEmpty() const { return firstChild() == 0; }

#ifndef NDEBUG
    void setHasAXObject(bool flag) { m_hasAXObject = flag; }
    bool hasAXObject() const { return m_hasAXObject; }

    // Helper class forbidding calls to setNeedsLayout() during its lifetime.
    class SetLayoutNeededForbiddenScope {
    public:
        explicit SetLayoutNeededForbiddenScope(RenderObject*, bool isForbidden = true);
        ~SetLayoutNeededForbiddenScope();
    private:
        RenderObject* m_renderObject;
        bool m_preexistingForbidden;
    };
#endif

    // Obtains the nearest enclosing block (including this block) that contributes a first-line style to our inline
    // children.
    virtual RenderBlock* firstLineBlock() const;

    // Called when an object that was floating or positioned becomes a normal flow object
    // again.  We have to make sure the render tree updates as needed to accommodate the new
    // normal flow object.
    void handleDynamicFloatPositionChange();
    
    // RenderObject tree manipulation
    //////////////////////////////////////////
    virtual bool canHaveChildren() const { return virtualChildren(); }
    virtual bool canDOMChildrenHaveRenderParent() const { return false; } // Even if this render object can't have render children, the children in the DOM tree may have a render parent (that is different from this object).
    virtual bool canHaveGeneratedChildren() const;
    virtual bool isChildAllowed(RenderObject*, RenderStyle*) const { return true; }
    virtual void addChild(RenderObject* newChild, RenderObject* beforeChild = 0);
    virtual void addChildIgnoringContinuation(RenderObject* newChild, RenderObject* beforeChild = 0) { return addChild(newChild, beforeChild); }
    virtual void removeChild(RenderObject*);
    virtual bool createsAnonymousWrapper() const { return false; }
    //////////////////////////////////////////

protected:
    //////////////////////////////////////////
    // Helper functions. Dangerous to use!
    void setPreviousSibling(RenderObject* previous) { m_previous = previous; }
    void setNextSibling(RenderObject* next) { m_next = next; }
    void setParent(RenderObject* parent)
    {
        m_parent = parent;
        
        // Only update if our flow thread state is different from our new parent and if we're not a RenderFlowThread.
        // A RenderFlowThread is always considered to be inside itself, so it never has to change its state
        // in response to parent changes.
        FlowThreadState newState = parent ? parent->flowThreadState() : NotInsideFlowThread;
        if (newState != flowThreadState() && !isRenderFlowThread())
            setFlowThreadStateIncludingDescendants(newState);
    }

    //////////////////////////////////////////
private:
#ifndef NDEBUG
    bool isSetNeedsLayoutForbidden() const { return m_setNeedsLayoutForbidden; }
    void setNeedsLayoutIsForbidden(bool flag) { m_setNeedsLayoutForbidden = flag; }
#endif

    void addAbsoluteRectForLayer(LayoutRect& result);
    void setLayerNeedsFullRepaint();
    void setLayerNeedsFullRepaintForPositionedMovementLayout();

public:
#ifndef NDEBUG
    void showTreeForThis() const;
    void showRenderTreeForThis() const;
    void showLineTreeForThis() const;

    void showRenderObject() const;
    // We don't make printedCharacters an optional parameter so that
    // showRenderObject can be called from gdb easily.
    void showRenderObject(int printedCharacters) const;
    void showRenderTreeAndMark(const RenderObject* markedObject1 = 0, const char* markedLabel1 = 0, const RenderObject* markedObject2 = 0, const char* markedLabel2 = 0, int depth = 0) const;
#endif

    static RenderObject* createObject(Element*, RenderStyle*);

    // Overloaded new operator.  Derived classes must override operator new
    // in order to allocate out of the RenderArena.
    void* operator new(size_t, RenderArena*);

    // Overridden to prevent the normal delete from being called.
    void operator delete(void*, size_t);

private:
    // The normal operator new is disallowed on all render objects.
    void* operator new(size_t) throw();

public:
    RenderArena* renderArena() const { return document()->renderArena(); }

    bool isPseudoElement() const { return node() && node()->isPseudoElement(); }

    virtual bool isBR() const { return false; }
    virtual bool isBlockFlow() const { return false; }
    virtual bool isBoxModelObject() const { return false; }
    virtual bool isCounter() const { return false; }
#if ENABLE(DIALOG_ELEMENT)
    virtual bool isDialog() const { return false; }
#endif
    virtual bool isQuote() const { return false; }

#if ENABLE(DETAILS_ELEMENT) || ENABLE(INPUT_MULTIPLE_FIELDS_UI)
    virtual bool isDetailsMarker() const { return false; }
#endif
    virtual bool isEmbeddedObject() const { return false; }
    virtual bool isFieldset() const { return false; }
    virtual bool isFileUploadControl() const { return false; }
    virtual bool isFrame() const { return false; }
    virtual bool isFrameSet() const { return false; }
    virtual bool isImage() const { return false; }
    virtual bool isInlineBlockOrInlineTable() const { return false; }
    virtual bool isLayerModelObject() const { return false; }
    virtual bool isListBox() const { return false; }
    virtual bool isListItem() const { return false; }
    virtual bool isListMarker() const { return false; }
    virtual bool isMedia() const { return false; }
    virtual bool isMenuList() const { return false; }
#if ENABLE(METER_ELEMENT)
    virtual bool isMeter() const { return false; }
#endif
    virtual bool isSnapshottedPlugIn() const { return false; }
#if ENABLE(PROGRESS_ELEMENT)
    virtual bool isProgress() const { return false; }
#endif
    virtual bool isRenderBlock() const { return false; }
    virtual bool isRenderButton() const { return false; }
    virtual bool isRenderIFrame() const { return false; }
    virtual bool isRenderImage() const { return false; }
    virtual bool isRenderInline() const { return false; }
    virtual bool isRenderPart() const { return false; }
    virtual bool isRenderRegion() const { return false; }
    virtual bool isRenderView() const { return false; }
    virtual bool isReplica() const { return false; }

    virtual bool isRuby() const { return false; }
    virtual bool isRubyBase() const { return false; }
    virtual bool isRubyRun() const { return false; }
    virtual bool isRubyText() const { return false; }

    virtual bool isSlider() const { return false; }
    virtual bool isSliderThumb() const { return false; }
    virtual bool isTable() const { return false; }
    virtual bool isTableCell() const { return false; }
    virtual bool isRenderTableCol() const { return false; }
    virtual bool isTableCaption() const { return false; }
    virtual bool isTableRow() const { return false; }
    virtual bool isTableSection() const { return false; }
    virtual bool isTextControl() const { return false; }
    virtual bool isTextArea() const { return false; }
    virtual bool isTextField() const { return false; }
    virtual bool isVideo() const { return false; }
    virtual bool isWidget() const { return false; }
    virtual bool isCanvas() const { return false; }
#if ENABLE(FULLSCREEN_API)
    virtual bool isRenderFullScreen() const { return false; }
    virtual bool isRenderFullScreenPlaceholder() const { return false; }
#endif

    virtual bool isRenderGrid() const { return false; }

    virtual bool isRenderFlowThread() const { return false; }
    virtual bool isRenderNamedFlowThread() const { return false; }
    bool isInFlowRenderFlowThread() const { return isRenderFlowThread() && !isOutOfFlowPositioned(); }
    bool isOutOfFlowRenderFlowThread() const { return isRenderFlowThread() && isOutOfFlowPositioned(); }

    virtual bool isRenderMultiColumnBlock() const { return false; }
    virtual bool isRenderMultiColumnSet() const { return false; }

    virtual bool isRenderScrollbarPart() const { return false; }

    bool isRoot() const { return document()->documentElement() == m_node; }
    bool isBody() const;
    bool isHR() const;
    bool isLegend() const;

    bool isHTMLMarquee() const;

    bool isTablePart() const { return isTableCell() || isRenderTableCol() || isTableCaption() || isTableRow() || isTableSection(); }

    inline bool isBeforeContent() const;
    inline bool isAfterContent() const;
    inline bool isBeforeOrAfterContent() const;
    static inline bool isBeforeContent(const RenderObject* obj) { return obj && obj->isBeforeContent(); }
    static inline bool isAfterContent(const RenderObject* obj) { return obj && obj->isAfterContent(); }
    static inline bool isBeforeOrAfterContent(const RenderObject* obj) { return obj && obj->isBeforeOrAfterContent(); }

    bool hasCounterNodeMap() const { return m_bitfields.hasCounterNodeMap(); }
    void setHasCounterNodeMap(bool hasCounterNodeMap) { m_bitfields.setHasCounterNodeMap(hasCounterNodeMap); }
    bool everHadLayout() const { return m_bitfields.everHadLayout(); }

    bool childrenInline() const { return m_bitfields.childrenInline(); }
    void setChildrenInline(bool b) { m_bitfields.setChildrenInline(b); }
    bool hasColumns() const { return m_bitfields.hasColumns(); }
    void setHasColumns(bool b = true) { m_bitfields.setHasColumns(b); }

    bool ancestorLineBoxDirty() const { return s_ancestorLineboxDirtySet && s_ancestorLineboxDirtySet->contains(this); }
    void setAncestorLineBoxDirty(bool b = true)
    {
        if (b) {
            if (!s_ancestorLineboxDirtySet)
                s_ancestorLineboxDirtySet = new RenderObjectAncestorLineboxDirtySet;
            s_ancestorLineboxDirtySet->add(this);
            setNeedsLayout(true);
        } else if (s_ancestorLineboxDirtySet) {
            s_ancestorLineboxDirtySet->remove(this);
            if (s_ancestorLineboxDirtySet->isEmpty()) {
                delete s_ancestorLineboxDirtySet;
                s_ancestorLineboxDirtySet = 0;
            }
        }
    }

    enum FlowThreadState {
        NotInsideFlowThread = 0,
        InsideOutOfFlowThread = 1,
        InsideInFlowThread = 2,
    };

    void setFlowThreadStateIncludingDescendants(FlowThreadState);

    FlowThreadState flowThreadState() const { return m_bitfields.flowThreadState(); }
    void setFlowThreadState(FlowThreadState state) { m_bitfields.setFlowThreadState(state); }

    virtual bool requiresForcedStyleRecalcPropagation() const { return false; }

#if ENABLE(MATHML)
    virtual bool isRenderMathMLBlock() const { return false; }
#endif // ENABLE(MATHML)

#if ENABLE(SVG)
    // FIXME: Until all SVG renders can be subclasses of RenderSVGModelObject we have
    // to add SVG renderer methods to RenderObject with an ASSERT_NOT_REACHED() default implementation.
    virtual bool isSVGRoot() const { return false; }
    virtual bool isSVGContainer() const { return false; }
    virtual bool isSVGTransformableContainer() const { return false; }
    virtual bool isSVGViewportContainer() const { return false; }
    virtual bool isSVGGradientStop() const { return false; }
    virtual bool isSVGHiddenContainer() const { return false; }
    virtual bool isSVGPath() const { return false; }
    virtual bool isSVGShape() const { return false; }
    virtual bool isSVGText() const { return false; }
    virtual bool isSVGTextPath() const { return false; }
    virtual bool isSVGInline() const { return false; }
    virtual bool isSVGInlineText() const { return false; }
    virtual bool isSVGImage() const { return false; }
    virtual bool isSVGForeignObject() const { return false; }
    virtual bool isSVGResourceContainer() const { return false; }
    virtual bool isSVGResourceFilter() const { return false; }
    virtual bool isSVGResourceFilterPrimitive() const { return false; }

    virtual RenderSVGResourceContainer* toRenderSVGResourceContainer();

    // FIXME: Those belong into a SVG specific base-class for all renderers (see above)
    // Unfortunately we don't have such a class yet, because it's not possible for all renderers
    // to inherit from RenderSVGObject -> RenderObject (some need RenderBlock inheritance for instance)
    virtual void setNeedsTransformUpdate() { }
    virtual void setNeedsBoundariesUpdate();
    virtual bool needsBoundariesUpdate() { return false; }

    // Per SVG 1.1 objectBoundingBox ignores clipping, masking, filter effects, opacity and stroke-width.
    // This is used for all computation of objectBoundingBox relative units and by SVGLocatable::getBBox().
    // NOTE: Markers are not specifically ignored here by SVG 1.1 spec, but we ignore them
    // since stroke-width is ignored (and marker size can depend on stroke-width).
    // objectBoundingBox is returned local coordinates.
    // The name objectBoundingBox is taken from the SVG 1.1 spec.
    virtual FloatRect objectBoundingBox() const;
    virtual FloatRect strokeBoundingBox() const;

    // Returns the smallest rectangle enclosing all of the painted content
    // respecting clipping, masking, filters, opacity, stroke-width and markers
    virtual FloatRect repaintRectInLocalCoordinates() const;

    // This only returns the transform="" value from the element
    // most callsites want localToParentTransform() instead.
    virtual AffineTransform localTransform() const;

    // Returns the full transform mapping from local coordinates to local coords for the parent SVG renderer
    // This includes any viewport transforms and x/y offsets as well as the transform="" value off the element.
    virtual const AffineTransform& localToParentTransform() const;

    // SVG uses FloatPoint precise hit testing, and passes the point in parent
    // coordinates instead of in repaint container coordinates.  Eventually the
    // rest of the rendering tree will move to a similar model.
    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction);
#endif

    bool isAnonymous() const { return m_bitfields.isAnonymous(); }
    bool isAnonymousBlock() const
    {
        // This function is kept in sync with anonymous block creation conditions in
        // RenderBlock::createAnonymousBlock(). This includes creating an anonymous
        // RenderBlock having a BLOCK or BOX display. Other classes such as RenderTextFragment
        // are not RenderBlocks and will return false. See https://bugs.webkit.org/show_bug.cgi?id=56709. 
        return isAnonymous() && (style()->display() == BLOCK || style()->display() == BOX) && style()->styleType() == NOPSEUDO && isRenderBlock() && !isListMarker() && !isRenderFlowThread()
#if ENABLE(FULLSCREEN_API)
            && !isRenderFullScreen()
            && !isRenderFullScreenPlaceholder()
#endif
#if ENABLE(MATHML)
            && !isRenderMathMLBlock()
#endif
            ;
    }
    bool isAnonymousColumnsBlock() const { return style()->specifiesColumns() && isAnonymousBlock(); }
    bool isAnonymousColumnSpanBlock() const { return style()->columnSpan() && isAnonymousBlock(); }
    bool isElementContinuation() const { return node() && node()->renderer() != this; }
    bool isInlineElementContinuation() const { return isElementContinuation() && isInline(); }
    bool isBlockElementContinuation() const { return isElementContinuation() && !isInline(); }
    virtual RenderBoxModelObject* virtualContinuation() const { return 0; }

    bool isFloating() const { return m_bitfields.floating(); }

    bool isOutOfFlowPositioned() const { return m_bitfields.isOutOfFlowPositioned(); } // absolute or fixed positioning
    bool isInFlowPositioned() const { return m_bitfields.isRelPositioned() || m_bitfields.isStickyPositioned(); } // relative or sticky positioning
    bool hasPaintOffset() const
    {
        bool positioned = isInFlowPositioned();
#if ENABLE(CSS_EXCLUSIONS)
        // Shape outside on a float can reposition the float in much the
        // same way as relative positioning, so treat it as such.
        positioned = positioned || isFloatingWithShapeOutside();
#endif
        return positioned;
    }
    bool isRelPositioned() const { return m_bitfields.isRelPositioned(); } // relative positioning
    bool isStickyPositioned() const { return m_bitfields.isStickyPositioned(); }
    bool isPositioned() const { return m_bitfields.isPositioned(); }

    bool isText() const  { return m_bitfields.isText(); }
    bool isBox() const { return m_bitfields.isBox(); }
    bool isInline() const { return m_bitfields.isInline(); } // inline object
    bool isRunIn() const { return style()->display() == RUN_IN; } // run-in object
    bool isDragging() const { return m_bitfields.isDragging(); }
    bool isReplaced() const { return m_bitfields.isReplaced(); } // a "replaced" element (see CSS)
    bool isHorizontalWritingMode() const { return m_bitfields.horizontalWritingMode(); }

    bool hasLayer() const { return m_bitfields.hasLayer(); }

    enum BoxDecorationState {
        NoBoxDecorations,
        HasBoxDecorationsAndBackgroundObscurationStatusInvalid,
        HasBoxDecorationsAndBackgroundIsKnownToBeObscured,
        HasBoxDecorationsAndBackgroundMayBeVisible,
    };
    bool hasBoxDecorations() const { return m_bitfields.boxDecorationState() != NoBoxDecorations; }
    bool backgroundIsKnownToBeObscured();
    bool borderImageIsLoadedAndCanBeRendered() const;
    bool mustRepaintBackgroundOrBorder() const;
    bool hasBackground() const { return style()->hasBackground(); }
    bool hasEntirelyFixedBackground() const;

    bool needsLayout() const
    {
        return m_bitfields.needsLayout() || m_bitfields.normalChildNeedsLayout() || m_bitfields.posChildNeedsLayout()
            || m_bitfields.needsSimplifiedNormalFlowLayout() || m_bitfields.needsPositionedMovementLayout();
    }

    bool selfNeedsLayout() const { return m_bitfields.needsLayout(); }
    bool needsPositionedMovementLayout() const { return m_bitfields.needsPositionedMovementLayout(); }
    bool needsPositionedMovementLayoutOnly() const
    {
        return m_bitfields.needsPositionedMovementLayout() && !m_bitfields.needsLayout() && !m_bitfields.normalChildNeedsLayout()
            && !m_bitfields.posChildNeedsLayout() && !m_bitfields.needsSimplifiedNormalFlowLayout();
    }

    bool posChildNeedsLayout() const { return m_bitfields.posChildNeedsLayout(); }
    bool needsSimplifiedNormalFlowLayout() const { return m_bitfields.needsSimplifiedNormalFlowLayout(); }
    bool normalChildNeedsLayout() const { return m_bitfields.normalChildNeedsLayout(); }
    
    bool preferredLogicalWidthsDirty() const { return m_bitfields.preferredLogicalWidthsDirty(); }

    bool isSelectionBorder() const;

    bool hasClip() const { return isOutOfFlowPositioned() && style()->hasClip(); }
    bool hasOverflowClip() const { return m_bitfields.hasOverflowClip(); }
    bool hasClipOrOverflowClip() const { return hasClip() || hasOverflowClip(); }

    bool hasTransform() const { return m_bitfields.hasTransform(); }
    bool hasMask() const { return style() && style()->hasMask(); }
    bool hasClipPath() const { return style() && style()->clipPath(); }
    bool hasHiddenBackface() const { return style() && style()->backfaceVisibility() == BackfaceVisibilityHidden; }

#if ENABLE(CSS_FILTERS)
    bool hasFilter() const { return style() && style()->hasFilter(); }
#else
    bool hasFilter() const { return false; }
#endif

#if ENABLE(CSS_COMPOSITING)
    bool hasBlendMode() const { return style() && style()->hasBlendMode(); }
#else
    bool hasBlendMode() const { return false; }
#endif

    inline bool preservesNewline() const;

    // The pseudo element style can be cached or uncached.  Use the cached method if the pseudo element doesn't respect
    // any pseudo classes (and therefore has no concept of changing state).
    RenderStyle* getCachedPseudoStyle(PseudoId, RenderStyle* parentStyle = 0) const;
    PassRefPtr<RenderStyle> getUncachedPseudoStyle(const PseudoStyleRequest&, RenderStyle* parentStyle = 0, RenderStyle* ownStyle = 0) const;
    
    virtual void updateDragState(bool dragOn);

    RenderView* view() const { return document()->renderView(); };

    // Returns true if this renderer is rooted, and optionally returns the hosting view (the root of the hierarchy).
    bool isRooted(RenderView** = 0) const;

    Node* node() const { return isAnonymous() ? 0 : m_node; }
    Node* nonPseudoNode() const { return isPseudoElement() ? 0 : node(); }

    // FIXME: Why does RenderWidget need this?
    void clearNode() { m_node = 0; }

    // Returns the styled node that caused the generation of this renderer.
    // This is the same as node() except for renderers of :before and :after
    // pseudo elements for which their parent node is returned.
    Node* generatingNode() const { return isPseudoElement() ? node()->parentOrShadowHostNode() : node(); }

    Document* document() const { return m_node->document(); }
    Frame* frame() const { return document()->frame(); }

    bool hasOutlineAnnotation() const;
    bool hasOutline() const { return style()->hasOutline() || hasOutlineAnnotation(); }

    // Returns the object containing this one. Can be different from parent for positioned elements.
    // If repaintContainer and repaintContainerSkipped are not null, on return *repaintContainerSkipped
    // is true if the renderer returned is an ancestor of repaintContainer.
    RenderObject* container(const RenderLayerModelObject* repaintContainer = 0, bool* repaintContainerSkipped = 0) const;

    virtual RenderObject* hoverAncestor() const { return parent(); }

    Element* offsetParent() const;

    void markContainingBlocksForLayout(bool scheduleRelayout = true, RenderObject* newRoot = 0);
    void setNeedsLayout(bool needsLayout, MarkingBehavior = MarkContainingBlockChain);
    void setChildNeedsLayout(bool childNeedsLayout, MarkingBehavior = MarkContainingBlockChain);
    void setNeedsPositionedMovementLayout();
    void setNeedsSimplifiedNormalFlowLayout();
    void setPreferredLogicalWidthsDirty(bool, MarkingBehavior = MarkContainingBlockChain);
    void invalidateContainerPreferredLogicalWidths();
    
    void setNeedsLayoutAndPrefWidthsRecalc()
    {
        setNeedsLayout(true);
        setPreferredLogicalWidthsDirty(true);
    }

    void setPositionState(EPosition position)
    {
        ASSERT((position != AbsolutePosition && position != FixedPosition) || isBox());
        m_bitfields.setPositionedState(position);
    }
    void clearPositionedState() { m_bitfields.clearPositionedState(); }

    void setFloating(bool b = true) { m_bitfields.setFloating(b); }
    void setInline(bool b = true) { m_bitfields.setIsInline(b); }

    void setHasBoxDecorations(bool = true);
    void invalidateBackgroundObscurationStatus();
    virtual bool computeBackgroundIsKnownToBeObscured() { return false; }

    void setIsText() { m_bitfields.setIsText(true); }
    void setIsBox() { m_bitfields.setIsBox(true); }
    void setReplaced(bool b = true) { m_bitfields.setIsReplaced(b); }
    void setHorizontalWritingMode(bool b = true) { m_bitfields.setHorizontalWritingMode(b); }
    void setHasOverflowClip(bool b = true) { m_bitfields.setHasOverflowClip(b); }
    void setHasLayer(bool b = true) { m_bitfields.setHasLayer(b); }
    void setHasTransform(bool b = true) { m_bitfields.setHasTransform(b); }
    void setHasReflection(bool b = true) { m_bitfields.setHasReflection(b); }

    void scheduleRelayout();

    void updateFillImages(const FillLayer*, const FillLayer*);
    void updateImage(StyleImage*, StyleImage*);

    virtual void paint(PaintInfo&, const LayoutPoint&);

    // Recursive function that computes the size and position of this object and all its descendants.
    virtual void layout();

    /* This function performs a layout only if one is needed. */
    void layoutIfNeeded() { if (needsLayout()) layout(); }
    
    // used for element state updates that cannot be fixed with a
    // repaint and do not need a relayout
    virtual void updateFromElement() { }

#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(DRAGGABLE_REGION)
    virtual void addAnnotatedRegions(Vector<AnnotatedRegionValue>&);
    void collectAnnotatedRegions(Vector<AnnotatedRegionValue>&);
#endif

    bool isComposited() const;

    bool hitTest(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestFilter = HitTestAll);
    virtual void updateHitTestResult(HitTestResult&, const LayoutPoint&);
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction);

    virtual VisiblePosition positionForPoint(const LayoutPoint&);
    VisiblePosition createVisiblePosition(int offset, EAffinity);
    VisiblePosition createVisiblePosition(const Position&);

    virtual void dirtyLinesFromChangedChild(RenderObject*);

    // Called to update a style that is allowed to trigger animations.
    // FIXME: Right now this will typically be called only when updating happens from the DOM on explicit elements.
    // We don't yet handle generated content animation such as first-letter or before/after (we'll worry about this later).
    void setAnimatableStyle(PassRefPtr<RenderStyle>);

    // Set the style of the object and update the state of the object accordingly.
    virtual void setStyle(PassRefPtr<RenderStyle>);

    // Set the style of the object if it's generated content.
    void setPseudoStyle(PassRefPtr<RenderStyle>);

    // Updates only the local style ptr of the object.  Does not update the state of the object,
    // and so only should be called when the style is known not to have changed (or from setStyle).
    void setStyleInternal(PassRefPtr<RenderStyle> style) { m_style = style; }

    // returns the containing block level element for this element.
    RenderBlock* containingBlock() const;

    bool canContainFixedPositionObjects() const
    {
        return isRenderView() || (hasTransform() && isRenderBlock())
#if ENABLE(SVG)
                || isSVGForeignObject()
#endif
                || isOutOfFlowRenderFlowThread();
    }

    // Convert the given local point to absolute coordinates
    // FIXME: Temporary. If UseTransforms is true, take transforms into account. Eventually localToAbsolute() will always be transform-aware.
    FloatPoint localToAbsolute(const FloatPoint& localPoint = FloatPoint(), MapCoordinatesFlags = 0) const;
    FloatPoint absoluteToLocal(const FloatPoint&, MapCoordinatesFlags = 0) const;

    // Convert a local quad to absolute coordinates, taking transforms into account.
    FloatQuad localToAbsoluteQuad(const FloatQuad& quad, MapCoordinatesFlags mode = 0, bool* wasFixed = 0) const
    {
        return localToContainerQuad(quad, 0, mode, wasFixed);
    }
    // Convert an absolute quad to local coordinates.
    FloatQuad absoluteToLocalQuad(const FloatQuad&, MapCoordinatesFlags mode = 0) const;

    // Convert a local quad into the coordinate system of container, taking transforms into account.
    FloatQuad localToContainerQuad(const FloatQuad&, const RenderLayerModelObject* repaintContainer, MapCoordinatesFlags = 0, bool* wasFixed = 0) const;
    FloatPoint localToContainerPoint(const FloatPoint&, const RenderLayerModelObject* repaintContainer, MapCoordinatesFlags = 0, bool* wasFixed = 0) const;

    // Return the offset from the container() renderer (excluding transforms). In multi-column layout,
    // different offsets apply at different points, so return the offset that applies to the given point.
    virtual LayoutSize offsetFromContainer(RenderObject*, const LayoutPoint&, bool* offsetDependsOnPoint = 0) const;
    // Return the offset from an object up the container() chain. Asserts that none of the intermediate objects have transforms.
    LayoutSize offsetFromAncestorContainer(RenderObject*) const;
    
    virtual void absoluteRects(Vector<IntRect>&, const LayoutPoint&) const { }

    // FIXME: useTransforms should go away eventually
    IntRect absoluteBoundingBoxRect(bool useTransform = true) const;
    IntRect absoluteBoundingBoxRectIgnoringTransforms() const { return absoluteBoundingBoxRect(false); }

    // Build an array of quads in absolute coords for line boxes
    virtual void absoluteQuads(Vector<FloatQuad>&, bool* /*wasFixed*/ = 0) const { }

    virtual void absoluteFocusRingQuads(Vector<FloatQuad>&);

    static FloatRect absoluteBoundingBoxRectForRange(const Range*);

    // the rect that will be painted if this object is passed as the paintingRoot
    LayoutRect paintingRootRect(LayoutRect& topLevelRect);

    virtual LayoutUnit minPreferredLogicalWidth() const { return 0; }
    virtual LayoutUnit maxPreferredLogicalWidth() const { return 0; }

    RenderStyle* style() const { return m_style.get(); }
    RenderStyle* firstLineStyle() const { return document()->styleSheetCollection()->usesFirstLineRules() ? cachedFirstLineStyle() : style(); }
    RenderStyle* style(bool firstLine) const { return firstLine ? firstLineStyle() : style(); }

    // Used only by Element::pseudoStyleCacheIsInvalid to get a first line style based off of a
    // given new style, without accessing the cache.
    PassRefPtr<RenderStyle> uncachedFirstLineStyle(RenderStyle*) const;

    // Anonymous blocks that are part of of a continuation chain will return their inline continuation's outline style instead.
    // This is typically only relevant when repainting.
    virtual RenderStyle* outlineStyleForRepaint() const { return style(); }
    
    virtual CursorDirective getCursor(const LayoutPoint&, Cursor&) const;

    void getTextDecorationColors(int decorations, Color& underline, Color& overline, Color& linethrough, bool quirksMode = false, bool firstlineStyle = false);

    // Return the RenderLayerModelObject in the container chain which is responsible for painting this object, or 0
    // if painting is root-relative. This is the container that should be passed to the 'forRepaint'
    // methods.
    RenderLayerModelObject* containerForRepaint() const;
    // Actually do the repaint of rect r for this object which has been computed in the coordinate space
    // of repaintContainer. If repaintContainer is 0, repaint via the view.
    void repaintUsingContainer(const RenderLayerModelObject* repaintContainer, const IntRect&, bool immediate = false) const;
    
    // Repaint the entire object.  Called when, e.g., the color of a border changes, or when a border
    // style changes.
    void repaint(bool immediate = false) const;

    // Repaint a specific subrectangle within a given object.  The rect |r| is in the object's coordinate space.
    void repaintRectangle(const LayoutRect&, bool immediate = false) const;

    // Repaint only if our old bounds and new bounds are different. The caller may pass in newBounds and newOutlineBox if they are known.
    bool repaintAfterLayoutIfNeeded(const RenderLayerModelObject* repaintContainer, const LayoutRect& oldBounds, const LayoutRect& oldOutlineBox, const LayoutRect* newBoundsPtr = 0, const LayoutRect* newOutlineBoxPtr = 0);

    bool checkForRepaintDuringLayout() const;

    // Returns the rect that should be repainted whenever this object changes.  The rect is in the view's
    // coordinate space.  This method deals with outlines and overflow.
    LayoutRect absoluteClippedOverflowRect() const
    {
        return clippedOverflowRectForRepaint(0);
    }
    IntRect pixelSnappedAbsoluteClippedOverflowRect() const;
    virtual LayoutRect clippedOverflowRectForRepaint(const RenderLayerModelObject* repaintContainer) const;
    virtual LayoutRect rectWithOutlineForRepaint(const RenderLayerModelObject* repaintContainer, LayoutUnit outlineWidth) const;
    virtual LayoutRect outlineBoundsForRepaint(const RenderLayerModelObject* /*repaintContainer*/, const RenderGeometryMap* = 0) const { return LayoutRect(); }

    // Given a rect in the object's coordinate space, compute a rect suitable for repainting
    // that rect in view coordinates.
    void computeAbsoluteRepaintRect(LayoutRect& r, bool fixed = false) const
    {
        computeRectForRepaint(0, r, fixed);
    }
    // Given a rect in the object's coordinate space, compute a rect suitable for repainting
    // that rect in the coordinate space of repaintContainer.
    virtual void computeRectForRepaint(const RenderLayerModelObject* repaintContainer, LayoutRect&, bool fixed = false) const;
    virtual void computeFloatRectForRepaint(const RenderLayerModelObject* repaintContainer, FloatRect& repaintRect, bool fixed = false) const;

    // If multiple-column layout results in applying an offset to the given point, add the same
    // offset to the given size.
    virtual void adjustForColumns(LayoutSize&, const LayoutPoint&) const { }
    LayoutSize offsetForColumns(const LayoutPoint& point) const
    {
        LayoutSize offset;
        adjustForColumns(offset, point);
        return offset;
    }

    virtual unsigned int length() const { return 1; }

    bool isFloatingOrOutOfFlowPositioned() const { return (isFloating() || isOutOfFlowPositioned()); }
    bool isFloatingWithShapeOutside() const { return isBox() && isFloating() && style()->shapeOutside(); }

    bool isTransparent() const { return style()->opacity() < 1.0f; }
    float opacity() const { return style()->opacity(); }

    bool hasReflection() const { return m_bitfields.hasReflection(); }

    // Applied as a "slop" to dirty rect checks during the outline painting phase's dirty-rect checks.
    int maximalOutlineSize(PaintPhase) const;

    enum SelectionState {
        SelectionNone, // The object is not selected.
        SelectionStart, // The object either contains the start of a selection run or is the start of a run
        SelectionInside, // The object is fully encompassed by a selection run
        SelectionEnd, // The object either contains the end of a selection run or is the end of a run
        SelectionBoth // The object contains an entire run or is the sole selected object in that run
    };

    // The current selection state for an object.  For blocks, the state refers to the state of the leaf
    // descendants (as described above in the SelectionState enum declaration).
    SelectionState selectionState() const { return m_bitfields.selectionState(); }
    virtual void setSelectionState(SelectionState state) { m_bitfields.setSelectionState(state); }
    inline void setSelectionStateIfNeeded(SelectionState);
    bool canUpdateSelectionOnRootLineBoxes();

    // A single rectangle that encompasses all of the selected objects within this object.  Used to determine the tightest
    // possible bounding box for the selection.
    LayoutRect selectionRect(bool clipToVisibleContent = true) { return selectionRectForRepaint(0, clipToVisibleContent); }
    virtual LayoutRect selectionRectForRepaint(const RenderLayerModelObject* /*repaintContainer*/, bool /*clipToVisibleContent*/ = true) { return LayoutRect(); }

    virtual bool canBeSelectionLeaf() const { return false; }
    bool hasSelectedChildren() const { return selectionState() != SelectionNone; }

    // Obtains the selection colors that should be used when painting a selection.
    Color selectionBackgroundColor() const;
    Color selectionForegroundColor() const;
    Color selectionEmphasisMarkColor() const;

    // Whether or not a given block needs to paint selection gaps.
    virtual bool shouldPaintSelectionGaps() const { return false; }

    /**
     * Returns the local coordinates of the caret within this render object.
     * @param caretOffset zero-based offset determining position within the render object.
     * @param extraWidthToEndOfLine optional out arg to give extra width to end of line -
     * useful for character range rect computations
     */
    virtual LayoutRect localCaretRect(InlineBox*, int caretOffset, LayoutUnit* extraWidthToEndOfLine = 0);

    // When performing a global document tear-down, the renderer of the document is cleared.  We use this
    // as a hook to detect the case of document destruction and don't waste time doing unnecessary work.
    bool documentBeingDestroyed() const;

    void destroyAndCleanupAnonymousWrappers();
    virtual void destroy();

    // Virtual function helpers for the deprecated Flexible Box Layout (display: -webkit-box).
    virtual bool isDeprecatedFlexibleBox() const { return false; }
    virtual bool isStretchingChildren() const { return false; }

    // Virtual function helper for the new FlexibleBox Layout (display: -webkit-flex).
    virtual bool isFlexibleBox() const { return false; }

    bool isFlexibleBoxIncludingDeprecated() const
    {
        return isFlexibleBox() || isDeprecatedFlexibleBox();
    }

    virtual bool isCombineText() const { return false; }

    virtual int caretMinOffset() const;
    virtual int caretMaxOffset() const;

    virtual int previousOffset(int current) const;
    virtual int previousOffsetForBackwardDeletion(int current) const;
    virtual int nextOffset(int current) const;

    virtual void imageChanged(CachedImage*, const IntRect* = 0);
    virtual void imageChanged(WrappedImagePtr, const IntRect* = 0) { }
    virtual bool willRenderImage(CachedImage*);

    void selectionStartEnd(int& spos, int& epos) const;
    
    void remove() { if (parent()) parent()->removeChild(this); }

    AnimationController* animation() const;

    bool visibleToHitTesting() const { return style()->visibility() == VISIBLE && style()->pointerEvents() != PE_NONE; }

    // Map points and quads through elements, potentially via 3d transforms. You should never need to call these directly; use
    // localToAbsolute/absoluteToLocal methods instead.
    virtual void mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState&, MapCoordinatesFlags = ApplyContainerFlip, bool* wasFixed = 0) const;
    virtual void mapAbsoluteToLocalPoint(MapCoordinatesFlags, TransformState&) const;

    // Pushes state onto RenderGeometryMap about how to map coordinates from this renderer to its container, or ancestorToStopAt (whichever is encountered first).
    // Returns the renderer which was mapped to (container or ancestorToStopAt).
    virtual const RenderObject* pushMappingToContainer(const RenderLayerModelObject* ancestorToStopAt, RenderGeometryMap&) const;
    
    bool shouldUseTransformFromContainer(const RenderObject* container) const;
    void getTransformFromContainer(const RenderObject* container, const LayoutSize& offsetInContainer, TransformationMatrix&) const;
    
    // return true if this object requires a new stacking context
    bool createsGroup() const { return isTransparent() || hasMask() || hasFilter() || hasBlendMode(); } 
    
    virtual void addFocusRingRects(Vector<IntRect>&, const LayoutPoint& /* additionalOffset */, const RenderLayerModelObject* /* paintContainer */ = 0) { };

    LayoutRect absoluteOutlineBounds() const
    {
        return outlineBoundsForRepaint(0);
    }

    // Return the renderer whose background style is used to paint the root background. Should only be called on the renderer for which isRoot() is true.
    RenderObject* rendererForRootBackground();

    RespectImageOrientationEnum shouldRespectImageOrientation() const;

protected:
    inline bool layerCreationAllowedForSubtree() const;

    // Overrides should call the superclass at the end
    virtual void styleWillChange(StyleDifference, const RenderStyle* newStyle);
    // Overrides should call the superclass at the start
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);
    void propagateStyleToAnonymousChildren(bool blockChildrenOnly = false);

    void drawLineForBoxSide(GraphicsContext*, int x1, int y1, int x2, int y2, BoxSide,
                            Color, EBorderStyle, int adjbw1, int adjbw2, bool antialias = false);

    void paintFocusRing(PaintInfo&, const LayoutPoint&, RenderStyle*);
    void paintOutline(PaintInfo&, const LayoutRect&);
    void addPDFURLRect(GraphicsContext*, const LayoutRect&);
    
    virtual LayoutRect viewRect() const;

    void adjustRectForOutlineAndShadow(LayoutRect&) const;

    void clearLayoutRootIfNeeded() const;
    virtual void willBeDestroyed();
    void arenaDelete(RenderArena*, void* objectBase);

    virtual bool canBeReplacedWithInlineRunIn() const;

    virtual void insertedIntoTree();
    virtual void willBeRemovedFromTree();

    void setDocumentForAnonymous(Document* document) { ASSERT(isAnonymous()); m_node = document; }

private:
    RenderFlowThread* locateFlowThreadContainingBlock() const;
    void removeFromRenderFlowThread();
    void removeFromRenderFlowThreadRecursive(RenderFlowThread*);

    RenderStyle* cachedFirstLineStyle() const;
    StyleDifference adjustStyleDifference(StyleDifference, unsigned contextSensitiveProperties) const;

    Color selectionColor(int colorProperty) const;

#ifndef NDEBUG
    void checkBlockPositionedObjectsNeedLayout();
#endif

    RefPtr<RenderStyle> m_style;

    Node* m_node;

    RenderObject* m_parent;
    RenderObject* m_previous;
    RenderObject* m_next;

    static RenderObjectAncestorLineboxDirtySet* s_ancestorLineboxDirtySet;

#ifndef NDEBUG
    bool m_hasAXObject             : 1;
    bool m_setNeedsLayoutForbidden : 1;
#endif

#define ADD_BOOLEAN_BITFIELD(name, Name) \
    private:\
        unsigned m_##name : 1;\
    public:\
        bool name() const { return m_##name; }\
        void set##Name(bool name) { m_##name = name; }\

    class RenderObjectBitfields {
        enum PositionedState {
            IsStaticallyPositioned = 0,
            IsRelativelyPositioned = 1,
            IsOutOfFlowPositioned = 2,
            IsStickyPositioned = 3
        };

    public:
        RenderObjectBitfields(Node* node)
            : m_needsLayout(false)
            , m_needsPositionedMovementLayout(false)
            , m_normalChildNeedsLayout(false)
            , m_posChildNeedsLayout(false)
            , m_needsSimplifiedNormalFlowLayout(false)
            , m_preferredLogicalWidthsDirty(false)
            , m_floating(false)
            , m_isAnonymous(!node)
            , m_isText(false)
            , m_isBox(false)
            , m_isInline(true)
            , m_isReplaced(false)
            , m_horizontalWritingMode(true)
            , m_isDragging(false)
            , m_hasLayer(false)
            , m_hasOverflowClip(false)
            , m_hasTransform(false)
            , m_hasReflection(false)
            , m_hasCounterNodeMap(false)
            , m_everHadLayout(false)
            , m_childrenInline(false)
            , m_hasColumns(false)
            , m_positionedState(IsStaticallyPositioned)
            , m_selectionState(SelectionNone)
            , m_flowThreadState(NotInsideFlowThread)
            , m_boxDecorationState(NoBoxDecorations)
        {
        }
        
        // 31 bits have been used here. There is one bit available.
        ADD_BOOLEAN_BITFIELD(needsLayout, NeedsLayout);
        ADD_BOOLEAN_BITFIELD(needsPositionedMovementLayout, NeedsPositionedMovementLayout);
        ADD_BOOLEAN_BITFIELD(normalChildNeedsLayout, NormalChildNeedsLayout);
        ADD_BOOLEAN_BITFIELD(posChildNeedsLayout, PosChildNeedsLayout);
        ADD_BOOLEAN_BITFIELD(needsSimplifiedNormalFlowLayout, NeedsSimplifiedNormalFlowLayout);
        ADD_BOOLEAN_BITFIELD(preferredLogicalWidthsDirty, PreferredLogicalWidthsDirty);
        ADD_BOOLEAN_BITFIELD(floating, Floating);

        ADD_BOOLEAN_BITFIELD(isAnonymous, IsAnonymous);
        ADD_BOOLEAN_BITFIELD(isText, IsText);
        ADD_BOOLEAN_BITFIELD(isBox, IsBox);
        ADD_BOOLEAN_BITFIELD(isInline, IsInline);
        ADD_BOOLEAN_BITFIELD(isReplaced, IsReplaced);
        ADD_BOOLEAN_BITFIELD(horizontalWritingMode, HorizontalWritingMode);
        ADD_BOOLEAN_BITFIELD(isDragging, IsDragging);

        ADD_BOOLEAN_BITFIELD(hasLayer, HasLayer);
        ADD_BOOLEAN_BITFIELD(hasOverflowClip, HasOverflowClip); // Set in the case of overflow:auto/scroll/hidden
        ADD_BOOLEAN_BITFIELD(hasTransform, HasTransform);
        ADD_BOOLEAN_BITFIELD(hasReflection, HasReflection);

        ADD_BOOLEAN_BITFIELD(hasCounterNodeMap, HasCounterNodeMap);
        ADD_BOOLEAN_BITFIELD(everHadLayout, EverHadLayout);

        // from RenderBlock
        ADD_BOOLEAN_BITFIELD(childrenInline, ChildrenInline);
        ADD_BOOLEAN_BITFIELD(hasColumns, HasColumns);

    private:
        unsigned m_positionedState : 2; // PositionedState
        unsigned m_selectionState : 3; // SelectionState
        unsigned m_flowThreadState : 2; // FlowThreadState
        unsigned m_boxDecorationState : 2; // BoxDecorationState

    public:
        bool isOutOfFlowPositioned() const { return m_positionedState == IsOutOfFlowPositioned; }
        bool isRelPositioned() const { return m_positionedState == IsRelativelyPositioned; }
        bool isStickyPositioned() const { return m_positionedState == IsStickyPositioned; }
        bool isPositioned() const { return m_positionedState != IsStaticallyPositioned; }

        void setPositionedState(int positionState)
        {
            // This mask maps FixedPosition and AbsolutePosition to IsOutOfFlowPositioned, saving one bit.
            m_positionedState = static_cast<PositionedState>(positionState & 0x3);
        }
        void clearPositionedState() { m_positionedState = StaticPosition; }

        ALWAYS_INLINE SelectionState selectionState() const { return static_cast<SelectionState>(m_selectionState); }
        ALWAYS_INLINE void setSelectionState(SelectionState selectionState) { m_selectionState = selectionState; }
        
        ALWAYS_INLINE FlowThreadState flowThreadState() const { return static_cast<FlowThreadState>(m_flowThreadState); }
        ALWAYS_INLINE void setFlowThreadState(FlowThreadState flowThreadState) { m_flowThreadState = flowThreadState; }

        ALWAYS_INLINE BoxDecorationState boxDecorationState() const { return static_cast<BoxDecorationState>(m_boxDecorationState); }
        ALWAYS_INLINE void setBoxDecorationState(BoxDecorationState boxDecorationState) { m_boxDecorationState = boxDecorationState; }
    };

#undef ADD_BOOLEAN_BITFIELD

    RenderObjectBitfields m_bitfields;

    void setNeedsPositionedMovementLayout(bool b) { m_bitfields.setNeedsPositionedMovementLayout(b); }
    void setNormalChildNeedsLayout(bool b) { m_bitfields.setNormalChildNeedsLayout(b); }
    void setPosChildNeedsLayout(bool b) { m_bitfields.setPosChildNeedsLayout(b); }
    void setNeedsSimplifiedNormalFlowLayout(bool b) { m_bitfields.setNeedsSimplifiedNormalFlowLayout(b); }
    void setIsDragging(bool b) { m_bitfields.setIsDragging(b); }
    void setEverHadLayout(bool b) { m_bitfields.setEverHadLayout(b); }

private:
    // Store state between styleWillChange and styleDidChange
    static bool s_affectsParentBlock;
};

inline bool RenderObject::documentBeingDestroyed() const
{
    return !document()->renderer();
}

inline bool RenderObject::isBeforeContent() const
{
    if (style()->styleType() != BEFORE)
        return false;
    // Text nodes don't have their own styles, so ignore the style on a text node.
    if (isText() && !isBR())
        return false;
    return true;
}

inline bool RenderObject::isAfterContent() const
{
    if (style()->styleType() != AFTER)
        return false;
    // Text nodes don't have their own styles, so ignore the style on a text node.
    if (isText() && !isBR())
        return false;
    return true;
}

inline bool RenderObject::isBeforeOrAfterContent() const
{
    return isBeforeContent() || isAfterContent();
}

inline void RenderObject::setNeedsLayout(bool needsLayout, MarkingBehavior markParents)
{
    bool alreadyNeededLayout = m_bitfields.needsLayout();
    m_bitfields.setNeedsLayout(needsLayout);
    if (needsLayout) {
        ASSERT(!isSetNeedsLayoutForbidden());
        if (!alreadyNeededLayout) {
            if (markParents == MarkContainingBlockChain)
                markContainingBlocksForLayout();
            if (hasLayer())
                setLayerNeedsFullRepaint();
        }
    } else {
        setEverHadLayout(true);
        setPosChildNeedsLayout(false);
        setNeedsSimplifiedNormalFlowLayout(false);
        setNormalChildNeedsLayout(false);
        setNeedsPositionedMovementLayout(false);
        setAncestorLineBoxDirty(false);
#ifndef NDEBUG
        checkBlockPositionedObjectsNeedLayout();
#endif
    }
}

inline void RenderObject::setChildNeedsLayout(bool childNeedsLayout, MarkingBehavior markParents)
{
    bool alreadyNeededLayout = normalChildNeedsLayout();
    setNormalChildNeedsLayout(childNeedsLayout);
    if (childNeedsLayout) {
        ASSERT(!isSetNeedsLayoutForbidden());
        if (!alreadyNeededLayout && markParents == MarkContainingBlockChain)
            markContainingBlocksForLayout();
    } else {
        setPosChildNeedsLayout(false);
        setNeedsSimplifiedNormalFlowLayout(false);
        setNormalChildNeedsLayout(false);
        setNeedsPositionedMovementLayout(false);
    }
}

inline void RenderObject::setNeedsPositionedMovementLayout()
{
    bool alreadyNeededLayout = needsPositionedMovementLayout();
    setNeedsPositionedMovementLayout(true);
    ASSERT(!isSetNeedsLayoutForbidden());
    if (!alreadyNeededLayout) {
        markContainingBlocksForLayout();
        if (hasLayer())
            setLayerNeedsFullRepaintForPositionedMovementLayout();
    }
}

inline void RenderObject::setNeedsSimplifiedNormalFlowLayout()
{
    bool alreadyNeededLayout = needsSimplifiedNormalFlowLayout();
    setNeedsSimplifiedNormalFlowLayout(true);
    ASSERT(!isSetNeedsLayoutForbidden());
    if (!alreadyNeededLayout) {
        markContainingBlocksForLayout();
        if (hasLayer())
            setLayerNeedsFullRepaint();
    }
}

inline bool RenderObject::preservesNewline() const
{
#if ENABLE(SVG)
    if (isSVGInlineText())
        return false;
#endif
        
    return style()->preserveNewline();
}

inline bool RenderObject::layerCreationAllowedForSubtree() const
{
#if ENABLE(SVG)
    RenderObject* parentRenderer = parent();
    while (parentRenderer) {
        if (parentRenderer->isSVGHiddenContainer())
            return false;
        parentRenderer = parentRenderer->parent();
    }
#endif

    return true;
}

inline void RenderObject::setSelectionStateIfNeeded(SelectionState state)
{
    if (selectionState() == state)
        return;

    setSelectionState(state);
}

inline void RenderObject::setHasBoxDecorations(bool b)
{
    if (!b) {
        m_bitfields.setBoxDecorationState(NoBoxDecorations);
        return;
    }
    if (hasBoxDecorations())
        return;
    m_bitfields.setBoxDecorationState(HasBoxDecorationsAndBackgroundObscurationStatusInvalid);
}

inline void RenderObject::invalidateBackgroundObscurationStatus()
{
    if (!hasBoxDecorations())
        return;
    m_bitfields.setBoxDecorationState(HasBoxDecorationsAndBackgroundObscurationStatusInvalid);
}

inline bool RenderObject::backgroundIsKnownToBeObscured()
{
    if (m_bitfields.boxDecorationState() == HasBoxDecorationsAndBackgroundObscurationStatusInvalid) {
        BoxDecorationState boxDecorationState = computeBackgroundIsKnownToBeObscured() ? HasBoxDecorationsAndBackgroundIsKnownToBeObscured : HasBoxDecorationsAndBackgroundMayBeVisible;
        m_bitfields.setBoxDecorationState(boxDecorationState);
    }
    return m_bitfields.boxDecorationState() == HasBoxDecorationsAndBackgroundIsKnownToBeObscured;
}

inline int adjustForAbsoluteZoom(int value, RenderObject* renderer)
{
    return adjustForAbsoluteZoom(value, renderer->style());
}

#if ENABLE(SUBPIXEL_LAYOUT)
inline LayoutUnit adjustLayoutUnitForAbsoluteZoom(LayoutUnit value, RenderObject* renderer)
{
    return adjustLayoutUnitForAbsoluteZoom(value, renderer->style());
}
#endif

inline void adjustFloatQuadForAbsoluteZoom(FloatQuad& quad, RenderObject* renderer)
{
    float zoom = renderer->style()->effectiveZoom();
    if (zoom != 1)
        quad.scale(1 / zoom, 1 / zoom);
}

inline void adjustFloatRectForAbsoluteZoom(FloatRect& rect, RenderObject* renderer)
{
    float zoom = renderer->style()->effectiveZoom();
    if (zoom != 1)
        rect.scale(1 / zoom, 1 / zoom);
}

} // namespace WebCore

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showTree(const WebCore::RenderObject*);
void showLineTree(const WebCore::RenderObject*);
void showRenderTree(const WebCore::RenderObject* object1);
// We don't make object2 an optional parameter so that showRenderTree
// can be called from gdb easily.
void showRenderTree(const WebCore::RenderObject* object1, const WebCore::RenderObject* object2);
#endif

#endif // RenderObject_h
