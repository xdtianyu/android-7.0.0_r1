/*
 * Copyright (C) 2013 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.doclava;

import java.io.*;
import java.text.BreakIterator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.regex.Pattern;
import java.util.regex.Matcher;
import java.io.File;

import com.google.clearsilver.jsilver.data.Data;

import org.ccil.cowan.tagsoup.*;
import org.xml.sax.XMLReader;
import org.xml.sax.InputSource;
import org.xml.sax.Attributes;
import org.xml.sax.helpers.DefaultHandler;

import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import javax.xml.transform.dom.DOMResult;
import javax.xml.transform.sax.SAXSource;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathFactory;

/**
* Metadata associated with a specific documentation page. Extracts
* metadata based on the page's declared hdf vars (meta.tags and others)
* as well as implicit data relating to the page, such as url, type, etc.
* Includes a Node class that represents the metadata and lets it attach
* to parent/child elements in the tree metadata nodes for all pages.
* Node also includes methods for rendering the node tree to a json file
* in docs output, which is then used by JavaScript to load metadata
* objects into html pages.
*/

public class PageMetadata {
  File mSource;
  String mDest;
  String mTagList;
  static boolean sLowercaseTags = true;
  static boolean sLowercaseKeywords = true;
  //static String linkPrefix = (Doclava.META_DBG) ? "/" : "http://developer.android.com/";
  /**
   * regex pattern to match javadoc @link and similar tags. Extracts
   * root symbol to $1.
   */
  private static final Pattern JD_TAG_PATTERN =
      Pattern.compile("\\{@.*?[\\s\\.\\#]([A-Za-z\\(\\)\\d_]+)(?=\u007D)\u007D");

  public PageMetadata(File source, String dest, List<Node> taglist) {
    mSource = source;
    mDest = dest;

    if (dest != null) {
      int len = dest.length();
      if (len > 1 && dest.charAt(len - 1) != '/') {
        mDest = dest + '/';
      } else {
        mDest = dest;
      }
    }
  }

  /**
  * Given a list of metadata nodes organized by type, sort the
  * root nodes by type name and render the types and their child
  * metadata nodes to a json file in the out dir.
  *
  * @param rootTypeNodesList A list of root metadata nodes, each
  *        representing a type and it's member child pages.
  */
  public static void WriteList(List<Node> rootTypeNodesList) {

    Collections.sort(rootTypeNodesList, BY_TYPE_NAME);
    Node pageMeta = new Node.Builder().setLabel("TOP").setChildren(rootTypeNodesList).build();

    StringBuilder buf = new StringBuilder();
    // write the taglist to string format
    pageMeta.renderTypeResources(buf);
    pageMeta.renderTypesByTag(buf);
    // write the taglist to js file
    Data data = Doclava.makeHDF();
    data.setValue("reference_tree", buf.toString());
    ClearPage.write(data, "jd_lists_unified.cs", "jd_lists_unified.js");
  }

  /**
  * Given a list of metadata nodes organized by lang, sort the
  * root nodes by type name and render the types and their child
  * metadata nodes to separate lang-specific json files in the out dir.
  *
  * @param rootNodesList A list of root metadata nodes, each
  *        representing a type and it's member child pages.
  */
  public static void WriteListByLang(List<Node> rootNodesList) {
    Collections.sort(rootNodesList, BY_LANG_NAME);
    for (Node n : rootNodesList) {
      String langFilename = "";
      String langname = n.getLang();
      langFilename = "_" + langname;
      Collections.sort(n.getChildren(), BY_TYPE_NAME);
      Node pageMeta = new Node.Builder().setLabel("TOP").setChildren(n.getChildren()).build();

      StringBuilder buf = new StringBuilder();
      // write the taglist to string format
      pageMeta.renderLangResources(buf,langname);
      //pageMeta.renderTypesByTag(buf);
      // write the taglist to js file
      Data data = Doclava.makeHDF();
      data.setValue("reference_tree", buf.toString());
      data.setValue("metadata.lang", langname);
      String unifiedFilename = "jd_lists_unified" + langFilename + ".js";
      String extrasFilename = "jd_extras" + langFilename + ".js";
      // write out jd_lists_unified for each lang
      ClearPage.write(data, "jd_lists_unified.cs", unifiedFilename);
      // append jd_extras to jd_lists_unified for each lang, then delete.
      appendExtrasMetadata(extrasFilename, unifiedFilename);
    }
  }

