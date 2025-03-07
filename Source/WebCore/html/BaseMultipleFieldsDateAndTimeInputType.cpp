/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#if ENABLE(INPUT_MULTIPLE_FIELDS_UI)
#include "BaseMultipleFieldsDateAndTimeInputType.h"

#include "CSSValueKeywords.h"
#include "DateComponents.h"
#include "DateTimeFieldsState.h"
#include "DateTimeFormat.h"
#include "ElementShadow.h"
#include "FocusController.h"
#include "FormController.h"
#include "HTMLDataListElement.h"
#include "HTMLInputElement.h"
#include "HTMLOptionElement.h"
#include "KeyboardEvent.h"
#include "LocalizedStrings.h"
#include "NodeTraversal.h"
#include "Page.h"
#include "PickerIndicatorElement.h"
#include "PlatformLocale.h"
#include "RenderTheme.h"
#include "ShadowRoot.h"
#include <wtf/DateMath.h>

namespace WebCore {

class DateTimeFormatValidator : public DateTimeFormat::TokenHandler {
public:
    DateTimeFormatValidator()
        : m_hasYear(false)
        , m_hasMonth(false)
        , m_hasWeek(false)
        , m_hasDay(false)
        , m_hasAMPM(false)
        , m_hasHour(false)
        , m_hasMinute(false)
        , m_hasSecond(false) { }

    virtual void visitField(DateTimeFormat::FieldType, int) OVERRIDE FINAL;
    virtual void visitLiteral(const String&) OVERRIDE FINAL { }

