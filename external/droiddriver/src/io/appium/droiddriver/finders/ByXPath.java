/*
 * Copyright (C) 2013 DroidDriver committers
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package io.appium.droiddriver.finders;

import android.util.Log;

import org.w3c.dom.DOMException;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import java.io.BufferedOutputStream;
import java.util.HashMap;
import java.util.Map;

import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.base.BaseUiElement;
import io.appium.droiddriver.exceptions.DroidDriverException;
import io.appium.droiddriver.exceptions.ElementNotFoundException;
import io.appium.droiddriver.util.FileUtils;
import io.appium.droiddriver.util.Logs;
import io.appium.droiddriver.util.Preconditions;
import io.appium.droiddriver.util.Strings;

/**
 * Find matching UiElement by XPath.
 */
public class ByXPath implements Finder {
  private static final XPath XPATH_COMPILER = XPathFactory.newInstance().newXPath();
  // document needs to be static so that when buildDomNode is called recursively
  // on children they are in the same document to be appended.
  private static Document document;
  // The two maps should be kept in sync
  private static final Map<BaseUiElement<?, ?>, Element> TO_DOM_MAP =
      new HashMap<BaseUiElement<?, ?>, Element>();
  private static final Map<Element, BaseUiElement<?, ?>> FROM_DOM_MAP =
      new HashMap<Element, BaseUiElement<?, ?>>();

  public static void clearData() {
    TO_DOM_MAP.clear();
    FROM_DOM_MAP.clear();
    document = null;
  }

  private final String xPathString;
  private final XPathExpression xPathExpression;

  protected ByXPath(String xPathString) {
    this.xPathString = Preconditions.checkNotNull(xPathString);
    try {
      xPathExpression = XPATH_COMPILER.compile(xPathString);
    } catch (XPathExpressionException e) {
      throw new DroidDriverException("xPathString=" + xPathString, e);
    }
  }

  @Override
  public String toString() {
    return Strings.toStringHelper(this).addValue(xPathString).toString();
  }

  @Override
  public UiElement find(UiElement context) {
    Element domNode = getDomNode((BaseUiElement<?, ?>) context, UiElement.VISIBLE);
    try {
      getDocument().appendChild(domNode);
      Element foundNode = (Element) xPathExpression.evaluate(domNode, XPathConstants.NODE);
      if (foundNode == null) {
        Logs.log(Log.DEBUG, "XPath evaluation returns null for " + xPathString);
        throw new ElementNotFoundException(this);
      }

      UiElement match = FROM_DOM_MAP.get(foundNode);
      Logs.log(Log.INFO, "Found match: " + match);
      return match;
    } catch (XPathExpressionException e) {
      throw new ElementNotFoundException(this, e);
    } finally {
      try {
        getDocument().removeChild(domNode);
      } catch (DOMException e) {
        Logs.log(Log.ERROR, e, "Failed to clear document");
        document = null; // getDocument will create new
      }
    }
  }

  private static Document getDocument() {
    if (document == null) {
      try {
        document = DocumentBuilderFactory.newInstance().newDocumentBuilder().newDocument();
      } catch (ParserConfigurationException e) {
        throw new DroidDriverException(e);
      }
    }
    return document;
  }

  /**
   * Returns the DOM node representing this UiElement.
   */
  private static Element getDomNode(BaseUiElement<?, ?> uiElement,
      Predicate<? super UiElement> predicate) {
    Element domNode = TO_DOM_MAP.get(uiElement);
    if (domNode == null) {
      domNode = buildDomNode(uiElement, predicate);
    }
    return domNode;
  }

  private static Element buildDomNode(BaseUiElement<?, ?> uiElement,
      Predicate<? super UiElement> predicate) {
    String className = uiElement.getClassName();
    if (className == null) {
      className = "UNKNOWN";
    }
    Element element = getDocument().createElement(XPaths.tag(className));
    TO_DOM_MAP.put(uiElement, element);
    FROM_DOM_MAP.put(element, uiElement);

    setAttribute(element, Attribute.CLASS, className);
    setAttribute(element, Attribute.RESOURCE_ID, uiElement.getResourceId());
    setAttribute(element, Attribute.PACKAGE, uiElement.getPackageName());
    setAttribute(element, Attribute.CONTENT_DESC, uiElement.getContentDescription());
    setAttribute(element, Attribute.TEXT, uiElement.getText());
    setAttribute(element, Attribute.CHECKABLE, uiElement.isCheckable());
    setAttribute(element, Attribute.CHECKED, uiElement.isChecked());
    setAttribute(element, Attribute.CLICKABLE, uiElement.isClickable());
    setAttribute(element, Attribute.ENABLED, uiElement.isEnabled());
    setAttribute(element, Attribute.FOCUSABLE, uiElement.isFocusable());
    setAttribute(element, Attribute.FOCUSED, uiElement.isFocused());
    setAttribute(element, Attribute.SCROLLABLE, uiElement.isScrollable());
    setAttribute(element, Attribute.LONG_CLICKABLE, uiElement.isLongClickable());
    setAttribute(element, Attribute.PASSWORD, uiElement.isPassword());
    if (uiElement.hasSelection()) {
      element.setAttribute(Attribute.SELECTION_START.getName(),
          Integer.toString(uiElement.getSelectionStart()));
      element.setAttribute(Attribute.SELECTION_END.getName(),
          Integer.toString(uiElement.getSelectionEnd()));
    }
    setAttribute(element, Attribute.SELECTED, uiElement.isSelected());
    element.setAttribute(Attribute.BOUNDS.getName(), uiElement.getBounds().toShortString());

    // If we're dumping for debugging, add extra information
    if (!UiElement.VISIBLE.equals(predicate)) {
      if (!uiElement.isVisible()) {
        element.setAttribute(BaseUiElement.ATTRIB_NOT_VISIBLE, "");
      } else if (!uiElement.getVisibleBounds().equals(uiElement.getBounds())) {
        element.setAttribute(BaseUiElement.ATTRIB_VISIBLE_BOUNDS, uiElement.getVisibleBounds()
            .toShortString());
      }
    }

    for (BaseUiElement<?, ?> child : uiElement.getChildren(predicate)) {
      element.appendChild(getDomNode(child, predicate));
    }
    return element;
  }

  private static void setAttribute(Element element, Attribute attr, String value) {
    if (value != null) {
      element.setAttribute(attr.getName(), value);
    }
  }

  // add attribute only if it's true
  private static void setAttribute(Element element, Attribute attr, boolean value) {
    if (value) {
      element.setAttribute(attr.getName(), "");
    }
  }

  public static boolean dumpDom(String path, BaseUiElement<?, ?> uiElement) {
    BufferedOutputStream bos = null;
    try {
      bos = FileUtils.open(path);
      Transformer transformer = TransformerFactory.newInstance().newTransformer();
      transformer.setOutputProperty(OutputKeys.INDENT, "yes");
      // find() filters invisible UiElements, but this is for debugging and
      // invisible UiElements may be of interest.
      clearData();
      Element domNode = getDomNode(uiElement, null);
      transformer.transform(new DOMSource(domNode), new StreamResult(bos));
      Logs.log(Log.INFO, "Wrote dom to " + path);
    } catch (Exception e) {
      Logs.log(Log.ERROR, e, "Failed to transform node");
      return false;
    } finally {
      // We built DOM with invisible UiElements. Don't use it for find()!
      clearData();
      if (bos != null) {
        try {
          bos.close();
        } catch (Exception e) {
          // ignore
        }
      }
    }
    return true;
  }
}