  /**
  * Extract supported metadata values from a page and add them as
  * a child node of a root node based on type. Some metadata values
  * are normalized. Unsupported metadata fields are ignored. See
  * Node for supported metadata fields and methods for accessing values.
  *
  * @param docfile The file from which to extract metadata.
  * @param dest The output path for the file, used to set link to page.
  * @param filename The file from which to extract metadata.
  * @param hdf Data object in which to store the metadata values.
  * @param tagList The file from which to extract metadata.
  */
  public static void setPageMetadata(String docfile, String dest, String filename,
      Data hdf, List<Node> tagList) {
    //exclude this page if author does not want it included
    boolean excludeNode = "true".equals(hdf.getValue("excludeFromSuggestions",""));

    //check whether summary and image exist and if not, get them from itemprop/markup
    Boolean needsSummary = "".equals(hdf.getValue("page.metaDescription", ""));
    Boolean needsImage = "".equals(hdf.getValue("page.image", ""));
    if ((needsSummary) || (needsImage)) {
      //try to extract the metadata from itemprop and markup
      inferMetadata(docfile, hdf, needsSummary, needsImage);
    }

    //extract available metadata and set it in a node
    if (!excludeNode) {
      Node pageMeta = new Node.Builder().build();
      pageMeta.setLabel(getTitleNormalized(hdf, "page.title"));
      pageMeta.setCategory(hdf.getValue("page.category",""));
      pageMeta.setSummary(hdf.getValue("page.metaDescription",""));
      pageMeta.setLink(getPageUrlNormalized(filename));
      pageMeta.setGroup(getStringValueNormalized(hdf,"sample.group"));
      pageMeta.setKeywords(getPageTagsNormalized(hdf, "page.tags"));
      pageMeta.setTags(getPageTagsNormalized(hdf, "meta.tags"));
      pageMeta.setImage(getImageUrlNormalized(hdf.getValue("page.image", "")));
      pageMeta.setLang(getLangStringNormalized(hdf, filename));
      pageMeta.setType(getStringValueNormalized(hdf, "page.type"));
      pageMeta.setTimestamp(hdf.getValue("page.timestamp",""));
      if (Doclava.USE_UPDATED_TEMPLATES) {
        appendMetaNodeByLang(pageMeta, tagList);
      } else {
        appendMetaNodeByType(pageMeta, tagList);
      }
    }
  }