    bool validateFormat(const String& format, const BaseMultipleFieldsDateAndTimeInputType&);

private:
    bool m_hasYear;
    bool m_hasMonth;
    bool m_hasWeek;
    bool m_hasDay;
    bool m_hasAMPM;
    bool m_hasHour;
    bool m_hasMinute;
    bool m_hasSecond;
};

void DateTimeFormatValidator::visitField(DateTimeFormat::FieldType fieldType, int)
{
    switch (fieldType) {
    case DateTimeFormat::FieldTypeYear:
        m_hasYear = true;
        break;
    case DateTimeFormat::FieldTypeMonth: // Fallthrough.
    case DateTimeFormat::FieldTypeMonthStandAlone:
        m_hasMonth = true;
        break;
    case DateTimeFormat::FieldTypeWeekOfYear:
        m_hasWeek = true;
        break;
    case DateTimeFormat::FieldTypeDayOfMonth:
        m_hasDay = true;
        break;
    case DateTimeFormat::FieldTypePeriod:
        m_hasAMPM = true;
        break;
    case DateTimeFormat::FieldTypeHour11: // Fallthrough.
    case DateTimeFormat::FieldTypeHour12:
        m_hasHour = true;
        break;
    case DateTimeFormat::FieldTypeHour23: // Fallthrough.
    case DateTimeFormat::FieldTypeHour24:
        m_hasHour = true;
        m_hasAMPM = true;
        break;
    case DateTimeFormat::FieldTypeMinute:
        m_hasMinute = true;
        break;
    case DateTimeFormat::FieldTypeSecond:
        m_hasSecond = true;
        break;
    default:
        break;
    }
}

bool DateTimeFormatValidator::validateFormat(const String& format, const BaseMultipleFieldsDateAndTimeInputType& inputType)
{
    if (!DateTimeFormat::parse(format, *this))
        return false;
    return inputType.isValidFormat(m_hasYear, m_hasMonth, m_hasWeek, m_hasDay, m_hasAMPM, m_hasHour, m_hasMinute, m_hasSecond);
}

void BaseMultipleFieldsDateAndTimeInputType::didBlurFromControl()
{
    // We don't need to call blur(). This function is called when control
    // lost focus.

    // Remove focus ring by CSS "focus" pseudo class.
    element()->setFocus(false);
}

void BaseMultipleFieldsDateAndTimeInputType::didFocusOnControl()
{
    // We don't need to call focus(). This function is called when control
    // got focus.

    // Add focus ring by CSS "focus" pseudo class.
    element()->setFocus(true);
}

void BaseMultipleFieldsDateAndTimeInputType::editControlValueChanged()
{
    RefPtr<HTMLInputElement> input(element());
    String oldValue = input->value();
    String newValue = sanitizeValue(m_dateTimeEditElement->value());
    // Even if oldValue is null and newValue is "", we should assume they are same.
    if ((oldValue.isEmpty() && newValue.isEmpty()) || oldValue == newValue)
        input->setNeedsValidityCheck();
    else {
        input->setValueInternal(newValue, DispatchNoEvent);
        input->setNeedsStyleRecalc();
        input->dispatchFormControlInputEvent();
        input->dispatchFormControlChangeEvent();
    }
    input->notifyFormStateChanged();
    input->updateClearButtonVisibility();
}

bool BaseMultipleFieldsDateAndTimeInputType::hasCustomFocusLogic() const
{
    return false;
}

bool BaseMultipleFieldsDateAndTimeInputType::isEditControlOwnerDisabled() const
{
    return element()->isDisabledFormControl();
}

bool BaseMultipleFieldsDateAndTimeInputType::isEditControlOwnerReadOnly() const
{
    return element()->isReadOnly();
}

void BaseMultipleFieldsDateAndTimeInputType::focusAndSelectSpinButtonOwner()
{
    if (m_dateTimeEditElement)
        m_dateTimeEditElement->focusIfNoFocus();
}

bool BaseMultipleFieldsDateAndTimeInputType::shouldSpinButtonRespondToMouseEvents()
{
    return !element()->isDisabledOrReadOnly();
}

bool BaseMultipleFieldsDateAndTimeInputType::shouldSpinButtonRespondToWheelEvents()
{
    if (!shouldSpinButtonRespondToMouseEvents())
        return false;
    return m_dateTimeEditElement && m_dateTimeEditElement->hasFocusedField();
}

void BaseMultipleFieldsDateAndTimeInputType::spinButtonStepDown()
{
    if (m_dateTimeEditElement)
        m_dateTimeEditElement->stepDown();
}

void BaseMultipleFieldsDateAndTimeInputType::spinButtonStepUp()
{
    if (m_dateTimeEditElement)
        m_dateTimeEditElement->stepUp();
}

bool BaseMultipleFieldsDateAndTimeInputType::isPickerIndicatorOwnerDisabledOrReadOnly() const
{
    return element()->isDisabledOrReadOnly();
}

void BaseMultipleFieldsDateAndTimeInputType::pickerIndicatorChooseValue(const String& value)
{
    if (element()->isValidValue(value)) {
        element()->setValue(value, DispatchInputAndChangeEvent);
        return;
    }

    if (!m_dateTimeEditElement)
        return;
    DateComponents date;
    unsigned end;
    if (date.parseDate(value.characters(), value.length(), 0, end) && end == value.length())
        m_dateTimeEditElement->setOnlyYearMonthDay(date);
}

bool BaseMultipleFieldsDateAndTimeInputType::setupDateTimeChooserParameters(DateTimeChooserParameters& parameters)
{
    return element()->setupDateTimeChooserParameters(parameters);
}

BaseMultipleFieldsDateAndTimeInputType::BaseMultipleFieldsDateAndTimeInputType(HTMLInputElement* element)
    : BaseDateAndTimeInputType(element)
    , m_dateTimeEditElement(0)
    , m_spinButtonElement(0)
    , m_clearButton(0)
    , m_pickerIndicatorElement(0)
    , m_pickerIndicatorIsVisible(false)
    , m_pickerIndicatorIsAlwaysVisible(false)
{
}

BaseMultipleFieldsDateAndTimeInputType::~BaseMultipleFieldsDateAndTimeInputType()
{
    if (m_spinButtonElement)
        m_spinButtonElement->removeSpinButtonOwner();
    if (m_clearButton)
        m_clearButton->removeClearButtonOwner();
    if (m_dateTimeEditElement)
        m_dateTimeEditElement->removeEditControlOwner();
    if (m_pickerIndicatorElement)
        m_pickerIndicatorElement->removePickerIndicatorOwner();
}

String BaseMultipleFieldsDateAndTimeInputType::badInputText() const
{
    return validationMessageBadInputForDateTimeText();
}

void BaseMultipleFieldsDateAndTimeInputType::blur()
{
    if (m_dateTimeEditElement)
        m_dateTimeEditElement->blurByOwner();
}

PassRefPtr<RenderStyle> BaseMultipleFieldsDateAndTimeInputType::customStyleForRenderer(PassRefPtr<RenderStyle> originalStyle)
{
    EDisplay originalDisplay = originalStyle->display();
    EDisplay newDisplay = originalDisplay;
    if (originalDisplay == INLINE || originalDisplay == INLINE_BLOCK)
        newDisplay = INLINE_FLEX;
    else if (originalDisplay == BLOCK)
        newDisplay = FLEX;
    TextDirection contentDirection = element()->locale().isRTL() ? RTL : LTR;
    if (originalStyle->direction() == contentDirection && originalDisplay == newDisplay)
        return originalStyle;

    RefPtr<RenderStyle> style = RenderStyle::clone(originalStyle.get());
    style->setDirection(contentDirection);
    style->setDisplay(newDisplay);
    return style.release();
}

void BaseMultipleFieldsDateAndTimeInputType::createShadowSubtree()
{
    ASSERT(element()->shadow());

    Document* document = element()->document();
    ContainerNode* container = element()->userAgentShadowRoot();

    RefPtr<DateTimeEditElement> dateTimeEditElement(DateTimeEditElement::create(document, *this));
    m_dateTimeEditElement = dateTimeEditElement.get();
    container->appendChild(m_dateTimeEditElement);
    updateInnerTextValue();

    RefPtr<ClearButtonElement> clearButton = ClearButtonElement::create(document, *this);
    m_clearButton = clearButton.get();
    container->appendChild(clearButton);

    RefPtr<SpinButtonElement> spinButton = SpinButtonElement::create(document, *this);
    m_spinButtonElement = spinButton.get();
    container->appendChild(spinButton);

    bool shouldAddPickerIndicator = false;
#if ENABLE(DATALIST_ELEMENT)
    if (InputType::themeSupportsDataListUI(this))
        shouldAddPickerIndicator = true;
#endif
    RefPtr<RenderTheme> theme = document->page() ? document->page()->theme() : RenderTheme::defaultTheme();
    if (theme->supportsCalendarPicker(formControlType())) {
        shouldAddPickerIndicator = true;
        m_pickerIndicatorIsAlwaysVisible = true;
    }
    if (shouldAddPickerIndicator) {
        RefPtr<PickerIndicatorElement> pickerElement = PickerIndicatorElement::create(document, *this);
        m_pickerIndicatorElement = pickerElement.get();
        container->appendChild(m_pickerIndicatorElement);
        m_pickerIndicatorIsVisible = true;
        updatePickerIndicatorVisibility();
    }
}

void BaseMultipleFieldsDateAndTimeInputType::destroyShadowSubtree()
{
    if (m_spinButtonElement) {
        m_spinButtonElement->removeSpinButtonOwner();
        m_spinButtonElement = 0;
    }
    if (m_clearButton) {
        m_clearButton->removeClearButtonOwner();
        m_clearButton = 0;
    }
    if (m_dateTimeEditElement) {
        m_dateTimeEditElement->removeEditControlOwner();
        m_dateTimeEditElement = 0;
    }
    if (m_pickerIndicatorElement) {
        m_pickerIndicatorElement->removePickerIndicatorOwner();
        m_pickerIndicatorElement = 0;
    }
    BaseDateAndTimeInputType::destroyShadowSubtree();
}

void BaseMultipleFieldsDateAndTimeInputType::handleFocusEvent(Node* oldFocusedNode, FocusDirection direction)
{
    if (!m_dateTimeEditElement)
        return;
    if (direction == FocusDirectionBackward) {
        if (element()->document()->page())
            element()->document()->page()->focusController()->advanceFocus(direction, 0);
    } else if (direction == FocusDirectionNone) {
        m_dateTimeEditElement->focusByOwner(oldFocusedNode);
    } else
        m_dateTimeEditElement->focusByOwner();
}

void BaseMultipleFieldsDateAndTimeInputType::forwardEvent(Event* event)
{
    if (m_spinButtonElement) {
        m_spinButtonElement->forwardEvent(event);
        if (event->defaultHandled())
            return;
    }
        
    if (m_dateTimeEditElement)
        m_dateTimeEditElement->defaultEventHandler(event);
}

void BaseMultipleFieldsDateAndTimeInputType::disabledAttributeChanged()
{
    m_spinButtonElement->releaseCapture();
    m_clearButton->releaseCapture();
    if (m_dateTimeEditElement)
        m_dateTimeEditElement->disabledStateChanged();
}

void BaseMultipleFieldsDateAndTimeInputType::requiredAttributeChanged()
{
    m_clearButton->releaseCapture();
    updateClearButtonVisibility();
}

void BaseMultipleFieldsDateAndTimeInputType::handleKeydownEvent(KeyboardEvent* event)
{
    Document* document = element()->document();
    RefPtr<RenderTheme> theme = document->page() ? document->page()->theme() : RenderTheme::defaultTheme();
    if (m_pickerIndicatorIsVisible
        && ((event->keyIdentifier() == "Down" && event->getModifierState("Alt")) || (theme->shouldOpenPickerWithF4Key() && event->keyIdentifier() == "F4"))) {
        if (m_pickerIndicatorElement)
            m_pickerIndicatorElement->openPopup();
        event->setDefaultHandled();
    } else
        forwardEvent(event);
}

bool BaseMultipleFieldsDateAndTimeInputType::hasBadInput() const
{
    return element()->value().isEmpty() && m_dateTimeEditElement && m_dateTimeEditElement->anyEditableFieldsHaveValues();
}

bool BaseMultipleFieldsDateAndTimeInputType::isKeyboardFocusable(KeyboardEvent*) const
{
    return element()->isTextFormControlFocusable();
}

bool BaseMultipleFieldsDateAndTimeInputType::isMouseFocusable() const
{
    return element()->isTextFormControlFocusable();
}

AtomicString BaseMultipleFieldsDateAndTimeInputType::localeIdentifier() const
{
    return element()->computeInheritedLanguage();
}

void BaseMultipleFieldsDateAndTimeInputType::minOrMaxAttributeChanged()
{
    updateInnerTextValue();
}

void BaseMultipleFieldsDateAndTimeInputType::readonlyAttributeChanged()
{
    m_spinButtonElement->releaseCapture();
    m_clearButton->releaseCapture();
    if (m_dateTimeEditElement)
        m_dateTimeEditElement->readOnlyStateChanged();
}

void BaseMultipleFieldsDateAndTimeInputType::restoreFormControlState(const FormControlState& state)
{
    if (!m_dateTimeEditElement)
        return;
    DateTimeFieldsState dateTimeFieldsState = DateTimeFieldsState::restoreFormControlState(state);
    m_dateTimeEditElement->setValueAsDateTimeFieldsState(dateTimeFieldsState);
    element()->setValueInternal(sanitizeValue(m_dateTimeEditElement->value()), DispatchNoEvent);
    updateClearButtonVisibility();
}

FormControlState BaseMultipleFieldsDateAndTimeInputType::saveFormControlState() const
{
    if (!m_dateTimeEditElement)
        return FormControlState();

    return m_dateTimeEditElement->valueAsDateTimeFieldsState().saveFormControlState();
}

void BaseMultipleFieldsDateAndTimeInputType::setValue(const String& sanitizedValue, bool valueChanged, TextFieldEventBehavior eventBehavior)
{
    InputType::setValue(sanitizedValue, valueChanged, eventBehavior);
    if (valueChanged || (sanitizedValue.isEmpty() && m_dateTimeEditElement && m_dateTimeEditElement->anyEditableFieldsHaveValues())) {
        updateInnerTextValue();
        element()->setNeedsValidityCheck();
    }
}

bool BaseMultipleFieldsDateAndTimeInputType::shouldUseInputMethod() const
{
    return false;
}

void BaseMultipleFieldsDateAndTimeInputType::stepAttributeChanged()
{
    updateInnerTextValue();
}

void BaseMultipleFieldsDateAndTimeInputType::updateInnerTextValue()
{
    if (!m_dateTimeEditElement)
        return;

    DateTimeEditElement::LayoutParameters layoutParameters(element()->locale(), createStepRange(AnyIsDefaultStep));

    DateComponents date;
    const bool hasValue = parseToDateComponents(element()->value(), &date);
    if (!hasValue)
        setMillisecondToDateComponents(layoutParameters.stepRange.minimum().toDouble(), &date);

    setupLayoutParameters(layoutParameters, date);

    const AtomicString pattern = m_dateTimeEditElement->fastGetAttribute(HTMLNames::patternAttr);
    if (!pattern.isEmpty())
        layoutParameters.dateTimeFormat = pattern;

    if (!DateTimeFormatValidator().validateFormat(layoutParameters.dateTimeFormat, *this))
        layoutParameters.dateTimeFormat = layoutParameters.fallbackDateTimeFormat;

    if (hasValue)
        m_dateTimeEditElement->setValueAsDate(layoutParameters, date);
    else
        m_dateTimeEditElement->setEmptyValue(layoutParameters, date);
    updateClearButtonVisibility();
}

void BaseMultipleFieldsDateAndTimeInputType::valueAttributeChanged()
{
    if (!element()->hasDirtyValue())
        updateInnerTextValue();
}

#if ENABLE(DATALIST_ELEMENT)
void BaseMultipleFieldsDateAndTimeInputType::listAttributeTargetChanged()
{
    updatePickerIndicatorVisibility();
}
#endif

void BaseMultipleFieldsDateAndTimeInputType::updatePickerIndicatorVisibility()
{
    if (m_pickerIndicatorIsAlwaysVisible) {
        showPickerIndicator();
        return;
    }
#if ENABLE(DATALIST_ELEMENT)
    if (HTMLDataListElement* dataList = element()->dataList()) {
        RefPtr<HTMLCollection> options = dataList->options();
        for (unsigned i = 0; HTMLOptionElement* option = toHTMLOptionElement(options->item(i)); ++i) {
            if (element()->isValidValue(option->value())) {
                showPickerIndicator();
                return;
            }
        }
    }
    hidePickerIndicator();
#endif
}

void BaseMultipleFieldsDateAndTimeInputType::hidePickerIndicator()
{
    if (!m_pickerIndicatorIsVisible)
        return;
    m_pickerIndicatorIsVisible = false;
    ASSERT(m_pickerIndicatorElement);
    m_pickerIndicatorElement->setInlineStyleProperty(CSSPropertyDisplay, CSSValueNone);
}

void BaseMultipleFieldsDateAndTimeInputType::showPickerIndicator()
{
    if (m_pickerIndicatorIsVisible)
        return;
    m_pickerIndicatorIsVisible = true;
    ASSERT(m_pickerIndicatorElement);
    m_pickerIndicatorElement->removeInlineStyleProperty(CSSPropertyDisplay);
}

bool BaseMultipleFieldsDateAndTimeInputType::shouldHaveSecondField(const DateComponents& date) const
{
    StepRange stepRange = createStepRange(AnyIsDefaultStep);
    return date.second() || date.millisecond()
        || !stepRange.minimum().remainder(static_cast<int>(msPerMinute)).isZero()
        || !stepRange.step().remainder(static_cast<int>(msPerMinute)).isZero();
}

void BaseMultipleFieldsDateAndTimeInputType::focusAndSelectClearButtonOwner()
{
    element()->focus();
}

bool BaseMultipleFieldsDateAndTimeInputType::shouldClearButtonRespondToMouseEvents()
{
    return !element()->isDisabledOrReadOnly() && !element()->isRequired();
}

void BaseMultipleFieldsDateAndTimeInputType::clearValue()
{
    RefPtr<HTMLInputElement> input(element());
    input->setValue("", DispatchInputAndChangeEvent);
    input->updateClearButtonVisibility();
}

void BaseMultipleFieldsDateAndTimeInputType::updateClearButtonVisibility()
{
    if (!m_clearButton)
        return;

    if (element()->isRequired() || !m_dateTimeEditElement->anyEditableFieldsHaveValues())
        m_clearButton->setInlineStyleProperty(CSSPropertyVisibility, CSSValueHidden);
    else
        m_clearButton->removeInlineStyleProperty(CSSPropertyVisibility);
}

} // namespace WebCore

#endif
