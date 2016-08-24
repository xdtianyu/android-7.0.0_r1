/*
 * Copyright (C) 2010 Google Inc.
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

import java.util.Arrays;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.regex.Pattern;
import java.util.regex.Matcher;
import java.io.File;

import com.google.clearsilver.jsilver.data.Data;

/**
* Represents a browsable sample code project, with methods for managing
* metadata collection, file output, sorting, etc.
*/
public class SampleCode {
  String mSource;
  String mDest;
  String mTitle;
  String mProjectDir;
  String mTags;

  /** Max size for browseable images/video. If a source file exceeds this size,
  * a file is generated with a generic placeholder and the original file is not
  * copied to out.
  */
  private static final double MAX_FILE_SIZE_BYTES = 2097152;

  /** When full tree nav is enabled, generate an index for every dir
  * and linkify the breadcrumb paths in all files.
  */
  private static final boolean FULL_TREE_NAVIGATION = false;

  public SampleCode(String source, String dest, String title) {
    mSource = source;
    mTitle = title;
    mTags = null;

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
  * Iterates a given sample code project gathering  metadata for files and building
  * a node tree that reflects the project's directory structure. After iterating
  * the project, this method adds the project's metadata to jd_lists_unified,
  * so that it is accessible for dynamic content and search suggestions.
  *
  * @param offlineMode Ignored -- offline-docs mode is not currently supported for
  *        browsable sample code projects.
  * @return A root Node for the project containing its metadata and tree structure.
  */
  public Node setSamplesTOC(boolean offlineMode) {
    List<Node> filelist = new ArrayList<Node>();
    File f = new File(mSource);
    mProjectDir = f.getName();
    String name = mProjectDir;
    String mOut = mDest + name;
    if (!f.isDirectory()) {
      System.out.println("-samplecode not a directory: " + mSource);
      return null;
    }

    Data hdf = Doclava.makeHDF();
    setProjectStructure(filelist, f, mDest);
    String link = ClearPage.toroot + "samples/" + name + "/index" + Doclava.htmlExtension;
    Node rootNode = writeSampleIndexCs(hdf, f,
        new Node.Builder().setLabel(mProjectDir).setLink(link).setChildren(filelist).build(),false);
    return rootNode;
  }

  /**
  * For a given sample code project dir, iterate through the project generating
  * browsable html for all valid sample code files. After iterating the project
  * generate a templated index file to the project output root.
  *
  * @param offlineMode Ignored -- offline-docs mode is not currently supported for
  *        browsable sample code projects.
  */
  public void writeSamplesFiles(boolean offlineMode) {
    List<Node> filelist = new ArrayList<Node>();
    File f = new File(mSource);
    mProjectDir = f.getName();
    String name = mProjectDir;
    String mOut = mDest + name;
    if (!f.isDirectory()) {
      System.out.println("-samplecode not a directory: " + mSource);
    }

    Data hdf = Doclava.makeHDF();
    if (Doclava.samplesNavTree != null) {
      hdf.setValue("samples_toc_tree", Doclava.samplesNavTree.getValue("samples_toc_tree", ""));
    }
    hdf.setValue("samples", "true");
    hdf.setValue("projectDir", mProjectDir);
    writeProjectDirectory(f, mDest, false, hdf, "Files.");
    writeProjectStructure(name, hdf);
    hdf.removeTree("parentdirs");
    hdf.setValue("parentdirs.0.Name", name);
    boolean writeFiles = true;
    String link = "samples/" + name + "/index" + Doclava.htmlExtension;
    //Write root _index.jd to out and add metadata to Node.
    writeSampleIndexCs(hdf, f,
        new Node.Builder().setLabel(mProjectDir).setLink(link).build(), true);
  }

  /**
  * Given the root Node for a sample code project, iterates through the project
  * gathering metadata and project tree structure. Unsupported file types are
  * filtered from the project output. The collected project Nodes are appended to
  * the root project node.
  *
  * @param parent The root Node that represents this sample code project.
  * @param dir The current dir being processed.
  * @param relative Relative path for creating links to this file.
  */
  public void setProjectStructure(List<Node> parent, File dir, String relative) {
    String name, link;
    File[] dirContents = dir.listFiles();
    Arrays.sort(dirContents, byTypeAndName);
    for (File f: dirContents) {
      name = f.getName();
      if (!isValidFiletype(name)) {
        continue;
      }
      if (f.isFile() && name.contains(".")) {
        String path = relative + name;
        link = convertExtension(path, Doclava.htmlExtension);
        if (inList(path, IMAGES) || inList(path, VIDEOS) || inList(path, TEMPLATED)) {
          parent.add(new Node.Builder().setLabel(name).setLink(ClearPage.toroot + link).build());
        }
      } else if (f.isDirectory()) {
        List<Node> mchildren = new ArrayList<Node>();
        String dirpath = relative + name + "/";
        setProjectStructure(mchildren, f, dirpath);
        if (mchildren.size() > 0) {
          parent.add(new Node.Builder().setLabel(name).setLink(ClearPage.toroot
            + dirpath).setChildren(mchildren).build());
        }
      }
    }
  }

  /**
  * Given a root sample code project path, iterates through the project
  * setting page metadata to manage html output and writing/copying files to
  * the output directory. Source files are templated and images are templated
  * and linked to the original image.
  *
  * @param dir The current dir being processed.
  * @param relative Relative path for creating links to this file.
  * @param recursed Whether the method is being called recursively.
  * @param hdf The data to read/write for files in this project.
  * @param newKey Key passed in recursion for managing cs child trees.
  */
  public void writeProjectDirectory(File dir, String relative, Boolean recursed,
      Data hdf, String newkey) {
    String name = "";
    String link = "";
    String type = "";
    int i = 0;
    String expansion = ".Sub.";
    String key = newkey;
    String prefixRoot = Doclava.USE_DEVSITE_LOCALE_OUTPUT_PATHS ? "en/" : "";
    if (recursed) {
      key = (key + expansion);
    } else {
      expansion = "";
    }

    File[] dirContents = dir.listFiles();
    Arrays.sort(dirContents, byTypeAndName);
    for (File f: dirContents) {
      name = f.getName();
      if (!isValidFiletype(name)) {
        continue;
      }
      if (f.isFile() && name.contains(".")) {
        String baseRelative = relative;
        if (Doclava.USE_DEVSITE_LOCALE_OUTPUT_PATHS) {
          // don't nest root path
          baseRelative = baseRelative.replaceFirst("^en/", "");
        }
        String path = baseRelative + name;
        type = mapTypes(name);
        link = convertExtension(path, Doclava.htmlExtension);
        if (inList(path, IMAGES)) {
          type = "img";
          if (f.length() < MAX_FILE_SIZE_BYTES) {
            ClearPage.copyFile(false, f, prefixRoot + path, false);
            writeImageVideoPage(f, convertExtension(prefixRoot + path, Doclava.htmlExtension),
                relative, type, true);
          } else {
            writeImageVideoPage(f, convertExtension(prefixRoot + path, Doclava.htmlExtension),
                relative, type, false);
          }
          hdf.setValue(key + i + ".Type", "img");
          hdf.setValue(key + i + ".Name", name);
          hdf.setValue(key + i + ".Href", link);
          hdf.setValue(key + i + ".RelPath", relative);
        } else if (inList(path, VIDEOS)) {
          type = "video";
          if (f.length() < MAX_FILE_SIZE_BYTES) {
            ClearPage.copyFile(false, f, prefixRoot + path, false);
            writeImageVideoPage(f, convertExtension(prefixRoot + path, Doclava.htmlExtension),
                relative, type, true);
          } else {
            writeImageVideoPage(f, convertExtension(prefixRoot + path, Doclava.htmlExtension),
                relative, type, false);
          }
          hdf.setValue(key + i + ".Type", "video");
          hdf.setValue(key + i + ".Name", name);
          hdf.setValue(key + i + ".Href", link);
          hdf.setValue(key + i + ".RelPath", relative);
        } else if (inList(path, TEMPLATED)) {
          writePage(f, convertExtension(prefixRoot + path, Doclava.htmlExtension), relative, hdf);
          hdf.setValue(key + i + ".Type", type);
          hdf.setValue(key + i + ".Name", name);
          hdf.setValue(key + i + ".Href", link);
          hdf.setValue(key + i + ".RelPath", relative);
        }
        i++;
      } else if (f.isDirectory()) {
        List<Node> mchildren = new ArrayList<Node>();
        type = "dir";
        String dirpath = relative + name;
        link = dirpath + "/index" + Doclava.htmlExtension;
         String hdfkeyName = (key + i + ".Name");
         String hdfkeyType = (key + i + ".Type");
         String hdfkeyHref = (key + i + ".Href");
        hdf.setValue(hdfkeyName, name);
        hdf.setValue(hdfkeyType, type);
        hdf.setValue(hdfkeyHref, relative + name + "/" + "index" + Doclava.htmlExtension);
        writeProjectDirectory(f, relative + name + "/", true, hdf, (key + i));
        i++;
      }
    }

    setParentDirs(hdf, relative, name, false);
    //Generate an index.html page for each dir being processed
    if (FULL_TREE_NAVIGATION) {
      ClearPage.write(hdf, "sampleindex.cs", prefixRoot + relative + "/index" + Doclava.htmlExtension);
    }
  }

  /**
  * Processes a templated project index page from _index.jd in a project root.
  * Each sample project must have an index, and each index locally defines it's own
  * page.tags and sample.group cs vars. This method takes a SC node on input, reads
  * any local vars from the _index.jd, optionally generates an html file to out,
  * then updates the SC node with the page vars and returns it to the caller.
  *
  * @param hdf The data source to read/write for this index file.
  * @param dir The sample project root directory.
  * @param tnode A Node to serve as the project's root node.
  * @param writeFiles If true, generates output files only. If false, collects
  *        metadata only.
  * @return The tnode root with any metadata/child Nodes appended.
  */
  public Node writeSampleIndexCs(Data hdf, File dir, Node tnode, boolean writeFiles) {

    String filename = dir.getAbsolutePath() + "/_index.jd";
    String mGroup = "";
    File f = new File(filename);
    String rel = dir.getPath();
    String prefixRoot = Doclava.USE_DEVSITE_LOCALE_OUTPUT_PATHS ? "en/" : "";
    if (writeFiles) {

      hdf.setValue("samples", "true");
      //set any default page variables for root index
      hdf.setValue("page.title", mProjectDir);
      hdf.setValue("projectDir", mProjectDir);
      hdf.setValue("projectTitle", mTitle);
      //add the download/project links to the landing pages.
      hdf.setValue("samplesProjectIndex", "true");
      if (!f.isFile()) {
        //The directory didn't have an _index.jd, so create a stub.
        ClearPage.write(hdf, "sampleindex.cs", prefixRoot + mDest + "index" + Doclava.htmlExtension);
      } else {
        DocFile.writePage(filename, rel, prefixRoot + mDest + "index" + Doclava.htmlExtension, hdf);
      }
    } else if (f.isFile()) {
      //gather metadata for toc and jd_lists_unified
      DocFile.getPageMetadata(filename, hdf);
      mGroup = hdf.getValue("sample.group", "");
      if (!"".equals(mGroup)) {
        tnode.setGroup(hdf.getValue("sample.group", ""));
      } else {
        //Errors.error(Errors.INVALID_SAMPLE_INDEX, null, "Sample " + mProjectDir
        //          + ": Root _index.jd must be present and must define sample.group"
        //          + " tag. Please see ... for details.");
      }
    }
    return tnode;
  }

  /**
  * Sets metadata for managing html output and generates the project view page
  * for a project.
  *
  * @param dir The project root dir.
  * @param hdf The data to read/write for files in this project.
  */
  public void writeProjectStructure(String dir, Data hdf) {
    String prefixRoot = Doclava.USE_DEVSITE_LOCALE_OUTPUT_PATHS ? "en/" : "";
    hdf.setValue("projectStructure", "true");
    hdf.setValue("projectDir", mProjectDir);
    hdf.setValue("page.title", mProjectDir + " Structure");
    hdf.setValue("projectTitle", mTitle);
    ClearPage.write(hdf, "sampleindex.cs", prefixRoot + mDest + "project" + Doclava.htmlExtension);
    hdf.setValue("projectStructure", "");
  }

  /**
  * Keeps track of each file's parent dirs. Used for generating path breadcrumbs in html.
  *
  * @param dir The data to read/write for this file.
  * @param hdf The relative path for this file, from samples root.
  * @param subdir The relative path for this file, from samples root.
  * @param name The name of the file (minus extension).
  * @param isFile Whether this is a file (not a dir).
  */
  Data setParentDirs(Data hdf, String subdir, String name, Boolean isFile) {
    if (FULL_TREE_NAVIGATION) {
      hdf.setValue("linkfyPathCrumb", "");
    }
    int iter;
    hdf.removeTree("parentdirs");
    String s = subdir;
    String urlParts[] = s.split("/");
    int n, l = 1;
    for (iter=1; iter < urlParts.length; iter++) {
      n = iter-1;
      hdf.setValue("parentdirs." + n + ".Name", urlParts[iter]);
      hdf.setValue("parentdirs." + n + ".Link", subdir + "index" + Doclava.htmlExtension);
    }
    return hdf;
  }

  /**
  * Writes a templated source code file to out.
  */
  public void writePage(File f, String out, String subdir, Data hdf) {
    String name = f.getName();
    String path = f.getPath();
    String data = SampleTagInfo.readFile(new SourcePositionInfo(path, -1, -1), path,
        "sample code", true, true, true, true);
    data = Doclava.escape(data);

    String relative = subdir.replaceFirst("samples/", "");
    setParentDirs(hdf, subdir, name, true);
    hdf.setValue("projectTitle", mTitle);
    hdf.setValue("projectDir", mProjectDir);
    hdf.setValue("page.title", name);
    hdf.setValue("subdir", subdir);
    hdf.setValue("relative", relative);
    hdf.setValue("realFile", name);
    hdf.setValue("fileContents", data);
    hdf.setValue("resTag", "sample");

    ClearPage.write(hdf, "sample.cs", out);
  }

  /**
  * Writes a templated image or video file to out.
  */
  public void writeImageVideoPage(File f, String out, String subdir,
        String resourceType, boolean browsable) {
    Data hdf = Doclava.makeHDF();
    if (Doclava.samplesNavTree != null) {
      hdf.setValue("samples_toc_tree", Doclava.samplesNavTree.getValue("samples_toc_tree", ""));
    }
    hdf.setValue("samples", "true");

    String name = f.getName();
    if (!browsable) {
      hdf.setValue("noDisplay", "true");
    }
    setParentDirs(hdf, subdir, name, true);
    hdf.setValue("samples", "true");
    hdf.setValue("page.title", name);
    hdf.setValue("projectTitle", mTitle);
    hdf.setValue("projectDir", mProjectDir);
    hdf.setValue("subdir", subdir);
    hdf.setValue("resType", resourceType);
    hdf.setValue("realFile", name);
    ClearPage.write(hdf, "sample.cs", out);
  }

  /**
  * Given a node containing sample code projects and a node containing all valid
  * group nodes, extract project nodes from tnode and append them to the group node
  * that matches their sample.group metadata.
  *
  * @param tnode A list of nodes containing sample code projects.
  * @param groupnodes A list of nodes that represent the valid sample groups.
  * @return The groupnodes list with all projects appended properly to their
  *         associated sample groups.
  */
  public static void writeSamplesNavTree(List<Node> tnode, List<Node> groupnodes) {

    Node node = new Node.Builder().setLabel("Samples").setLink(ClearPage.toroot
        + "samples/index" + Doclava.htmlExtension).setChildren(tnode).build();

    if (groupnodes != null) {
      for (int i = 0; i < tnode.size(); i++) {
        if (tnode.get(i) != null) {
          groupnodes = appendNodeGroups(tnode.get(i), groupnodes);
        }
      }
      for (int n = 0; n < groupnodes.size(); n++) {
        if (groupnodes.get(n).getChildren() == null) {
          groupnodes.remove(n);
          n--;
        } else {
          Collections.sort(groupnodes.get(n).getChildren(), byLabel);
        }
      }
      node.setChildren(groupnodes);
    }

    StringBuilder buf = new StringBuilder();
    if (Doclava.USE_DEVSITE_LOCALE_OUTPUT_PATHS) {
      node.renderGroupNodesTOCYaml(buf, "", false);
    } else {
      node.renderGroupNodesTOC(buf);
    }
    if (Doclava.samplesNavTree != null) {
      Doclava.samplesNavTree.setValue("samples_toc_tree", buf.toString());
      if (Doclava.USE_DEVSITE_LOCALE_OUTPUT_PATHS) {
        ClearPage.write(Doclava.samplesNavTree, "samples_navtree_data.cs", "en/samples/_book.yaml");
      }
    }
  }

  /**
  * For a given project root node, get the group and then iterate the list of valid
  * groups looking for a match. If found, append the project to that group node.
  * Samples that reference a valid sample group tag are added to a list for that
  * group. Samples declare a sample.group tag in their _index.jd files.
  */
  private static List<Node> appendNodeGroups(Node gNode, List<Node> groupnodes) {
    List<Node> mgrouplist = new ArrayList<Node>();
    for (int i = 0; i < groupnodes.size(); i++) {
      if (gNode.getGroup().equals(groupnodes.get(i).getLabel())) {
        if (groupnodes.get(i).getChildren() == null) {
          mgrouplist.add(gNode);
          groupnodes.get(i).setChildren(mgrouplist);
        } else {
          groupnodes.get(i).getChildren().add(gNode);
        }
        break;
      }
    }
    return groupnodes;
  }

  /**
  * Sorts an array of files by type and name (alpha), with manifest always at top.
  */
  Comparator<File> byTypeAndName = new Comparator<File>() {
    public int compare (File one, File other) {
      if (one.isDirectory() && !other.isDirectory()) {
        return 1;
      } else if (!one.isDirectory() && other.isDirectory()) {
        return -1;
      } else if ("AndroidManifest.xml".equals(one.getName())) {
        return -1;
      } else {
        return one.compareTo(other);
      }
    }
  };

  /**
  * Sorts a list of Nodes by label.
  */
  public static Comparator<Node> byLabel = new Comparator<Node>() {
    public int compare(Node one, Node other) {
      return one.getLabel().compareTo(other.getLabel());
    }
  };

  /**
  * Concatenates dirs that only hold dirs, to simplify nav tree
  */
  public static List<Node> squashNodes(List<Node> tnode) {
    List<Node> list = tnode;

    for(int i = 0; i < list.size(); ++i) {
      if (("dir".equals(list.get(i).getType())) &&
          (list.size() == 1) &&
          (list.get(i).getChildren().get(0).getChildren() != null)) {
        String thisLabel = list.get(i).getLabel();
        String childLabel =  list.get(i).getChildren().get(0).getLabel();
        String newLabel = thisLabel + "/" + childLabel;
        list.get(i).setLabel(newLabel);
        list.get(i).setChildren(list.get(i).getChildren().get(0).getChildren());
      } else {
        continue;
      }
    }
    return list;
  }

  public static String convertExtension(String s, String ext) {
    return s.substring(0, s.lastIndexOf('.')) + ext;
  }

  /**
  * Whitelists of valid image/video and source code types.
  */
  public static String[] IMAGES = {".png", ".jpg", ".gif"};
  public static String[] VIDEOS = {".mp4", ".ogv", ".webm"};
  public static String[] TEMPLATED = {".java", ".xml", ".aidl", ".rs",".txt", ".TXT"};

  public static boolean inList(String s, String[] list) {
    for (String t : list) {
      if (s.endsWith(t)) {
        return true;
      }
    }
    return false;
  }

  /**
  * Maps filenames to a set of generic types. Used for displaying files/dirs
  * in the project view page.
  */
  public static String mapTypes(String name) {
    String type = name.substring(name.lastIndexOf('.') + 1, name.length());
    if ("xml".equals(type) || "java".equals(type)) {
      if ("AndroidManifest.xml".equals(name)) type = "manifest";
      return type;
    } else {
      return type = "file";
    }
  }

  /**
  * Validates a source file from a project against restrictions to determine
  * whether to include the file in the browsable project output.
  */
  public boolean isValidFiletype(String name) {
    if (name.startsWith(".") ||
        name.startsWith("_") ||
        "default.properties".equals(name) ||
        "build.properties".equals(name) ||
        name.endsWith(".ttf") ||
        name.endsWith(".gradle") ||
        name.endsWith(".bat") ||
        "Android.mk".equals(name)) {
      return false;
    } else {
      return true;
    }
  }

  /**
  * SampleCode variant of NavTree node.
  */
  public static class Node {
    private String mLabel;
    private String mLink;
    private String mGroup; // from sample.group in _index.jd
    private List<Node> mChildren;
    private String mType;

    private Node(Builder builder) {
      mLabel = builder.mLabel;
      mLink = builder.mLink;
      mGroup = builder.mGroup;
      mChildren = builder.mChildren;
      mType = builder.mType;
    }

    public static class Builder {
      private String mLabel, mLink, mGroup, mType;
      private List<Node> mChildren = null;
      public Builder setLabel(String mLabel) { this.mLabel = mLabel; return this;}
      public Builder setLink(String mLink) { this.mLink = mLink; return this;}
      public Builder setGroup(String mGroup) { this.mGroup = mGroup; return this;}
      public Builder setChildren(List<Node> mChildren) { this.mChildren = mChildren; return this;}
      public Builder setType(String mType) { this.mType = mType; return this;}
      public Node build() {return new Node(this);}
    }

    /**
    * Renders browsable sample groups and projects to a _book.yaml file, starting
    * from the group nodes and then rendering their project nodes and finally their
    * child dirs and files.
    */
    void renderGroupNodesTOCYaml(StringBuilder buf, String indent, Boolean isChild) {
      List<Node> list = mChildren;
      if (list == null || list.size() == 0) {
        return;
      } else {
        final int n = list.size();
        if (indent.length() > 0) {
          buf.append(indent + "section:\n");
        } // else append 'toc:\n' if needed
        for (int i = 0; i < n; i++) {
          if (isChild == true && list.get(i).getChildren() != null) {
            buf.append(indent + "- title: " + list.get(i).getLabel() + "/\n");
            if (list.get(i).getLink().indexOf(".html") > -1) {
              buf.append(indent + "  path: " + list.get(i).getLink() + "\n");
              buf.append(indent + "  path_attributes:\n");
              buf.append(indent + "  - name: title\n");
              buf.append(indent + "    value: " + list.get(i).getLabel() + "\n");
            } else {
              buf.append(indent + "  path: \"#\"\n");
              buf.append(indent + "  path_attributes:\n");
              buf.append(indent + "  - name: onclick\n");
              buf.append(indent + "    value: return false;\n");
              buf.append(indent + "  - name: title\n");
              buf.append(indent + "    value: " + list.get(i).getLabel() + "\n");
            }
          } else {
            String xmlToHtmlPath = list.get(i).getLink().replace(".xml", ".html");
            buf.append(indent + "- title: " + list.get(i).getLabel() + "\n");
            buf.append(indent + "  path: " + xmlToHtmlPath + "\n");
          }
          if (list.get(i).getChildren() != null) {
            list.get(i).renderGroupNodesTOCYaml(buf, indent + "  ", true);
          }
        }
      }
    }

    /**
    * Renders browsable sample groups and projects to an html list, starting
    * from the group nodes and then rendering their project nodes and finally their
    * child dirs and files.
    */
    void renderGroupNodesTOC(StringBuilder buf) {
      List<Node> list = mChildren;
      if (list == null || list.size() == 0) {
        return;
      } else {
        final int n = list.size();
        for (int i = 0; i < n; i++) {
          if (list.get(i).getChildren() == null) {
            continue;
          } else {
            buf.append("<li class=\"nav-section\">");
            buf.append("<div class=\"nav-section-header\">");
            buf.append("<a href=\"" + list.get(i).getLink() + "\" title=\""
                + list.get(i).getLabel() + "\">"
                + list.get(i).getLabel() + "</a>");
            buf.append("</div>");
            buf.append("<ul>");
            list.get(i).renderProjectNodesTOC(buf);
          }
        }
        buf.append("</ul>");
        buf.append("</li>");
      }
    }

    /**
    * Renders a list of sample code projects associated with a group node.
    */
    void renderProjectNodesTOC(StringBuilder buf) {
      List<Node> list = mChildren;
      if (list == null || list.size() == 0) {
        return;
      } else {
        final int n = list.size();
        for (int i = 0; i < n; i++) {
          if (list.get(i).getChildren() == null) {
            continue;
          } else {
            buf.append("<li class=\"nav-section\">");
            buf.append("<div class=\"nav-section-header\">");
            buf.append("<a href=\"" + list.get(i).getLink() + "\" title=\""
                + list.get(i).getLabel() + "\">"
                + list.get(i).getLabel() + "</a>");
            buf.append("</div>");
            buf.append("<ul>");
            list.get(i).renderChildrenToc(buf);
          }
        }
        buf.append("</ul>");
        buf.append("</li>");
      }
    }

    /**
    * Renders child dirs and files associated with a project node.
    */
    void renderChildrenToc(StringBuilder buf) {
      List<Node> list = mChildren;
      if (list == null || list.size() == 0) {
        buf.append("null");
      } else {
        final int n = list.size();
        for (int i = 0; i < n; i++) {
          if (list.get(i).getChildren() == null) {
            buf.append("<li>");
            buf.append("<a href=\"" + list.get(i).getLink() + "\" title=\""
                + list.get(i).getLabel() + "\">"
                + list.get(i).getLabel() + "</a>");
            buf.append("  </li>");
          } else {
            buf.append("<li class=\"nav-section sticky\">");
            buf.append("<div class=\"nav-section-header empty\">");
            buf.append("<a href=\"#\" onclick=\"return false;\" title=\""
                + list.get(i).getLabel() + "\">"
                + list.get(i).getLabel() + "/</a>");
            buf.append("</div>");
            buf.append("<ul>");
            list.get(i).renderChildrenToc(buf);
          }
        }
        buf.append("</ul>");
        buf.append("</li>");
      }
    }

    /**
    * Node getters and setters
    */
    public String getLabel() {
      return mLabel;
    }

    public void setLabel(String label) {
       mLabel = label;
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

    public List<Node> getChildren() {
        return mChildren;
    }

    public void setChildren(List<Node> node) {
        mChildren = node;
    }

    public String getType() {
      return mType;
    }

    public void setType(String type) {
      mType = type;
    }
  }
}