  /**
  * Attempt to infer page metadata based on the contents of the
  * file. Load and parse the file as a dom tree. Select values
  * in this order: 1. dom node specifically tagged with
  * microdata (itemprop). 2. first qualitifed p or img node.
  *
  * @param docfile The file from which to extract metadata.
  * @param hdf Data object in which to store the metadata values.
  * @param needsSummary Whether to extract summary metadata.
  * @param needsImage Whether to extract image metadata.
  */
  public static void inferMetadata(String docfile, Data hdf,
      Boolean needsSummary, Boolean needsImage) {
    String sum = "";
    String imageUrl = "";
    String sumFrom = needsSummary ? "none" : "hdf";
    String imgFrom = needsImage ? "none" : "hdf";
    String filedata = hdf.getValue("commentText", "");
    if (Doclava.META_DBG) System.out.println("----- " + docfile + "\n");

    try {
      XPathFactory xpathFac = XPathFactory.newInstance();
      XPath xpath = xpathFac.newXPath();
      InputStream inputStream = new ByteArrayInputStream(filedata.getBytes());
      XMLReader reader = new Parser();
      reader.setFeature(Parser.namespacesFeature, false);
      reader.setFeature(Parser.namespacePrefixesFeature, false);
      reader.setFeature(Parser.ignoreBogonsFeature, true);

      Transformer transformer = TransformerFactory.newInstance().newTransformer();
      DOMResult result = new DOMResult();
      transformer.transform(new SAXSource(reader, new InputSource(inputStream)), result);
      org.w3c.dom.Node htmlNode = result.getNode();

      if (needsSummary) {
        StringBuilder sumStrings = new StringBuilder();
        XPathExpression ItempropDescExpr = xpath.compile("/descendant-or-self::*"
            + "[@itemprop='description'][1]//text()[string(.)]");
        org.w3c.dom.NodeList nodes = (org.w3c.dom.NodeList) ItempropDescExpr.evaluate(htmlNode,
            XPathConstants.NODESET);
        if (nodes.getLength() > 0) {
          for (int i = 0; i < nodes.getLength(); i++) {
            String tx = nodes.item(i).getNodeValue();
            sumStrings.append(tx);
            sumFrom = "itemprop";
          }
        } else {
          XPathExpression FirstParaExpr = xpath.compile("//p[not(../../../"
              + "@class='notice-developers') and not(../@class='sidebox')"
              + "and not(@class)]//text()");
          nodes = (org.w3c.dom.NodeList) FirstParaExpr.evaluate(htmlNode, XPathConstants.NODESET);
          if (nodes.getLength() > 0) {
            for (int i = 0; i < nodes.getLength(); i++) {
              String tx = nodes.item(i).getNodeValue();
              sumStrings.append(tx + " ");
              sumFrom = "markup";
            }
          }
        }
        //found a summary string, now normalize it
        sum = sumStrings.toString().trim();
        if ((sum != null) && (!"".equals(sum))) {
          sum = getSummaryNormalized(sum);
        }
        //normalized summary ended up being too short to be meaningful
        if ("".equals(sum)) {
           if (Doclava.META_DBG) System.out.println("Warning: description too short! ("
            + sum.length() + "chars) ...\n\n");
        }
        //summary looks good, store it to the file hdf data
        hdf.setValue("page.metaDescription", sum);
      }
      if (needsImage) {
        XPathExpression ItempropImageExpr = xpath.compile("//*[@itemprop='image']/@src");
        org.w3c.dom.NodeList imgNodes = (org.w3c.dom.NodeList) ItempropImageExpr.evaluate(htmlNode,
            XPathConstants.NODESET);
        if (imgNodes.getLength() > 0) {
          imageUrl = imgNodes.item(0).getNodeValue();
          imgFrom = "itemprop";
        } else {
          XPathExpression FirstImgExpr = xpath.compile("//img/@src");
          imgNodes = (org.w3c.dom.NodeList) FirstImgExpr.evaluate(htmlNode, XPathConstants.NODESET);
          if (imgNodes.getLength() > 0) {
            //iterate nodes looking for valid image url and normalize.
            for (int i = 0; i < imgNodes.getLength(); i++) {
              String tx = imgNodes.item(i).getNodeValue();
              //qualify and normalize the image
              imageUrl = getImageUrlNormalized(tx);
              //this img src did not qualify, keep looking...
              if ("".equals(imageUrl)) {
                if (Doclava.META_DBG) System.out.println("    >>>>> Discarded image: " + tx);
                continue;
              } else {
                imgFrom = "markup";
                break;
              }
            }
          }
        }
        //img src url looks good, store it to the file hdf data
        hdf.setValue("page.image", imageUrl);
      }
      if (Doclava.META_DBG) System.out.println("Image (" + imgFrom + "): " + imageUrl);
      if (Doclava.META_DBG) System.out.println("Summary (" + sumFrom + "): " + sum.length()
          + " chars\n\n" + sum + "\n");
      return;

    } catch (Exception e) {
      if (Doclava.META_DBG) System.out.println("    >>>>> Exception: " + e + "\n");
    }
  }

  /**
  * Normalize a comma-delimited, multi-string value. Split on commas, remove
  * quotes, trim whitespace, optionally make keywords/tags lowercase for
  * easier matching.
  *
  * @param hdf Data object in which the metadata values are stored.
  * @param tag The hdf var from which the metadata was extracted.
  * @return A normalized string value for the specified tag.
  */
  public static String getPageTagsNormalized(Data hdf, String tag) {

    String normTags = "";
    StringBuilder tags = new StringBuilder();
    String tagList = hdf.getValue(tag, "");
    if (tag.equals("meta.tags") && (tagList.equals(""))) {
      //use keywords as tags if no meta tags are available
      tagList = hdf.getValue("page.tags", "");
    }
    if (!tagList.equals("")) {
      tagList = tagList.replaceAll("\"", "");

      String[] tagParts = tagList.split("[,\u3001]");
      for (int iter = 0; iter < tagParts.length; iter++) {
        tags.append("\"");
        if (tag.equals("meta.tags") && sLowercaseTags) {
          tagParts[iter] = tagParts[iter].toLowerCase();
        } else if (tag.equals("page.tags") && sLowercaseKeywords) {
          tagParts[iter] = tagParts[iter].toLowerCase();
        }
        if (tag.equals("meta.tags")) {
          //tags.append("#"); //to match hashtag format used with yt/blogger resources
          tagParts[iter] = tagParts[iter].replaceAll(" ","");
        }
        tags.append(tagParts[iter].trim());
        tags.append("\"");
        if (iter < tagParts.length - 1) {
          tags.append(",");
        }
      }
    }
    //write this back to hdf to expose through js
    if (tag.equals("meta.tags")) {
      hdf.setValue(tag, tags.toString());
    }
    return tags.toString();
  }

