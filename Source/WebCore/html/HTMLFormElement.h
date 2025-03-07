/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLFormElement_h
#define HTMLFormElement_h

#include "CheckedRadioButtons.h"
#include "FormState.h"
#include "FormSubmission.h"
#include "HTMLElement.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class Event;
class FormAssociatedElement;
class FormData;
class HTMLFormControlElement;
class HTMLImageElement;
class HTMLInputElement;
class TextEncoding;

class HTMLFormElement : public HTMLElement {
public:
    static PassRefPtr<HTMLFormElement> create(Document*);
    static PassRefPtr<HTMLFormElement> create(const QualifiedName&, Document*);
    virtual ~HTMLFormElement();

    PassRefPtr<HTMLCollection> elements();
    void getNamedElements(const AtomicString&, Vector<RefPtr<Node> >&);

    unsigned length() const;
    Node* item(unsigned index);

    String enctype() const { return m_attributes.encodingType(); }
    void setEnctype(const String&);

    String encoding() const { return m_attributes.encodingType(); }
    void setEncoding(const String& value) { setEnctype(value); }

    bool shouldAutocomplete() const;

    // FIXME: Should rename these two functions to say "form control" or "form-associated element" instead of "form element".
    void registerFormElement(FormAssociatedElement*);
    void removeFormElement(FormAssociatedElement*);

    void registerImgElement(HTMLImageElement*);
    void removeImgElement(HTMLImageElement*);

    bool prepareForSubmission(Event*);
    void submit();
    void submitFromJavaScript();
    void reset();

    void setDemoted(bool demoted) { m_wasDemoted = demoted; }

    void submitImplicitly(Event*, bool fromImplicitSubmissionTrigger);
    bool formWouldHaveSecureSubmission(const String& url);

    String name() const;

    bool noValidate() const;

    String acceptCharset() const { return m_attributes.acceptCharset(); }
    void setAcceptCharset(const String&);

    String action() const;
    void setAction(const String&);

    String method() const;
    void setMethod(const String&);

    virtual String target() const;

    bool wasUserSubmitted() const;

    HTMLFormControlElement* defaultButton() const;

    bool checkValidity();

    HTMLFormControlElement* elementForAlias(const AtomicString&);
    void addElementAlias(HTMLFormControlElement*, const AtomicString& alias);

    CheckedRadioButtons& checkedRadioButtons() { return m_checkedRadioButtons; }

    const Vector<FormAssociatedElement*>& associatedElements() const { return m_associatedElements; }
    const Vector<HTMLImageElement*>& imageElements() const { return m_imageElements; }

    void getTextFieldValues(StringPairVector& fieldNamesAndValues) const;

private:
    HTMLFormElement(const QualifiedName&, Document*);

    virtual bool rendererIsNeeded(const NodeRenderingContext&);
    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void removedFrom(ContainerNode*) OVERRIDE;
    virtual void finishParsingChildren() OVERRIDE;

    virtual void handleLocalEvents(Event*);

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual bool isURLAttribute(const Attribute&) const OVERRIDE;

    virtual void documentDidResumeFromPageCache();

    virtual void didMoveToNewDocument(Document* oldDocument) OVERRIDE;

    virtual bool shouldRegisterAsNamedItem() const OVERRIDE { return true; }

    virtual void copyNonAttributePropertiesFromElement(const Element&) OVERRIDE;

    void submit(Event*, bool activateSubmitButton, bool processingUserGesture, FormSubmissionTrigger);

    unsigned formElementIndexWithFormAttribute(Element*, unsigned rangeStart, unsigned rangeEnd);
    unsigned formElementIndex(FormAssociatedElement*);

    // Returns true if the submission should proceed.
    bool validateInteractively(Event*);

    // Validates each of the controls, and stores controls of which 'invalid'
    // event was not canceled to the specified vector. Returns true if there
    // are any invalid controls in this form.
    bool checkInvalidControlsAndCollectUnhandled(Vector<RefPtr<FormAssociatedElement> >&);

    typedef HashMap<RefPtr<AtomicStringImpl>, RefPtr<HTMLFormControlElement> > AliasMap;

    FormSubmission::Attributes m_attributes;
    OwnPtr<AliasMap> m_elementAliases;

    CheckedRadioButtons m_checkedRadioButtons;

    unsigned m_associatedElementsBeforeIndex;
    unsigned m_associatedElementsAfterIndex;
    Vector<FormAssociatedElement*> m_associatedElements;
    Vector<HTMLImageElement*> m_imageElements;

    bool m_wasUserSubmitted;
    bool m_isSubmittingOrPreparingForSubmission;
    bool m_shouldSubmit;

    bool m_isInResetFunction;

    bool m_wasDemoted;
};

} // namespace WebCore

#endif // HTMLFormElement_h
