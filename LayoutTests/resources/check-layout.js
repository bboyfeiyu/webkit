if (window.testRunner)
    testRunner.dumpAsText();

(function() {

function insertAfter(nodeToAdd, referenceNode)
{
    if (referenceNode == document.body) {
        document.body.appendChild(nodeToAdd);
        return;
    }

    if (referenceNode.nextSibling)
        referenceNode.parentNode.insertBefore(nodeToAdd, referenceNode.nextSibling);
    else
        referenceNode.parentNode.appendChild(nodeToAdd);
}

function checkSubtreeExpectedValues(parent, failures)
{
    var checkedLayout = checkExpectedValues(parent, failures);
    Array.prototype.forEach.call(parent.childNodes, function(node) {
        checkedLayout |= checkSubtreeExpectedValues(node, failures);
    });
    return checkedLayout;
}

function checkAttribute(output, node, attribute)
{
    var result = node.getAttribute && node.getAttribute(attribute);
    output.checked |= !!result;
    return result;
}

function checkExpectedValues(node, failures)
{
    var output = { checked: false };
    var expectedWidth = checkAttribute(output, node, "data-expected-width");
    if (expectedWidth) {
        if (node.offsetWidth != parseInt(expectedWidth))
            failures.push("Expected " + expectedWidth + " for width, but got " + node.offsetWidth + ". ");
    }

    var expectedHeight = checkAttribute(output, node, "data-expected-height");
    if (expectedHeight) {
        if (node.offsetHeight != parseInt(expectedHeight))
            failures.push("Expected " + expectedHeight + " for height, but got " + node.offsetHeight + ". ");
    }

    var expectedOffset = checkAttribute(output, node, "data-offset-x");
    if (expectedOffset) {
        if (node.offsetLeft != parseInt(expectedOffset))
            failures.push("Expected " + expectedOffset + " for offsetLeft, but got " + node.offsetLeft + ". ");
    }

    var expectedOffset = checkAttribute(output, node, "data-offset-y");
    if (expectedOffset) {
        if (node.offsetTop != parseInt(expectedOffset))
            failures.push("Expected " + expectedOffset + " for offsetTop, but got " + node.offsetTop + ". ");
    }

    var expectedWidth = checkAttribute(output, node, "data-expected-client-width");
    if (expectedWidth) {
        if (node.clientWidth != parseInt(expectedWidth))
            failures.push("Expected " + expectedWidth + " for clientWidth, but got " + node.clientWidth + ". ");
    }

    var expectedHeight = checkAttribute(output, node, "data-expected-client-height");
    if (expectedHeight) {
        if (node.clientHeight != parseInt(expectedHeight))
            failures.push("Expected " + expectedHeight + " for clientHeight, but got " + node.clientHeight + ". ");
    }

    var expectedWidth = checkAttribute(output, node, "data-expected-scroll-width");
    if (expectedWidth) {
        if (node.scrollWidth != parseInt(expectedWidth))
            failures.push("Expected " + expectedWidth + " for scrollWidth, but got " + node.scrollWidth + ". ");
    }

    var expectedHeight = checkAttribute(output, node, "data-expected-scroll-height");
    if (expectedHeight) {
        if (node.scrollHeight != parseInt(expectedHeight))
            failures.push("Expected " + expectedHeight + " for scrollHeight, but got " + node.scrollHeight + ". ");
    }

    var expectedOffset = checkAttribute(output, node, "data-total-x");
    if (expectedOffset) {
        var totalLeft = node.clientLeft + node.offsetLeft;
        if (totalLeft != parseInt(expectedOffset))
            failures.push("Expected " + expectedOffset + " for clientLeft+offsetLeft, but got " + totalLeft + ", clientLeft: " + node.clientLeft + ", offsetLeft: " + node.offsetLeft + ". ");
    }

    var expectedOffset = checkAttribute(output, node, "data-total-y");
    if (expectedOffset) {
        var totalTop = node.clientTop + node.offsetTop;
        if (totalTop != parseInt(expectedOffset))
            failures.push("Expected " + expectedOffset + " for clientTop+offsetTop, but got " + totalTop + ", clientTop: " + node.clientTop + ", + offsetTop: " + node.offsetTop + ". ");
    }

    var expectedDisplay = checkAttribute(output, node, "data-expected-display");
    if (expectedDisplay) {
        var actualDisplay = getComputedStyle(node).display;
        if (actualDisplay != expectedDisplay)
            failures.push("Expected " + expectedDisplay + " for display, but got " + actualDisplay + ". ");
    }

    var expectedPaddingTop = checkAttribute(output, node, "data-expected-padding-top");
    if (expectedPaddingTop) {
        var actualPaddingTop = getComputedStyle(node).paddingTop;
        // Trim the unit "px" from the output.
        actualPaddingTop = actualPaddingTop.substring(0, actualPaddingTop.length - 2);
        if (actualPaddingTop != expectedPaddingTop)
            failures.push("Expected " + expectedPaddingTop + " for padding-top, but got " + actualPaddingTop + ". ");
    }

    var expectedPaddingBottom = checkAttribute(output, node, "data-expected-padding-bottom");
    if (expectedPaddingBottom) {
        var actualPaddingBottom = getComputedStyle(node).paddingBottom;
        // Trim the unit "px" from the output.
        actualPaddingBottom = actualPaddingBottom.substring(0, actualPaddingBottom.length - 2);
        if (actualPaddingBottom != expectedPaddingBottom)
            failures.push("Expected " + expectedPaddingBottom + " for padding-bottom, but got " + actualPaddingBottom + ". ");
    }

    var expectedPaddingLeft = checkAttribute(output, node, "data-expected-padding-left");
    if (expectedPaddingLeft) {
        var actualPaddingLeft = getComputedStyle(node).paddingLeft;
        // Trim the unit "px" from the output.
        actualPaddingLeft = actualPaddingLeft.substring(0, actualPaddingLeft.length - 2);
        if (actualPaddingLeft != expectedPaddingLeft)
            failures.push("Expected " + expectedPaddingLeft + " for padding-left, but got " + actualPaddingLeft + ". ");
    }

    var expectedPaddingRight = checkAttribute(output, node, "data-expected-padding-right");
    if (expectedPaddingRight) {
        var actualPaddingRight = getComputedStyle(node).paddingRight;
        // Trim the unit "px" from the output.
        actualPaddingRight = actualPaddingRight.substring(0, actualPaddingRight.length - 2);
        if (actualPaddingRight != expectedPaddingRight)
            failures.push("Expected " + expectedPaddingRight + " for padding-right, but got " + actualPaddingRight + ". ");
    }

    var expectedMarginTop = checkAttribute(output, node, "data-expected-margin-top");
    if (expectedMarginTop) {
        var actualMarginTop = getComputedStyle(node).marginTop;
        // Trim the unit "px" from the output.
        actualMarginTop = actualMarginTop.substring(0, actualMarginTop.length - 2);
        if (actualMarginTop != expectedMarginTop)
            failures.push("Expected " + expectedMarginTop + " for margin-top, but got " + actualMarginTop + ". ");
    }

    var expectedMarginBottom = checkAttribute(output, node, "data-expected-margin-bottom");
    if (expectedMarginBottom) {
        var actualMarginBottom = getComputedStyle(node).marginBottom;
        // Trim the unit "px" from the output.
        actualMarginBottom = actualMarginBottom.substring(0, actualMarginBottom.length - 2);
        if (actualMarginBottom != expectedMarginBottom)
            failures.push("Expected " + expectedMarginBottom + " for margin-bottom, but got " + actualMarginBottom + ". ");
    }

    var expectedMarginLeft = checkAttribute(output, node, "data-expected-margin-left");
    if (expectedMarginLeft) {
        var actualMarginLeft = getComputedStyle(node).marginLeft;
        // Trim the unit "px" from the output.
        actualMarginLeft = actualMarginLeft.substring(0, actualMarginLeft.length - 2);
        if (actualMarginLeft != expectedMarginLeft)
            failures.push("Expected " + expectedMarginLeft + " for margin-left, but got " + actualMarginLeft + ". ");
    }

    var expectedMarginRight = checkAttribute(output, node, "data-expected-margin-right");
    if (expectedMarginRight) {
        var actualMarginRight = getComputedStyle(node).marginRight;
        // Trim the unit "px" from the output.
        actualMarginRight = actualMarginRight.substring(0, actualMarginRight.length - 2);
        if (actualMarginRight != expectedMarginRight)
            failures.push("Expected " + expectedMarginRight + " for margin-right, but got " + actualMarginRight + ". ");
    }

    return output.checked;
}

window.checkLayout = function(selectorList)
{
    if (!selectorList) {
        console.error("You must provide a CSS selector of nodes to check.");
        return;
    }
    var nodes = document.querySelectorAll(selectorList);
    nodes = Array.prototype.slice.call(nodes);
    nodes.reverse();
    var checkedLayout = false;
    Array.prototype.forEach.call(nodes, function(node) {
        var failures = [];
        checkedLayout |= checkExpectedValues(node.parentNode, failures);
        checkedLayout |= checkSubtreeExpectedValues(node, failures);

        var container = node.parentNode.className == 'container' ? node.parentNode : node;

        var pre = document.createElement('pre');
        if (failures.length)
            pre.className = 'FAIL';
        pre.appendChild(document.createTextNode(failures.length ? "FAIL:\n" + failures.join('\n') + '\n\n' + container.outerHTML : "PASS"));
        insertAfter(pre, container);
    });

    if (!checkedLayout) {
        document.body.innerHTML = "FAIL: No valid data-* attributes found in selector list : " + selectorList;
        return;
    }

    var pre = document.querySelector('.FAIL');
    if (pre)
        setTimeout(function() { pre.previousSibling.scrollIntoView(); }, 0);
}

})();