  /**
  * Normalize a string for which only a single value is supported.
  * Extract the string up to the first comma, remove quotes, remove
  * any forward-slash prefix, trim any whitespace, optionally make
  * lowercase for easier matching.
  *
  * @param hdf Data object in which the metadata values are stored.
  * @param tag The hdf var from which the metadata should be extracted.
  * @return A normalized string value for the specified tag.
  */
  public static String getStringValueNormalized(Data hdf, String tag) {
    StringBuilder outString =  new StringBuilder();
    String tagList = hdf.getValue(tag, "");
    tagList.replaceAll("\"", "");
    if ("".equals(tagList)) {
      return tagList;
    } else {
      int end = tagList.indexOf(",");
      if (end != -1) {
        tagList = tagList.substring(0,end);
      }
      tagList = tagList.startsWith("/") ? tagList.substring(1) : tagList;
      if ("sample.group".equals(tag) && sLowercaseTags) {
        tagList = tagList.toLowerCase();
      }
      outString.append(tagList.trim());
      return outString.toString();
    } 
  }

  /**
  * Normalize a page title. Extract the string, remove quotes, remove
  * markup, and trim any whitespace.
  *
  * @param hdf Data object in which the metadata values are stored.
  * @param tag The hdf var from which the metadata should be extracted.
  * @return A normalized string value for the specified tag.
  */
  public static String getTitleNormalized(Data hdf, String tag) {
    StringBuilder outTitle =  new StringBuilder();
    String title = hdf.getValue(tag, "");
    if (!title.isEmpty()) {
      title = escapeString(title);
      if (title.indexOf("<span") != -1) {
        String[] splitTitle = title.split("<span(.*?)</span>");
        title = splitTitle[0];
        for (int j = 1; j < splitTitle.length; j++) {
          title.concat(splitTitle[j]);
        }
      }
      outTitle.append(title.trim());
    }
    return outTitle.toString();
  }

  /**
  * Extract and normalize a page's language string based on the
  * lowercased dir path. Non-supported langs are ignored and assigned
  * the default lang string of "en".
  *
  * @param filename A path string to the file relative to root.
  * @return A normalized lang value.
  */
  public static String getLangStringNormalized(Data data, String filename) {
    String[] stripStr = filename.toLowerCase().split("\\/", 3);
    String outFrag = "en";
    String pathCanonical = filename;
    if (stripStr.length > 0) {
      for (String t : DocFile.DEVSITE_VALID_LANGS) {
        if ("intl".equals(stripStr[0])) {
          if (t.equals(stripStr[1])) {
            outFrag = stripStr[1];
            //extract the root url (exclusive of intl/nn)
            pathCanonical = stripStr[2];
            break;
          }
        }
      }
    }
    //extract the root url (exclusive of intl/nn)
    data.setValue("path.canonical", pathCanonical);
    return outFrag;
  }

  /**
  * Normalize a page summary string and truncate as needed. Strings
  * exceeding max_chars are truncated at the first word boundary
  * following the max_size marker. Strings smaller than min_chars
  * are discarded (as they are assumed to be too little context).
  *
  * @param s String extracted from the page as it's summary.
  * @return A normalized string value.
  */
  public static String getSummaryNormalized(String s) {
    String str = "";
    int max_chars = 250;
    int min_chars = 50;
    int marker = 0;
    if (s.length() < min_chars) {
      return str;
    } else {
      str = s.replaceAll("^\"|\"$", "");
      str = str.replaceAll("\\s+", " ");
      str = JD_TAG_PATTERN.matcher(str).replaceAll("$1");
      str = escapeString(str);
      BreakIterator bi = BreakIterator.getWordInstance();
      bi.setText(str);
      if (str.length() > max_chars) {
        marker = bi.following(max_chars);
      } else {
        marker = bi.last();
      }
      str = str.substring(0, marker);
      str = str.concat("\u2026" );
    }
    return str;
  }

  public static String escapeString(String s) {
    s = s.replaceAll("\"", "&quot;");
    s = s.replaceAll("\'", "&#39;");
    s = s.replaceAll("<", "&lt;");
    s = s.replaceAll(">", "&gt;");
    s = s.replaceAll("/", "&#47;");
    return s;
  }

  //Disqualify img src urls that include these substrings
  public static String[] IMAGE_EXCLUDE = {"/triangle-", "favicon","android-logo",
      "icon_play.png", "robot-tiny"};

  public static boolean inList(String s, String[] list) {
    for (String t : list) {
      if (s.contains(t)) {
        return true;
      }
    }
    return false;
  }

  /**
  * Normalize an img src url by removing docRoot and leading
  * slash for local image references. These are added later
  * in js to support offline mode and keep path reference
  * format consistent with hrefs.
  *
  * @param url Abs or rel url sourced from img src.
  * @return Normalized url if qualified, else empty
  */
  public static String getImageUrlNormalized(String url) {
    String absUrl = "";
    // validate to avoid choosing using specific images
    if ((url != null) && (!url.equals("")) && (!inList(url, IMAGE_EXCLUDE))) {
      absUrl = url.replace("{@docRoot}", "");
      absUrl = absUrl.replaceFirst("^/(?!/)", "");
    }
    return absUrl;
  }

  /**
  * Normalize an href url by removing docRoot and leading
  * slash for local image references. These are added later
  * in js to support offline mode and keep path reference
  * format consistent with hrefs.
  *
  * @param url Abs or rel page url sourced from href
  * @return Normalized url, either abs or rel to root
  */
  public static String getPageUrlNormalized(String url) {
    String absUrl = "";

    if ((url !=null) && (!url.equals(""))) {
      absUrl = url.replace("{@docRoot}", "");
      if (Doclava.USE_DEVSITE_LOCALE_OUTPUT_PATHS) {
        absUrl = absUrl.replaceFirst("^en/", "");
      }
      absUrl = absUrl.replaceFirst("^/(?!/)", "");
    }
    return absUrl;
  }

  /**
  * Given a metadata node, add it as a child of a root node based on its
  * type. If there is no root node that matches the node's type, create one
  * and add the metadata node as a child node.
  *
  * @param gNode The node to attach to a root node or add as a new root node.
  * @param rootList The current list of root nodes.
  * @return The updated list of root nodes.
  */
  public static List<Node> appendMetaNodeByLang(Node gNode, List<Node> rootList) {

    String nodeLang = gNode.getLang();
    boolean matched = false;
    for (Node n : rootList) {
      if (n.getLang().equals(nodeLang)) {  //find any matching lang node
        appendMetaNodeByType(gNode,n.getChildren());
        //n.getChildren().add(gNode);
        matched = true;
        break; // add to the first root node only
      } // tag did not match
    } // end rootnodes matching iterator
    if (!matched) {
      List<Node> mlangList = new ArrayList<Node>(); // list of file objects that have a given lang
      //mlangList.add(gNode);
      Node tnode = new Node.Builder().setChildren(mlangList).setLang(nodeLang).build();
      rootList.add(tnode);
      appendMetaNodeByType(gNode, mlangList);
    }
    return rootList;
  }

  /**
  * Given a metadata node, add it as a child of a root node based on its
  * type. If there is no root node that matches the node's type, create one
  * and add the metadata node as a child node.
  *
  * @param gNode The node to attach to a root node or add as a new root node.
  * @param rootList The current list of root nodes.
  * @return The updated list of root nodes.
  */
  public static List<Node> appendMetaNodeByType(Node gNode, List<Node> rootList) {

    String nodeTags = gNode.getType();
    boolean matched = false;
    for (Node n : rootList) {
      if (n.getType().equals(nodeTags)) {  //find any matching type node
        n.getChildren().add(gNode);
        matched = true;
        break; // add to the first root node only
      } // tag did not match
    } // end rootnodes matching iterator
    if (!matched) {
      List<Node> mtaglist = new ArrayList<Node>(); // list of file objects that have a given type
      mtaglist.add(gNode);
      Node tnode = new Node.Builder().setChildren(mtaglist).setType(nodeTags).build();
      rootList.add(tnode);
    }
    return rootList;
  }

  /**
  * Given a metadata node, add it as a child of a root node based on its
  * tag. If there is no root node matching the tag, create one for it
  * and add the metadata node as a child node.
  *
  * @param gNode The node to attach to a root node or add as a new root node.
  * @param rootTagNodesList The current list of root nodes.
  * @return The updated list of root nodes.
  */
  public static List<Node> appendMetaNodeByTagIndex(Node gNode, List<Node> rootTagNodesList) {

    for (int iter = 0; iter < gNode.getChildren().size(); iter++) {
      if (gNode.getChildren().get(iter).getTags() != null) {
        List<String> nodeTags = gNode.getChildren().get(iter).getTags();
        boolean matched = false;
        for (String t : nodeTags) { //process each of the meta.tags
          for (Node n : rootTagNodesList) {
            if (n.getLabel().equals(t.toString())) {
              n.getTags().add(String.valueOf(iter));
              matched = true;
              break; // add to the first root node only
            } // tag did not match
          } // end rootnodes matching iterator
          if (!matched) {
            List<String> mtaglist = new ArrayList<String>(); // list of objects with a given tag
            mtaglist.add(String.valueOf(iter));
            Node tnode = new Node.Builder().setLabel(t.toString()).setTags(mtaglist).build();
            rootTagNodesList.add(tnode);
          }
        }
      }
    }
    return rootTagNodesList;
  }

  /**
  * Append the contents of jd_extras to jd_lists_unified for each language.
  *
  * @param extrasFilename The lang-specific extras file to append.
  * @param unifiedFilename The lang-specific unified metadata file.
  */
  public static void appendExtrasMetadata (String extrasFilename, String unifiedFilename) {

    File f = new File(ClearPage.outputDir + "/" + extrasFilename);
    if (f.exists() && !f.isDirectory()) {
      ClearPage.copyFile(true, f, unifiedFilename, true);
      try {
        if (f.delete()) {
          if (Doclava.META_DBG) System.out.println("    >>>>> Delete succeeded");
        } else {
          if (Doclava.META_DBG) System.out.println("    >>>>> Delete failed");
        }
      } catch (Exception e) {
        if (Doclava.META_DBG) System.out.println("    >>>>> Exception: " + e + "\n");
      }
    }
  }

  public static final Comparator<Node> BY_TAG_NAME = new Comparator<Node>() {
    public int compare (Node one, Node other) {
      return one.getLabel().compareTo(other.getLabel());
    }
  };

  public static final Comparator<Node> BY_TYPE_NAME = new Comparator<Node>() {
    public int compare (Node one, Node other) {
      return one.getType().compareTo(other.getType());
    }
  };

    public static final Comparator<Node> BY_LANG_NAME = new Comparator<Node>() {
    public int compare (Node one, Node other) {
      return one.getLang().compareTo(other.getLang());
    }
  };

  /**
  * A node for storing page metadata. Use Builder.build() to instantiate.
  */
  public static class Node {

    private String mLabel; // holds page.title or similar identifier
    private String mCategory; // subtabs, example 'training' 'guides'
    private String mSummary; // Summary for card or similar use
    private String mLink; //link href for item click
    private String mGroup; // from sample.group in _index.jd
    private List<String> mKeywords; // from page.tags
    private List<String> mTags; // from meta.tags
    private String mImage; // holds an href, fully qualified or relative to root
    private List<Node> mChildren;
    private String mLang;
    private String mType; // design, develop, distribute, youtube, blog, etc
    private String mTimestamp; // optional timestamp eg 1447452827

    private Node(Builder builder) {
      mLabel = builder.mLabel;
      mCategory = builder.mCategory;
      mSummary = builder.mSummary;
      mLink = builder.mLink;
      mGroup = builder.mGroup;
      mKeywords = builder.mKeywords;
      mTags = builder.mTags;
      mImage = builder.mImage;
      mChildren = builder.mChildren;
      mLang = builder.mLang;
      mType = builder.mType;
      mTimestamp = builder.mTimestamp;
    }

    private static class Builder {
      private String mLabel, mCategory, mSummary, mLink, mGroup, mImage, mLang, mType, mTimestamp;
      private List<String> mKeywords = null;
      private List<String> mTags = null;
      private List<Node> mChildren = null;
      public Builder setLabel(String mLabel) { this.mLabel = mLabel; return this;}
      public Builder setCategory(String mCategory) {
        this.mCategory = mCategory; return this;
      }
      public Builder setSummary(String mSummary) {this.mSummary = mSummary; return this;}
      public Builder setLink(String mLink) {this.mLink = mLink; return this;}
      public Builder setGroup(String mGroup) {this.mGroup = mGroup; return this;}
      public Builder setKeywords(List<String> mKeywords) {
        this.mKeywords = mKeywords; return this;
      }
      public Builder setTags(List<String> mTags) {this.mTags = mTags; return this;}
      public Builder setImage(String mImage) {this.mImage = mImage; return this;}
      public Builder setChildren(List<Node> mChildren) {this.mChildren = mChildren; return this;}
      public Builder setLang(String mLang) {this.mLang = mLang; return this;}
      public Builder setType(String mType) {this.mType = mType; return this;}
      public Builder setTimestamp(String mTimestamp) {this.mTimestamp = mTimestamp; return this;}
      public Node build() {return new Node(this);}
    }

    /**
    * Render a tree of metadata nodes organized by type.
    * @param buf Output buffer to render to.
    */
    void renderTypeResources(StringBuilder buf) {
      List<Node> list = mChildren; //list of type rootnodes
      if (list == null || list.size() == 0) {
        buf.append("null");
      } else {
        final int n = list.size();
        for (int i = 0; i < n; i++) {
          buf.append("var " + list.get(i).mType.toUpperCase() + "_RESOURCES = [");
          list.get(i).renderTypes(buf); //render this type's children
          buf.append("\n];\n\n");
        }
      }
    }

    /**
    * Render a tree of metadata nodes organized by lang.
    * @param buf Output buffer to render to.
    */
    void renderLangResources(StringBuilder buf, String langname) {
      List<Node> list = mChildren; //list of type rootnodes
      if (list == null || list.size() == 0) {
        buf.append("null");
      } else {
        final int n = list.size();
        for (int i = 0; i < n; i++) {
          buf.append("METADATA['" + langname + "']." + list.get(i).mType + " = [");
          list.get(i).renderTypes(buf); //render this lang's children
          buf.append("\n];\n\n");
        }
      }
    }
    /**
    * Render all metadata nodes for a specific type.
    * @param buf Output buffer to render to.
    */
    void renderTypes(StringBuilder buf) {
      List<Node> list = mChildren;
      if (list == null || list.size() == 0) {
        buf.append("nulltype");
      } else {
        final int n = list.size();
        for (int i = 0; i < n; i++) {
          buf.append("\n      {\n");
          buf.append("        \"title\":\"");
          renderStrWithUcs(buf, list.get(i).mLabel);
          buf.append("\",\n" );
          buf.append("        \"summary\":\"");
          renderStrWithUcs(buf, list.get(i).mSummary);
          buf.append("\",\n" );
          buf.append("        \"url\":\"" + list.get(i).mLink + "\",\n" );
          if (!"".equals(list.get(i).mImage)) {
            buf.append("        \"image\":\"" + list.get(i).mImage + "\",\n" );
          }
          if (!"".equals(list.get(i).mGroup)) {
            buf.append("        \"group\":\"");
            renderStrWithUcs(buf, list.get(i).mGroup);
            buf.append("\",\n" );
          }
          if (!"".equals(list.get(i).mCategory)) {
            buf.append("        \"category\":\"" + list.get(i).mCategory + "\",\n" );
          }
          if ((list.get(i).mType != null) && (list.get(i).mType != "")) {
            buf.append("        \"type\":\"" + list.get(i).mType + "\",\n");
          }
          list.get(i).renderArrayType(buf, list.get(i).mKeywords, "keywords");
          list.get(i).renderArrayType(buf, list.get(i).mTags, "tags");
          if (!"".equals(list.get(i).mTimestamp)) {
            buf.append("        \"timestamp\":\"" + list.get(i).mTimestamp + "\",\n");
          }
          buf.append("        \"lang\":\"" + list.get(i).mLang + "\"" );
          buf.append("\n      }");
          if (i != n - 1) {
            buf.append(", ");
          }
        }
      }
    }

    /**
    * Build and render a list of tags associated with each type.
    * @param buf Output buffer to render to.
    */
    void renderTypesByTag(StringBuilder buf) {
      List<Node> list = mChildren; //list of rootnodes
      if (list == null || list.size() == 0) {
        buf.append("null");
      } else {
        final int n = list.size();
        for (int i = 0; i < n; i++) {
        buf.append("var " + list.get(i).mType.toUpperCase() + "_BY_TAG = {");
        List<Node> mTagList = new ArrayList(); //list of rootnodes
        mTagList = appendMetaNodeByTagIndex(list.get(i), mTagList);
        list.get(i).renderTagIndices(buf, mTagList);
          buf.append("\n};\n\n");
        }
      }
    }

    /**
    * Render a list of tags associated with a type, including the
    * tag's indices in the type array.
    * @param buf Output buffer to render to.
    * @param tagList Node tree of types to render.
    */
    void renderTagIndices(StringBuilder buf, List<Node> tagList) {
      List<Node> list = tagList;
      if (list == null || list.size() == 0) {
        buf.append("");
      } else {
        final int n = list.size();
        for (int i = 0; i < n; i++) {
          buf.append("\n    " + list.get(i).mLabel + ":[");
          renderArrayValue(buf, list.get(i).mTags);
          buf.append("]");
          if (i != n - 1) {
            buf.append(", ");
          }
        }
      }
    }

    /**
    * Render key:arrayvalue pair.
    * @param buf Output buffer to render to.
    * @param type The list value to render as an arrayvalue.
    * @param key The key for the pair.
    */
    void renderArrayType(StringBuilder buf, List<String> type, String key) {
      buf.append("        \"" + key + "\": [");
      renderArrayValue(buf, type);
      buf.append("],\n");
    }

    /**
    * Render an array value to buf, with special handling of unicode characters.
    * @param buf Output buffer to render to.
    * @param type The list value to render as an arrayvalue.
    */
    void renderArrayValue(StringBuilder buf, List<String> type) {
      List<String> list = type;
      if (list != null) {
        final int n = list.size();
        for (int i = 0; i < n; i++) {
          String tagval = list.get(i).toString();
          renderStrWithUcs(buf,tagval);
          if (i != n - 1) {
            buf.append(",");
          }
        }
      }
    }

    /**
    * Render a string that can include ucs2 encoded characters.
    * @param buf Output buffer to render to.
    * @param chars String to append to buf with any necessary encoding
    */
    void renderStrWithUcs(StringBuilder buf, String chars) {
      String strval = chars;
      final int L = strval.length();
      for (int t = 0; t < L; t++) {
        char c = strval.charAt(t);
        if (c >= Character.MIN_HIGH_SURROGATE && c <= Character.MAX_HIGH_SURROGATE ) {
          // we have a UTF-16 multi-byte character
          int codePoint = strval.codePointAt(t);
          int charSize = Character.charCount(codePoint);
          t += charSize - 1;
          buf.append(String.format("\\u%04x",codePoint));
        } else if (c >= ' ' && c <= '~' && c != '\\') {
          buf.append(c);
        } else { 
          // we are encoding a two byte character
          buf.append(String.format("\\u%04x", (int) c));
        }
      }
    }

    public String getLabel() {
      return mLabel;
    }

    public void setLabel(String label) {
       mLabel = label;
    }

    public String getCategory() {
      return mCategory;
    }

    public void setCategory(String title) {
       mCategory = title;
    }

    public String getSummary() {
      return mSummary;
    }

    public void setSummary(String summary) {
       mSummary = summary;
    }

    public String getLink() {
      return mLink;
    }

    public void setLink(String ref) {
       mLink = ref;
    }

    public String getGroup() {
      return mGroup;
    }

    public void setGroup(String group) {
      mGroup = group;
    }

    public List<String> getTags() {
        return mTags;
    }

    public void setTags(String tags) {
      if ("".equals(tags)) {
        mTags = null;
      } else {
        List<String> tagList = new ArrayList();
        String[] tagParts = tags.split(",");

        for (String t : tagParts) {
          tagList.add(t);
        }
        mTags = tagList;
      }
    }

    public List<String> getKeywords() {
        return mKeywords;
    }

    public void setKeywords(String keywords) {
      if ("".equals(keywords)) {
        mKeywords = null;
      } else {
        List<String> keywordList = new ArrayList();
        String[] keywordParts = keywords.split(",");

        for (String k : keywordParts) {
          keywordList.add(k);
        }
        mKeywords = keywordList;
      }
    }

    public String getImage() {
        return mImage;
    }

    public void setImage(String ref) {
       mImage = ref;
    }

    public List<Node> getChildren() {
        return mChildren;
    }

    public void setChildren(List<Node> node) {
        mChildren = node;
    }

    public String getLang() {
      return mLang;
    }

    public void setLang(String lang) {
      mLang = lang;
    }

    public String getType() {
      return mType;
    }

    public String getTimestamp() {
      return mTimestamp;
    }

    public void setType(String type) {
      mType = type;
    }

    public void setTimestamp(String timestamp) {
      mTimestamp = timestamp;
    }
  }
}
